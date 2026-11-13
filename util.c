/*
 * util.c - helper routines built only on write() (no stdio formatting, no string library)
 */
#include <errno.h>
#include <unistd.h>

#include "util.h"

int my_strlen(const char *s)
{
    int n = 0;
    while (s[n] != '\0')
    {
        n++;
    }
    return n;
}

int write_all(int fd, const char *buf, int len)
{
    int done = 0;
    while (done < len)
    {
        ssize_t n = write(fd, buf + done, (size_t)(len - done));
        if (n < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            return -1;
        }
        done += (int)n;
    }
    return 0;
}

int write_str(int fd, const char *s)
{
    return write_all(fd, s, my_strlen(s));
}

int parse_int(const char *buf, int len, int *pos, int *value)
{
    int p = *pos;
    int v = 0;

    /* skip everything that is not a digit */
    while (p < len && (buf[p] < '0' || buf[p] > '9'))
    {
        p++;
    }
    if (p >= len)
    {
        *pos = p;
        return 0;
    }
    while (p < len && buf[p] >= '0' && buf[p] <= '9')
    {
        v = v * 10 + (buf[p] - '0');
        p++;
    }
    *pos = p;
    *value = v;
    return 1;
}

void ob_init(OutBuf *b)
{
    b->len = 0;
}

void ob_char(OutBuf *b, char c)
{
    if (b->len < OUT_CAP)
    {
        b->data[b->len] = c;
        b->len++;
    }
}

void ob_str(OutBuf *b, const char *s)
{
    int i = 0;
    while (s[i] != '\0')
    {
        ob_char(b, s[i]);
        i++;
    }
}

void ob_int(OutBuf *b, int v)
{
    char tmp[12];
    int  n = 0;

    if (v < 0)
    {
        ob_char(b, '-');
        v = -v;
    }
    if (v == 0)
    {
        ob_char(b, '0');
        return;
    }
    while (v > 0)
    {
        tmp[n] = (char)('0' + (v % 10));
        n++;
        v /= 10;
    }
    while (n > 0)
    {
        n--;
        ob_char(b, tmp[n]);
    }
}
