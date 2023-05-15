// This is free and unencumbered software released into the public domain.
// For more information, please refer to <https://unlicense.org>
// bbbbbr 2020

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
// #include <unistd.h>
#include <stdbool.h>
#include <stdint.h>

#include "common.h"
#include "list.h"
#include "banks.h"
#include "banks_print.h"
#include "banks_summarized.h"

// Bank info from pandocs
//  0000-3FFF   16KB ROM Bank 00            (in cartridge, fixed at bank 00)
//  4000-7FFF   16KB ROM Bank 01..NN        (in cartridge, switchable bank number)
//  8000-9FFF   8KB Video RAM (VRAM)        (switchable bank 0-1 in CGB Mode)
//  A000-BFFF   8KB External RAM            (in cartridge, switchable bank, if any)
//  C000-CFFF   4KB Work RAM Bank 0 (WRAM)
//  D000-DFFF   4KB Work RAM Bank 1 (WRAM)  (switchable bank 1-7 in CGB Mode)
//  E000-FDFF   Same as C000-DDFF (ECHO)    (typically not used)

static int area_item_compare(const void* a, const void* b);
static int bank_item_compare(const void* a, const void* b);


const bank_item bank_templates[] = {
    {"ROM_0",   0x0000, 0x3FFF, BANKED_NO,  0x7FFF, 0,0,0, BANK_MEM_TYPE_ROM,  BANK_STARTNUM_0},
    {"ROM_",    0x4000, 0x7FFF, BANKED_YES, 0x7FFF, 0,0,0, BANK_MEM_TYPE_ROM,  BANK_STARTNUM_1},
    {"VRAM_",   0x8000, 0x9FFF, BANKED_YES, 0x9FFF, 0,0,0, BANK_MEM_TYPE_VRAM, BANK_STARTNUM_0},
    {"SRAM_",   0xA000, 0xBFFF, BANKED_YES, 0xBFFF, 0,0,0, BANK_MEM_TYPE_SRAM, BANK_STARTNUM_0},
    {"WRAM_LO", 0xC000, 0xCFFF, BANKED_NO,  0xDFFF, 0,0,0, BANK_MEM_TYPE_WRAM, BANK_STARTNUM_0},
    {"WRAM_HI_",0xD000, 0xDFFF, BANKED_YES, 0xDFFF, 0,0,0, BANK_MEM_TYPE_WRAM, BANK_STARTNUM_1},
    {"HRAM",    0xFF80, 0xFFFE, BANKED_NO,  0xFFFE, 0,0,0, BANK_MEM_TYPE_HRAM, BANK_STARTNUM_0},
};


list_type bank_list;
list_type bank_list_summarized;

uint32_t min(uint32_t a, uint32_t b) {
    return (a < b) ? a : b;
}

uint32_t max(uint32_t a, uint32_t b) {
    return (a > b) ? a : b;
}



// Initialize the main banklist
void banks_init(void) {

    list_init(&bank_list, sizeof(bank_item));
    list_init(&bank_list_summarized, sizeof(bank_item));
}


// Free all banks and their areas
void banks_cleanup(void) {

    bank_item * banks = (bank_item *)bank_list.p_array;
    int c;

    for (c = 0; c < bank_list.count; c++) {
        list_cleanup(&(banks[c].area_list));
    }
    list_cleanup(&bank_list);
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


// Fixes up the missing bank number virtual addressing mask bits
// for when ROM0 overflows into ROM1. Ex: 0x00004000 -> 0x00014000
//
// Note: When called from banks_check() the address will be
//       clipped to the start of the matched bank template
static uint32_t addr_fixup_ROM0_overflow_bank_num(uint32_t addr) {

    // If it's in the upper bank range yet has
    // a bank number of zero then it needs fixing
    if ((WITHOUT_BANK(addr) >= BANK_ADDR_ROM_UPPER_ST) &&
        (WITHOUT_BANK(addr) <= BANK_ADDR_ROM_UPPER_END) &&
        (BANK_GET_NUM(addr) == BANK_NUM_ROM0)) {
        // OR in the virtual addressing equivalent of bank ROM1
        addr |= BANK_NUM_ROM1_VADDR;
    }

    return addr;
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
        if ((WITHOUT_BANK(area.start) >= bank_templates[c].start) &&
            (WITHOUT_BANK(area.start) <= bank_templates[c].end) &&
             (area.end   > (BANK_ONLY(area.start) + bank_templates[c].overflow_end))) {
            printf("* WARNING: Area %-8s at %5x -> %5x extends past end of memory region at %5x (Overflow by %d bytes)\n",
                   area.name,
                   // BANK_GET_NUM(area.start),
                   area.start, area.end,
                   BANK_ONLY(area.start) + bank_templates[c].overflow_end,
                   area.end - (BANK_ONLY(area.start) + bank_templates[c].overflow_end));

            if (option_error_on_warning)
                set_exit_error();
        }
    }
}


// Warn if length extends past end of unbanked address space (0xFFFF)
// and will underflow / wrap around
static bool area_check_underflow(area_item area, bool notify) {

    if (area.end > (BANK_ONLY(area.start) + MAX_ADDR_UNBANKED)) {

        if (notify) {
            printf("* WARNING: Area %-8s at %5x -> %5x extends past end of address space at %5x (Underflow error by %d bytes)\n",
                area.name,
                area.start, area.end,
                BANK_ONLY(area.start) + MAX_ADDR_UNBANKED,
                area.end - (BANK_ONLY(area.start) + MAX_ADDR_UNBANKED));

            if (option_error_on_warning)
                set_exit_error();
        }

        return true;
    }
    else return false;
}


static void area_check_warnings(area_item area, uint32_t size_assigned) {

    // Unassigned warning is mostly redundant with area_check_bank_overflow()
    //
    // // Warn if there are unassigned bytes left over
    // if (size_assigned < RANGE_SIZE(area.start, area.end)) {
    //     printf("\n* Warning: Area %s 0x%x -> 0x%x (%d bytes): %d bytes not assigned to any bank (overflow error)\n",
    //         area.name,
    //         area.start, area.end,
    //         RANGE_SIZE(area.start, area.end),
    //         RANGE_SIZE(area.start, area.end) - size_assigned);
    // }

    area_check_underflow(area, true);
    area_check_region_overflow(area);
}


static void area_check_warn_overlap(area_item area_a, area_item area_b) {

    uint32_t overlap_size;

    // HEADER areas almost always overlap, ignore them
    if ((strstr(area_a.name,"HEADER")) || (strstr(area_b.name,"HEADER")))
        return;

    // Check to see if an there are overlaps with exclusive areas
    if (area_a.exclusive || area_b.exclusive) {
        overlap_size = addrs_get_overlap(WITHOUT_BANK(area_a.start), WITHOUT_BANK(area_a.end),
                                         WITHOUT_BANK(area_b.start), WITHOUT_BANK(area_b.end));
        if (overlap_size > 0) {
            printf("\n* WARNING: Areas overlapp by %d bytes: Possible bank overflow.\n"
                   "%15s 0x%04x -> 0x%04x (%d bytes%s)\n"
                   "%15s 0x%04x -> 0x%04x (%d bytes%s)\n",
                overlap_size,
                area_a.name, area_a.start, area_a.end, RANGE_SIZE(area_a.start, area_a.end),
                (area_a.exclusive) ? ", EXCLUSIVE" : " ",
                area_b.name, area_b.start, area_b.end, RANGE_SIZE(area_b.start, area_b.end),
                (area_b.exclusive) ? ", EXCLUSIVE" : " ");

            if (option_error_on_warning)
                set_exit_error();
        }
    }
}



// Calculate free and used percentages of space in a given bank
int bank_calc_percent_free(bank_item * p_bank) {

    return (((int32_t)p_bank->size_total - (int32_t)p_bank->size_used)
             * (int32_t)100) / (int32_t)p_bank->size_total;
}

int bank_calc_percent_used(bank_item * p_bank) {

    return ((p_bank->size_used * (uint32_t)100) / p_bank->size_total);
}


// Calculates amount of space used by areas in a bank.
// Attempts to merge overlapping areas to avoid
// counting shared space multiple times.
//
uint32_t bank_areas_calc_used(bank_item * p_bank, uint32_t clip_start, uint32_t clip_end) {

    area_item * areas = (area_item *)p_bank->area_list.p_array;
    int c,sub;
    uint32_t start, end;
    uint32_t size_used;
    area_item t_area, sub_area;

    size_used = 0;

    // The calculation requires areas to first be
    // sorted ascending by .start addr then by .end addr
    qsort (p_bank->area_list.p_array, p_bank->area_list.count, sizeof(area_item), area_item_compare);

    // Iterate over all areas
    c = 0;
    while (c < p_bank->area_list.count) {

        // Copy area so it can be clipped, then clip to param range
        t_area = areas[c];
        area_clip_to_range(clip_start, clip_end, &t_area); // clip to param range

        // // Store start/end of range for current area
        start = t_area.start;
        end = t_area.end;

        // Iterate over remaining areas and stop when they cease to overlap
        sub = c + 1;
        while (sub < p_bank->area_list.count) {

            // Copy area so it can be clipped, then clip to param range
            sub_area = areas[sub];
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
        // fprintf(stdout,"  * %d, %d Final Size> 0x%04X -> 0x%04X = %d ((%d))\n",c, sub, start, end, RANGE_SIZE(start, end), size_used);
    }

    return size_used;
}



// Add an area to a bank's list of areas
static void bank_add_area(bank_item * p_bank, area_item area) {

    area_item * areas = (area_item *)p_bank->area_list.p_array;
    int c;

    // Make sure the area length/size is set
    area.length = RANGE_SIZE(area.start, area.end);

    // Check for duplicate entries
    // (happens due to paginating in .map file)
    for(c=0;c < p_bank->area_list.count; c++) {
        // Abort add if it's already present
        if (option_suppress_duplicates == true) {
            if ((strstr(area.name, areas[c].name)) &&
                (area.start == areas[c].start) &&
                (area.end == areas[c].end)) {
                return;
            }
        }

        area_check_warn_overlap(area, areas[c]);
    }

    // no match was found, add area
    list_additem(&(p_bank->area_list), &area);
    p_bank->size_used += area.length;
}


// Add/Update a bank with an area entry
static void banklist_addto(bank_item bank_template, area_item area, int bank_num) {

    int c;
    bank_item * banks = (bank_item *)bank_list.p_array;
    bank_item newbank;

    // Strip bank indicator bits and limit area range to within bank
    area.start = area.start_unbanked;
    area.end = area.end_unbanked;
    area_clip_to_range(bank_template.start, bank_template.end, &area);

    // Check to see if key matches any entries,
    for (c=0; c < bank_list.count; c++) {

        // If a match was found, update it
        if ((bank_template.start == banks[c].start) &&
            (bank_num == banks[c].bank_num)) {

            // Append area
            bank_add_area(&(banks[c]), area);
            return;
        }
    }

    // No match was found, initialize new bank

    // Copy bank info from template
    newbank = bank_template;

    // Update size used, total size and append bank name if needed
    newbank.size_used = 0;
    newbank.size_total = RANGE_SIZE(bank_template.start, bank_template.end);
    newbank.bank_num = bank_num;

    if (bank_template.is_banked == BANKED_YES) {
        // save result to suppress truncation warning, bank names will never be > 100 chars and truncation is fine if it got to that
        int ret = snprintf(newbank.name, sizeof(newbank.name), "%s%d", bank_template.name, bank_num);
    }

    // Initialize new bank's area list and add the area
    list_init(&(newbank.area_list), sizeof(area_item));
    bank_add_area(&newbank, area);

    // Now add the new bank to the main list
    list_additem(&bank_list, &newbank);
}


// Strip banks from address start and end, set start/end_unbanked
static void area_calc_unbanked_range(area_item * p_area) {

    p_area->start_unbanked = WITHOUT_BANK(p_area->start);

    // * Calculating End relative to start is important for
    //   not accidentally loosing it's full size.
    // * Unbanked End is also capped at 0xFFFF to
    //   prevent wraparound range size errors
    if (area_check_underflow(*p_area, false)) {
        // area_check_warnings() will warn about this later
        p_area->end_unbanked = MAX_ADDR_UNBANKED;
    } else {
        p_area->end_unbanked = UNBANKED_END(p_area->start, p_area->end);
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

    // Set the unbanked address range for comparison
    // with (unbanked) bank templates
    area_calc_unbanked_range(&area);

    // Loop through all banks and log any that overlap
    // (may be more than one)
    for(c = 0; c < ARRAY_LEN(bank_templates); c++) {

        // Check a given ROM/RAM bank template for overlap
        size_used = addrs_get_overlap(bank_templates[c].start, bank_templates[c].end,
                                      area.start_unbanked, area.end_unbanked);

        // If overlap was found, determine bank number and log it
        if (size_used > 0) {
            // Area items can span multiple banks, so don't use area.start
            // on it's own to get bank number since it might originate
            // in a lower bank (handled in a previous iteration of the loop).
            // Instead use the current matched bank template start address.
            // Then fixup missing bank number if needed
            uint32_t addr_start_banknum = BANK_ONLY(area.start) | WITHOUT_BANK(bank_templates[c].start);
            addr_start_banknum = addr_fixup_ROM0_overflow_bank_num(addr_start_banknum);
            bank_num = BANK_GET_NUM(addr_start_banknum);

            // Area range added to bank will get clipped to bank range
            banklist_addto(bank_templates[c], area, bank_num);
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
bool area_manual_add(char * arg_str) {

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

// NOTE: All the comparisons and their particular order are
//       required for bank_areas_calc_used() to work properly.
// qsort compare rule function
static int area_item_compare(const void* a, const void* b) {

    // First sort by start address
    if (((area_item *)a)->start != ((area_item *)b)->start)
        return (((area_item *)a)->start < ((area_item *)b)->start) ? -1 : 1;

    // Otherwise end address
    if (((area_item *)a)->end != ((area_item *)b)->end)
        return (((area_item *)a)->end < ((area_item *)b)->end) ? -1 : 1;

    // If above match, then sort based on name
    return strcmp(((area_item *)a)->name, ((area_item *)b)->name);

}


// qsort compare rule function: sort by size descending first, then name
static int area_item_compare_size_desc(const void* a, const void* b) {

    if (((area_item *)a)->length != ((area_item *)b)->length)
        return (((area_item *)a)->length < ((area_item *)b)->length) ? 1 : -1;
    else
        return strcmp(((area_item *)a)->name, ((area_item *)b)->name);
}


// qsort compare rule function: sort by start address ascending
static int area_item_compare_addr_asc(const void* a, const void* b) {

    return (((area_item *)a)->start < ((area_item *)b)->start) ? -1 : 1;
}


// qsort compare rule function
static int bank_item_compare(const void* a, const void* b) {

    // First sort by start address
    if (((bank_item *)a)->start != ((bank_item *)b)->start)
        return (((bank_item *)a)->start < ((bank_item *)b)->start) ? -1 : 1;

    // Otherwise based on bank number
    if (((bank_item *)a)->bank_num != ((bank_item *)b)->bank_num)
        return (((bank_item *)a)->bank_num < ((bank_item *)b)->bank_num) ? -1 : 1;

    return 0; // Otherwise return equivalent
}



// Fill in gaps between symbols with "?" symbols --TODO: rename function to symbols
static void bank_fill_area_gaps_with_unknown(void) {

    bank_item * banks = (bank_item *)bank_list.p_array;
    area_item * areas;
    uint32_t last_addr, cur_addr;
    int c, b, t_area_count;
    area_item area;

    for (c = 0; c < bank_list.count; c++) {
        // Load the area list for the bank
        areas = (area_item *)banks[c].area_list.p_array;

        // Sort areas by ascending address so that gaps can be found
        qsort (banks[c].area_list.p_array, banks[c].area_list.count, sizeof(area_item), area_item_compare_addr_asc);

        t_area_count = banks[c].area_list.count; // Temp area count to avoid processing newly added areas
        last_addr = banks[c].start;         // Set last to start of current bank

        for(b = 0; b < t_area_count; b++) {

            if ((banks_display_headers) || !(strstr(areas[b].name,"HEADER"))) {

                cur_addr = areas[b].start;

                if (cur_addr > last_addr + 1) {

                    snprintf(area.name, sizeof(area.name), "-?-");
                    area.start  = last_addr + 1;
                    area.end    = cur_addr - 1;
                    area.length = area.end - area.start + 1;
                    area.exclusive = false;
                    bank_add_area(&(banks[c]), area); // Add to bank, skip bank_check since parent bank is known
                }

                // Update previous area reference
                last_addr = areas[b].end;
            }
        }
    }
}



// Print banks to output
void banklist_finalize_and_show(void) {

    bank_item * banks = (bank_item *)bank_list.p_array;
    int c;

    // Sort banks by start address then bank num
    qsort (bank_list.p_array, bank_list.count, sizeof(bank_item), bank_item_compare);

    if (get_option_input_source() == OPT_INPUT_SRC_CDB)
        bank_fill_area_gaps_with_unknown();

    for (c = 0; c < bank_list.count; c++) {
        // Sort areas in bank and calculate usage
        banks[c].size_used = bank_areas_calc_used(&banks[c], banks[c].start, banks[c].end);

        if (get_option_area_sort() == OPT_AREA_SORT_SIZE_DESC)
            qsort (banks[c].area_list.p_array, banks[c].area_list.count, sizeof(area_item), area_item_compare_size_desc);
        else if (get_option_area_sort() == OPT_AREA_SORT_ADDR_ASC)
            qsort (banks[c].area_list.p_array, banks[c].area_list.count, sizeof(area_item), area_item_compare_addr_asc);
        else
            qsort (banks[c].area_list.p_array, banks[c].area_list.count, sizeof(area_item), area_item_compare);
    }

    // Only print if quiet mode is not enabled
    if (!option_quiet_mode) {
        if (option_summarized_mode) {
            banklist_collapse_to_summary(&bank_list, &bank_list_summarized);
            banklist_printall(&bank_list_summarized);
        }
        else {
            banklist_printall(&bank_list);
        }
    }
}
