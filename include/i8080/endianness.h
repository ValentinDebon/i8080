#ifndef I8080_ENDIANNESS_H
#define I8080_ENDIANNESS_H

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define I8080_TARGET_LITTLE_ENDIAN
#elif __BYTE_ORDER == __BIG_ENDIAN
#define I8080_TARGET_BIG_ENDIAN
#endif

#if !defined(I8080_TARGET_LITTLE_ENDIAN) && !defined(I8080_TARGET_BIG_ENDIAN)
#error "Unable to determine target endianness"
#endif

/* I8080_ENDIANNESS_H */
#endif
