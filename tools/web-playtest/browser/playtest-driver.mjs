import { mkdir, writeFile } from "node:fs/promises";
import path from "node:path";
import process from "node:process";

import { runPlaytest } from "./test-playtest.mjs";

function parseArguments(arguments_) {
    const options = {};
    for (let index = 0; index < arguments_.length; index += 2) {
        const name = arguments_[index];
        const value = arguments_[index + 1];
        if (
            ![
                "--input-sequence",
                "--runtime-directory",
                "--trace-output",
            ].includes(name)
            || !value
            || value.startsWith("--")
        ) {
            throw new Error(`Invalid playtest-driver argument: ${name}`);
        }
        if (name === "--input-sequence") {
            options.inputSequence = path.resolve(value);
        } else if (name === "--runtime-directory") {
            options.runtimeDirectory = path.resolve(value);
        } else {
            options.traceOutput = path.resolve(value);
        }
    }
    for (const required of [
        "inputSequence",
        "runtimeDirectory",
        "traceOutput",
    ]) {
        if (!options[required]) {
            throw new Error(`Missing required driver option: ${required}`);
        }
    }
    return Object.freeze(options);
}

const options = parseArguments(process.argv.slice(2));
const result = await runPlaytest({
    completeForTrace: true,
    inputSequence: options.inputSequence,
    runtimeDirectory: options.runtimeDirectory,
});
await mkdir(path.dirname(options.traceOutput), { recursive: true });
await writeFile(options.traceOutput, result.traceBytes);
process.stdout.write(`Wrote canonical Wasm trace: ${options.traceOutput}\n`);
