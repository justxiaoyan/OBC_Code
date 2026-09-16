#include <stddef.h>
#include <stdarg.h>

// 内存操作函数
void *memset(void *s, int c, size_t n) {
    unsigned char *p = s;
    while (n--) {
        *p++ = (unsigned char)c;
    }
    return s;
}

void *memcpy(void *dest, const void *src, size_t n) {
    unsigned char *d = dest;
    const unsigned char *s = src;
    while (n--) {
        *d++ = *s++;
    }
    return dest;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    const unsigned char *p1 = s1, *p2 = s2;
    while (n--) {
        if (*p1 != *p2) {
            return *p1 - *p2;
        }
        p1++;
        p2++;
    }
    return 0;
}

// 简化的数字转字符串函数
static void itoa(int value, char *str, int base) {
    char *p = str;
    char *p1, *p2;
    int is_negative = 0;
    
    if (value < 0 && base == 10) {
        is_negative = 1;
        value = -value;
    }
    
    p1 = p;
    do {
        int digit = value % base;
        *p++ = (digit < 10) ? '0' + digit : 'A' + digit - 10;
        value /= base;
    } while (value > 0);
    
    if (is_negative) {
        *p++ = '-';
    }
    
    *p = '\0';
    p2 = p - 1;
    while (p1 < p2) {
        char tmp = *p1;
        *p1 = *p2;
        *p2 = tmp;
        p1++;
        p2--;
    }
}

// 简化的 vsnprintf 实现
int vsnprintf(char *str, size_t size, const char *format, va_list ap) {
    char *p = str;
    size_t count = 0;
    char buffer[32];
    
    while (*format && count < size - 1) {
        if (*format == '%') {
            format++;
            switch (*format) {
                case 'd': {
                    int num = va_arg(ap, int);
                    itoa(num, buffer, 10);
                    char *s = buffer;
                    while (*s && count < size - 1) {
                        *p++ = *s++;
                        count++;
                    }
                    break;
                }
                case 'x': 
                case 'X': {
                    unsigned int num = va_arg(ap, unsigned int);
                    itoa(num, buffer, 16);
                    char *s = buffer;
                    while (*s && count < size - 1) {
                        *p++ = *s++;
                        count++;
                    }
                    break;
                }
                case 's': {
                    char *s = va_arg(ap, char *);
                    while (*s && count < size - 1) {
                        *p++ = *s++;
                        count++;
                    }
                    break;
                }
                case 'c': {
                    char ch = (char)va_arg(ap, int);
                    if (count < size - 1) {
                        *p++ = ch;
                        count++;
                    }
                    break;
                }
                case '%': {
                    if (count < size - 1) {
                        *p++ = '%';
                        count++;
                    }
                    break;
                }
                default:
                    if (count < size - 1) {
                        *p++ = '%';
                        count++;
                    }
                    if (count < size - 1) {
                        *p++ = *format;
                        count++;
                    }
                    break;
            }
            format++;
        } else {
            if (count < size - 1) {
                *p++ = *format;
                count++;
            }
            format++;
        }
    }
    
    if (size > 0) {
        *p = '\0';
    }
    
    return count;
}

// 其他可能需要的函数
size_t strlen(const char *s) {
    const char *p = s;
    while (*p) p++;
    return p - s;
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}
