// This is free and unencumbered software released into the public domain.
// For more information, please refer to <https://unlicense.org>
// bbbbbr 2020

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "common.h"
#include "list.h"
#include "banks.h"
#include "banks_print.h"


#ifdef _WIN32
  #ifndef _WIN32
     #define __WIN32__
  #endif
#endif



// Ascii art style characters
// Print a block character to stdout based on the percentage used (int 0 - 100)
static void print_graph_char_asciistyle(uint32_t perc_used) {

    #ifdef __WIN32__
        // https://en.wikipedia.org/wiki/Code_page_437
        // https://sourceforge.net/p/mingw/mailman/message/14065664/
        // Code Page 437 (appears to be default for windows console, 
        if      (perc_used >= 95) fprintf(stdout, "%c", 219u); // Full  Shade Block
        else if (perc_used >= 75) fprintf(stdout, "%c", 178u); // Dark  Shade Block
        else if (perc_used >= 50) fprintf(stdout, "%c", 177u); // Med   Shade Block
        else if (perc_used >= 25) fprintf(stdout, "%c", 176u); // Light Shade Block
        else                      fprintf(stdout, "."); // "_");

        // All Blocks        
        // if      (perc_used >= 99) fprintf(stdout, "%c", 219u); // Full  Shade Block
        // else if (perc_used >= 75) fprintf(stdout, "%c", 178u); // Dark  Shade Block
        // else if (perc_used >= 25) fprintf(stdout, "%c", 177u); // Med   Shade Block
        // else                      fprintf(stdout, "%c", 176u); // Light Shade Block


    #else // Non-Windows
        // https://www.fileformat.info/info/unicode/block/block_elements/utf8test.htm
        // https://www.fileformat.info/info/unicode/char/2588/index.htm
        if      (perc_used >= 95) fprintf(stdout, "%s", u8"\xE2\x96\x88"); // Full         Block in UTF-8 0xE2 0x96 0x88 (e29688)
        else if (perc_used >= 75) fprintf(stdout, "%s", u8"\xE2\x96\x93"); // Dark Shade   Block in UTF-8 0xE2 0x96 0x93 (e29693)
        else if (perc_used >= 50) fprintf(stdout, "%s", u8"\xE2\x96\x92"); // Med Shade    Block in UTF-8 0xE2 0x96 0x92 (e29692)
        else if (perc_used >= 25) fprintf(stdout, "%s", u8"\xE2\x96\x91"); // Light Shade  Block in UTF-8 0xE2 0x96 0x91 (e29691)
        else                      fprintf(stdout, "%s", u8"."); //  u8"_"

        // All Blocks
        // if      (perc_used >= 95) fprintf(stdout, "%s", u8"\xE2\x96\x88"); // Full         Block in UTF-8 0xE2 0x96 0x88 (e29688)
        // else if (perc_used >= 75) fprintf(stdout, "%s", u8"\xE2\x96\x93"); // Dark Shade   Block in UTF-8 0xE2 0x96 0x93 (e29693)
        // else if (perc_used >= 25) fprintf(stdout, "%s", u8"\xE2\x96\x92"); // Med Shade    Block in UTF-8 0xE2 0x96 0x92 (e29692)
        // else                      fprintf(stdout, "%s", u8"\xE2\x96\x91"); // Light Shade  Block in UTF-8 0xE2 0x96 0x91 (e29691)
    #endif
}


// Standard character style
// Print a block character to stdout based on the percentage used (int 0 - 100)
static void print_graph_char_standard(uint32_t perc_used) {

    char ch;

    if (perc_used > 95)      ch = '#';
    else if (perc_used > 25) ch = '-';
    else                     ch = '.';
    fprintf(stdout, "%c", ch);
}


static void bank_print_graph(bank_item * p_bank, uint32_t num_chars) {

    int c;
    uint32_t range_size = RANGE_SIZE(p_bank->start, p_bank->end) / num_chars;
    uint32_t perc_used;

    for (c = 0; c <= (num_chars - 1); c++) {

        perc_used = (bank_areas_calc_used(p_bank,
                                          p_bank->start + (c * range_size),
                                          p_bank->start + ((c + 1) * range_size) - 1)
                                           * (uint32_t)100) / range_size;

        // Non-ascii style output
        if (!get_option_display_asciistyle())
            print_graph_char_standard(perc_used);
        else
            print_graph_char_asciistyle(perc_used);

        // Periodic line break if needed (for multi-line large graphs)
        if (((c + 1) % 64) == 0)
            fprintf(stdout, "\n");
    }
}


// Show a large usage graph for each bank
// Currently 16 bytes per character
static void banklist_print_large_graph(list_type * p_bank_list) {

    bank_item * banks = (bank_item *)p_bank_list->p_array;
    int c;

        for (c = 0; c < p_bank_list->count; c++) {

            fprintf(stdout,"\n\nStart: %s  ",banks[c].name); // Name
            fprintf(stdout,"0x%04X -> 0x%04X",banks[c].start,
                                              banks[c].end); // Address Start -> End
            fprintf(stdout,"\n"); // Name

            bank_print_graph(&banks[c], banks[c].size_total / LARGEGRAPH_BYTES_PER_CHAR);

            fprintf(stdout,"End: %s\n",banks[c].name); // Name
        }
}


// Display all areas for a bank
static void bank_print_area(bank_item *p_bank) {

    area_item * areas;
    int b;
    int hidden_count = 0;
    uint32_t hidden_total = 0;

    for(b = 0; b < p_bank->area_list.count; b++) {

        // Load the area list for the bank
        areas = (area_item *)p_bank->area_list.p_array;

        if (b == 0) {
            fprintf(stdout,"|\n");

            // Only show sub column headers for CDB output since there are a lot more areas
            if (get_option_input_source() == OPT_INPUT_SRC_CDB)
                fprintf(stdout,
                           "| Name                            Start  -> End      Size \n"
                           "| ---------------------           ----------------   -----\n");
        }

        // Don't display headers unless requested
        if ((banks_display_headers) || !(strstr(areas[b].name,"HEADER"))) {

            // Optionally hide areas below a given size
            if (areas[b].length >= get_option_area_hide_size()) {

                fprintf(stdout,"+ %-32s", areas[b].name);           // Name
                fprintf(stdout,"0x%04X -> 0x%04X",areas[b].start,
                                                  areas[b].end); // Address Start -> End
                fprintf(stdout,"%8d", areas[b].length);
                fprintf(stdout,"\n");
            } else {
                hidden_count++;
                hidden_total += areas[b].length;
            }
        }
    }
    if (hidden_count > 0)
        fprintf(stdout,"+ (%d items < %d hidden = %d total bytes)\n",
                        hidden_count, get_option_area_hide_size(), hidden_total);

    fprintf(stdout,"\n");
}


static void bank_print_info(bank_item *p_bank) {

    fprintf(stdout,"%-15s",p_bank->name);           // Name
    fprintf(stdout,"0x%04X -> 0x%04X",p_bank->start,
                                      p_bank->end); // Address Start -> End
    fprintf(stdout,"%7d", p_bank->size_total);      // Total size
    fprintf(stdout,"%7d", p_bank->size_used);       // Used
    fprintf(stdout,"  %3d%%", (p_bank->size_used * (uint32_t)100)
                               / p_bank->size_total); // Percent Used
    fprintf(stdout,"%8d", (int32_t)p_bank->size_total - (int32_t)p_bank->size_used); // Free
    fprintf(stdout,"   %3d%%", (((int32_t)p_bank->size_total - (int32_t)p_bank->size_used) * (int32_t)100)
                               / (int32_t)p_bank->size_total); // Percent Free

    // Print a small bar graph if requested
    if (banks_display_minigraph) {
        fprintf(stdout, " |");
        bank_print_graph(p_bank, MINIGRAPH_SIZE);
        fprintf(stdout, "|");
    }
}


// Display all banks along with space used.
// Optionally show areas.
void banklist_printall(list_type * p_bank_list) {

    bank_item * banks = (bank_item *)p_bank_list->p_array;
    int c;
    int b;

    fprintf(stdout, "\n");
    fprintf(stdout,"Bank           Range             Size   Used   Used%%   Free  Free%% \n"
                   "----------     ----------------  -----  -----  -----  -----  -----\n");

    // Print all banks
    for (c = 0; c < p_bank_list->count; c++) {

        bank_print_info(&banks[c]);
        fprintf(stdout,"\n");

        if (option_area_sort != OPT_AREA_SORT_HIDE) // This is a hack-workaround, TODO:fixme
            if (banks_display_areas)
                bank_print_area(&banks[c]);

    } // End: Print all banks loop

        // Print a large graph per-bank if requested
    if (banks_display_largegraph)
        banklist_print_large_graph(p_bank_list);

}
