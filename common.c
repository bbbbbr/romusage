// This is free and unencumbered software released into the public domain.
// For more information, please refer to <https://unlicense.org>
// bbbbbr 2020

#include <stdbool.h>
#include <stdint.h>

#include "common.h"

bool banks_display_areas        = false;
bool banks_display_headers      = false;
bool banks_display_minigraph    = false;
bool banks_display_largegraph   = false;
bool option_all_areas_exclusive = false;
bool option_quiet_mode          = false;
bool option_suppress_duplicates = true;
bool option_error_on_warning    = false;
bool option_hide_banners        = false;
int  option_input_source        = OPT_INPUT_SRC_NONE;
int  option_area_sort           = OPT_AREA_SORT_DEFAULT;
uint32_t option_area_hide_size  = OPT_AREA_HIDE_SIZE_DEFAULT;
bool exit_error                 = false;



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
// Turn on/off quiet mode
void set_option_quiet_mode(bool value) {
    option_quiet_mode = value;
}

// Turn on/off suppression of duplicates
void set_option_suppress_duplicates(bool value) {
    option_suppress_duplicates = value;
}

// Turn on/off setting an error on exit for serious warnings encountered
void set_option_error_on_warning(bool value) {
    option_error_on_warning = value;
}

// Turn on/off banners
void set_option_hide_banners(bool value) {
    option_hide_banners = value;
}

// Input source file format
void set_option_input_source(int value) {
    option_input_source = value;
}

// Area output sort order
void set_option_area_sort(int value) {
    option_area_sort = value;
}

// Hide areas smaller than size
void set_option_area_hide_size(uint32_t value) {
    option_area_hide_size = value;
}

// Area output sort order
int get_option_area_sort(void) {
    return option_area_sort;
}

// Turn on/off banners
bool get_option_hide_banners() {
    return option_hide_banners;
}

// Hide areas smaller than size
uint32_t  get_option_area_hide_size() {
    return option_area_hide_size;
}



void set_exit_error(void) {
    exit_error = true;
}

bool get_exit_error(void) {
    return exit_error;
}

