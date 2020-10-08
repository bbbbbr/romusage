// console_main.c

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

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


int handle_args(int, char * []);
void add_rec(char * str_area, long addr, long size);
static int area_rec_compare(const void* a, const void* b);
void print_all_recs(void);
void parse_symfile_line(char * str_in);


#define MAX_STR_LEN     4096
#define MAX_STR_AREALEN 100
#define MAX_SPLIT_WORDS 6
#define MAX_AREAS       30

enum {
    USAGE_NONE,
    USAGE_16K,
    USAGE_32K
};

typedef struct area_rec {
    char str_area[MAX_STR_AREALEN];
    long addr;
    long size;
} area_rec;

area_rec area_list[MAX_AREAS];
int area_count = 0;

int usage_mode = USAGE_NONE; // default to usage turned off


void add_rec(char * str_area, long addr, long size) {
    int c;

    // check to see if key matches any entries
    for (c = 0; c < area_count; c++) {

        // prevent duplicates, reject if entry already exists
        if (0 == strncmp(area_list[c].str_area,
                         str_area,
                         sizeof(area_list[c].str_area))) {
            return;
        }
    }

    // If no match was found, create a new entry (if possible)
    if (area_count < MAX_AREAS) {
        area_count++;
        strncpy(area_list[c].str_area,
                str_area,
                sizeof(area_list[c].str_area));
        area_list[c].addr = addr;
        area_list[c].size = size;
    }
}


// qsort compare function
static int area_rec_compare(const void* a, const void* b) {
    // rule for comparison
    area_rec *recA = (area_rec *)a;
    area_rec *recB = (area_rec *)b;
    return strcmp(((area_rec *)a)->str_area,
                  ((area_rec *)b)->str_area);
}


void print_all_recs(void) {
    int c;
    long area_size = 0x3FFF; // Default area size is 16K

    if (usage_mode == USAGE_32K) area_size = 0x7FFF;  // Update area size if needed

    // Sort first
    qsort (area_list, area_count, sizeof(area_rec), area_rec_compare);

    if (usage_mode == USAGE_NONE) {
        fprintf(stdout,"Area        Addr                Size \n"
                       "-----       -----------------   -----\n");
    } else {
        fprintf(stdout,"Area        Addr                Size         Used  Remains\n"
                       "-----       -----------------   -----        ----  -------\n");
    }


    // Print all records
    for (c = 0; c < area_count; c++) {
            fprintf(stdout,"%-13s",area_list[c].str_area); // Area name
            fprintf(stdout,"%6lx -> %6lx",area_list[c].addr,
                                         area_list[c].addr + area_list[c].size -1 ); // Address Start -> End
            fprintf(stdout,"%8ld bytes",area_list[c].size); // Area size


        // Only print utilization for CODE areas
        if ((usage_mode != USAGE_NONE)  &&
            strstr(area_list[c].str_area, "CODE")) {

            fprintf(stdout,"  %%%-5ld", (area_list[c].size * 100) / area_size); // Percentage
            fprintf(stdout,"%5ld ", area_size - area_list[c].size); // Remaining
        }

        fprintf(stdout,"\n");
    }
}


void parse_symfile_line(char * str_in) {

    char cols = 0;
    char * p_words[MAX_SPLIT_WORDS];
    char * p_str = strtok (str_in," ");
    static char str_area_last[MAX_STR_LEN] = "";

    // Split string into words separated by spaces
    while (p_str != NULL)
    {
        p_words[cols++] = p_str;
        p_str = strtok(NULL, " =.");
        if (cols >= MAX_SPLIT_WORDS) break;
    }

    // Only use if exact match on columns in Area table (6)
    if ((cols == 6) &&
        !(strstr(str_in, "SFR")) && // Exclude SFR areas
        !(strstr(str_in, "HEADER")) && // Exclude HEADER areas
        !(strtol(p_words[3], NULL, 10) == 0))  // Exclude empty areas
     {
        add_rec(p_words[0], strtol(p_words[1], NULL, 16), strtol(p_words[3], NULL, 10)); // [0] Area, [1] Hex Address, [3] Decimal Size
    }


}


int main( int argc, char *argv[] )  {

    char strline_in[MAX_STR_LEN];

    if (handle_args(argc, argv)) {

        fprintf(stdout, "\n");

        // Read one line at a time into \0 terminated string
        while ( fgets(strline_in, sizeof(strline_in), stdin) != NULL) {

            // Only parse lines that start with '_' character (area summary lines)
            // if (strstr(strline_in, "flags"))
            if (strline_in[0] == '_')
                parse_symfile_line(strline_in);
        }

        print_all_recs();
    }

    return 0;
}



int handle_args( int argc, char * argv[] ) {

    int i;

    // Start at first optional argument, argc is zero based
    for (i = 1; i <= (argc -1); i++ ) {

        if (strstr(argv[i], "-h")) {
            printf("romusage\n\n"
                   "-h : Show this help\n"
                   "-u32k : Estimate usage with 32k Areas\n"
                   "-u16k : Estimate usage with 16k Areas\n"
                   "\n"
                   "Use: Pipe symbol files in to parse them.\n"
                   "Example: \"more build/*.sym build/res/*.sym | romusage\"\n"
                   "\n"
                   "Note: Usage estimates are for a given Area only.\n"
                   "They **do not** factor in whether multiple areas share\n"
                   "the same bank (such as HOME, CODE, GS_INIT, etc).\n"
                   );
            return false;  // Don't parse input when -h is used
        }
        else if (strstr(argv[i], "-u16k")) {
            usage_mode = USAGE_16K;
        }
        else if (strstr(argv[i], "-u32k")) {
            usage_mode = USAGE_32K;
        }
    }

    return true;
}
