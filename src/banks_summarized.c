// This is free and unencumbered software released into the public domain.
// For more information, please refer to <https://unlicense.org>
// bbbbbr 2023

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "common.h"
#include "list.h"
#include "banks.h"

// === Summarized mode ===

static void summarize_copy_areas(bank_item *, const bank_item *);
static void summarize_copy_bank(bank_item *, const bank_item *);
static void summarize_fixup_sizes_and_names(list_type *);
static bool summarize_try_merge_bank(const bank_item *, list_type *);

// Copy (append) areas from one bank to another
//
// Translate banked areas addresses based on their bank number
// Assumes: source and dest bank are valid, that areas from different banks don't overlap
static void summarize_copy_modified_areas(bank_item * p_dest_bank, const bank_item * p_src_bank) {

    area_item * areas_to_copy = (area_item *)p_src_bank->area_list.p_array;

    // Copy areas from source bank to new summary bank
    for (int c = 0; c < p_src_bank->area_list.count; c++) {

        area_item new_area = areas_to_copy[c];
        // Scale area address up by bank number * bank size (factoring in whether bank start is 0 or 1 based)
        new_area.start += ((p_src_bank->bank_num - p_src_bank->base_bank_num) * p_src_bank->size_total);
        new_area.end = new_area.start + (new_area.length - 1);

        list_additem(&(p_dest_bank->area_list), &new_area);
        p_dest_bank->size_used += new_area.length;
    }
}


// Copy a bank entry and it's areas
static void summarize_copy_bank(bank_item * p_dest_bank, const bank_item * p_src_bank) {

    // Duplicate bank, re-initialize bank list & copy areas from source
    *p_dest_bank = *p_src_bank;
    p_dest_bank->size_used = 0;
    list_init(&(p_dest_bank->area_list), sizeof(area_item));
    summarize_copy_modified_areas(p_dest_bank, p_src_bank);
}


// After banked regions have been merged, fix their names and sizes
static void summarize_fixup_sizes_and_names(list_type * p_bank_list_summarized) {

    bank_item * banks_summarized = (bank_item *)p_bank_list_summarized->p_array;

    for (int c=0; c < p_bank_list_summarized->count; c++) {
        if (banks_summarized[c].is_banked) {

            // Some banked region bank nums are 0, don't process those
            // (no need to resize, resize code not built to handle it)
            if (banks_summarized[c].bank_num) {

                // Save current as max "used" before replacing with max possible
                int bank_num_max_used = banks_summarized[c].bank_num;

                // ROM_X size: Round up bank num to next (power of 2) -1
                // SRAM_X size: Round up to next (power of 4) - 1
                // WRAM_X: Fixed number of banks on CGB, 7 always max
                switch (banks_summarized[c].bank_mem_type) {
                    case BANK_MEM_TYPE_ROM: banks_summarized[c].bank_num = round_up_power_of_2((uint32_t)banks_summarized[c].bank_num + 1) - 1;
                            break;
                    case BANK_MEM_TYPE_SRAM:
                            if (     banks_summarized[c].bank_num > 7) banks_summarized[c].bank_num = 15;
                            else if (banks_summarized[c].bank_num > 3) banks_summarized[c].bank_num = 7;
                            else if (banks_summarized[c].bank_num > 1) banks_summarized[c].bank_num = 3;
                            break;
                    case BANK_MEM_TYPE_WRAM: banks_summarized[c].bank_num = WRAM_X_MAX_BANKS;
                            break;
                }

                // Update bank total size (factoring in whether bank start is 0 or 1 based)
                banks_summarized[c].size_total = ((banks_summarized[c].bank_num - banks_summarized[c].base_bank_num) + 1) *  banks_summarized[c].size_total;
                // Don't resize .end so that bank region shows correctly when rendered. charts will use ".size_total"
                // banks_summarized[c].end = (banks_summarized[c].size_total - 1) + banks_summarized[c].start;

                // Update Bank name with Used / Max - example: "ROM_1" with "ROM_Used/Max"
                char new_name[BANK_MAX_STR];
                if (strlen(banks_summarized[c].name) >= 2);
                    banks_summarized[c].name[ strlen(banks_summarized[c].name) - 2 ] = '\0'; // Truncate existing name (makes assumption name is "..._N")

                int ret; // save result to suppress truncation warning, bank names will never be > 100 chars and truncation is fine if it got to that
                ret = snprintf(new_name, sizeof(new_name), "%s_%d/%d", banks_summarized[c].name, bank_num_max_used, banks_summarized[c].bank_num);
                ret = snprintf(banks_summarized[c].name, sizeof(banks_summarized[c].name), "%s", new_name);
            }
        }
    }
}


// Try to find a matching bank in summarized data
// If found merge source bank into it
static bool summarize_try_merge_bank(const bank_item * p_src_bank, list_type * p_bank_list_summarized) {

    bank_item * banks_summarized = (bank_item *)p_bank_list_summarized->p_array;

    // Check if bank type already exists in summarized data by start of memory region
    for (int dest_idx=0; dest_idx < p_bank_list_summarized->count; dest_idx++) {
        if (banks_summarized[dest_idx].start == p_src_bank->start) {
            // Found matching bank type, copy areas and bump up max bank used if needed
            summarize_copy_modified_areas(&banks_summarized[dest_idx], p_src_bank);
            banks_summarized[dest_idx].bank_num = max(banks_summarized[dest_idx].bank_num, p_src_bank->bank_num);
            return true; // Success
        }
    }
    return false; // Failed: no existing bank found to merge into
}


// Generate a summarized view with all banked
// categories collapsed into to single ranges
//
// Data in the bank list is assumed to be good
// and not requiring validation at this point
void banklist_collapse_to_summary(const list_type * p_bank_list, list_type * p_bank_list_summarized) {

    bank_item * banks            = (bank_item *)p_bank_list->p_array;
    bank_item * banks_summarized = (bank_item *)p_bank_list_summarized->p_array;
    bank_item   new_bank;

    for (int src_idx=0; src_idx < p_bank_list->count; src_idx++) {

        // Add non-banked items directly without aggregating them
        // Assumes no dupes, or if dupes then intentional
        if (! banks[src_idx].is_banked) {
            summarize_copy_bank(&new_bank, &banks[src_idx]);
            list_additem(p_bank_list_summarized, &new_bank);
        } else {
            // Otherwise try to merge with an existing bank, or create a new entry in sumamrized data if needed
            if (! summarize_try_merge_bank(&banks[src_idx], p_bank_list_summarized)) {
                summarize_copy_bank(&new_bank, &banks[src_idx]);
                list_additem(p_bank_list_summarized, &new_bank);
            }
        }
    }

    summarize_fixup_sizes_and_names(p_bank_list_summarized);
}
