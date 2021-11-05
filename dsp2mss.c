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
    printf("Usage: %s music.dsp [music2.dsp...]\n", name);
};

void openerror(char* name) {
    printf("error opening %s\n", name);
};

int convert(char* inName) {
    // Input/output file pointers
    FILE* p_inFile;
    FILE* p_outFile;

    // Headers
    char ourHead[0xC0] = { 0 };
    // basic mono DSP subheader (should be empty)
    const char dspHead[0x20] = { 0 };
    // basic stereo DSP subheader
    const char sdspHead[0x20] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x04, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };
    // MSS subheader (from TS2 mss files)
    const char mssHead[0x20] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x53, 0x00, 0x60, 0xA9, 0x40, 0x00,
    0x64, 0xFF, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };
    char inBuffer[0x4000] = { 0 };
    char outBuffer[0x4000] = { 0 };
    char outName[FILENAME_MAX] = { 0 };

    // Change file extension of input file to .mss for output file
    strcpy(outName, inName);
    const char ext[] = ".mss";
    char* p_ext;
    p_ext = strrchr(outName, *ext);
    sprintf(p_ext, ext);

    printf("\nInput file:  %s\nOutput file: %s\n", inName, outName);

    if (!(p_inFile = fopen(inName, "rb"))) { openerror(inName); return 1; };
    if (!(p_outFile = fopen(outName, "wb"))) { openerror(outName); return 1; };

    printf("Checking DSP header... ");
    int check = 0;
    fread(ourHead, sizeof(ourHead), 1, p_inFile);
    check += memcmp(ourHead + 0x40, sdspHead, sizeof(dspHead));
    if (check != 0) { printf("left channel header not basic DSP format!"); return 1; };
    check += memcmp(ourHead + 0xA0, sdspHead, sizeof(dspHead));
    if (check != 0) { printf("right channel header not basic DSP format or mono file!"); return 1; };

    printf("done!\nPatching header for MSS format...");
    // Fix header for MSS
    memcpy(ourHead + 0x40, mssHead, sizeof(mssHead));
    memcpy(ourHead + 0xA0, mssHead, sizeof(mssHead));
    // Write header to output file
    fwrite(ourHead, sizeof(ourHead), 1, p_outFile);

    printf("done!\nRe-interleaving audio... ");

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

    printf("done!\nConverted successfully.\n");

    fclose(p_inFile);
    fclose(p_outFile);

    return 0;
};

int main(int argc, char* argv[]) {

    printf("\ndsp2mss - VGAudio stereo DSP to stereo MSS converter\nAuthor: stealthii\nSite: https://timesplitters.dev\n\n");

    // Usage help
    if (argc < 2) { usage(argv[0]); return 1; }

    // Convert all files passed as arguments
    int ret = 0;
    for (char** p_argv = argv + 1; *p_argv != argv[argc]; p_argv++) {
        ret += convert(*p_argv);
    };

    return ret;
};
