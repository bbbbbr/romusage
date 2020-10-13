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


#define MAX_AREA_COUNT    1000
#define ERR_NO_AREAS_LEFT -1
#define AREA_VAL_UNSET   0xFFFFFFFF

#define NOI_REC_START_LEN 7 // length of "DEF l__"
#define NOI_REC_START   's'
#define NOI_REC_LENGTH  'l'


static area_item area_list[MAX_AREA_COUNT];
static int area_count = 0;


int areas_process_map_file(char * filename_in) {

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



// Example data to parse from a .map file (excluding unwanted lines):
/*
DEF l__BSS 0x41
DEF l__HEADERe 0x75
DEF l__HOME 0xED
DEF l__GSINIT 0x1F9
DEF s__CODE 0x200
DEF l__BASE 0x2A3
DEF l__DATA 0x1F20
DEF l__CODE 0x6A62
DEF s__HOME 0x6C62
DEF s__BASE 0x6D4F
*/



// Find a matching area, if none matches a new one is added and returned
static int arealist_get_id_by_name(char * area_name) {
    int c;

    // Check for matching area name
    for(c=0;c < area_count; c++) {
        // Return matching area index if present
        if (strncmp(area_name, area_list[c].name, AREA_MAX_STR) == 0) {
            return c;
        }
    }

    // no match was found, add area if possible
    if (area_count < MAX_AREA_COUNT) {
        snprintf(area_list[area_count].name, sizeof(area_list[area_count].name), "%s", area_name);
        area_list[area_count].start  = AREA_VAL_UNSET;
        area_list[area_count].end    = AREA_VAL_UNSET;
        area_list[area_count].length = AREA_VAL_UNSET;
        area_count++;
        return (area_count - 1);
    }

    return ERR_NO_AREAS_LEFT;
}


// Process list of areas and add them to banks
static void noi_arealist_add_all_to_banks() {

    int c;

    // Only process completed areas (start and length both set)
    for(c=0;c < area_count; c++) {

        if ((area_list[c].start != AREA_VAL_UNSET) &&
            (area_list[c].length != AREA_VAL_UNSET)) {
            area_list[c].end = area_list[c].start + area_list[c].length - 1;
            banks_check(area_list[c]);
        }
    }
}

int areas_process_noi_file(char * filename_in) {

    char cols;
    char * p_str;
    char * p_words[MAX_SPLIT_WORDS];
    char strline_in[MAX_STR_LEN] = "";
    FILE * noi_file = fopen(filename_in, "r");
    area_item area;
    int area_id;

    if (noi_file) {

        // Read one line at a time into \0 terminated string
        while ( fgets(strline_in, sizeof(strline_in), noi_file) != NULL) {

            // Require minimum length to match
            if (strlen(strline_in) >= NOI_REC_START_LEN) {

                // Match either _S_egment or _L_ength records
                if ( (strncmp(strline_in, "DEF l__", NOI_REC_START_LEN) == 0) ||
                     (strncmp(strline_in, "DEF s__", NOI_REC_START_LEN) == 0)) {


                    // Split string into words separated by spaces
                    cols = 0;
                    p_str = strtok(strline_in," _");
                    while (p_str != NULL)
                    {
                        p_words[cols++] = p_str;
                        // Only split on underscore for the second match
                        if (cols == 1)
                            p_str = strtok(NULL, " _");
                        else
                            p_str = strtok(NULL, " ");
                        if (cols >= MAX_SPLIT_WORDS) break;
                    }

                    if (cols == 4) {
                        if ( !(strstr(p_words[2], "SFR")) &&        // Exclude SFR areas (not actually located at addresses in area listing)
                             !(strstr(p_words[2], "HRAM10")) ) {    // Exclude HRAM area

                            int area_id = arealist_get_id_by_name(p_words[2]); // [2] Area Name1
                            if (area_id != ERR_NO_AREAS_LEFT) {

                                // Handle whether it's a start-of-address or a length record for the given area
                                if (p_words[1][0] == NOI_REC_START)
                                    area_list[area_id].start = strtol(p_words[3], NULL, 16); // [2] Area Hex Address Start
                                else if (p_words[1][0] == NOI_REC_LENGTH) {
                                    // Don't add lengths of zero
                                    if (strtol(p_words[3], NULL, 16) > 0)
                                        area_list[area_id].length = strtol(p_words[3], NULL, 16); // [2] Area Hex Length
                                }
                            }
                        }
                    }
                } // end: if valid start of line
            } // end: valid min chars to process line

        } // end: while still lines to process

        fclose(noi_file);

        // Process all the areas
        noi_arealist_add_all_to_banks();

    } // end: if valid file
    else return (false);

   return true;
}
