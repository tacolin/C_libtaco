#include "basic.h"
#include "pack.h"

static void test_ipv4_mac_to_str(void)
{
    unsigned char ipv4[] = {1, 2, 3, 4};
    unsigned char mac[]  = {0xa, 0xb, 0xc, 0xd, 0xe, 0xf};

    char ipstr[INET_ADDRSTRLEN];
    char macstr[30];

    ipv4_to_str(ipv4, ipstr, INET_ADDRSTRLEN);
    mac_to_str(mac, macstr, 30);

    dprint("ipstr = %s", ipstr);
    dprint("macstr = %s", macstr);        
}

static void test_str_to_ipv4_mac(void)
{
    char ipstr[] = "10.20.30.40";
    char macstr[] = "11:22:33:44:55:66";

    unsigned char ipv4[4];
    unsigned char macaddr[6];

    str_to_ipv4(ipstr, ipv4);
    str_to_mac(macstr, macaddr);

    int i;

    for (i=0; i<4; i++) dprint("ipv4[%d] = %d", i, ipv4[i]);

    for (i=0; i<6; i++) dprint("macaddr[%d] = 0x%02x", i, macaddr[i]);

    return;
}

static void test_bitmap_to_str(void)
{
    unsigned int bitmap = 0;
    int ofs = 0;

    bits_put(&bitmap, &ofs, 1, 1);
    bits_put(&bitmap, &ofs, 3, 0);
    bits_put(&bitmap, &ofs, 4, 7);

    dprint("bitmap = 0x%08x", bitmap);

    char str[40] = {0};
    bitmap_to_str(bitmap, 8, str, 40);
    dprint("bitmap = %s", str);

    return;
}

static void test_bits_put_get(void)
{
    unsigned char bytes[4] = {0};
    int ofs = 0;

    bits_put(bytes, &ofs, 1, 1);
    bits_put(bytes, &ofs, 5, 0);
    bits_put(bytes, &ofs, 10, 500);

    ofs = 0;
    int val;

    val = bits_get(bytes, &ofs, 1);
    dprint("val = %d", val);
    val = bits_get(bytes, &ofs, 5);
    dprint("val = %d", val);
    val = bits_get(bytes, &ofs, 10);
    dprint("val = %d", val);

    return;
}

static void test_sockaddr(void)
{
    char ipstr[] = "192.168.1.1";
    struct sockaddr_in s4 = {0};
    unsigned char ipv4[4] = {0};

    str_to_sockaddr(ipstr, &s4);
    sockaddr_to_ipv4(&s4, ipv4);

    int i;
    for (i=0; i<4; i++) dprint("ipv4[%d] = %d", i, ipv4[i]);

    ipv4[3] = 10;
    char tmp_str[INET_ADDRSTRLEN];

    ipv4_to_sockaddr(ipv4, &s4);
    sockaddr_to_str(&s4, tmp_str, INET_ADDRSTRLEN);

    dprint("tmp_str = %s", tmp_str);

    return;

}

static void test_bytes_to_uint(void)
{
    unsigned char bytes[16] = {0};
    unsigned char u8;
    unsigned short u16;
    unsigned int u32;
    unsigned long long u64;
    unsigned char* ptr = bytes;

    u8 = 100;
    u16 = 200;
    u32 = 300;
    u64 = 400;

    *ptr = u8;
    ptr += 1;

    u16_to_bytes(u16, ptr);
    ptr += 2;

    u32_to_bytes(u32, ptr);
    ptr += 4;

    u64_to_bytes(u64, ptr);
    ptr += 8;

    *ptr = 0;
    ptr += 1;

    u8 = u16 = u32 = u64 = 0;
    ptr = bytes;

    u8 = *ptr;
    ptr += 1;    
    dprint("u8 = %d", u8);

    bytes_to_u16(ptr, u16);
    ptr += 2;
    dprint("u16 = %d", u16);

    bytes_to_u32(ptr, u32);
    ptr += 4;
    dprint("u32 = %d", u32);

    bytes_to_u64(ptr, u64);
    ptr += 8;
    dprint("u64 = %lld", u64);

    u8 = *ptr;
    ptr += 1;
    dprint("u8 = %d", u8);

    return;
}

int main(int argc, char const *argv[])
{
    test_ipv4_mac_to_str();
    test_str_to_ipv4_mac();
    test_bitmap_to_str();
    test_bits_put_get();
    test_sockaddr();
    test_bytes_to_uint();

    return 0;
}
