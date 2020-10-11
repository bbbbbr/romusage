// This is free and unencumbered software released into the public domain.
// For more information, please refer to <https://unlicense.org>
// bbbbbr 2020

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>

#include "banks.h"
#include "areas.h"

// Example data to parse from a .sym file (excluding unwanted lines):
/*
_CODE                  00000200    00006A62 =       27234. bytes (REL,CON)
_CODE                  00000200    00006A62 =       27234. bytes (REL,CON)
_HOME                  00006C62    000000ED =         237. bytes (REL,CON)
_BASE                  00006D4F    000002A3 =         675. bytes (REL,CON)
_GSINIT                00006FF2    000001F9 =         505. bytes (REL,CON)
_GSINITTAIL            000071EB    00000001 =           1. bytes (REL,CON)
_DATA                  0000C0A0    00001684 =        5764. bytes (REL,CON)
_BSS                   0000D724    00000041 =          65. bytes (REL,CON)
_HEAP                  0000D765    00000000 =           0. bytes (REL,CON)
_HRAM10                00000000    00000001 =           1. bytes (ABS,CON)
*/


int areas_process_map_file(char * filename_in) {

    char cols;
    char * p_str;
    char * p_words[MAX_SPLIT_WORDS];
    char strline_in[MAX_STR_LEN] = "";
    FILE * map_file = fopen(filename_in, "r");

    if (map_file) {

        // Read one line at a time into \0 terminated string
        while ( fgets(strline_in, sizeof(strline_in), map_file) != NULL) {

            // Only parse lines that start with '_' character (Area summary lines)
            if (strline_in[0] == '_') {

                // Split string into words separated by spaces
                cols = 0;
                p_str = strtok (strline_in," =.");
                while (p_str != NULL)
                {
                    p_words[cols++] = p_str;
                    p_str = strtok(NULL, " =.");
                    if (cols >= MAX_SPLIT_WORDS) break;
                }

                if (cols == 6) {
                    printf(">> %s  %s  %s \n", p_words[0],
                                               p_words[1],
                                               p_words[2]);


// TODO!!!!!!!!!!! Prevent duplicates in 32K CODE mode, -> Log all entries and suppress duplciates from MULTI_PAGINATION
// collect all entries
// merge any that overlap
// then process them

// TODO: handle headers/etc better. Filter them out for now
                    if ((strtol(p_words[1], NULL, 16) != 0x00000000) &&
                       (strtol(p_words[2], NULL, 16) != 0))  // Size != 0
                    {
                        banks_check( strtol(p_words[1], NULL, 16),  // [1] Area Hex Address Start
                                     strtol(p_words[1], NULL, 16) +
                                     strtol(p_words[2], NULL, 16) - 1); // [1] + [3] Hex Size - 1 = Area End
                    }
                }
            } // end: if valid start of line

        } // end: while still lines to process

        fclose(map_file);

    } // end: if valid file
    else return (false);

   return true;
}
