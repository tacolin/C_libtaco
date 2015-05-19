#include <stdio.h>
#include <string.h>
#include "pack.h"

#define derror(a, b...) fprintf(stderr, "[ERROR] %s(): "a"\n", __func__, ##b)
#define CHECK_IF(assertion, error_action, ...) \
{\
    if (assertion) \
    { \
        derror(__VA_ARGS__); \
        {error_action;} \
    }\
}

#define bit_address(bit)   ((bit) >> 3)
#define bit_offset(bit)    ((bit) %  8)

static unsigned char _get_bits(unsigned char* bytes, int ofs, int bit_num)
{
    CHECK_IF(bytes == NULL, return 0, "bytes is null");
    CHECK_IF(bit_offset(ofs) + bit_num > 8, return 0, "get more than 8 bits");

    unsigned char ret = bytes[bit_address(ofs)];
    ret >>= (8 - bit_offset(ofs) - bit_num);
    ret  &= (0xFF >> (8 - bit_num));
    return ret;
}

static void _put_bits(unsigned char* bytes, int ofs, int bit_num, unsigned char value)
{
    CHECK_IF(bytes == NULL, return, "bytes is null");
    CHECK_IF(bit_offset(ofs) + bit_num > 8, return, "put more than 8 bits");

    unsigned char mask = 0xFF << (8 - bit_num);
    mask = mask >> bit_offset(ofs);
    bytes    += bit_address(ofs);
    value   <<= (8 - bit_offset(ofs) - bit_num);
    bytes[0] &= ~mask;
    bytes[0] |= (value & mask);
    return;
}

unsigned int bits_get(void* bytes, int* ofs, int bit_num)
{
    CHECK_IF(bytes == NULL, return 0, "bytes is null");
    CHECK_IF(ofs == NULL, return 0, "ofs is null");
    CHECK_IF(bit_num <= 0, return 0, "bit_num = %d invalid", bit_num);
    CHECK_IF(bit_num > 32, return 0, "bit_num = %d invalid", bit_num);

    unsigned char* start = (unsigned char*)bytes + bit_address(*ofs);
    unsigned char* stop  = (unsigned char*)bytes + bit_address(*ofs+bit_num);
    int            bits  = bit_num;
    unsigned int   tmp   = 0;
    unsigned int   ret   = 0;

    if ((bit_offset(*ofs) + bit_num) <= 8)
    {
        ret = _get_bits((unsigned char*)bytes, *ofs, bit_num);
        *ofs += bit_num;
    }
    else if ((bit_offset(*ofs) + bit_num) < 40)
    {
        bits -= (8 - bit_offset(*ofs));
        tmp   = _get_bits(start, bit_offset(*ofs), (8 - bit_offset(*ofs))) << bits;
        ret  |= tmp;

        while (start++ < (stop - 1))
        {
            bits -= 8;
            tmp   = *start << bits;
            ret  |= tmp;
        }

        bits -= bit_offset(*ofs+bit_num);
        tmp  = _get_bits(stop, 0, bit_offset(*ofs+bit_num)) << bits;
        ret |= tmp;

        CHECK_IF(bits != 0, return 0, "some thing error");

        *ofs += bit_num;
    }
    return ret;
}

void bits_put(void* bytes, int* ofs, int bit_num, unsigned int value)
{
    CHECK_IF(bytes == NULL, return, "bytes is null");
    CHECK_IF(ofs == NULL, return, "ofs is null");
    CHECK_IF(bit_num <= 0, return, "bit_num = %d invalid", bit_num);
    CHECK_IF(bit_num > 32, return, "bit_num = %d invalid", bit_num);

    unsigned char* start = (unsigned char*)bytes + bit_address(*ofs);
    unsigned char* stop  = (unsigned char*)bytes + bit_address(*ofs + bit_num);
    int            bits  = bit_num;

    value = value & (0xFFFFFFFF >> (32 - bit_num));

    if ((bit_offset(*ofs) + bit_num) <= 8)
    {
        _put_bits((unsigned char*)bytes, *ofs, bit_num, (unsigned char)value);
    }
    else
    {
        bits -= (8 - bit_offset(*ofs));
        _put_bits(start, bit_offset(*ofs), (8 - bit_offset(*ofs)), value >> bits);

        while (start++ < (stop - 1))
        {
            bits   -= 8;
            *start  = (value >> bits) & 0xFF;
        }

        bits -= bit_offset(*ofs+bit_num);
        _put_bits(stop, 0, bit_offset(*ofs+bit_num), value & (0xFF >> (8 - bit_offset(*ofs+bit_num))));

        CHECK_IF(bits != 0, return, "some thing error");
    }
    *ofs += bit_num;
    return;
}

static unsigned char _get_bits_le(unsigned char* bytes, int ofs, int bit_num)
{
    unsigned char ret = bytes[bit_address(ofs)];

    CHECK_IF(bytes == NULL, return 0, "bytes is null");
    CHECK_IF((bit_offset(ofs)+bit_num) > 8, return 0, "get more than 8 bits");

    ret >>= bit_offset(ofs);
    ret  &= (0xFF >> (8 - bit_num));
    return ret;
}

static void _put_bits_le(unsigned char* bytes, int ofs, int bit_num, unsigned char value)
{
    unsigned char mask = 0xFF << (8 - bit_num);
    mask = mask >> (8 - bit_offset(ofs) - bit_num);

    CHECK_IF(bytes == NULL, return, "bytes is null");
    CHECK_IF((bit_offset(ofs)+bit_num) > 8, return, "put more than 8 bits");

    bytes    += bit_address(ofs);
    value   <<= bit_offset(ofs);
    bytes[0] &= ~mask;
    bytes[0] |= (value & mask);
}

unsigned int bits_get_le(void* bytes, int* ofs, int bit_num)
{
    unsigned char* start = ((unsigned char*)bytes) + bit_address(*ofs);
    unsigned char* stop  = ((unsigned char*)bytes) + bit_address(*ofs+bit_num);
    unsigned char  bits = 0;

    unsigned int tmp    = 0;
    unsigned int ret    = 0;

    CHECK_IF(bytes == NULL, return 0, "bytes is null");
    CHECK_IF(ofs == NULL, return 0, "ofs is null");
    CHECK_IF(bit_num <= 0, return 0, "bit_num = %d invalid", bit_num);
    CHECK_IF(bit_num > 32, return 0, "get more than 32 bits");

    if ((bit_offset(*ofs) + bit_num) <= 8)
    {
        ret = _get_bits_le((unsigned char*)bytes, *ofs, bit_num);
    }
    else if ((bit_offset(*ofs) + bit_num) < 40)
    {
        tmp  = _get_bits_le(start, bit_offset(*ofs), (8 - bit_offset(*ofs)));
        ret |= tmp;

        while (start++ < (stop - 1))
        {
            tmp      = *start << (8 - bit_offset(*ofs) + bits);
            ret     |= tmp;
            bits  += 8;
        }

        tmp  = _get_bits_le(stop, 0, bit_offset(*ofs+bit_num)) << (8 - bit_offset(*ofs) + bits);
        ret |= tmp;
    }
    *ofs += bit_num;
    return ret;
}

void bits_put_le(void* bytes, int *ofs, int bit_num, unsigned int value)
{
    unsigned char* start = ((unsigned char*)bytes) + bit_address(*ofs);
    unsigned char* stop  = ((unsigned char*)bytes) + bit_address(*ofs+bit_num);
    unsigned char  bits = 0;

    CHECK_IF(bytes == NULL, return, "bytes is null");
    CHECK_IF(ofs == NULL, return, "ofs is null");
    CHECK_IF(bit_num <= 0, return, "bit_num = %d invalid", bit_num);
    CHECK_IF(bit_num > 32, return, "bit_num = %d invalid", bit_num);

    value = value & (0xFFFFFFFF >> (32 - bit_num));

    if ((bit_offset(*ofs) + bit_num) <= 8)
    {
        _put_bits_le((unsigned char*)bytes, *ofs, bit_num, (unsigned char)value);
    }
    else
    {
        _put_bits_le(start, bit_offset(*ofs), (8 - bit_offset(*ofs)), value);

        while (start++ < (stop - 1))
        {
            *start = (value >> (8 - bit_offset(*ofs) + bits)) & 0xFF;
            bits += 8;
        }

        _put_bits_le(stop, 0, bit_offset(*ofs+bit_num), 0xFF & (value >> (8 - bit_offset(*ofs) + bits)));
    }
    *ofs += bit_num;
    return;
}

int bitmap_to_str(unsigned int bitmap, int bit_num, char* str_buf, int buf_size)
{
    CHECK_IF(str_buf == NULL, return -1, "str_buf is null");
    CHECK_IF(bit_num > 32, return -1, "bit_num = %d > 32", bit_num);

    char tmp_buf[39] = {0};
    int  tmp_size;

    bitmap <<= 32 - bit_num;
    if ((bit_num % 4) != 0)
    {
        bitmap >>= 4-(bit_num%4);
    }

    tmp_size = bit_num + bit_num / 4;
    if ((bit_num % 4) == 0)
    {
        tmp_size--;
    }
    else
    {
        tmp_size += 4 - (bit_num % 4);
    }

    while (tmp_size > 0)
    {
        if (bitmap & 0x80000000)
        {
            strcat(tmp_buf, "1");
        }
        else
        {
            strcat(tmp_buf, "0");
        }

        bitmap <<= 1;
        tmp_size--;

        if ((tmp_size > 0) && ((tmp_size % (4+1)) == 0))
        {
            strcat(tmp_buf, "-");
            tmp_size--;
        }
    }
    strncpy(str_buf, tmp_buf, buf_size);
    return 0;
}

int str_to_mac(char* str, unsigned char* mac_addr)
{
    CHECK_IF(str == NULL, return -1, "str is null");
    CHECK_IF(mac_addr == NULL, return -1, "mac_addr is null");
    CHECK_IF(strlen(str) != 17, return -1, "xx:xx:xx:xx:xx:xx:xx is 17 characters");

    unsigned int tmp_mac[6] = {0};
    int scan_ret = sscanf(str, "%02x:%02x:%02x:%02x:%02x:%02x",
                       (unsigned int*)&tmp_mac[0], (unsigned int*)&tmp_mac[1],
                       (unsigned int*)&tmp_mac[2], (unsigned int*)&tmp_mac[3],
                       (unsigned int*)&tmp_mac[4], (unsigned int*)&tmp_mac[5]);
    CHECK_IF(scan_ret != 6, return -1, "sscanf failed");

    *(mac_addr  ) = (unsigned char)tmp_mac[0];
    *(mac_addr+1) = (unsigned char)tmp_mac[1];
    *(mac_addr+2) = (unsigned char)tmp_mac[2];
    *(mac_addr+3) = (unsigned char)tmp_mac[3];
    *(mac_addr+4) = (unsigned char)tmp_mac[4];
    *(mac_addr+5) = (unsigned char)tmp_mac[5];
    return 0;
}

int mac_to_str(unsigned char* mac_addr, char* str_buf, int buf_size)
{
    CHECK_IF(mac_addr == NULL, return -1, "mac_addr is null");
    CHECK_IF(str_buf == NULL, return -1, "str_buf is null");
    CHECK_IF(buf_size < 18, return -1, "buf_size = %d < 18 (17+1)", buf_size);

    snprintf(str_buf, buf_size, "%02x:%02x:%02x:%02x:%02x:%02x",
             mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3],
             mac_addr[4], mac_addr[5]);
    return 0;
}

int str_to_ipv4(char* str, unsigned char* ipv4)
{
    CHECK_IF(str == NULL, return -1, "str is null");
    CHECK_IF(ipv4 == NULL, return -1, "ipv4 is null");
    CHECK_IF(strlen(str) < 7, return -1, "x.x.x.x is min 7 characters");
    CHECK_IF(strlen(str) > 15, return -1, "xxx.xxx.xxx.xxx is max 15 characters");

    unsigned int tmp_ipv4[4] = {0};
    int scan_ret = sscanf(str, "%u.%u.%u.%u",
                         (unsigned int*)&tmp_ipv4[0],
                         (unsigned int*)&tmp_ipv4[1],
                         (unsigned int*)&tmp_ipv4[2],
                         (unsigned int*)&tmp_ipv4[3]);
    CHECK_IF(scan_ret != 4, return -1, "sscanf failed");

    *(ipv4  ) = (unsigned char)tmp_ipv4[0];
    *(ipv4+1) = (unsigned char)tmp_ipv4[1];
    *(ipv4+2) = (unsigned char)tmp_ipv4[2];
    *(ipv4+3) = (unsigned char)tmp_ipv4[3];
    return 0;
}

int ipv4_to_str(unsigned char* ipv4, char* str_buf, int buf_size)
{
    CHECK_IF(ipv4 == NULL, return -1, "ipv4 is null");
    CHECK_IF(str_buf == NULL, return -1, "str_buf is null");
    CHECK_IF(buf_size < INET_ADDRSTRLEN, return -1, "buf_size = %d < INET_ADDRSTRLEN %d", buf_size, INET_ADDRSTRLEN);

    char tmp_buf[16];
    int  tmp_size = snprintf(tmp_buf, 16, "%u.%u.%u.%u",
                             ipv4[0], ipv4[1], ipv4[2], ipv4[3]);
    CHECK_IF(buf_size - 1 < tmp_size, return -1, "buf_size = %d < necessary size = %d", buf_size, tmp_size);

    strcpy(str_buf, tmp_buf);
    return 0;
}

int str_to_sockaddr(char* str, struct sockaddr_in* s4)
{
    CHECK_IF(str == NULL, return -1, "str is null");
    CHECK_IF(s4 == NULL, return -1, "s4 is null");

    s4->sin_family = AF_INET;

    int check = inet_pton(AF_INET, str, &s4->sin_addr);
    CHECK_IF(check != 1, return -1, "inet_pton failed");
    return 0;
}

int sockaddr_to_str(struct sockaddr_in* s4, char* str_buf, int buf_size)
{
    CHECK_IF(s4 == NULL, return -1, "s4 is null");
    CHECK_IF(str_buf == NULL, return -1, "str_buf is null");
    CHECK_IF(buf_size < INET_ADDRSTRLEN, return -1, "buf_size = %d < INET_ADDRSTRLEN %d", buf_size, INET_ADDRSTRLEN);

    inet_ntop(AF_INET, &s4->sin_addr, str_buf, INET_ADDRSTRLEN);
    return 0;
}

int ipv4_to_sockaddr(unsigned char* ipv4, struct sockaddr_in* s4)
{
    CHECK_IF(ipv4 == NULL, return -1, "ipv4 is null");
    CHECK_IF(s4 == NULL, return -1, "s4 is null");

    char buf[INET_ADDRSTRLEN] = {0};
    int ret = ipv4_to_str(ipv4, buf, INET_ADDRSTRLEN);
    CHECK_IF(ret < 0, return -1, "ipv4_to_str failed");
    return str_to_sockaddr(buf, s4);
}

int sockaddr_to_ipv4(struct sockaddr_in* s4, unsigned char* ipv4)
{
    CHECK_IF(ipv4 == NULL, return -1, "ipv4 is null");
    CHECK_IF(s4 == NULL, return -1, "s4 is null");

    char buf[INET_ADDRSTRLEN] = {0};
    int ret = sockaddr_to_str(s4, buf, INET_ADDRSTRLEN);
    CHECK_IF(ret < 0, return -1, "sockaddr_to_str failed");
    return str_to_ipv4(buf, ipv4);
}
