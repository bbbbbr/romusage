// This is free and unencumbered software released into the public domain.
// For more information, please refer to <https://unlicense.org>
// bbbbbr 2020

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>

#include "banks.h"
#include "areas.h"

void display_help(void);
int handle_args(int argc, char * argv[]);

char filename_in[MAX_STR_LEN] = {'\0'};


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


int handle_args(int argc, char * argv[]) {

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

        if (areas_process_map_file(filename_in)) {

            bank_list_printall();
            return 0; // Exit with success
        } else {

            printf("Unable to open file! %s\n", filename_in);
        }
    }

    return 1; // Exit with failure by default
}