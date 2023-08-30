#include "test.h"
#include "binary_stream.h"

int main()
{
    FILE *stream;
    char *buffer, *ptr;
    size_t buffer_size;

    assert(stream = open_memstream(&buffer, &buffer_size));
    assert(put_u8(stream,   0x12) == sizeof(uint8_t));
    assert(put_u8(stream,   -0x12) == sizeof(uint8_t));
    assert(put_le16(stream, 0x1234) == sizeof(uint16_t));
    assert(put_le16(stream, -0x1234) == sizeof(uint16_t));
    assert(put_le32(stream, 0x12345678) == sizeof(uint32_t));
    assert(put_le32(stream, -0x12345678) == sizeof(uint32_t));
    assert(put_le64(stream, 0x1234567890123456) == sizeof(uint64_t));
    assert(put_le64(stream, -0x1234567890123456) == sizeof(uint64_t));
    assert(put_be24(stream, 0x123456) == 3);
    assert(put_le32_ieee754(stream, 2.0f) == 4);
    assert(fclose(stream) != EOF);

    ptr = buffer;
    assert(!memcmp(ptr, (unsigned char[]) {0x12}, sizeof(uint8_t)));
    ptr += sizeof(uint8_t);
    assert(!memcmp(ptr, (unsigned char[]) {0xee}, sizeof(uint8_t)));
    ptr += sizeof(uint8_t);
    assert(!memcmp(ptr, (unsigned char[]) {0x34, 0x12}, sizeof(uint16_t)));
    ptr += sizeof(uint16_t);
    assert(!memcmp(ptr, (unsigned char[]) {0xcc, 0xed}, sizeof(uint16_t)));
    ptr += sizeof(uint16_t);
    assert(!memcmp(ptr, (unsigned char[]) {0x78, 0x56, 0x34, 0x12}, sizeof(uint32_t)));
    ptr += sizeof(uint32_t);
    assert(!memcmp(ptr, (unsigned char[]) {0x88, 0xa9, 0xcb, 0xed}, sizeof(uint32_t)));
    ptr += sizeof(uint32_t);
    assert(!memcmp(ptr, (unsigned char[]) {0x56, 0x34, 0x12, 0x90, 0x78, 0x56, 0x34, 0x12}, sizeof(uint64_t)));
    ptr += sizeof(uint64_t);
    assert(!memcmp(ptr, (unsigned char[]) {0xaa, 0xcb, 0xed, 0x6f, 0x87, 0xa9, 0xcb, 0xed}, sizeof(uint64_t)));
    ptr += sizeof(uint64_t);
    assert(!memcmp(ptr, (unsigned char[]) {0x12, 0x34, 0x56}, 3));
    ptr += 3;
    assert(!memcmp(ptr, (unsigned char[]) {0x0, 0x0, 0x0, 0x40}, 4));
    ptr += 4;

    uint8_t u8;
    uint16_t u16;
    uint32_t u32;
    uint64_t u64;
    int16_t i16;
    int32_t i32;
    int64_t i64;

    assert(stream = fmemopen(buffer, buffer_size, "rb"));
    assert(get_u8(stream, &u8) == sizeof(uint8_t) && u8 == 0x12);
    assert(get_i8_or_zero(stream) == -0x12);
    assert(get_leu16(stream, &u16) == sizeof(uint16_t) && u16 == 0x1234);
    assert(get_lei16(stream, &i16) == sizeof(int16_t)  && i16 == -0x1234);
    assert(get_leu32(stream, &u32) == sizeof(uint32_t) && u32 == 0x12345678);
    assert(get_lei32(stream, &i32) == sizeof(int32_t)  && i32 == -0x12345678);
    assert(get_leu64(stream, &u64) == sizeof(uint64_t) && u64 == 0x1234567890123456);
    assert(get_lei64(stream, &i64) == sizeof(int64_t)  && i64 == -0x1234567890123456);
    assert(get_beu24_or_zero(stream) == 0x123456);
    assert(get_le32_ieee754_or_zero(stream) == 2.0f);

    return EXIT_SUCCESS;
}