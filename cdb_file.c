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
#include "cdb_file.h"


static area_item symbol_list[CDB_MAX_AREA_COUNT]; // TODO: dynamic reallocation
static int symbol_count = 0;


// Example data to parse from a .cdb file:


// 0 1 2---------- 3-- 4 5----
// L:G$big_const_4$0_0$0:24039 <-- bank 2
// L:G$big_const_3$0_0$0:1784E <-- Bank 1

// 0 1 2---------- 3-- 4  5-- 6----- 7  8  9 10 11
// S:G$big_const_3$0_0$0({256}DA256d,SC:U),D,0,0     <--- size 256 bytes (SC=signed char)
// S:G$big_const_4$0_0$0({15360}DA15360d,SC:U),D,0,0 <--- size 15360 bytes (SC=signed char)

// Address Space field (S: record)
// A   External stack
// B   Internal stack
// C   Code
// D   Code / static segment
// E   Internal ram (lower 128) bytes
// F   External ram
// G   Internal ram
// H   Bit addressable
// I   SFR space
// J   SBIT space
// R   Register space
// Z   Used for function records, or any undefined space code 

// Types in a Section 4.4 Type Chain Record (S: record)
// DA <n>  Array of n elements
// DF  Function
// DG  Generic pointer
// DC  Code pointer
// DX  External ram pointer
// DD  Internal ram pointer
// DP  Paged pointer
// DI  Upper 128 byte pointer
// SL  long
// SI  int
// SC  char
// SS  short
// SV  void
// SF  float
// ST <name>   Structure of name <name>
// SX  sbit
// SB <n>  Bit field of <n> bits 






// Find a matching symbol, if none matches a new one is added and returned
static int symbollist_get_id_by_name(char * symbol_name) {
    int c;

    // Check for matching symbol name
    for(c=0;c < symbol_count; c++) {
        // Return matching symbol index if present
        if (strncmp(symbol_name, symbol_list[c].name, AREA_MAX_STR) == 0) {
            return c;
        }
    }

    // no match was found, add symbol if possible
    if (symbol_count < CDB_MAX_AREA_COUNT) {
        snprintf(symbol_list[symbol_count].name, sizeof(symbol_list[symbol_count].name), "%s", symbol_name);
        symbol_list[symbol_count].start  = AREA_VAL_UNSET;
        symbol_list[symbol_count].end    = AREA_VAL_UNSET;
        symbol_list[symbol_count].length = AREA_VAL_UNSET;
        if (strstr(symbol_name,"HEADER"))
            symbol_list[symbol_count].exclusive = false; // HEADER symbols almost always overlap, ignore them
        else
            symbol_list[symbol_count].exclusive = option_all_areas_exclusive; // Default is false
        symbol_count++;
        return (symbol_count - 1);
    }

    return ERR_NO_AREAS_LEFT;
}


// Process list of symbols and add them to banks
static void cdb_symbollist_add_all_to_banks() {

    int c;

    // Only process completed symbols (start and length both set)
    for(c=0;c < symbol_count; c++) {

        if ((symbol_list[c].start != AREA_VAL_UNSET) &&
            (symbol_list[c].length != AREA_VAL_UNSET)) {
            symbol_list[c].end = symbol_list[c].start + symbol_list[c].length - 1;
            banks_check(symbol_list[c]);
        }
    }
}

int cdb_file_process_symbols(char * filename_in) {

    char cols;
    char * p_str;
    char * p_words[CDB_MAX_SPLIT_WORDS];
    char strline_in[CDB_MAX_STR_LEN] = "";
    FILE * cdb_file = fopen(filename_in, "r");
    area_item symbol;
    int symbol_id;

    if (cdb_file) {

        // Read one line at a time into \0 terminated string
        while ( fgets(strline_in, sizeof(strline_in), cdb_file) != NULL) {

            // Require minimum length to match
            if (strlen(strline_in) >= CDB_REC_START_LEN) {
printf("%s", strline_in);
                // Match either _S_egment or _L_ength records
                if ( (strncmp(strline_in, "L:", CDB_REC_START_LEN) == 0) ||
                     (strncmp(strline_in, "S:", CDB_REC_START_LEN) == 0)) {

                    // Split string into words separated by spaces
                    cols = 0;
                    p_str = strtok(strline_in,":$({}),");
                    while (p_str != NULL)
                    {
                        p_words[cols++] = p_str;
                        // // Only split on underscore for the second match
                        // if (cols == 1)
                        //     p_str = strtok(NULL, ":$");
                        // else
                            p_str = strtok(NULL, ":$({}),");
                        if (cols >= CDB_MAX_SPLIT_WORDS) break;
                    }

// extract name
                    // Linker record (start address)
                    if ((p_words[0][0] == CDB_REC_L) &&
                        (cols == CDB_REC_L_COUNT_MATCH)) {

// printf(" %s %s %s\n", p_words[0], p_words[2], p_words[5]);
                        int symbol_id = symbollist_get_id_by_name(p_words[2]); // [2] Area Name1
                        if (symbol_id != ERR_NO_AREAS_LEFT) {
                            symbol_list[symbol_id].start = strtol(p_words[5], NULL, 16);   // [5]: Start address with 0x0xFF0000 as bank mask
printf(" (%d) --> %s %s %s\n", symbol_id, p_words[0], p_words[2], p_words[5]);
                        }
                    }

                    // Sybol record (length)
                    if ((p_words[0][0] == CDB_REC_S) &&
                        (cols == CDB_REC_S_COUNT_MATCH)) {
                        // Only allow certain address spaces
                        if ((p_words[9][0] == 'C') || // Address Space: Code
                            (p_words[9][0] == 'D') || // Address Space: Code / static segment
                            (p_words[9][0] == 'E') || // Address Space: Internal RAM (lower 128) bytes
                            (p_words[9][0] == 'F') || // Address Space: External RAM
                            (p_words[9][0] == 'G')) { // Address Space: Internal RAM
                            // Exclude zero length entries
                            if (strtol(p_words[5], NULL, 10) > 2) { //  0) {
                                int symbol_id = symbollist_get_id_by_name(p_words[2]); // [2] Area Name1
                                if (symbol_id != ERR_NO_AREAS_LEFT) {
                                    symbol_list[symbol_id].length = strtol(p_words[5], NULL, 10); // [2] Symbol decimal length
printf(" (%d) --> %s %s %s %s\n", symbol_id, p_words[0], p_words[2], p_words[5], p_words[9]);
// int t;
// for(t=0;t<cols;t++) printf("%d:--%s--\n", t, p_words[t]);
// printf("----> %s\n",  p_words[9]);
                                }
                            }
                        }
                    }

                } // end: if valid start of line
            } // end: valid min chars to process line

        } // end: while still lines to process

        fclose(cdb_file);

        // Process all the symbols
        cdb_symbollist_add_all_to_banks();

    } // end: if valid file
    else return (false);

   return true;
}
