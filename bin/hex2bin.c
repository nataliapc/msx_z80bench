#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int hex2bin(const char *infile, const char *outfile) {
    FILE *in = fopen(infile, "r");
    FILE *out = fopen(outfile, "wb");
    char line[256];
    int addr = 0;
    
    if (!in || !out) return 1;
    
    while (fgets(line, sizeof(line), in)) {
        if (line[0] != ':') continue;
        
        int len = (int)strtol((char[]){line[1], line[2], '\0'}, NULL, 16);
        int type = (int)strtol((char[]){line[7], line[8], '\0'}, NULL, 16);
        
        if (type == 0) {  // Data record
            for (int i = 0; i < len; i++) {
                char hex[3] = {line[9 + i*2], line[10 + i*2], '\0'};
                unsigned char byte = (unsigned char)strtol(hex, NULL, 16);
                fwrite(&byte, 1, 1, out);
            }
        } else if (type == 1) {  // EOF record
            break;
        }
    }
    
    fclose(in);
    fclose(out);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: hex2bin [-e ext] input.ihx\n");
        return 1;
    }
    
    char *infile = NULL;
    char *extension = "bin";
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-e") == 0 && i+1 < argc) {
            extension = argv[++i];
        } else {
            infile = argv[i];
        }
    }
    
    if (!infile) return 1;
    
    char outfile[256];
    strcpy(outfile, infile);
    char *dot = strrchr(outfile, '.');
    if (dot) *dot = '\0';
    strcat(outfile, ".");
    strcat(outfile, extension);
    
    return hex2bin(infile, outfile);
}
