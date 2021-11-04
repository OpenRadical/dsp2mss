#include <stdio.h>
#include <string.h>

// dsp2mss
// convert a stereo DSP from VGAudio into stereo MSS format

unsigned long read32(unsigned char* inWord) {
    return ((unsigned long)(inWord[0]) << 24) |
        ((unsigned long)(inWord[1]) << 16) |
        ((unsigned long)(inWord[2]) << 8) |
        inWord[3];
};

void usage(char* name) {
    printf("Usage: %s music.dsp [music.mss]\n", name);
};

void openerror(char* name) {
    printf("error opening %s\n", name);
};

int main(int argc, char** argV) {

    // Input/output file pointers
    FILE* p_inFile;
    FILE* p_outFile;

    // Headers
    char ourHead[0xC0] = { 0 };
    // MSS subheader (from TS2 mss files)
    const char mssHead[0x20] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x53, 0x00, 0x60, 0xA9, 0x40, 0x00,
    0x64, 0xFF, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };
    char inBuffer[0x4000] = { 0 };
    char outBuffer[0x4000] = { 0 };

    printf("dsp2mss - VGAudio stereo DSP to stereo MSS converter\nby stealthii\nhttps://timesplitters.dev\n\n");

    // Usage help
    if (argc < 2 || argc > 3) { usage(argV[0]); return 1; }

    char outName[FILENAME_MAX] = { 0 };
    if (argc == 2) {
        // Change file extension of input file to .mss for output file
        strcpy(outName, argV[1]);
        const char ext[] = ".mss";
        char* p_ext;
        p_ext = strrchr(outName, *ext);
        sprintf(p_ext, ext);
    }
    else {
        // Use second argument as output filename
        strcpy(outName, argV[2]);
    };

    if (!(p_inFile = fopen(argV[1], "rb"))) { openerror(argV[1]); return 1; }
    if (!(p_outFile = fopen(outName, "wb"))) { openerror(argV[2]); return 1; }

    printf("creating header...");

    fread(ourHead, sizeof(ourHead), 1, p_inFile);
    // Fix header for MSS
    memcpy(ourHead + 0x40, mssHead, sizeof(mssHead));
    memcpy(ourHead + 0xA0, mssHead, sizeof(mssHead));

    // Write header to output file
    fwrite(ourHead, sizeof(ourHead), 1, p_outFile);

    printf("done!\nre-interleaving stereo for DSP to MSS conversion...");

    // Interleave all but last chunk
    for (unsigned long c = 0; c < (read32(ourHead + 0x04)) / 0x4000; c++) {
        fread(inBuffer, 0x4000, 1, p_inFile);
        memcpy(outBuffer, inBuffer, 0x1000);
        memcpy(outBuffer + 0x1000, inBuffer + 0x2000, 0x1000);
        memcpy(outBuffer + 0x2000, inBuffer + 0x1000, 0x1000);
        memcpy(outBuffer + 0x3000, inBuffer + 0x3000, 0x1000);
        fwrite(outBuffer, 0x4000, 1, p_outFile);
    };

    /*
    * For our last chunk...
    *
    * VGaudio interleaves all channels in 8kB chunks, until the last two.
    * If these are shorter (likely), they just get appended to each other.
    * I don't know if that's correct in their DSP conversion, but it's not
    * correct for MSS files, which have full 4k padding.
    *
    * We'll calculate the length of the last chunk, split, and interleave if
    * necessary.
    *
    * NOTE: If the input is fully padded, this should also work as we use the
    * file length.
    */

    long remain = ftell(p_inFile);         // current position
    fseek(p_inFile, 0, SEEK_END);          // go to end
    remain = ftell(p_inFile) - remain;     // remaining bytes
    fseek(p_inFile, -remain, SEEK_END);    // return to offset

    memset(inBuffer, 0, sizeof(inBuffer)); // clearing input buffer
    fread(outBuffer, 0x4000, 1, p_inFile); // cheeky borrow of outBuffer
    // copy L/R chunks to the right offset
    memcpy(inBuffer, outBuffer, remain / 2);
    memcpy(inBuffer + 0x2000, outBuffer + remain / 2, remain / 2);
    // write out our final buffer
    memcpy(outBuffer, inBuffer, 0x1000);
    memcpy(outBuffer + 0x1000, inBuffer + 0x2000, 0x1000);
    memcpy(outBuffer + 0x2000, inBuffer + 0x1000, 0x1000);
    memcpy(outBuffer + 0x3000, inBuffer + 0x3000, 0x1000);
    // write out final chunk
    fwrite(outBuffer, 0x4000, 1, p_outFile);

    printf("done!\nOutput to %s.", outName);

    fclose(p_inFile);
    fclose(p_outFile);

    return 0;
};
