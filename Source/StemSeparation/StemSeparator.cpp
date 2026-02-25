#include "StemSeparator.h"
#include "../Project/AppState.h"

StemSeparator::StemSeparator() {}
StemSeparator::~StemSeparator() { cancel(); }

void StemSeparator::separate(const juce::File& inputFile, AppState& appState,
                              std::function<void(StemSeparationResult)> onComplete)
{
    if (running.load()) return;
    running.store(true);
    progress.store(0.0f);

    auto outputDir = inputFile.getParentDirectory()
                              .getChildFile(inputFile.getFileNameWithoutExtension() + "_stems");
    outputDir.createDirectory();

    // Run on background thread
    juce::Thread::launch([this, inputFile, outputDir, onComplete = std::move(onComplete)]
    {
        bool success = false;

        switch (currentModel)
        {
            case Model::Demucs4:
            case Model::Demucs4HQ:
                success = runDemucs(inputFile, outputDir);
                break;
            case Model::Spleeter2:
            case Model::Spleeter4:
            case Model::Spleeter5:
                success = runSpleeter(inputFile, outputDir);
                break;
            case Model::OpenUnmix:
                success = runOpenUnmix(inputFile, outputDir);
                break;
            case Model::UVR_MDXNet:
                success = runUVR(inputFile, outputDir);
                break;
        }

        if (success)
        {
            auto result = collectResults(outputDir);
            running.store(false);
            progress.store(1.0f);

            juce::MessageManager::callAsync([this, result, onComplete]
            {
                listeners.call(&Listener::stemSeparationComplete, result);
                if (onComplete) onComplete(result);
            });
        }
        else
        {
            running.store(false);
            juce::MessageManager::callAsync([this]
            {
                listeners.call(&Listener::stemSeparationFailed,
                    "Stem separation failed. Ensure the AI backend is installed.");
            });
        }
    });
}

void StemSeparator::cancel()
{
    running.store(false);
}

bool StemSeparator::runDemucs(const juce::File& inputFile, const juce::File& outputDir)
{
    //─────────────────────────────────────────────────────────────────────────
    // Demucs v4 integration
    //
    // Option 1: System Python with demucs installed
    //   python3 -m demucs --two-stems=vocals -o <outputDir> <inputFile>
    //
    // Option 2: Bundled Python environment
    //   <AppBundle>/Resources/python/bin/python3 -m demucs ...
    //
    // Option 3: ONNX Runtime (convert Demucs to ONNX first)
    //   Load model via onnxruntime C++ API, run inference on audio chunks
    //─────────────────────────────────────────────────────────────────────────

    // Try to find Python with demucs
    juce::String pythonPath = "python3";
    juce::String modelFlag;

    switch (currentModel)
    {
        case Model::Demucs4:   modelFlag = "htdemucs";    break;
        case Model::Demucs4HQ: modelFlag = "htdemucs_ft"; break;
        default:               modelFlag = "htdemucs";    break;
    }

    juce::StringArray args;
    args.add(pythonPath);
    args.add("-m");
    args.add("demucs");
    args.add("--name");
    args.add(modelFlag);
    args.add("-o");
    args.add(outputDir.getFullPathName());
    args.add(inputFile.getFullPathName());

    juce::ChildProcess proc;
    if (!proc.start(args))
    {
        // Python/demucs not found — try fallback spleeter
        return runSpleeter(inputFile, outputDir);
    }

    // Poll progress while running
    while (proc.isRunning())
    {
        if (!running.load()) { proc.kill(); return false; }
        juce::Thread::sleep(500);

        // TODO: parse demucs stdout for progress %
        progress.fetch_add(0.01f); // fake increment for now
        float p = juce::jmin(0.99f, progress.load());
        juce::MessageManager::callAsync([this, p] {
            listeners.call(&Listener::stemSeparationProgress, p);
        });
    }

    return (proc.getExitCode() == 0);
}

bool StemSeparator::runSpleeter(const juce::File& inputFile, const juce::File& outputDir)
{
    juce::String stemsFlag;
    switch (currentModel)
    {
        case Model::Spleeter2: stemsFlag = "spleeter:2stems"; break;
        case Model::Spleeter4: stemsFlag = "spleeter:4stems"; break;
        case Model::Spleeter5: stemsFlag = "spleeter:5stems"; break;
        default:               stemsFlag = "spleeter:4stems"; break;
    }

    juce::StringArray args;
    args.add("python3");
    args.add("-m");
    args.add("spleeter");
    args.add("separate");
    args.add("-p");
    args.add(stemsFlag);
    args.add("-o");
    args.add(outputDir.getFullPathName());
    args.add(inputFile.getFullPathName());

    juce::ChildProcess proc;
    if (!proc.start(args)) return false;

    while (proc.isRunning())
    {
        if (!running.load()) { proc.kill(); return false; }
        juce::Thread::sleep(500);
    }

    return (proc.getExitCode() == 0);
}

bool StemSeparator::runOpenUnmix(const juce::File& inputFile, const juce::File& outputDir)
{
    // umx <input.wav> --outdir <output_dir>
    juce::StringArray args;
    args.add("umx");
    args.add(inputFile.getFullPathName());
    args.add("--outdir");
    args.add(outputDir.getFullPathName());

    juce::ChildProcess proc;
    if (!proc.start(args)) return false;

    while (proc.isRunning())
    {
        if (!running.load()) { proc.kill(); return false; }
        juce::Thread::sleep(500);
    }

    return (proc.getExitCode() == 0);
}

bool StemSeparator::runUVR(const juce::File& inputFile, const juce::File& outputDir)
{
    // audio-separator <input.wav> --model_name UVR-MDX-NET-Inst_HQ_3 --output_dir <output_dir>
    juce::StringArray args;
    args.add("audio-separator");
    args.add(inputFile.getFullPathName());
    args.add("--model_name");
    args.add("UVR-MDX-NET-Inst_HQ_3");
    args.add("--output_dir");
    args.add(outputDir.getFullPathName());

    juce::ChildProcess proc;
    if (!proc.start(args)) return false;

    while (proc.isRunning())
    {
        if (!running.load()) { proc.kill(); return false; }
        juce::Thread::sleep(500);
    }

    return (proc.getExitCode() == 0);
}

StemSeparationResult StemSeparator::collectResults(const juce::File& outputDir)
{
    StemSeparationResult result;

    // Demucs output structure: outputDir/<model>/<filename>/vocals.wav etc
    // Spleeter: outputDir/<filename>/vocals.wav etc
    // We search recursively for known stem names

    auto findStem = [&](const juce::String& stemName) -> juce::File
    {
        juce::Array<juce::File> matches;
        outputDir.findChildFiles(matches, juce::File::findFiles, true,
                                 stemName + ".wav;" + stemName + ".mp3");
        return matches.isEmpty() ? juce::File{} : matches[0];
    };

    result.vocals = findStem("vocals");
    result.drums  = findStem("drums");
    result.bass   = findStem("bass");
    result.guitar = findStem("guitar");
    result.piano  = findStem("piano");
    result.other  = findStem("other");

    return result;
}

bool StemSeparator::isBackendAvailable()
{
    juce::ChildProcess proc;
    juce::StringArray testArgs { "python3", "-c", "import demucs; print('ok')" };
    return proc.start(testArgs) && proc.waitForProcessToFinish(5000) && proc.getExitCode() == 0;
}

juce::File StemSeparator::getModelPath(Model /*m*/)
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
               .getChildFile("OrpheusPlus/models");
}
