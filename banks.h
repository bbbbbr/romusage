// This is free and unencumbered software released into the public domain.
// For more information, please refer to <https://unlicense.org>
// bbbbbr 2020

#ifndef _BANKS_H
#define _BANKS_H

#define ARRAY_LEN(A)  (sizeof(A) / sizeof(A[0]))
#define WITHOUT_BANK(addr)  (addr & 0x0000FFFF)
#define BANK_GET_NUM(addr)     ((addr & 0xFFFF0000) >> 16)
#define RANGE_SIZE(MIN, MAX) (MAX - MIN + 1)

#define AREA_MAX_STR 20
#define MAX_AREAS     250 // Max areas per bank // TODO: dynamic area per-bank allocation

#define BANK_MAX_STR 20
#define BANKED_NO     0
#define BANKED_YES    1
#define MAX_BANKS     200  // TODO: Should probably make this grow dynamically

#define MINIGRAPH_SIZE (2 * 14) // Number of characters wide (inside edge brackets)
#define LARGEGRAPH_BYTES_PER_CHAR 16


typedef struct area_item {
    char     name[AREA_MAX_STR];
    uint32_t start;
    uint32_t end;
    uint32_t length;
    bool     exclusive;
} area_item;


typedef struct bank_item {
    // Fixed values
    char     name[BANK_MAX_STR];
    uint32_t start;
    uint32_t end;
    int      is_banked;
    uint32_t overflow_end;

    // Updateable values
    uint32_t size_total;
    uint32_t size_used;
    int      bank_num;

    area_item area_list[MAX_AREAS]; // TODO: dynamic allcoation
    int       area_count;
} bank_item;

extern bool banks_display_areas;
extern bool banks_display_headers;
extern bool banks_display_minigraph;
extern bool banks_display_largegraph;
extern bool option_all_areas_exclusive;
extern bool option_quiet_mode;
extern bool option_suppress_duplicates;
extern bool option_error_on_warning;
extern bool exit_error;

int area_manual_add(char * arg_str);
uint32_t bank_areas_calc_used(bank_item *, uint32_t, uint32_t);

void banks_output_show_areas(bool do_show);
void banks_output_show_headers(bool do_show);
void banks_output_show_minigraph(bool do_show);
void banks_output_show_largegraph(bool do_show);

void set_option_all_areas_exclusive(bool value);
void set_option_quiet_mode(bool value);
void set_option_suppress_duplicates(bool value);
void set_option_error_on_warning(bool value);
void set_exit_error(void);
bool get_exit_error(void);



void banks_check(area_item area);
void banklist_finalize_and_show(void);

#endif // _BANKS_H