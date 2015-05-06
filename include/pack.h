#ifndef _PACK_H_
#define _PACK_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define bytes_to_u16(bytes, u16) \
    do { \
        u16  = (*((unsigned char*)bytes  ) <<  8); \
        u16 |= (*((unsigned char*)bytes+1)      ); \
    } while(0)

#define u16_to_bytes(u16, bytes) \
    do { \
        *((unsigned char*)bytes  ) = ((u16 >>  8) & 0xFF); \
        *((unsigned char*)bytes+1) = ((u16      ) & 0xFF); \
    } while (0)

#define bytes_to_u32(bytes, u32) \
    do { \
        u32  = (*((unsigned char*)bytes  ) << 24); \
        u32 |= (*((unsigned char*)bytes+1) << 16); \
        u32 |= (*((unsigned char*)bytes+2) <<  8); \
        u32 |= (*((unsigned char*)bytes+3)      ); \
    } while (0)

#define u32_to_bytes(u32, bytes) \
    do { \
        *((unsigned char*)bytes  ) = ((u32 >> 24) & 0xFF); \
        *((unsigned char*)bytes+1) = ((u32 >> 16) & 0xFF); \
        *((unsigned char*)bytes+2) = ((u32 >>  8) & 0xFF); \
        *((unsigned char*)bytes+3) = ((u32      ) & 0xFF); \
    } while (0)

#define bytes_to_u64(bytes, u64) \
    do { \
        u64  = ((unsigned long long)(*((unsigned char*)bytes  )) << 56); \
        u64 |= ((unsigned long long)(*((unsigned char*)bytes+1)) << 48); \
        u64 |= ((unsigned long long)(*((unsigned char*)bytes+2)) << 40); \
        u64 |= ((unsigned long long)(*((unsigned char*)bytes+3)) << 32); \
        u64 |= ((unsigned long long)(*((unsigned char*)bytes+4)) << 24); \
        u64 |= ((unsigned long long)(*((unsigned char*)bytes+5)) << 16); \
        u64 |= ((unsigned long long)(*((unsigned char*)bytes+6)) <<  8); \
        u64 |= ((unsigned long long)(*((unsigned char*)bytes+7))      ); \
    } while (0)

#define u64_to_bytes(u64, bytes) \
    do { \
        *((unsigned char*)bytes  ) = ((u64 >> 56) & 0xFF); \
        *((unsigned char*)bytes+1) = ((u64 >> 48) & 0xFF); \
        *((unsigned char*)bytes+2) = ((u64 >> 40) & 0xFF); \
        *((unsigned char*)bytes+3) = ((u64 >> 32) & 0xFF); \
        *((unsigned char*)bytes+4) = ((u64 >> 24) & 0xFF); \
        *((unsigned char*)bytes+5) = ((u64 >> 16) & 0xFF); \
        *((unsigned char*)bytes+6) = ((u64 >>  8) & 0xFF); \
        *((unsigned char*)bytes+7) = ((u64      ) & 0xFF); \
    } while (0)

unsigned int bits_get(void* bytes, int* ofs, int bit_num);
void bits_put(void* bytes, int* ofs, int bit_num, unsigned int value);

int bitmap_to_str(unsigned int bitmap, int bit_num, char* str_buf, int buf_size);

int str_to_mac(char* str, unsigned char* mac_addr);
int mac_to_str(unsigned char* mac_addr, char* str_buf, int buf_size);

int str_to_ipv4(char* str, unsigned char* ipv4);
int ipv4_to_str(unsigned char* ipv4, char* str_buf, int buf_size);

int str_to_sockaddr(char* str, struct sockaddr_in* s4);
int sockaddr_to_str(struct sockaddr_in* s4, char* str_buf, int buf_size);

int ipv4_to_sockaddr(unsigned char* ipv4, struct sockaddr_in* s4);
int sockaddr_to_ipv4(struct sockaddr_in* s4, unsigned char* ipv4);

#endif //_PACK_H_
