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


static void bank_print_graph(bank_item * p_bank, uint32_t num_chars) {

    int c;
    char ch;
    uint32_t range_size = RANGE_SIZE(p_bank->start, p_bank->end) / num_chars;
    uint32_t perc_used;

    for (c = 0; c <= (num_chars - 1); c++) {

        perc_used = (bank_areas_calc_used(p_bank,
                                          p_bank->start + (c * range_size),
                                          p_bank->start + ((c + 1) * range_size) - 1)
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

    for(b = 0; b < p_bank->area_list.count; b++) {

        // Load the area list for the bank
        areas = (area_item *)p_bank->area_list.p_array;

        if (b == 0)
            fprintf(stdout,"|\n"
                           "| Name                            Start  -> End      Size \n"
                           "| ---------------------           ----------------   -----\n");

        // Don't display headers unless requested
        if ((banks_display_headers) || !(strstr(areas[b].name,"HEADER"))) {

            // Optionally hide areas below a given size
            if (RANGE_SIZE(areas[b].start, areas[b].end)
                >= get_option_area_hide_size()) {

                fprintf(stdout,"+ %-32s", areas[b].name);           // Name
                fprintf(stdout,"0x%04X -> 0x%04X",areas[b].start,
                                                  areas[b].end); // Address Start -> End

                fprintf(stdout,"%8d", RANGE_SIZE(areas[b].start,
                                                 areas[b].end));
                fprintf(stdout,"\n");
            } else
                hidden_count++;
        }
    }
    if (hidden_count > 0)
        fprintf(stdout,"+ (%d hidden items below %d bytes, -z:%d)\n",
                        hidden_count, get_option_area_hide_size(), get_option_area_hide_size());

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
