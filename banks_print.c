// This is free and unencumbered software released into the public domain.
// For more information, please refer to <https://unlicense.org>
// bbbbbr 2020

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "banks.h"
#include "banks_print.h"


static void bank_print_graph(bank_item * p_bank, uint32_t num_chars) {

    int c;
    char ch;
    uint32_t range_size = RANGE_SIZE(p_bank->start, p_bank->end) / num_chars;
    uint32_t perc_used;

    for (c = 0; c <= (num_chars - 1); c++) {

        perc_used = (bank_areas_calc_used(p_bank,
                                          WITHOUT_BANK(p_bank->start) + (c * range_size),
                                          WITHOUT_BANK(p_bank->start) + ((c + 1) * range_size) - 1)
                                           * (uint32_t)100) / range_size;

        if (perc_used > 95)      ch = '#';
        else if (perc_used > 25) ch = '-';
        else                     ch = '.';
        fprintf(stdout, "%c", ch);

        if (((c + 1) % 64) == 0)
            fprintf(stdout, "\n");
    }
}


// Show a large usage graph for each bank
// Currently 16 bytes per character
static void banklist_print_large_graph(bank_item bank_list[], int bank_count) {

    int c;

        for (c = 0; c < bank_count; c++) {

            fprintf(stdout,"\n\nStart: %s  ",bank_list[c].name); // Name
            fprintf(stdout,"0x%04X -> 0x%04X",bank_list[c].start,
                                              bank_list[c].end); // Address Start -> End
            fprintf(stdout,"\n"); // Name

            bank_print_graph(&bank_list[c], bank_list[c].size_total / LARGEGRAPH_BYTES_PER_CHAR);

            fprintf(stdout,"End: %s\n",bank_list[c].name); // Name
        }
}


// Display all areas for a bank
static void bank_print_area(bank_item *p_bank) {

    int b;

    for(b = 0; b < p_bank->area_count; b++) {
        if (b == 0) fprintf(stdout,"|\n");

        // Don't display headers unless requested
        if ((banks_display_headers) || !(strstr(p_bank->area_list[b].name,"HEADER"))) {
            fprintf(stdout,"+%-16s",p_bank->area_list[b].name);           // Name
            fprintf(stdout,"0x%04X -> 0x%04X",p_bank->area_list[b].start,
                                              p_bank->area_list[b].end); // Address Start -> End

            fprintf(stdout,"%8d", RANGE_SIZE(p_bank->area_list[b].start,
                                             p_bank->area_list[b].end));

            fprintf(stdout,"\n");
        }
    }
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
void banklist_printall(bank_item bank_list[], int bank_count) {
    int c;
    int b;

    fprintf(stdout, "\n");
    fprintf(stdout,"Bank           Range             Size   Used   Used%%   Free  Free%% \n"
                   "----------     ----------------  -----  -----  -----  -----  -----\n");

    // Print all banks
    for (c = 0; c < bank_count; c++) {

        bank_print_info(&bank_list[c]);
        fprintf(stdout,"\n");

        if (banks_display_areas)
            bank_print_area(&bank_list[c]);

    } // End: Print all banks loop

        // Print a large graph per-bank if requested
    if (banks_display_largegraph)
        banklist_print_large_graph(bank_list, bank_count);

}
