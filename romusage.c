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
#include "map_file.h"
#include "noi_file.h"

void display_help(void);
int handle_args(int argc, char * argv[]);

char filename_in[MAX_STR_LEN] = {'\0'};


void display_help(void) {
    fprintf(stdout,
           "romusage input_file.[map|noi] [options]\n"
           "\n"
           "Options\n"
           "-h  : Show this help\n"
           "-a  : Show Areas in each Bank\n"
           "-sH : Show HEADER Areas (normally hidden)\n"
           "-g  : Show a small usage graph per bank\n"
           "-G  : Show a large usage graph per bank\n"
           "-m  : Manually specify an Area -m:NAME:HEXADDR:HEXLENGTH\n"
           "-e  : Manually specify an Area that should not overlap -e:NAME:HEXADDR:HEXLENGTH\n"
           "-E  : All areas are exclusive (except HEADERs), warn for any overlaps\n"
           "-q  : Quiet, no output except warnings and errors\n"
           "\n"
           "Use: Read a .map or .noi file to display area sizes.\n"
           "Example 1: \"romusage build/MyProject.map\"\n"
           "Example 2: \"romusage build/MyProject.noi -a\"\n"
           "\n"
           "Note: Estimates are as close as possible, but may not be complete.\n"
           "      Unless specified with -m/-e they *do not* factor regions lacking\n"
           "      complete ranges in the Map/Noi file, for example Shadow OAM and Stack.\n"
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
        snprintf(filename_in, sizeof(filename_in), "%s", argv[1]);

    // Start at first optional argument, argc is zero based
    for (i = 1; i <= (argc -1); i++ ) {

        if (strstr(argv[i], "-h")) {
            display_help();
            return false;  // Don't parse input when -h is used
        } else if (strstr(argv[i], "-a")) {
            banks_output_show_areas(true);
        } else if (strstr(argv[i], "-sH")) {
            banks_output_show_headers(true);

        } else if (strstr(argv[i], "-g")) {
            banks_output_show_minigraph(true);
        } else if (strstr(argv[i], "-G")) {
            banks_output_show_largegraph(true);
        } else if (strstr(argv[i], "-E")) {
            set_option_all_areas_exclusive(true);
        } else if (strstr(argv[i], "-q")) {
            set_option_quiet_mode(true);

        } else if (strstr(argv[i], "-m") || strstr(argv[i], "-e")) {
            if (!area_manual_add(argv[i])) {
            fprintf(stdout,"malformed manual area argument: %s\n\n", argv[i]);
            display_help();
            return false;  // Don't parse input when -h is used
            }
        } else if (argv[i][0] == '-') {
            fprintf(stdout,"Unknown argument: %s\n\n", argv[i]);
            display_help();
            return false;  // Don't parse input when -h is used
        }

    }

    return true;
}


int matches_extension(char * filename, char * extension) {
    return (strcmp(filename + (strlen(filename) - strlen(extension)), extension) == 0);
}


int main( int argc, char *argv[] )  {

    if (handle_args(argc, argv)) {

        // Must at least have extension
        if (strlen(filename_in) >=5) {
            // detect file extension
            if (matches_extension(filename_in, (char *)".noi")) {
                if (noi_file_process_areas(filename_in)) {
                    banklist_finalize_and_show();
                    return 0; // Exit with success
                }
            } else if (matches_extension(filename_in, (char *)".map")) {
                if (map_file_process_areas(filename_in)) {
                    banklist_finalize_and_show();
                    return 0; // Exit with success
                }
            }
        }
    } else {
        return 1;
    }

    printf("Problem with filename or unable to open file! %s\n", filename_in);
    return 1; // Exit with failure by default
}