#include "strings.h"

// convert numeric ASCII characters to their integers (eg '0' -> 0)
const unsigned int OFFSET_BASE10 = 48;

// convert ASCII character to hex value ('b' -> 0xb)
const unsigned int OFFSET_BASE16_UPPER = 55;
const unsigned int OFFSET_BASE16_LOWER = 87;

void *memcpy(void *dst, const void *src, size_t n)
{
    /* Copy contents from src to dst one byte at a time */
    char *d = dst;
    const char *s = src;
    while (n--) {
        *d++ = *s++;
    }
    return dst;
}

void *memset(void *dst, int val, size_t n)
{
    char *d = dst;

    // copy values
    while (n--) {
        *d++ = val;
    }

    return dst;
}


size_t strlen(const char *str)
{
    /* Implementation a gift to you from lab3 */
    size_t n = 0;
    while (str[n] != '\0') {
        n++;
    }
    return n;
}

int strcmp(const char *s1, const char *s2) 
{   
    // while both aren't at end
    while (*s1 || *s2) {

        // if they are different, return the ASCII value difference
        if (*s1 != *s2) {
            return *s1 - *s2;
        }

        // compare next
        s1++;
        s2++;
    }

    return 0; // if we reached end of both
}

size_t strlcat(char *dst, const char *src, size_t dstsize)
{
    // find lengths of src/dst
    int srclen = strlen(src);
    int dstlen = strlen(dst);

    // if dst is not terminated, or dstsize is invalid
    if (dstlen >= dstsize) {
        return dstsize + srclen;
    }

    int final_index = dstlen; // index for NULL terminator

    // if src can fit with space left
    if (srclen + dstlen <= dstsize - 1) {
        memcpy(dst + dstlen, src, srclen); 
        final_index += srclen;

    // if it overflows the buffer size
    } else {
        memcpy(dst + dstlen, src, dstsize - dstlen - 1);
        final_index = dstsize - 1;
    }

    // append null terminator
    dst[final_index] = '\0';

    return dstlen + srclen;
}

unsigned int strtonum(const char *str, const char **endptr)
{
    // check if it's decimal or hex by looking for '0x'
    unsigned int base = 10;
    if (str[0] == '0' && str[1] == 'x') {
        base = 16;
        str += 2; // start from after '0x'
    }

    // traverse string and convert to numbers
    unsigned int num = 0;
    while (*str) {

        int offset = 0; // to convert ASCII code to numeric value

        // if it's a number
        if (*str >= '0' && *str <= '9') {
            offset = OFFSET_BASE10;
        
        // if it's a char
        } else if (base == 16 && // if it's in hex and
                ((*str >= 'A' && *str <= 'F') || // uppercase A-F
                (*str >= 'a'  && *str <= 'f'))) { // lowercase a-f
            
            offset = OFFSET_BASE16_UPPER; // uppercase A-F
            if (*str >= 'a' && *str <= 'f') {
                offset = OFFSET_BASE16_LOWER; // lowercase a-f
            }
        
        // all other characters are invalid
        } else {
            break;
        }
        
        // increment number
        num = base * num + (*str++ - offset);        
    }

    if (endptr != NULL) { // endptr is last character (\0 or other)
        *endptr = str;
    }

    return num;
}
