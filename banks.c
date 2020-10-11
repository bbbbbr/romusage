// This is free and unencumbered software released into the public domain.
// For more information, please refer to <https://unlicense.org>
// bbbbbr 2020

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
// #include <unistd.h>
// #include <stdbool.h>
#include <stdint.h>

#include "banks.h"

#define ARRAY_LEN(A)  (sizeof(A) / sizeof(A[0]))
#define WITHOUT_BANK(addr)  (addr & 0x0000FFFF)
#define BANK_GET_NUM(addr)     ((addr & 0xFFFF0000) >> 16)
#define BANK_SIZE(MIN, MAX) (MAX - MIN + 1)

#define BANK_MAX_STR 20
#define BANKED_NO     0
#define BANKED_YES    1
#define MAX_BANKS     1000

// Bank info from pandocs
//  0000-3FFF   16KB ROM Bank 00            (in cartridge, fixed at bank 00)
//  4000-7FFF   16KB ROM Bank 01..NN        (in cartridge, switchable bank number)
//  8000-9FFF   8KB Video RAM (VRAM)        (switchable bank 0-1 in CGB Mode)
//  A000-BFFF   8KB External RAM            (in cartridge, switchable bank, if any)
//  C000-CFFF   4KB Work RAM Bank 0 (WRAM)
//  D000-DFFF   4KB Work RAM Bank 1 (WRAM)  (switchable bank 1-7 in CGB Mode)
//  E000-FDFF   Same as C000-DDFF (ECHO)    (typically not used)


typedef struct bank_item {
    // Fixed values
    char     name[BANK_MAX_STR];
    uint32_t start;
    uint32_t end;
    int      is_banked;

    // Updateable values
    uint32_t size_total;
    uint32_t size_used;
    int      bank_num;
} bank_item;


const bank_item bank_templates[] = {
    {"ROM  ",   0x0000, 0x3FFF, BANKED_NO,  0,0,0},
    {"ROM_",    0x4000, 0x7FFF, BANKED_YES, 0,0,0},
    {"VRAM_",   0x8000, 0x9FFF, BANKED_YES, 0,0,0},
    {"XRAM_",   0xA000, 0xBFFF, BANKED_YES, 0,0,0},
    {"WRAM  ", 0xC000, 0xCFFF, BANKED_NO,  0,0,0},
    {"WRAM_1_", 0xD000, 0xDFFF, BANKED_YES, 0,0,0},
};

bank_item bank_list[MAX_BANKS];
int bank_count = 0;


uint32_t min(uint32_t a, uint32_t b) {
    return (a < b) ? a : b;
}

uint32_t max(uint32_t a, uint32_t b) {
    return (a > b) ? a : b;
}


// TODO: instead of simple size calc, store all used ranges and merge them when necessary
// TODO: log all area names
void bank_list_add(bank_item bank_template, uint32_t size_used, int bank_num) {

    int c;

    // Check to see if key matches any entries,
    for (c=0; c < bank_count; c++) {

        // If a match was found, update it
        if ((bank_template.start == bank_list[c].start) &&
            (bank_num == bank_list[c].bank_num)) {
            // Append size
            bank_list[c].size_used += size_used;
            return;
        }
    }

    // no match was found, initialize new area if possible
    if (bank_count < MAX_BANKS) {

        // Copy bank info from template
        bank_list[bank_count] = bank_template;

        // Update size used, total size and append bank name if needed
        bank_list[bank_count].size_used = size_used;
        bank_list[bank_count].size_total = BANK_SIZE(bank_template.start, bank_template.end);
        bank_list[bank_count].bank_num = bank_num;
        if (bank_template.is_banked == BANKED_YES)
            sprintf(bank_list[bank_count].name, "%s%d", bank_template.name, bank_num);

        bank_count++;
    }
}


// Returns amount of space used in the bank, if zero then no overlap or none used
uint32_t bank_get_overlap(bank_item bank_template, uint32_t addr_start, uint32_t addr_end) {

    uint32_t size_used;

    // Check whether the address range (stripped of bank bits)
    // *doesn't* overlap with the current bank
    if ((WITHOUT_BANK(addr_start) > bank_template.end) ||
        (WITHOUT_BANK(addr_end)   < bank_template.start)) {
        return 0; // no overlap
    } else {

        // Calculate size used in bank.
        // Note: Some address ranges can span multiple banks,
        //       such as CODE in 32K non-MBC mode (vs MBC 16K sizes)
        //       (in that case it will be added to both CODE and CODE_0)
// printf("min/max:%x - %x\n", max(WITHOUT_BANK(addr_start), bank_template.start),
//                             min(WITHOUT_BANK(addr_end), bank_template.end));

        size_used = min(WITHOUT_BANK(addr_end),   bank_template.end) -
                    max(WITHOUT_BANK(addr_start), bank_template.start) + 1;

    // printf("++++: %s %x %x : %x %x  (size %x) (bank %d)\n\n", bank_template.name,
    //                              bank_template.start, bank_template.end,
    //                              WITHOUT_BANK(addr_start),
    //                              WITHOUT_BANK(addr_end),
    //                              size_used,
    //                              BANK_GET_NUM(addr_start) );

        return size_used;

    }
}


void banks_check(uint32_t addr_start, uint32_t addr_end) {

    int      c;
    uint32_t size_used;
    int      bank_num;

    // Loop through all banks and log any that overlap
    // (may be more than one)
    for(c = 0; c < ARRAY_LEN(bank_templates); c++) {

        size_used = bank_get_overlap(bank_templates[c], addr_start, addr_end);

        if (size_used > 0) {
            bank_num = BANK_GET_NUM(addr_start);
            bank_list_add(bank_templates[c], size_used, bank_num);
        }
    }
}



// qsort compare rule function
static int bank_item_compare(const void* a, const void* b) {

    // sort by start address first, then bank number if needed
    if (((bank_item *)a)->start != ((bank_item *)b)->start)
        return (((bank_item *)a)->start > ((bank_item *)b)->start);
    else
        return (((bank_item *)a)->bank_num > ((bank_item *)b)->bank_num);
}


void bank_list_printall(void) {
    int c;

    // Sort by name
    qsort (bank_list, bank_count, sizeof(bank_item), bank_item_compare);

    fprintf(stdout, "\n");

    fprintf(stdout,"Bank + #     Bank Range          Size    Used  Used%%    Free  Free%% \n"
                   "----------   ----------------   -----   -----  -----   -----  -----\n");

    // Print all banks
    for (c = 0; c < bank_count; c++) {

        fprintf(stdout,"%-13s",bank_list[c].name);           // Name
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
    }
}
