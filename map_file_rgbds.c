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
#include "map_file_rgbds.h"

// Example data to parse from a RGBDS .map file (excluding unwanted lines):
/*
  SECTION: $0000-$0000 ($0000 bytes) [name]
ROM0 bank #0:
  SECTION: $0000-$0060 ($0061 bytes) ["Vectors"]
ROMX bank #1:
  SECTION: $4000-$7eff ($3f00 bytes) ["AnotherBank"]
  SECTION: $7f00-$7f07 ($0008 bytes) ["OAM DMA routine"]
*/

#define MAX_STR_LEN     4096
#define MAX_STR_AREALEN 20
#define MAX_SPLIT_WORDS  6
#define BANK_SPLIT_WORDS 3
#define SECT_SPLIT_WORDS 6
#define BANK_NUM_UNSET 0xFFFFFFFF


static int str_split(char * str_check, char * p_words[], const char * split_criteria) {

    char cols;
    char * p_str;

    cols = 0;
    p_str = strtok (str_check,split_criteria);
    while (p_str != NULL)
    {
        p_words[cols++] = p_str;
        p_str = strtok(NULL, split_criteria);
        if (cols >= MAX_SPLIT_WORDS) break;
    }

    return (cols);
}


int map_file_rgbds_process_areas(char * filename_in) {

    char * p_words[MAX_SPLIT_WORDS];
    char strline_in[MAX_STR_LEN] = "";
    uint32_t current_bank;
    FILE * map_file = fopen(filename_in, "r");
    area_item area;

    current_bank = BANK_NUM_UNSET;

    if (map_file) {

        // Read one line at a time into \0 terminated string
        while ( fgets(strline_in, sizeof(strline_in), map_file) != NULL) {

            // Bank lines precede Section lines, use them to set bank num
            if (strstr(strline_in, " bank #")) {
                // Set new bank number for following section if valid
                if (str_split(strline_in,p_words," #:\r\n") == BANK_SPLIT_WORDS) {
                    // Exclude sections following HRAM bank nums by unsetting the bank number
                    if (strstr(p_words[0], "HRAM"))
                        current_bank = BANK_NUM_UNSET;
                    else
                        current_bank = strtol(p_words[2], NULL, 10);
                }
            }
            // Don't process any section areas until bank number has been set
            else if (current_bank == BANK_NUM_UNSET)
                continue;

            // Otherwise only parse lines that have Section (Area) summary info
            else if (!strstr(strline_in, "  SECTION: "))
                continue;

            else if (str_split(strline_in,p_words," :$()[]\n\t\"") == SECT_SPLIT_WORDS) {
                snprintf(area.name, sizeof(area.name), "%s", p_words[5]);   // [5] Area Name
                area.start = strtol(p_words[1], NULL, 16) | (current_bank << 16);   // [1] Area Hex Address Start
                area.end   = strtol(p_words[2], NULL, 16) | (current_bank << 16);   // [2] Area Hex Address End
                area.exclusive = option_all_areas_exclusive; // Default is false
                banks_check(area);
            }
        } // end: while still lines to process

        fclose(map_file);

    } // end: if valid file
    else return (false);

   return true;
}



            // // Split string into words separated by spaces/etc
            // cols = 0;
            // p_str = strtok (strline_in," :$()[]");
            // while (p_str != NULL)
            // {
            //     p_words[cols++] = p_str;
            //     p_str = strtok(NULL, " :$()[]");
            //     if (cols >= MAX_SPLIT_WORDS) break;
            // }

            // if (cols == 6) {
            //     if ((strtol(p_words[2], NULL, 16) > 0))
            //      // &&  // Exclude empty areas
            //      //    !(strstr(p_words[0], "SFR")) &&        // Exclude SFR areas (not actually located at addresses in area listing)
            //      //    !(strstr(p_words[0], "HRAM"))          // Exclude HRAM area
            //      //    )
