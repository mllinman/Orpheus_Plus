import ffmpeg from 'fluent-ffmpeg';
import path from 'path';
import fs from 'fs';

export const processAudio = (inputPath, outputDir) => {
    return new Promise((resolve, reject) => {
        // Ensure output directory exists
        if (!fs.existsSync(outputDir)) {
            fs.mkdirSync(outputDir, { recursive: true });
        }

        const stems = ['bass', 'drums', 'vocals', 'other'];
        const commands = stems.map(stem => {
            const outputPath = path.join(outputDir, `${stem}.mp3`);
            let command = ffmpeg(inputPath);

            // Apply filters based on stem type (Simulation)
            switch (stem) {
                case 'bass':
                    // Low frequencies only
                    command.audioFilters('lowpass=f=300');
                    break;
                case 'drums':
                    // Enhance low kick and high cymbals, scoop mids
                    command.audioFilters('equalizer=f=60:t=q:w=1:g=5,equalizer=f=5000:t=q:w=1:g=5,equalizer=f=500:t=q:w=2:g=-10');
                    break;
                case 'vocals':
                    // Focus on human voice range
                    command.audioFilters('bandpass=f=1000:width_type=h:width=2000');
                    break;
                case 'other':
                    // Remove bass and vocal range
                    command.audioFilters('highpass=f=300,equalizer=f=1000:t=q:w=2:g=-20');
                    break;
            }

            return new Promise((res, rej) => {
                command
                    .output(outputPath)
                    .on('end', () => res({ stem, filename: `${stem}.mp3` }))
                    .on('error', (err) => rej(err))
                    .run();
            });
        });

        Promise.all(commands)
            .then(results => resolve(results))
            .catch(err => reject(err));
    });
};
