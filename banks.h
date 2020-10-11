// This is free and unencumbered software released into the public domain.
// For more information, please refer to <https://unlicense.org>
// bbbbbr 2020

#define ARRAY_LEN(A)  (sizeof(A) / sizeof(A[0]))
#define WITHOUT_BANK(addr)  (addr & 0x0000FFFF)
#define BANK_GET_NUM(addr)     ((addr & 0xFFFF0000) >> 16)
#define RANGE_SIZE(MIN, MAX) (MAX - MIN + 1)

#define AREA_MAX_STR 20
#define MAX_AREAS     100

#define BANK_MAX_STR 20
#define BANKED_NO     0
#define BANKED_YES    1
#define MAX_BANKS     1000


typedef struct area_item {
    char     name[AREA_MAX_STR];
    uint32_t start;
    uint32_t end;
} area_item;


typedef struct bank_item {
    // Fixed values
    char     name[BANK_MAX_STR];
    uint32_t start;
    uint32_t end;
    int      is_banked;

    // Updateable values
    uint32_t size_total;
    uint32_t size_used;
    int      bank_num;

    area_item area_list[MAX_AREAS]; // TODO: dynamic allcoation
    int       area_count;
} bank_item;


void banks_check(area_item area);
void bank_list_printall(void);