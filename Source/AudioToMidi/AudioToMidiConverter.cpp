#include "AudioToMidiConverter.h"
#include "../Project/AppState.h"

AudioToMidiConverter::AudioToMidiConverter() {}
AudioToMidiConverter::~AudioToMidiConverter() { cancel(); }

void AudioToMidiConverter::convert(const juce::File& audioFile, AppState& appState,
                                    std::function<void(AudioToMidiResult)> onComplete)
{
    if (running.load()) return;
    running.store(true);
    progress.store(0.0f);

    auto outMidi = audioFile.getSiblingFile(
        audioFile.getFileNameWithoutExtension() + "_converted.mid");

    juce::Thread::launch([this, audioFile, outMidi, onComplete = std::move(onComplete)]
    {
        bool success = false;

        switch (mode)
        {
            case Mode::Polyphonic:
            case Mode::Chords:
                success = runBasicPitch(audioFile, outMidi);
                break;
            case Mode::Monophonic:
                success = runCrepe(audioFile, outMidi);
                break;
            case Mode::Drums:
                success = runOnsetDetection(audioFile, outMidi);
                break;
        }

        running.store(false);

        if (success)
        {
            auto result = loadMidiResult(outMidi);
            juce::MessageManager::callAsync([this, result, onComplete]
            {
                listeners.call(&Listener::conversionComplete, result);
                if (onComplete) onComplete(result);
            });
        }
        else
        {
            juce::MessageManager::callAsync([this]
            {
                listeners.call(&Listener::conversionFailed,
                    "Audio-to-MIDI conversion failed. Ensure basic-pitch is installed:\n"
                    "pip install basic-pitch");
            });
        }
    });
}

void AudioToMidiConverter::cancel()
{
    running.store(false);
}

bool AudioToMidiConverter::runBasicPitch(const juce::File& audio, const juce::File& outMidi)
{
    //─────────────────────────────────────────────────────────────────────────
    // Spotify Basic Pitch (https://github.com/spotify/basic-pitch)
    // Install: pip install basic-pitch
    // Command: basic-pitch <output_dir> <audio_file> [--save-midi]
    //─────────────────────────────────────────────────────────────────────────
    juce::StringArray args;
    args.add("python3");
    args.add("-m");
    args.add("basic_pitch");
    args.add(outMidi.getParentDirectory().getFullPathName());
    args.add(audio.getFullPathName());
    args.add("--save-midi");

    juce::ChildProcess proc;
    if (!proc.start(args)) return false;

    while (proc.isRunning())
    {
        if (!running.load()) { proc.kill(); return false; }
        juce::Thread::sleep(300);
        float p = juce::jmin(0.95f, progress.load() + 0.03f);
        progress.store(p);
        juce::MessageManager::callAsync([this, p] {
            listeners.call(&Listener::conversionProgress, p);
        });
    }

    return proc.getExitCode() == 0;
}

bool AudioToMidiConverter::runCrepe(const juce::File& audio, const juce::File& outMidi)
{
    //─────────────────────────────────────────────────────────────────────────
    // CREPE pitch detection (https://github.com/marl/crepe)
    // Install: pip install crepe
    // Python script: use librosa + crepe to detect pitch, convert to MIDI
    //─────────────────────────────────────────────────────────────────────────
    juce::String script =
        "import crepe, librosa, numpy as np, pretty_midi\n"
        "y, sr = librosa.load(r'" + audio.getFullPathName() + "')\n"
        "time, freq, conf, act = crepe.predict(y, sr, viterbi=True)\n"
        "pm = pretty_midi.PrettyMIDI()\n"
        "inst = pretty_midi.Instrument(0)\n"
        "for i in range(1, len(freq)):\n"
        "    if conf[i] > " + juce::String(sensitivity, 2) + " and freq[i] > 0:\n"
        "        note = int(round(69 + 12 * np.log2(freq[i] / 440)))\n"
        "        note = max(0, min(127, note))\n"
        "        n = pretty_midi.Note(velocity=100, pitch=note, "
        "start=time[i-1], end=time[i])\n"
        "        inst.notes.append(n)\n"
        "pm.instruments.append(inst)\n"
        "pm.write(r'" + outMidi.getFullPathName() + "')\n";

    auto scriptFile = juce::File::createTempFile(".py");
    scriptFile.replaceWithText(script);

    juce::StringArray args { "python3", scriptFile.getFullPathName() };
    juce::ChildProcess proc;
    if (!proc.start(args)) return false;

    while (proc.isRunning())
    {
        if (!running.load()) { proc.kill(); return false; }
        juce::Thread::sleep(300);
    }

    scriptFile.deleteFile();
    return proc.getExitCode() == 0;
}

bool AudioToMidiConverter::runOnsetDetection(const juce::File& audio,
                                              const juce::File& outMidi)
{
    juce::String script =
        "import librosa, numpy as np, pretty_midi\n"
        "y, sr = librosa.load(r'" + audio.getFullPathName() + "')\n"
        "onsets = librosa.onset.onset_detect(y=y, sr=sr, units='time')\n"
        "pm = pretty_midi.PrettyMIDI()\n"
        "drums = pretty_midi.Instrument(0, is_drum=True)\n"
        "for t in onsets:\n"
        "    n = pretty_midi.Note(velocity=100, pitch=38, start=float(t), end=float(t)+0.1)\n"
        "    drums.notes.append(n)\n"
        "pm.instruments.append(drums)\n"
        "pm.write(r'" + outMidi.getFullPathName() + "')\n";

    auto scriptFile = juce::File::createTempFile(".py");
    scriptFile.replaceWithText(script);

    juce::StringArray args { "python3", scriptFile.getFullPathName() };
    juce::ChildProcess proc;
    if (!proc.start(args)) return false;
    while (proc.isRunning())
    {
        if (!running.load()) { proc.kill(); return false; }
        juce::Thread::sleep(300);
    }

    scriptFile.deleteFile();
    return proc.getExitCode() == 0;
}

AudioToMidiResult AudioToMidiConverter::loadMidiResult(const juce::File& midiFile)
{
    AudioToMidiResult result;
    result.midiFileOnDisk = midiFile;

    if (midiFile.existsAsFile())
    {
        juce::FileInputStream stream(midiFile);
        result.midiFile.readFrom(stream);

        // Estimate BPM from MIDI tempo events
        if (result.midiFile.getNumTracks() > 0)
        {
            auto* track = result.midiFile.getTrack(0);
            for (int i = 0; i < track->getNumEvents(); ++i)
            {
                auto& e = track->getEventPointer(i)->message;
                if (e.isTempoMetaEvent())
                {
                    double uspb = e.getTempoSecondsPerQuarterNote() * 1e6;
                    result.detectedBPM = 60000000.0 / uspb;
                    break;
                }
            }
        }
    }

    return result;
}
