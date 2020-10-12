// This is free and unencumbered software released into the public domain.
// For more information, please refer to <https://unlicense.org>
// bbbbbr 2020

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
// #include <unistd.h>
#include <stdbool.h>
#include <stdint.h>

#include "banks.h"

// Bank info from pandocs
//  0000-3FFF   16KB ROM Bank 00            (in cartridge, fixed at bank 00)
//  4000-7FFF   16KB ROM Bank 01..NN        (in cartridge, switchable bank number)
//  8000-9FFF   8KB Video RAM (VRAM)        (switchable bank 0-1 in CGB Mode)
//  A000-BFFF   8KB External RAM            (in cartridge, switchable bank, if any)
//  C000-CFFF   4KB Work RAM Bank 0 (WRAM)
//  D000-DFFF   4KB Work RAM Bank 1 (WRAM)  (switchable bank 1-7 in CGB Mode)
//  E000-FDFF   Same as C000-DDFF (ECHO)    (typically not used)


const bank_item bank_templates[] = {
    {"ROM  ",   0x0000, 0x3FFF, BANKED_NO,  0,0,0},
    {"ROM_",    0x4000, 0x7FFF, BANKED_YES, 0,0,0},
    {"VRAM_",   0x8000, 0x9FFF, BANKED_YES, 0,0,0},
    {"XRAM_",   0xA000, 0xBFFF, BANKED_YES, 0,0,0},
    {"WRAM  ",  0xC000, 0xCFFF, BANKED_NO,  0,0,0},
    {"WRAM_1_", 0xD000, 0xDFFF, BANKED_YES, 0,0,0},
};


bank_item bank_list[MAX_BANKS];
int bank_count = 0;
int banks_display_areas = false;
int banks_display_headers = false;


// Turn on/off display of areas within bank
void banks_output_show_areas(bool do_show) {
    banks_display_areas = do_show;
}

// Turn on/off display of areas within bank
void banks_output_show_headers(bool do_show) {
    banks_display_headers = do_show;
}


uint32_t min(uint32_t a, uint32_t b) {
    return (a < b) ? a : b;
}

uint32_t max(uint32_t a, uint32_t b) {
    return (a > b) ? a : b;
}


// Returns size of overlap between two address ranges,
// if zero then no overlap
uint32_t addrs_get_overlap(uint32_t a_start, uint32_t a_end, uint32_t b_start, uint32_t b_end) {

    uint32_t size_used;

    // Check whether the address range *doesn't* overlap
    if ((b_start > a_end) || (b_end < a_start)) {
        size_used =  0; // no overlap, size = 0
    } else {
        size_used = min(b_end, a_end) - max(b_start, a_start) + 1; // Calculate minimum overlap
    }
    return size_used;

}


// Clips an address range to be within a bank address range
void area_clip_range_to_bank(bank_item * p_bank, area_item * p_area) {
    // Clip address range to bank range
    p_area->start = max(p_area->start, p_bank->start);
    p_area->end   = min(p_area->end,   p_bank->end);
}


// Calculates amount of space used by areas in a bank.
// Attempts to merge overlapping areas to avoid
// counting shared space multiple times.
//
// Note: Expects areas to be sorted ascending
//       by .start then by .end before being called
void bank_areas_calc_used(bank_item * p_bank) {
    int c,sub;
    uint32_t start, end;

    p_bank->size_used = 0;

    // Iterate over all areas
    c = 0;
    while (c < p_bank->area_count) {

        // // Store start/end of range for current area
        start = p_bank->area_list[c].start;
        end = p_bank->area_list[c].end;

        // Iterate over remaining areas and stop when they cease to overlap
        sub = c + 1;
        while (sub < p_bank->area_count) {

            // Check for overlap with next entry
            if (addrs_get_overlap(start, end, p_bank->area_list[sub].start, p_bank->area_list[sub].end)) {

                // Expand overlapped area to new end size
                // Just end, start shouldn't be necessary due to expected sorting
                if (p_bank->area_list[sub].end > end) {
                    end = p_bank->area_list[sub].end;
                }

                // Update main loop to next area after current merged,
                c = sub;
            }
            // Increment to next area to check for overlap
            sub++;
        }
        // Move to next area
        c++;

        // Store space used by updated range
        p_bank->size_used += RANGE_SIZE(start, end);
        // fprintf(stdout,"  * %d, %d Final Size> 0x%04X -> 0x%04X = %d ((%d))\n",c, sub, start, end, RANGE_SIZE(start, end), p_bank->size_used);
    }

}


// Add an area to a bank's list of areas
void bank_add_area(bank_item * p_bank, area_item area) {
    int c;
    uint32_t area_size = RANGE_SIZE(area.start, area.end);

    // Check for duplicate entries
    // (happens due to paginating in .map file)
    for(c=0;c < p_bank->area_count; c++) {
        // Abort add if it's already present
        if ((strstr(area.name, p_bank->area_list[c].name)) &&
            (area.start == p_bank->area_list[c].start) &&
            (area.end == p_bank->area_list[c].end)) {
            return;
        }
    }

    // no match was found, add area if possible
    if (p_bank->area_count < MAX_AREAS) {
        p_bank->area_list[ p_bank->area_count ] = area;
        p_bank->area_count++;
    }

    p_bank->size_used += area_size;
}


// Add/Update a bank with an area entry
void banklist_add(bank_item bank_template, area_item area, int bank_num) {

    int c;

    // Strip bank indicator bits and limit area range to within bank
    area.start = WITHOUT_BANK(area.start);
    area.end = WITHOUT_BANK(area.end);
    area_clip_range_to_bank(&bank_template, &area);

    // Check to see if key matches any entries,
    for (c=0; c < bank_count; c++) {

        // If a match was found, update it
        if ((bank_template.start == bank_list[c].start) &&
            (bank_num == bank_list[c].bank_num)) {

            // Append area
            bank_add_area(&(bank_list[c]), area);
            return;
        }
    }

    // no match was found, initialize new area if possible
    if (bank_count < MAX_BANKS) {

        // Copy bank info from template
        bank_list[bank_count] = bank_template;

        // Update size used, total size and append bank name if needed
        bank_list[bank_count].size_used = 0;
        bank_list[bank_count].size_total = RANGE_SIZE(bank_template.start, bank_template.end);
        bank_list[bank_count].bank_num = bank_num;

        if (bank_template.is_banked == BANKED_YES)
            sprintf(bank_list[bank_count].name, "%s%d", bank_template.name, bank_num);

        bank_list[bank_count].area_count = 0;
        bank_add_area(&(bank_list[bank_count]), area);

        bank_count++;
    }
}


// Check to see if an area overlaps with any of the bank templates.
// If it does then try to create/update a bank entry
// and add/append the area entry
void banks_check(area_item area) {

    int      c;
    uint32_t size_used;
    int      bank_num;

    // Loop through all banks and log any that overlap
    // (may be more than one)
    for(c = 0; c < ARRAY_LEN(bank_templates); c++) {

        size_used = addrs_get_overlap(bank_templates[c].start, bank_templates[c].end,
                                      WITHOUT_BANK(area.start), WITHOUT_BANK(area.end));

        if (size_used > 0) {
            bank_num = BANK_GET_NUM(area.start);
            banklist_add(bank_templates[c], area, bank_num);
        }
    }
}


// qsort compare rule function
static int area_item_compare(const void* a, const void* b) {

    // sort by start address first, then bank number if needed
    if (((area_item *)a)->start != ((area_item *)b)->start)
        return (((area_item *)a)->start > ((area_item *)b)->start);
    else if (((area_item *)a)->end != ((area_item *)b)->end)
        return (((area_item *)a)->end > ((area_item *)b)->end);
    else
        return strcmp(((area_item *)a)->name, ((area_item *)b)->name);
}


// qsort compare rule function
static int bank_item_compare(const void* a, const void* b) {

    // sort by start address first, then bank number if needed
    if (((bank_item *)a)->start != ((bank_item *)b)->start)
        return (((bank_item *)a)->start > ((bank_item *)b)->start);
    else
        return (((bank_item *)a)->bank_num > ((bank_item *)b)->bank_num);
}



// Display all banks along with space used.
// Optionally show areas.
void banklist_printall(void) {
    int c;
    int b;

    // Sort by name
    qsort (bank_list, bank_count, sizeof(bank_item), bank_item_compare);

    fprintf(stdout, "\n");

    fprintf(stdout,"Bank             Range               Size    Used  Used%%    Free  Free%% \n"
                   "----------       ----------------   -----   -----  -----   -----  -----\n");

    // Print all banks
    for (c = 0; c < bank_count; c++) {

        // Sort areas in bank and calculate usage
        qsort (bank_list[c].area_list, bank_list[c].area_count, sizeof(area_item), area_item_compare);
        bank_areas_calc_used(&bank_list[c]);

        fprintf(stdout,"%-17s",bank_list[c].name);           // Name
        fprintf(stdout,"0x%04X -> 0x%04X",bank_list[c].start,
                                          bank_list[c].end); // Address Start -> End
        fprintf(stdout,"%8d", bank_list[c].size_total);      // Total size
        fprintf(stdout,"%8d", bank_list[c].size_used);       // Used
        fprintf(stdout,"   %3d%%", (bank_list[c].size_used * (uint32_t)100)
                                   / bank_list[c].size_total); // Percent Used
        fprintf(stdout,"%8d", bank_list[c].size_total - bank_list[c].size_used); // Free
        fprintf(stdout,"   %3d%%", ((bank_list[c].size_total - bank_list[c].size_used) * (uint32_t)100)
                                   / bank_list[c].size_total); // Percent Used
        fprintf(stdout,"\n");

        if (banks_display_areas) {
            for(b=0; b < bank_list[c].area_count; b++) {
                if (b == 0) fprintf(stdout,"│\n");

                // Don't display headers unless requested
                if ((banks_display_headers) || !(strstr(bank_list[c].area_list[b].name,"HEADER"))) {
                    fprintf(stdout,"└─%-15s",bank_list[c].area_list[b].name);           // Name
                    fprintf(stdout,"0x%04X -> 0x%04X",bank_list[c].area_list[b].start,
                                                      bank_list[c].area_list[b].end); // Address Start -> End

                    fprintf(stdout,"%8d", RANGE_SIZE(bank_list[c].area_list[b].start,
                                                     bank_list[c].area_list[b].end));

                    fprintf(stdout,"\n");
                }
            }
            fprintf(stdout,"\n");
        }
    }
}
