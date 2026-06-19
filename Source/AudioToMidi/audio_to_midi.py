import sys
import os

def main():
    if len(sys.argv) < 3:
        print("Usage: python audio_to_midi.py <input.wav> <output.mid>")
        sys.exit(1)

    input_audio = sys.argv[1]
    output_midi = sys.argv[2]
    
    out_dir = os.path.dirname(output_midi)
    if out_dir and not os.path.exists(out_dir):
        os.makedirs(out_dir)

    print("STATUS: Loading audio and running Basic Pitch...")
    sys.stdout.flush()

    try:
        from basic_pitch.inference import predict
        import pretty_midi

        print("STATUS: Model loaded, predicting...")
        sys.stdout.flush()

        # basic_pitch predict() returns: model_output, midi_data, note_events
        model_output, midi_data, note_events = predict(input_audio)
        
        print("STATUS: Prediction complete. Saving MIDI...")
        sys.stdout.flush()

        midi_data.write(output_midi)

        print("STATUS: DONE")
        sys.stdout.flush()

    except Exception as e:
        print(f"ERROR: {str(e)}")
        sys.exit(1)

if __name__ == "__main__":
    main()
