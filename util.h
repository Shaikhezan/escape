/*
 * util.h - small helper routines that replace the banned C library
 *          functions (formatted IO, string library, number conversion).
 */
#ifndef UTIL_H
#define UTIL_H

#define OUT_CAP 8192

/* A fixed-size output buffer. Everything for one screen frame (or one
 * state file) is assembled here and then sent with ONE write() call. */
typedef struct
{
    char data[OUT_CAP];
    int  len;
} OutBuf;

int  my_strlen(const char *s);

/* Write exactly 'len' bytes (handles partial writes). 0 = ok, -1 = error */
int  write_all(int fd, const char *buf, int len);
int  write_str(int fd, const char *s);

/* Find the next unsigned integer in buf[*pos..len). Skips any non-digit
 * characters (spaces, '\n', '\r'). Returns 1 on success, 0 if none left. */
int  parse_int(const char *buf, int len, int *pos, int *value);

/* OutBuf helpers (silently stop when the buffer is full) */
void ob_init(OutBuf *b);
void ob_char(OutBuf *b, char c);
void ob_str(OutBuf *b, const char *s);
void ob_int(OutBuf *b, int v);

#endif
