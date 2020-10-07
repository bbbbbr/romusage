// console_main.c

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

// Example data to parse from a .sym file (excluding unwanted lines):
// 0 _CODE                                      size    2B5   flags    0
// 1 _DATA                                      size    259   flags    0
// 2 _DABS                                      size      0   flags    8
// 3 _HOME                                      size      0   flags    0
// 4 _GSINIT                                    size      0   flags    0
// 5 _GSFINAL                                   size      0   flags    0
// 6 _CABS                                      size      0   flags    8

#define MAX_STR_LEN     4096
#define MAX_SPLIT_WORDS 6
#define MAX_AREAS       20


typedef struct area_rec {
    char str_area[MAX_STR_LEN];
    long value;
} area_rec;

area_rec area_list[MAX_AREAS];
int area_count = 0;


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
static int area_rec_compare(const void* a, const void* b)
{
    // rule for comparison
    area_rec *recA = (area_rec *)a;
    area_rec *recB = (area_rec *)b;
    return strcmp(((area_rec *)a)->str_area,
                  ((area_rec *)b)->str_area);
}


void print_all_recs(void) {
    int c;

    // Sort first
    qsort (area_list, area_count, sizeof(area_rec), area_rec_compare);

    fprintf(stdout,"Area         Size    Used  Free \n"
                   "-----        ----    ----  ----\n");
    // Print all records
    for (c = 0; c < area_count; c++) {
        fprintf(stdout,"%-12s %-7ld ",area_list[c].str_area, area_list[c].value);   // Area, Size

        // Only print utilization for CODE areas
        if (strstr(area_list[c].str_area, "CODE")) {

            // Percentage
            fprintf(stdout,"%%%-4ld ", (area_list[c].value * 100) / (area_list[c].value <= 0x3FFF ? 0x3FFF : 0x7FFF));

            // Free
            fprintf(stdout,"%-5ld ", (area_list[c].value <= 0x3FFF ? 0x3FFF : 0x7FFF) - area_list[c].value);
        }

        fprintf(stdout,"\n");
    }
}


void parse_symfile_line(char * str_in) {

    char cols = 0;
    char * p_words[MAX_SPLIT_WORDS];
    char * p_str = strtok (str_in," ");;

    // Split string into words separated by spaces
    while (p_str != NULL)
    {
        p_words[cols++] = p_str;
        p_str = strtok(NULL, " ");
        if (cols >= MAX_SPLIT_WORDS) break;
    }

    // Only use if exact match on columns in Area table (6)
    if (cols == 6) {
        // fprintf(stdout,"%s = ",p_words[1]);   // [1] = Area
        // fprintf(stdout,"%s --> ",p_words[3]); // [3] = Hex Size

        add_rec(p_words[1], strtol(p_words[3], NULL, 16)); // [1] = Area, [3] = Hex Size string converted to decimal
    }
}


int main( int argc, char *argv[] )  {

    char strline_in[MAX_STR_LEN];

    // Read one line at a time into \0 terminated string
    while ( fgets(strline_in, sizeof(strline_in), stdin) != NULL) {

        // Only parse lines that have a "flags" entry
        if (strstr(strline_in, "flags"))
            parse_symfile_line(strline_in);
    }

    print_all_recs();

    return 0;
}
