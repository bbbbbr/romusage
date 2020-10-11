// console_main.c

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>

// Example data to parse from a .sym file (excluding unwanted lines):
// _CODE                  00000200    00006A62 =       27234. bytes (REL,CON)
// _CODE                  00000200    00006A62 =       27234. bytes (REL,CON)
// _HOME                  00006C62    000000ED =         237. bytes (REL,CON)
// _BASE                  00006D4F    000002A3 =         675. bytes (REL,CON)
// _GSINIT                00006FF2    000001F9 =         505. bytes (REL,CON)
// _GSINITTAIL            000071EB    00000001 =           1. bytes (REL,CON)
// _DATA                  0000C0A0    00001684 =        5764. bytes (REL,CON)
// _BSS                   0000D724    00000041 =          65. bytes (REL,CON)
// _HEAP                  0000D765    00000000 =           0. bytes (REL,CON)
// _HRAM10                00000000    00000001 =           1. bytes (ABS,CON)

//  0000-3FFF   16KB ROM Bank 00     (in cartridge, fixed at bank 00)
//  4000-7FFF   16KB ROM Bank 01..NN (in cartridge, switchable bank number)
//  8000-9FFF   8KB Video RAM (VRAM) (switchable bank 0-1 in CGB Mode)
//  A000-BFFF   8KB External RAM     (in cartridge, switchable bank, if any)
//  C000-CFFF   4KB Work RAM Bank 0 (WRAM)
//  D000-DFFF   4KB Work RAM Bank 1 (WRAM)  (switchable bank 1-7 in CGB Mode)
//  E000-FDFF   Same as C000-DDFF (ECHO)    (typically not used)



// void display_help(void);
// int handle_args(int, char * []);
// void add_rec(char * str_area, uint32_t addr, uint32_t size);
// static int area_rec_compare(const void* a, const void* b);
// void print_all_recs(void);
// int parse_map_file(void);

#define ARRAY_LEN(A)  (sizeof(A) / sizeof(A[0]))

#define MAX_STR_LEN     4096
#define MAX_STR_AREALEN 20
#define MAX_SPLIT_WORDS 6
#define MAX_AREAS       1000

#define WITHOUT_BANK(addr)  (addr & 0x0000FFFF)
#define ONLY_BANK(addr)     ((addr & 0xFFFF0000) >> 16)

#define BANKED_NO     0
#define BANKED_YES    1


typedef struct bank_item {

    char     name[30];
    uint32_t start;
    uint32_t end;
    int      is_banked;
} bank_item;

typedef struct area_rec {
    bank_item bank;
    uint32_t  size_total;
    uint32_t  size_used;
    int       bank_num;
} area_rec;


const bank_item banks[] = {
    {"ROM  ",   0x0000, 0x3FFF, BANKED_NO},
    {"ROM_",    0x4000, 0x7FFF, BANKED_YES},
    {"VRAM_",    0x8000, 0x9FFF, BANKED_YES},
    {"XRAM_",  0xA000, 0xBFFF, BANKED_YES},
    {"WRAM_0  ", 0xC000, 0xCFFF, BANKED_NO},
    {"WRAM_1_",  0xD000, 0xDFFF, BANKED_YES},
};


char filename_in[MAX_STR_LEN] = {'\0'};
area_rec area_list[MAX_AREAS];
int area_count = 0;


uint32_t min(uint32_t a, uint32_t b) {
    return (a < b) ? a : b;
}

uint32_t max(uint32_t a, uint32_t b) {
    return (a > b) ? a : b;
}


// TODO: instead of simple size calc, store all used ranges and merge them when necessary
// TODO: log all area names
void add_rec(bank_item bank, uint32_t size, int bank_num) {

    int c;

    // Check to see if key matches any entries,
    for (c=0; c < area_count; c++) {

        // If a match was found, update it
        if ((bank.start == area_list[c].bank.start) &&
            (bank_num == area_list[c].bank_num)) {
            // Append size
            area_list[c].size_used += size;
            return;
        }
    }

    // no match was found, initialize new area if possible
    if (area_count < MAX_AREAS) {

        area_list[area_count].bank = bank;
        if (bank.is_banked == BANKED_YES) // Append bank name if banked
            sprintf(area_list[area_count].bank.name, "%s%d", bank.name, bank_num);
        area_list[area_count].size_used = size;
        area_list[area_count].size_total = bank.end - bank.start + 1;
        area_list[area_count].bank_num = bank_num;

        area_count++;
    }
}


// Returns amount of space used in the bank, if zero then no overlap or none used
uint32_t bank_overlap(bank_item bank, uint32_t addr_start, uint32_t addr_end) {

    uint32_t size_used;

    // Check whether the address range (stripped of bank bits)
    // *doesn't* overlap with the current bank
    if ((WITHOUT_BANK(addr_start) > bank.end) ||
        (WITHOUT_BANK(addr_end)   < bank.start)) {
        return 0; // no overlap
    } else {

        // Calculate size used in bank.
        // Note: Some address ranges can span multiple banks,
        //       such as CODE in 32K non-MBC mode (vs MBC 16K sizes)
        //       (in that case it will be added to both CODE and CODE_0)
printf("min/max:%x - %x\n", max(WITHOUT_BANK(addr_start), bank.start),
                            min(WITHOUT_BANK(addr_end), bank.end));
        size_used = min(WITHOUT_BANK(addr_end), bank.end) -
                    max(WITHOUT_BANK(addr_start), bank.start) + 1;

    printf("++++: %s %x %x : %x %x  (size %x) (bank %d)\n\n", bank.name,
                                 bank.start, bank.end,
                                 WITHOUT_BANK(addr_start),
                                 WITHOUT_BANK(addr_end),
                                 size_used,
                                 ONLY_BANK(addr_start) );

        return size_used;

    }
}


void check_banks(uint32_t addr_start, uint32_t addr_end) {

    int c;
    uint32_t size_used;
    int bank_num;

    // Loop through all banks and log any that overlap
    // (may be more than one)
    for(c = 0; c < ARRAY_LEN(banks); c++) {

        size_used = bank_overlap(banks[c], addr_start, addr_end);

        if (size_used > 0) {
            bank_num = ONLY_BANK(addr_start);
            add_rec(banks[c], size_used, bank_num);
        }
    }
}



// qsort compare function
static int area_rec_compare(const void* a, const void* b) {
    // rule for comparison

    return strcmp(((area_rec *)a)->bank.name,
                  ((area_rec *)b)->bank.name);
}


void print_all_recs(void) {
    int c;
    uint32_t area_size = 0x3FFF; // Default area size is 16K

    // Sort by name
    qsort (area_list, area_count, sizeof(area_rec), area_rec_compare);

    fprintf(stdout, "\n");

    fprintf(stdout,"Bank + #     Bank Range          Size    Used  Used%%    Free  Free%% \n"
                   "----------   ----------------   -----   -----  -----   -----  -----\n");

    // Print all records
    for (c = 0; c < area_count; c++) {

        fprintf(stdout,"%-13s",area_list[c].bank.name); // Bank name

        // if (area_list[c].bank.is_banked == BANKED_YES)
        //     fprintf(stdout,"%-4d",area_list[c].bank_num);    // Bank num
        // else
        //    fprintf(stdout,"    ");

        fprintf(stdout,"0x%04X -> 0x%04X",area_list[c].bank.start,
                                      area_list[c].bank.end); // Address Start -> End

        fprintf(stdout,"%8d", area_list[c].size_total); // Area size

        fprintf(stdout,"%8d", area_list[c].size_used); // Used

        fprintf(stdout,"   %3d%%", (area_list[c].size_used * (uint32_t)100) / area_list[c].size_total); // Percent Used

        fprintf(stdout,"%8d", area_list[c].size_total - area_list[c].size_used); // Free

        // fprintf(stdout,"   %3d%%",  100 - ((area_list[c].size_used * (uint32_t)100) / area_list[c].size_total)); // Percent Used
        fprintf(stdout,"   %3d%%", ((area_list[c].size_total - area_list[c].size_used) * (uint32_t)100) / area_list[c].size_total); // Percent Used

        fprintf(stdout,"\n");
    }
}


int parse_map_file(void) {

    char cols;
    char * p_str;
    char * p_words[MAX_SPLIT_WORDS];
    char strline_in[MAX_STR_LEN] = "";
    FILE * map_file = fopen(filename_in, "r");

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
                    printf(">> %s  %s  %s \n", p_words[0],
                                               p_words[1],
                                               p_words[2]);


// TODO!!!!!!!!!!! Prevent duplicates in 32K CODE mode, -> Log all entries and suppress duplciates from MULTI_PAGINATION
// collect all entries
// merge any that overlap
// then process them

// TODO: handle headers/etc better. Filter them out for now
                    if ((strtol(p_words[1], NULL, 16) != 0x00000000) &&
                       (strtol(p_words[2], NULL, 16) != 0))  // Size != 0
                    {
                        check_banks( strtol(p_words[1], NULL, 16),  // [1] Area Hex Address Start
                                     strtol(p_words[1], NULL, 16) +
                                     strtol(p_words[2], NULL, 16) - 1); // [1] + [3] Hex Size - 1 = Area End
                    }
                }
            } // end: if valid start of line

        } // end: while still lines to process

        fclose(map_file);

    } // end: if valid file
    else return (false);

   return true;
}


void display_help(void) {
    fprintf(stdout,
           "romusage input_file.map [options]\n"
           "\n"
           "Options\n"
           "-h : Show this help\n"
           "\n"
           "Use: Read a map file to display area sizes.\n"
           "Example: \"romusage build/MyProject.map\"\n"
           "\n"
           "Note: Usage estimates are for a given Area only.\n"
           "      They **do not** factor in whether multiple areas share\n"
           "      the same bank (such as HOME, CODE, GS_INIT, etc).\n"
           );
}


int handle_args( int argc, char * argv[] ) {

    int i;

    if( argc < 2 ) {
        display_help();
        return false;
    }

    // Copy input filename (if not preceded with option dash)
    if (argv[1][0] != '-')
        strncpy(filename_in, argv[1], sizeof(filename_in));

    // Start at first optional argument, argc is zero based
    for (i = 1; i <= (argc -1); i++ ) {

        if (strstr(argv[i], "-h")) {
            display_help();
            return false;  // Don't parse input when -h is used
        }
    }

    return true;
}


int main( int argc, char *argv[] )  {


    if (handle_args(argc, argv)) {

        if (parse_map_file()) {
            print_all_recs();
        } else {
            printf("Unable to open file! %s\n", filename_in);
            return 1; // Exit with failure
        }
    } else {
        return 1; // Exit with failure
    }

    return 0;
}