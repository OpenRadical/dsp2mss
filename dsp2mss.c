#include <stdio.h>

// dsp2mss
// convert a mono, devkit-standard gamecube DSP into stereo MSS format

unsigned long read32(unsigned char* inWord) {
    return ((unsigned long)(inWord[0]) << 24) |
        ((unsigned long)(inWord[1]) << 16) |
        ((unsigned long)(inWord[2]) << 8) |
        inWord[3];
};

void usage(char* name) {
    printf("Usage: %s LEFT.DSP [-s RIGHT.DSP] OUTPUT.MSS\n", name);
};

void openerror(char* name) {
    printf("error opening %s\n", name);
};

int main(int argc, char** argV) {

    FILE* p_inFile;
    FILE* p_inFile2 = NULL;
    FILE* p_outFile;

    char inHead[0x60] = { 0 };
    char outHead[0xC0] = { 0 };
    // MSS subheader (from TS2 mss files)
    char mssHead[0x20] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x53, 0x00, 0x60, 0xA9, 0x40, 0x00,
    0x64, 0xFF, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };
    // Interleave size (4kB)
    unsigned char buffer[0x1000 * 16];

    printf("dsp2mss - GC DSP mono to MSS stereo converter\nby stealthii\n\n");

    if (argc < 2 || argc > 5) { usage(argV[0]); return 1; }

    unsigned long i = 0;
    if (!(p_inFile = fopen(argV[1], "rb"))) { openerror(argV[1]); return 1; }
    if (argV[2][0] == '-' && argV[2][1] == 's') {
        if (!(p_inFile2 = fopen(argV[3], "rb"))) { openerror(argV[3]); return 1; }
        i += 2;
    };
    if (!(p_outFile = fopen(argV[2 + i], "wb"))) { openerror(argV[2 + i]); return 1; }

    printf("creating header...");

    // Left Channel Header
    fread(inHead, sizeof(inHead), 1, p_inFile);
    memcpy(outHead, inHead, sizeof(inHead));
    memcpy(outHead + 0x40, mssHead, sizeof(mssHead));

    // Right Channel Header
    if (p_inFile2) fread(inHead, sizeof(inHead), 1, p_inFile2);
    memcpy(outHead + 0x60, inHead, sizeof(inHead));
    memcpy(outHead + 0xA0, mssHead, sizeof(mssHead));

    // Write header to output file
    fwrite(outHead, sizeof(outHead), 1, p_outFile);

    printf("done\ninterleaving mono->stereo...");

    for (unsigned long c = 0; c < (read32(outHead + 0x04) / 2) / 0x1000 + 1; c++) {
        fread(buffer, 0x1000, 1, p_inFile);
        fwrite(buffer, 0x1000, 1, p_outFile);
        if (p_inFile2) fread(buffer, 0x1000, 1, p_inFile2);
        fwrite(buffer, 0x1000, 1, p_outFile);
    };

    printf("done\n");

    fclose(p_inFile);
    if (p_inFile2) fclose(p_inFile2);
    fclose(p_outFile);

    return 0;
};
