/*
 * map.c - reading/writing the map file with read()/write() only.
 *
 * File format (identical for the original map and the shared state file):
 *     <state>
 *     <rows> <cols>
 *     <rows lines of <cols> single-digit integers separated by spaces>
 *
 * Every integer after the header is ONE digit and the state is ONE digit,
 * so a saved file always has exactly the same length. That lets every
 * process simply overwrite the file from offset 0 (no truncation needed).
 */
#include <stdlib.h>
#include <unistd.h>

#include "map.h"
#include "util.h"

#define IO_BUF 4096

Map *map_load(int fd)
{
    char    buf[IO_BUF];
    int     len = 0;
    int     pos = 0;
    int     v, r, c;
    int     state, rows, cols;
    ssize_t n;
    Map    *m;

    if (lseek(fd, 0, SEEK_SET) < 0)
    {
        return NULL;
    }

    /* Read the WHOLE file with as few read() calls as possible */
    while (len < IO_BUF)
    {
        n = read(fd, buf + len, (size_t)(IO_BUF - len));
        if (n <= 0)
        {
            break;
        }
        len += (int)n;
    }

    if (!parse_int(buf, len, &pos, &state) ||
        !parse_int(buf, len, &pos, &rows)  ||
        !parse_int(buf, len, &pos, &cols))
    {
        return NULL;
    }
    if (rows < 1 || cols < 1 || rows > 20 || cols > 20)
    {
        return NULL;
    }

    m = malloc(sizeof(Map));
    if (m == NULL)
    {
        return NULL;
    }
    m->state = state;
    m->rows  = rows;
    m->cols  = cols;
    m->grid  = calloc((size_t)rows, sizeof(char *));
    if (m->grid == NULL)
    {
        free(m);
        return NULL;
    }
    for (r = 0; r < rows; r++)
    {
        m->grid[r] = calloc((size_t)cols, sizeof(char));
        if (m->grid[r] == NULL)
        {
            map_free(m);   /* rows not yet allocated are NULL: free(NULL) is safe */
            return NULL;
        }
    }

    for (r = 0; r < rows; r++)
    {
        for (c = 0; c < cols; c++)
        {
            if (!parse_int(buf, len, &pos, &v))
            {
                map_free(m);
                return NULL;
            }
            m->grid[r][c] = (char)v;
        }
    }
    return m;
}

int map_save(int fd, const Map *m)
{
    OutBuf b;
    int    r, c;

    ob_init(&b);
    ob_int(&b, m->state);
    ob_char(&b, '\n');
    ob_int(&b, m->rows);
    ob_char(&b, ' ');
    ob_int(&b, m->cols);
    ob_char(&b, '\n');
    for (r = 0; r < m->rows; r++)
    {
        for (c = 0; c < m->cols; c++)
        {
            if (c > 0)
            {
                ob_char(&b, ' ');
            }
            ob_int(&b, m->grid[r][c]);
        }
        ob_char(&b, '\n');
    }

    if (lseek(fd, 0, SEEK_SET) < 0)
    {
        return -1;
    }
    /* one write() for the entire map */
    return write_all(fd, b.data, b.len);
}

void map_free(Map *m)
{
    int r;

    if (m == NULL)
    {
        return;
    }
    if (m->grid != NULL)
    {
        for (r = 0; r < m->rows; r++)
        {
            free(m->grid[r]);
        }
        free(m->grid);
    }
    free(m);
}

int map_find(const Map *m, char obj, int *row, int *col)
{
    int r, c;

    for (r = 0; r < m->rows; r++)
    {
        for (c = 0; c < m->cols; c++)
        {
            if (m->grid[r][c] == obj)
            {
                *row = r;
                *col = c;
                return 1;
            }
        }
    }
    return 0;
}
