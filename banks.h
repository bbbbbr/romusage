// This is free and unencumbered software released into the public domain.
// For more information, please refer to <https://unlicense.org>
// bbbbbr 2020

#ifndef _BANKS_H
#define _BANKS_H

#include "list.h"

#define MAX_ADDR_UNBANKED 0x0000FFFFU

#define ARRAY_LEN(A)         (sizeof(A) / sizeof(A[0]))
#define WITHOUT_BANK(addr)   (addr & 0x0000FFFFU)
#define BANK_GET_NUM(addr)   ((addr & 0xFFFF0000U) >> 16)
#define BANK_ONLY(addr)      (addr & 0xFFFF0000U)
#define RANGE_SIZE(MIN, MAX) (MAX - MIN + 1)
#define UNBANKED_END(start, end)  ((end - start) + WITHOUT_BANK(start))

#define AREA_MAX_STR DEFAULT_STR_LEN

#define BANK_MAX_STR DEFAULT_STR_LEN
#define BANKED_NO     0
#define BANKED_YES    1

#define MINIGRAPH_SIZE (2 * 14) // Number of characters wide (inside edge brackets)
#define LARGEGRAPH_BYTES_PER_CHAR 16


typedef struct area_item {
    char     name[AREA_MAX_STR];
    uint32_t start;
    uint32_t end;
    uint32_t start_unbanked;
    uint32_t end_unbanked;
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

    // TODO: track overflow bytes and report them in graph
    list_type area_list;
} bank_item;


int area_manual_add(char * arg_str);
uint32_t bank_areas_calc_used(bank_item *, uint32_t, uint32_t);

void banks_output_show_areas(bool do_show);
void banks_output_show_headers(bool do_show);
void banks_output_show_minigraph(bool do_show);
void banks_output_show_largegraph(bool do_show);

void banks_init(void);
void banks_cleanup(void);


void set_exit_error(void);
bool get_exit_error(void);



void banks_check(area_item area);
void banklist_finalize_and_show(void);

#endif // _BANKS_H