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

#define ADDR_UNSET   0xFFFF

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

typedef struct ihx_record {
    uint16_t length;
    uint16_t byte_count;
    uint16_t address;
    uint16_t address_end;
    uint16_t type;
    uint8_t checksum;
} ihx_record;

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


// Parse and validate an IHX record
int ihx_parse_and_validate(char * p_str, ihx_record * p_rec) {

        int calc_length = 0;
        int c;
        uint8_t checksum_calc = 0;

        // Remove trailing CR and LF
        p_rec->length = strlen(p_str);
        for (c = 0;c < p_rec->length;c++) {
            if (p_str[c] == '\n' || p_str[c] == '\r') {
                p_str[c] = '\0';   // Replace char with string terminator
                p_rec->length = c; // Update length
                break;
            }
        }

       // Require minimum length
        if (p_rec->length < IHX_REC_LEN_MIN) {
            printf("Warning: IHX: Invalid line, too few characters: %s. Is %d, needs at least %d \n", p_str, p_rec->length, IHX_REC_LEN_MIN);
            return false;
        }

        // Only parse lines that start with ':' character (Start token for IHX record)
        if (p_str[0] != ':') {
            printf("Warning: IHX: Invalid start of line token for line: %s \n", p_str);
            return false;
        }

        // Only hex characters are allower after start token
        p_str++; // Advance past Start code
        if (check_hex(p_str)) {
            printf("Warning: IHX: Invalid line, non-hex characters present: %s\n", p_str);
            return false;
        }

        p_rec->byte_count  = (c2hex(*p_str++) << 4)  + c2hex(*p_str++);
        p_rec->address     = (c2hex(*p_str++) << 12) + (c2hex(*p_str++) << 8) + (c2hex(*p_str++) << 4) + c2hex(*p_str++);
        p_rec->address_end = p_rec->address + p_rec->byte_count - 1;
        p_rec->type = (c2hex(*p_str++) << 4)  + c2hex(*p_str++);

        calc_length = IHX_REC_LEN_MIN + (p_rec->byte_count * 2);

        // Require expected data byte count to fit within record length (at 2 chars per hex byte)
        if (p_rec->length != calc_length) {
            printf("Warning: IHX: byte count doesn't match length available in record! Record length = %d, Calc length = %d, bytecount = %d \n", p_rec->length, calc_length, p_rec->byte_count);
            return false;
        }

        // Read data segment and calculate it's checksum
        checksum_calc = p_rec->byte_count + (p_rec->address & 0xFF) + ((p_rec->address >> 8) & 0xFF) + p_rec->type;
        for (c = 0;c < p_rec->byte_count;c++)
            checksum_calc += (c2hex(*p_str++) << 4)  + c2hex(*p_str++);

        // Final calculated checeksum is 2's complement of LSByte
        checksum_calc = ((checksum_calc & 0xFF) ^ 0xFF) + 1;

        // Read checksum from data
        p_rec->checksum = (c2hex(*p_str++) << 4)  + c2hex(*p_str++);

        if (p_rec->checksum != checksum_calc) {
            printf("Warning: IHX: record checksum %x didn't match calculated checksum %x\n", p_rec->checksum, checksum_calc);
            return false;
        }

        return true;
}


int ihx_file_process_areas(char * filename_in) {

    char cols;
    char strline_in[MAX_STR_LEN] = "";
    FILE * ihx_file = fopen(filename_in, "r");
    area_item area;
    ihx_record ihx_rec;

    // Initialize area record
    snprintf(area.name, sizeof(area.name), "ihx record");
    area.exclusive = option_all_areas_exclusive; // Default is false
    area.start = ADDR_UNSET;
    area.end   = ADDR_UNSET;

    if (ihx_file) {

        // Read one line at a time into \0 terminated string
        while (fgets(strline_in, sizeof(strline_in), ihx_file) != NULL) {

            // Parse record, skip if fails validation
            if (!ihx_parse_and_validate(strline_in, &ihx_rec))
                continue;

            // Process the pending area and exit if last record (EOF)
            // Also ignore non de-default data records (don't seem to occur for gbz80)
            if (ihx_rec.type == IHX_REC_EOF) {
                banks_check(area);
                continue;
            } else if (ihx_rec.type != IHX_REC_DATA)
                continue;

            // Try to merge address-adjacent records to reduce number stored
            // (since most are only 32 bytes long)
            if (ihx_rec.address == area.end + 1) {
                area.end = ihx_rec.address_end;  // append to previous area
            } else if (ihx_rec.address_end == area.start + 1) {
                area.start = ihx_rec.address;   // pre-pend to previuos area
            } else {
                // New record was not adjacent to last,
                // so process the last/pending record
                if (area.start != ADDR_UNSET)
                    banks_check(area);

                // Now queue current record as pending for next loop
                area.start = ihx_rec.address;
                area.end   = ihx_rec.address + ihx_rec.byte_count - 1;
            }

        } // end: while still lines to process

        fclose(ihx_file);

    } // end: if valid file
    else return (false);

   return true;
}

