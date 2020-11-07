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
#include "ihx_file.h"

// Example data to parse from a .ihx file
// No area names
// : 01 0020 00 E9 F6
// S BB AAAA RR DD CC
//
// S:    Start (":") (1 char)
// BB:   ByteCount   (2 chars, one byte)
// AAAA: Address     (4 chars, two bytes)
// RR:   Record Type (2 chars, one byte. 00=data, 01=EOF, Others)
// DD:   Data        (<ByteCount>  bytes)
// CC:   Checksum    (2 chars, one byte. Sum record bytes, 2's complement of LSByte)
/*
:01002000E9F6
:05002800220D20FCC9BF
:070030001A22130D20FAC98A
.
.
.
:00000001FF (EOF indicator)
*/

#define MAX_STR_LEN     4096
#define IHX_DATA_LEN_MAX 255
#define IHX_REC_LEN_MIN  (1 + 2 + 4 + 2 + 0 + 2) // Start(1), ByteCount(2), Addr(4), Rec(2), Data(0..255x2), Checksum(2)

// IHX record types
#define IHX_REC_DATA     0x00
#define IHX_REC_EOF      0x01
#define IHX_REC_EXTSEG   0x02
#define IHX_REC_STARTSEG 0x03
#define IHX_REC_EXTLIN   0x04
#define IHX_REC_STARTLIN 0x05

// TODO: merge this with c2hex() and also add bytes for checksum
int check_hex(char * c) {
    while (*c != '\0') {
        if ((*c >= '0') && (*c <= '9'))
            c++;
        if ((*c >= 'A') && (*c <= 'F'))
            c++;
        if ((*c >= 'a') && (*c <= 'f'))
            c++;
        else
            return false;

    }
    return true;
}

//TODO: readbyte ... update checksum
uint16_t c2hex(char c) {
    if ((c >= '0') && (c <= '9'))
        return (uint16_t)(c - '0');
    if ((c >= 'A') && (c <= 'F'))
        return (uint16_t)(c - 'A' + 10);
    if ((c >= 'a') && (c <= 'f'))
        return (uint16_t)(c - 'a' + 10);
}

int ihx_file_process_areas(char * filename_in) {

    char cols;
    char * p_str;
    char strline_in[MAX_STR_LEN] = "";
    FILE * ihx_file = fopen(filename_in, "r");
    area_item area;

    int rec_length;
    uint16_t byte_count;
    uint16_t address;
    uint16_t address_end;
    uint16_t record_type;

    // Initialize area record
    snprintf(area.name, sizeof(area.name), "ihx record");
    area.exclusive = option_all_areas_exclusive; // Default is false
    area.start = 0xFFFF;
    area.end   = 0xFFFF;

int rec=0;
int t_rec=0;
    if (ihx_file) {

        // Read one line at a time into \0 terminated string
        while (fgets(strline_in, sizeof(strline_in), ihx_file) != NULL) {

            p_str = strline_in;

            // TODO: move into ihx_record_validate() or ihx_record_read()

            // remove line feed if needed
            rec_length = strlen(p_str);
            if (p_str[rec_length - 1] == '\n' || p_str[rec_length - 1] == '\r') {
                p_str[rec_length - 1] = '\0';
                rec_length--;
            }

           // Require minimum length
            if (rec_length < IHX_REC_LEN_MIN) {
                printf("Warning: IHX: Invalid line, too few characters: %s. Is %d, needs at least %d \n", p_str, rec_length, IHX_REC_LEN_MIN);
                continue;
            }

            // Only parse lines that start with ':' character (Start token for IHX record)
            if (p_str[0] != ':') {
                printf("Warning: IHX: Invalid start of line token for line: %s \n", strline_in);
                continue;
            }

            // Only hex characters are allower after start token
            p_str++; // Advance past Start code
            if (check_hex(p_str)) {
                printf("Warning: IHX: Invalid line, non-hex characters present: %s\n", strline_in);
                continue;
            }

            byte_count  = (c2hex(*p_str++) << 4)  + c2hex(*p_str++);
            address     = (c2hex(*p_str++) << 12) + (c2hex(*p_str++) << 8) + (c2hex(*p_str++) << 4) + c2hex(*p_str++);
            address_end = address + byte_count - 1;
            record_type = (c2hex(*p_str++) << 4)  + c2hex(*p_str++);

            int calc_length = IHX_REC_LEN_MIN + (byte_count * 2);

            // Require expected data byte count to fit within record length (at 2 chars per hex byte)
            if (rec_length != (IHX_REC_LEN_MIN + (byte_count * 2))) {
                printf("Warning: IHX: byte count doesn't match length available in record! Record length = %d, Calc length = %d, bytecount = %d \n", rec_length, calc_length, byte_count);
                continue;
            }

            t_rec++;
// TODO: handle other data record types
            // If this is the last record then process the pending banks check and exit
            // Ignore other non de-default data records since they don't seem to occur for gbz80
            if (record_type == IHX_REC_EOF) {
                rec++;
                printf("%d (%d)\n", rec, t_rec);
                banks_check(area);
                continue;
            } else if (record_type != IHX_REC_DATA)
                continue;


// TODO: checksum

            // Try to merge address-adjacent records to reduce number stored
            // (since most are only 32 bytes long)
            if (address == area.end + 1) {
                area.end = address_end;  // append to previous area
                printf("merge (append ): %4x - %4x ... %4x - %4x\n", area.start, area.end, address, address_end);
            } else if (address_end == area.start + 1) {
                area.start = address;   // pre-pend to previuos area
                printf("merge (prepend): %4x - %4x ... %4x - %4x\n", area.start, area.end, address, address_end);
            } else {
                rec++;
                printf("%d (%d)\n", rec, t_rec);
                // New record was not adjacent to last, so process the last/pending record
                banks_check(area);

                // Now queue current record as pending for next loop
                area.start = address;
                area.end   = area.start + byte_count - 1;
            }

        } // end: while still lines to process

        fclose(ihx_file);

    } // end: if valid file
    else return (false);

   return true;
}

