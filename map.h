/*
 * map.h - the game map / state file: loading, saving, searching
 */
#ifndef MAP_H
#define MAP_H

/* Cell values (same numbers as in the map text file) */
#define EMPTY  0
#define WALL   1
#define PLAYER 2
#define GOAL   3
#define SNAKE  4
#define WOLF   5

/* Game state values (first integer of the file) */
#define STATE_RUNNING 0
#define STATE_WIN     1
#define STATE_LOSE    2
#define STATE_QUIT    3

typedef struct
{
    int    state;   /* 0 = running, non-zero = game over */
    int    rows;
    int    cols;
    char **grid;    /* grid[row][col], each cell holds 0..5 */
} Map;

/* Read the whole file behind 'fd' and build a Map. NULL on failure. */
Map *map_load(int fd);

/* Overwrite the file behind 'fd' with the Map. 0 = ok, -1 = error. */
int  map_save(int fd, const Map *m);

void map_free(Map *m);

/* Find the first cell holding 'obj'. 1 = found, 0 = not found. */
int  map_find(const Map *m, char obj, int *row, int *col);

#endif
