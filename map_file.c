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
#include "map_file.h"

// Example data to parse from a .map file (excluding unwanted lines):
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

int map_file_process_areas(char * filename_in) {

    char cols;
    char * p_str;
    char * p_words[MAX_SPLIT_WORDS];
    char strline_in[MAX_STR_LEN] = "";
    FILE * map_file = fopen(filename_in, "r");
    area_item area;

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
                    if ((strtol(p_words[2], NULL, 16) > 0) &&  // Exclude empty areas
                        !(strstr(p_words[0], "SFR")) &&        // Exclude SFR areas (not actually located at addresses in area listing)
                        !(strstr(p_words[0], "HRAM10"))        // Exclude HRAM area
                        )
                    {
                        snprintf(area.name, sizeof(area.name), "%s", p_words[0]); // [0] Area Name
                        area.start = strtol(p_words[1], NULL, 16);         // [1] Area Hex Address Start
                        area.end   = area.start + strtol(p_words[2], NULL, 16) - 1; // Start + [3] Hex Size - 1 = Area End
                        area.exclusive = false;
                        banks_check(area);
                    }
                }
            } // end: if valid start of line

        } // end: while still lines to process

        fclose(map_file);

    } // end: if valid file
    else return (false);

   return true;
}

