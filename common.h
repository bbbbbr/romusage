// This is free and unencumbered software released into the public domain.
// For more information, please refer to <https://unlicense.org>
// bbbbbr 2020

#ifndef _COMMON_H
#define _COMMON_H

#define DEFAULT_STR_LEN 100

#define OPT_AREA_SORT_DEFAULT    0
#define OPT_AREA_SORT_SIZE_DESC  1
#define OPT_AREA_SORT_ADDR_ASC   2
#define OPT_AREA_SORT_HIDE       3

#define OPT_INPUT_SRC_NONE 0
#define OPT_INPUT_SRC_CDB  1
#define OPT_INPUT_SRC_NOI  2
#define OPT_INPUT_SRC_MAP  3
#define OPT_INPUT_SRC_IHX  4
#define OPT_INPUT_SRC_ROM  5

#define OPT_AREA_HIDE_SIZE_DEFAULT 0

extern bool banks_display_areas;
extern bool banks_display_headers;
extern bool banks_display_minigraph;
extern bool banks_display_largegraph;
extern bool option_all_areas_exclusive;
extern bool option_quiet_mode;
extern bool option_suppress_duplicates;
extern bool option_error_on_warning;
extern bool option_hide_banners;
extern int  option_input_source;
extern int  option_area_sort ;
extern uint32_t option_area_hide_size;
extern bool exit_error;


void set_option_all_areas_exclusive(bool value);
void set_option_quiet_mode(bool value);
void set_option_suppress_duplicates(bool value);
void set_option_error_on_warning(bool value);
void set_option_hide_banners(bool value);
void set_option_input_source(int value);
void set_option_area_sort(int value);
void set_option_area_hide_size(uint32_t value);

int get_option_input_source(void);
int get_option_area_sort(void);
bool get_option_hide_banners(void);
uint32_t get_option_area_hide_size(void);

#endif // _COMMON_H