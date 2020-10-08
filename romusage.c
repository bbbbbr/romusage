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
void add_rec(char * str_area, long value);
static int area_rec_compare(const void* a, const void* b);
void print_all_recs(void);
void parse_symfile_line(char * str_in);


#define MAX_STR_LEN     4096
#define MAX_SPLIT_WORDS 6
#define MAX_AREAS       20

enum {
    USAGE_NONE,
    USAGE_16K,
    USAGE_32K
};

typedef struct area_rec {
    char str_area[MAX_STR_LEN];
    long value;
} area_rec;

area_rec area_list[MAX_AREAS];
int area_count = 0;

int usage_mode = USAGE_NONE; // default to usage turned off


void add_rec(char * str_area, long value) {
    int c;

    // check to see if key matches any entries
    for (c = 0; c < area_count; c++) {

        // if a match is found, add it
        if (0 == strncmp(area_list[c].str_area,
                         str_area,
                         sizeof(area_list[c].str_area))) {
            area_list[c].value += value;
            return;
        }
    }

    // If no match was found, create a new entry (if possible)
    if (area_count < MAX_AREAS) {
        area_count++;
        strncpy(area_list[c].str_area,
                str_area,
                sizeof(area_list[c].str_area));
        area_list[c].value = value;
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
        fprintf(stdout,"Area         Size\n"
                       "-----        ----\n");
    } else {
        fprintf(stdout,"Area         Size    Used  Remains\n"
                       "-----        ----    ----  -------\n");
    }


    // Print all records
    for (c = 0; c < area_count; c++) {
        fprintf(stdout,"%-12s %-7ld ",area_list[c].str_area, area_list[c].value);   // Area, Size

        // Only print utilization for CODE areas
        if ((usage_mode != USAGE_NONE)  &&
            strstr(area_list[c].str_area, "CODE")) {

            // Percentage
            fprintf(stdout,"%%%-4ld ", (area_list[c].value * 100) / area_size);

            // Free
            fprintf(stdout,"%-5ld ", area_size - area_list[c].value);
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
        // Don't repeat areas (can be duplicated across multiple output "pages"
        if (0 != strncmp(str_area_last,p_words[0], sizeof(str_area_last))) {
            // [0] = Area Name
            // [1] = Hex Address Start
            // [2] = Hex Size
            // [3] = Decimal Size
            // [4] = 'bytes'
            // [5] = '(ABS/REL,CON)'
            fprintf(stdout,"%-12s:",p_words[0]);   // [0] = Area
            fprintf(stdout,"%8lx - %-8lx",strtol(p_words[1], NULL, 16),
                                          strtol(p_words[1], NULL, 16) + strtol(p_words[2], NULL, 16) -1 ); // Address Start -> End
            fprintf(stdout,"%8ld %s",strtol(p_words[3], NULL, 10), p_words[4]);

            fprintf(stdout, "\n");
        }

        // Copy area name to check against next pass
        strncpy(str_area_last, p_words[0], sizeof(str_area_last));
//        add_rec(p_words[0], strtol(p_words[3], NULL, 16)); // [1] = Area, [3] = Hex Size string converted to decimal
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
                   "Note: Usage estimates are for Area only, and do not factor in whether multiple areas share the same bank (such as HOME, CODE, GS_INIT, etc).\n"
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
