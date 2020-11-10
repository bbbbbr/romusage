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
#include "banks_print.h"

// Bank info from pandocs
//  0000-3FFF   16KB ROM Bank 00            (in cartridge, fixed at bank 00)
//  4000-7FFF   16KB ROM Bank 01..NN        (in cartridge, switchable bank number)
//  8000-9FFF   8KB Video RAM (VRAM)        (switchable bank 0-1 in CGB Mode)
//  A000-BFFF   8KB External RAM            (in cartridge, switchable bank, if any)
//  C000-CFFF   4KB Work RAM Bank 0 (WRAM)
//  D000-DFFF   4KB Work RAM Bank 1 (WRAM)  (switchable bank 1-7 in CGB Mode)
//  E000-FDFF   Same as C000-DDFF (ECHO)    (typically not used)


const bank_item bank_templates[] = {
    {"ROM  ",   0x0000, 0x3FFF, BANKED_NO,  0x7FFF, 0,0,0},
    {"ROM_",    0x4000, 0x7FFF, BANKED_YES, 0x7FFF, 0,0,0},
    {"VRAM_",   0x8000, 0x9FFF, BANKED_YES, 0x9FFF, 0,0,0},
    {"XRAM_",   0xA000, 0xBFFF, BANKED_YES, 0xBFFF, 0,0,0},
    {"WRAM  ",  0xC000, 0xCFFF, BANKED_NO,  0xDFFF, 0,0,0},
    {"WRAM_1_", 0xD000, 0xDFFF, BANKED_YES, 0xDFFF, 0,0,0},
};


static bank_item bank_list[MAX_BANKS];
static int bank_count = 0;
bool banks_display_areas        = false;
bool banks_display_headers      = false;
bool banks_display_minigraph    = false;
bool banks_display_largegraph   = false;
bool option_all_areas_exclusive = false;
bool option_quiet_mode          = false;
bool option_suppress_duplicates = true;


// Turn on/off display of areas within bank
void banks_output_show_areas(bool do_show) {
    banks_display_areas = do_show;
}

// Turn on/off display of areas within bank
void banks_output_show_headers(bool do_show) {
    banks_display_headers = do_show;
}

// Turn on/off display of mini usage graph per bank
void banks_output_show_minigraph(bool do_show) {
    banks_display_minigraph = do_show;
}

// Turn on/off display of large usage graph per bank
void banks_output_show_largegraph(bool do_show) {
    banks_display_largegraph = do_show;
}

// Turn on/off whether all areas are exclusive,
// and whether to warn for any overlap
void set_option_all_areas_exclusive(bool value) {
    option_all_areas_exclusive = value;
}
// Turn on/off display of large usage graph per bank
void set_option_quiet_mode(bool value) {
    option_quiet_mode = value;
}

// Turn on/off display of large usage graph per bank
void set_option_suppress_duplicates(bool value) {
    option_suppress_duplicates = value;
}

uint32_t min(uint32_t a, uint32_t b) {
    return (a < b) ? a : b;
}

uint32_t max(uint32_t a, uint32_t b) {
    return (a > b) ? a : b;
}


// Returns size of overlap between two address ranges,
// if zero then no overlap
static uint32_t addrs_get_overlap(uint32_t a_start, uint32_t a_end, uint32_t b_start, uint32_t b_end) {

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
static void area_clip_to_range(uint32_t start, uint32_t end, area_item * p_area) {
    // Clip address range to bank range
    p_area->start = max(p_area->start, start);
    p_area->end   = min(p_area->end,   end);
    // Trim to zero length if end is before start
    if (p_area->end < p_area->start) p_area->end = p_area->start - 1;
}


static void area_check_region_overflow(area_item area) {

    int c;

    // Find bank template the area starts in and check to see
    // whether the area extends past the end of it's memory region.
    //
    // Non-banked areas with banks above them have the upper bound
    // set to the end of the bank above them.
    for(c = 0; c < ARRAY_LEN(bank_templates); c++) {

        // Warn about overflow in any ROM bank GBZ80 areas that cross past the (relative) end of their region
        if (((area.start & 0x0000FFFF) >= bank_templates[c].start) &&
            ((area.start & 0x0000FFFF) <= bank_templates[c].end) &&
             (area.end   > ((area.start & 0xFFFF0000U) + bank_templates[c].overflow_end))) {
            printf("* WARNING: Area %-8s at %6x -> %6x extends past end of memory region at %6x (overflow? %5d bytes)\n",
                   area.name,
                   // BANK_GET_NUM(area.start),
                   area.start, area.end,
                   (area.start & 0xFFFF0000U) + bank_templates[c].overflow_end,
                   (area.end - ((area.start & 0xFFFF0000U) + bank_templates[c].overflow_end)));
        }
    }
}


static void area_check_warnings(area_item area, uint32_t size_assigned) {

    // Unassigned warning is mostly redundant with area_check_bank_overflow()
    //
    // // Warn if there are unassigned bytes left over
    // if (size_assigned < RANGE_SIZE(area.start, area.end)) {
    //     printf("\n* Warning: Area %s 0x%x -> 0x%x (%d bytes): %d bytes not assigned to any bank (overflow?)\n",
    //         area.name,
    //         area.start, area.end,
    //         RANGE_SIZE(area.start, area.end),
    //         RANGE_SIZE(area.start, area.end) - size_assigned);
    // }

    area_check_region_overflow(area);

    // Warn if length will wrap around into a bank
    if ((WITHOUT_BANK(area.start) + RANGE_SIZE(area.start, area.end) - 1)  > 0xFFFF) {
            printf("\n* WARNING: Area %s 0x%x -> 0x%x (%d bytes): extends past relative 0xFFFF into bank addressing bits\n",
                area.name,
                area.start, area.end,
                RANGE_SIZE(area.start, area.end));
    }
}


static void area_check_warn_overlap(area_item area_a, area_item area_b) {
    // Check to see if an there are overlaps with exclusive areas
    if (area_a.exclusive || area_b.exclusive) {
        if (addrs_get_overlap(WITHOUT_BANK(area_a.start), WITHOUT_BANK(area_a.end),
                              WITHOUT_BANK(area_b.start), WITHOUT_BANK(area_b.end)) > 0) {
            printf("\n* WARNING: Overlap with exclusive area: "
                   "%s 0x%x -> 0x%x (%d bytes%s) --and-- "
                   "%s 0x%x -> 0x%x (%d bytes%s)\n",
                area_a.name, area_a.start, area_a.end, RANGE_SIZE(area_a.start, area_a.end),
                (area_a.exclusive) ? ", EXCLUSIVE" : " ",
                area_b.name, area_b.start, area_b.end, RANGE_SIZE(area_b.start, area_b.end),
                (area_b.exclusive) ? ", EXCLUSIVE" : " ");

        }
    }
}



// Calculates amount of space used by areas in a bank.
// Attempts to merge overlapping areas to avoid
// counting shared space multiple times.
//
// Note: Expects areas to be sorted ascending
//       by .start then by .end before being called
uint32_t bank_areas_calc_used(bank_item * p_bank, uint32_t clip_start, uint32_t clip_end) {
    int c,sub;
    uint32_t start, end;
    uint32_t size_used;
    area_item t_area, sub_area;

    size_used = 0;

    // Iterate over all areas
    c = 0;
    while (c < p_bank->area_count) {

        // Copy area so it can be clipped, then clip to param range
        t_area = p_bank->area_list[c];
        area_clip_to_range(clip_start, clip_end, &t_area); // clip to param range

        // // Store start/end of range for current area
        start = t_area.start;
        end = t_area.end;

        // Iterate over remaining areas and stop when they cease to overlap
        sub = c + 1;
        while (sub < p_bank->area_count) {

            // Copy area so it can be clipped, then clip to param range
            sub_area = p_bank->area_list[sub];
            area_clip_to_range(clip_start, clip_end, &sub_area);

            // Check for overlap with next entry
            if (addrs_get_overlap(start, end, sub_area.start, sub_area.end)) {

                // Expand overlapped area to new end size
                // Just end, start shouldn't be necessary due to expected sorting
                if (sub_area.end > end) {
                    end = sub_area.end;
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
        size_used += RANGE_SIZE(start, end);
        // fprintf(stdout,"  * %d, %d Final Size> 0x%04X -> 0x%04X = %d ((%d))\n",c, sub, start, end, RANGE_SIZE(start, end), p_bank->size_used);
    }

    return size_used;

}



// Add an area to a bank's list of areas
static void bank_add_area(bank_item * p_bank, area_item area) {
    int c;
    uint32_t area_size = RANGE_SIZE(area.start, area.end);

    // Check for duplicate entries
    // (happens due to paginating in .map file)
    for(c=0;c < p_bank->area_count; c++) {
        // Abort add if it's already present
        if (option_suppress_duplicates == true) {
            if ((strstr(area.name, p_bank->area_list[c].name)) &&
                (area.start == p_bank->area_list[c].start) &&
                (area.end == p_bank->area_list[c].end)) {
                return;
            }
        }

        area_check_warn_overlap(area, p_bank->area_list[c]);
    }

    // no match was found, add area if possible
    if (p_bank->area_count < MAX_AREAS) {
        p_bank->area_list[ p_bank->area_count ] = area;
        p_bank->area_count++;
    } else
        printf("WARNING: ran out of areas in bank %s\n", p_bank->name);

    p_bank->size_used += area_size;
}


// Add/Update a bank with an area entry
static void banklist_add(bank_item bank_template, area_item area, int bank_num) {

    int c;

    // Strip bank indicator bits and limit area range to within bank
    area.start = WITHOUT_BANK(area.start);
    area.end = WITHOUT_BANK(area.end);
    area_clip_to_range(bank_template.start, bank_template.end, &area);

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
    uint32_t size_assigned = 0;
    int      bank_num;
    uint32_t start, end;

    // Strip banks from address.
    // Calculating END relative is important to not accidentally loosing it's full size
    start = WITHOUT_BANK(area.start);
    end   = (area.end - area.start) + WITHOUT_BANK(area.start);

    // Loop through all banks and log any that overlap
    // (may be more than one)
    for(c = 0; c < ARRAY_LEN(bank_templates); c++) {

        // Check a given ROM/RAM bank template for overlap
        size_used = addrs_get_overlap(bank_templates[c].start, bank_templates[c].end,
                                      start, end);

        // If overlap was found, determine bank number and log it
        if (size_used > 0) {
            bank_num = BANK_GET_NUM(area.start);
            banklist_add(bank_templates[c], area, bank_num);
            size_assigned += size_used; // Log space assigned to bank

            // Only allow overflow to other banks if first bank is non-banked
            if (bank_templates[c].is_banked != BANKED_NO)
                break;
        }
    }

    area_check_warnings(area, size_assigned);
}

#define MAX_SPLIT_WORDS 4
#define ARG_AREA_REC_COUNT_MATCH 4


// -m:NAME:HEX_ADDR:HEX_LENGTH or -e[same]
// Add areas manually from command line arguments
int area_manual_add(char * arg_str) {

    char cols;
    char * p_str;
    char * p_words[MAX_SPLIT_WORDS];
    area_item area;

    // Split string into words separated by spaces
    cols = 0;
    p_str = strtok(arg_str,"-:");
    while (p_str != NULL)
    {
        p_words[cols++] = p_str;
        // Only split on underscore for the second match
        p_str = strtok(NULL, "-:");
        if (cols >= MAX_SPLIT_WORDS) break;
    }

    if (cols == ARG_AREA_REC_COUNT_MATCH) {
        snprintf(area.name, sizeof(area.name), "%s", p_words[1]);   // [1] Area Name
        area.start = strtol(p_words[2], NULL, 16);                  // [2] Area Hex Address Start
        area.end   = area.start + strtol(p_words[3], NULL, 16) - 1; // Start + [3] Hex Size - 1 = Area End
        area.exclusive = (p_words[0][0] == 'e') ? true : false;        // [0] shared/exclusive

        banks_check(area);
        return true;
    } else
        return false; // Signal failure

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


// Print banks to output
void banklist_finalize_and_show(void) {

    int c;

    // Sort banks by name
    qsort (bank_list, bank_count, sizeof(bank_item), bank_item_compare);

    for (c = 0; c < bank_count; c++) {
        // Sort areas in bank and calculate usage
        qsort (bank_list[c].area_list, bank_list[c].area_count, sizeof(area_item), area_item_compare);
        bank_list[c].size_used = bank_areas_calc_used(&bank_list[c], bank_list[c].start, bank_list[c].end);
    }

    // Only print if quiet mode is not enabled
    if (!option_quiet_mode)
        banklist_printall(bank_list, bank_count);
}

