import { exec } from 'child_process';
import path from 'path';
import fs from 'fs/promises';
import { promisify } from 'util';

const execAsync = promisify(exec);

export const checkAiAvailability = async () => {
    try {
        await execAsync('spleeter --version');
        return true;
    } catch (e) {
        return false;
    }
};

export const processAudioAi = async (inputPath, outputDir) => {
    console.log(`Starting AI separation for ${inputPath} to ${outputDir}`);

    // Ensure output directory exists
    await fs.mkdir(outputDir, { recursive: true });

    // Run Spleeter
    // -p spleeter:4stems separates into: vocals, drums, bass, other
    const cmd = `spleeter separate -p spleeter:4stems -o "${outputDir}" "${inputPath}"`;

    console.log(`Running command: ${cmd}`);
    const { stdout, stderr } = await execAsync(cmd);
    console.log('Spleeter output:', stdout);
    if (stderr) console.error('Spleeter stderr:', stderr);

    // Spleeter creates a subdirectory named after the input filename (without extension)
    const filename = path.parse(inputPath).name;
    const spleeterOutDir = path.join(outputDir, filename);

    // We need to move/rename files to expected flat structure or mapping
    // Expected files: vocals.wav, drums.wav, bass.wav, other.wav
    const stems = ['vocals', 'drums', 'bass', 'other'];
    const results = [];

    for (const stem of stems) {
        const sourceFile = path.join(spleeterOutDir, `${stem}.wav`);
        const targetFile = path.join(outputDir, `${stem}.mp3`); // Spleeter outputs WAV by default, we might rename or convert

        // Check if source exists
        try {
            await fs.access(sourceFile);
            // Rename to flat structure
            await fs.rename(sourceFile, targetFile);
            results.push({ stem, filename: `${stem}.mp3` });
        } catch (e) {
            console.warn(`Missing stem: ${stem} in ${spleeterOutDir}`);
        }
    }

    return results;
};
