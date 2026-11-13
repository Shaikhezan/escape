/*
 * game.c - Player, Snake and Wolf logic.
 *
 * Every process (parent = Player, child 1 = Snake, child 2 = Wolf) opens
 * the state file ITSELF, so each has its own open file description and
 * flock() really excludes the other processes.
 *
 * One "move" is always the same critical section:
 *     flock(LOCK_EX) -> read whole file -> change array -> write whole file
 *     -> redraw screen -> flock(LOCK_UN)
 */
#include <fcntl.h>
#include <stdlib.h>
#include <sys/file.h>
#include <time.h>
#include <unistd.h>

#include "game.h"
#include "map.h"
#include "terminal.h"

#define SNAKE_DELAY_MS 2000
#define WOLF_DELAY_MS  1000
#define SLICE_MS        100   /* enemies sleep in slices so they notice game over */

/* 8 directions (Snake) and 4 directions (Wolf: first four entries) */
static const int DIR_ROW[8] = { -1,  1,  0, 0, -1, -1,  1, 1 };
static const int DIR_COL[8] = {  0,  0, -1, 1, -1,  1, -1, 1 };

/* ------------------------------------------------------------------ */
/* small helpers                                                       */
/* ------------------------------------------------------------------ */

static void msleep(int ms)
{
    struct timespec ts;
    ts.tv_sec  = ms / 1000;
    ts.tv_nsec = (long)(ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
}

/* Lock the file and load the latest map. On failure the lock is released. */
static Map *lock_and_load(int fd)
{
    Map *m;

    flock(fd, LOCK_EX);
    m = map_load(fd);
    if (m == NULL)
    {
        flock(fd, LOCK_UN);
    }
    return m;
}

/* Optionally save + redraw, then unlock */
static void commit_and_unlock(int fd, const Map *m, int changed)
{
    if (changed)
    {
        map_save(fd, m);
        display_map(m);
    }
    flock(fd, LOCK_UN);
}

/* Cheap check of the first byte of the file under a shared lock.
 * Returns 1 when the game is over (or the file is unreadable). */
static int game_over(int fd)
{
    char    c = '0';
    ssize_t n;

    flock(fd, LOCK_SH);
    n = -1;
    if (lseek(fd, 0, SEEK_SET) >= 0)
    {
        n = read(fd, &c, 1);
    }
    flock(fd, LOCK_UN);

    if (n != 1)
    {
        return 1;
    }
    return c != '0';
}

static int in_bounds(const Map *m, int r, int c)
{
    return r >= 0 && r < m->rows && c >= 0 && c < m->cols;
}

/* ------------------------------------------------------------------ */
/* Player                                                              */
/* ------------------------------------------------------------------ */

/* Returns 1 if the map changed, 0 if the move was invalid. */
static int player_step(Map *m, int dr, int dc)
{
    int r, c, nr, nc;
    int cell;

    if (!map_find(m, PLAYER, &r, &c))
    {
        return 0;
    }
    nr = r + dr;
    nc = c + dc;
    if (!in_bounds(m, nr, nc))
    {
        return 0;                       /* map border */
    }
    cell = m->grid[nr][nc];
    if (cell == WALL || cell == SNAKE || cell == WOLF)
    {
        return 0;                       /* cannot walk into walls / enemies */
    }
    if (cell == GOAL)
    {
        m->state = STATE_WIN;
    }
    m->grid[r][c]   = EMPTY;
    m->grid[nr][nc] = PLAYER;
    return 1;
}

void player_loop(const char *state_path)
{
    int  fd = open(state_path, O_RDWR);
    char key;
    int  dr, dc;
    int  done = 0;

    if (fd < 0)
    {
        return;
    }

    while (!done)
    {
        ssize_t n = read(STDIN_FILENO, &key, 1);   /* returns after <= 0.1 s */
        Map    *m;
        int     changed = 0;

        if (n <= 0)
        {
            /* no key pressed: has an enemy ended the game meanwhile? */
            if (game_over(fd))
            {
                done = 1;
            }
            continue;
        }

        dr = 0;
        dc = 0;
        switch (key)
        {
            case 'w': case 'W': dr = -1; break;
            case 's': case 'S': dr =  1; break;
            case 'a': case 'A': dc = -1; break;
            case 'd': case 'D': dc =  1; break;
            case 'q': case 'Q': break;
            default:  continue;         /* ignore any other key */
        }

        m = lock_and_load(fd);
        if (m == NULL)
        {
            break;
        }
        if (m->state != STATE_RUNNING)  /* someone else already ended it */
        {
            done = 1;
        }
        else if (key == 'q' || key == 'Q')
        {
            m->state = STATE_QUIT;
            changed  = 1;
            done     = 1;
        }
        else
        {
            changed = player_step(m, dr, dc);
            if (m->state != STATE_RUNNING)
            {
                done = 1;               /* reached the Goal */
            }
        }
        commit_and_unlock(fd, m, changed);
        map_free(m);
    }
    close(fd);
}

/* ------------------------------------------------------------------ */
/* Enemies                                                             */
/* ------------------------------------------------------------------ */

/* Move the Snake / Wolf one step. Returns 1 if the map changed. */
static int enemy_step(Map *m, char who)
{
    int ndir = (who == SNAKE) ? 8 : 4;
    int r, c, pr, pc;
    int valid[8];
    int count = 0;
    int d;

    if (!map_find(m, who, &r, &c) || !map_find(m, PLAYER, &pr, &pc))
    {
        return 0;
    }

    /* 1. Player one move away -> attack directly (no randomness) */
    for (d = 0; d < ndir; d++)
    {
        if (r + DIR_ROW[d] == pr && c + DIR_COL[d] == pc)
        {
            m->grid[r][c]   = EMPTY;
            m->grid[pr][pc] = who;
            m->state        = STATE_LOSE;
            return 1;
        }
    }

    /* 2. Otherwise choose RANDOMLY among the legal directions.
     *    Legal = inside the map and an empty cell (no Wall, Goal, Player
     *    or other Enemy). */
    for (d = 0; d < ndir; d++)
    {
        int nr = r + DIR_ROW[d];
        int nc = c + DIR_COL[d];
        if (in_bounds(m, nr, nc) && m->grid[nr][nc] == EMPTY)
        {
            valid[count] = d;
            count++;
        }
    }
    if (count == 0)
    {
        return 0;                       /* boxed in: stay still */
    }
    d = valid[rand() % count];
    m->grid[r][c]                     = EMPTY;
    m->grid[r + DIR_ROW[d]][c + DIR_COL[d]] = who;
    return 1;
}

void enemy_loop(const char *state_path, char who)
{
    int fd    = open(state_path, O_RDWR);
    int delay = (who == SNAKE) ? SNAKE_DELAY_MS : WOLF_DELAY_MS;
    int slept;
    int done  = 0;

    if (fd < 0)
    {
        return;
    }

    /* different seed for every process */
    srand((unsigned int)getpid() * 2654435761u + (unsigned int)who);

    while (!done)
    {
        /* nanosleep in small slices; stop early if the game ended */
        for (slept = 0; slept < delay && !done; slept += SLICE_MS)
        {
            msleep(SLICE_MS);
            if (game_over(fd))
            {
                done = 1;
            }
        }
        if (!done)
        {
            Map *m = lock_and_load(fd);
            int  changed = 0;

            if (m == NULL)
            {
                break;
            }
            if (m->state != STATE_RUNNING)
            {
                done = 1;
            }
            else
            {
                changed = enemy_step(m, who);
                if (m->state != STATE_RUNNING)
                {
                    done = 1;           /* caught the Player */
                }
            }
            commit_and_unlock(fd, m, changed);
            map_free(m);
        }
    }
    close(fd);
}

/* ------------------------------------------------------------------ */
/* start-up / shut-down helpers used by main                           */
/* ------------------------------------------------------------------ */

void game_show(const char *state_path)
{
    int  fd = open(state_path, O_RDWR);
    Map *m;

    if (fd < 0)
    {
        return;
    }
    m = lock_and_load(fd);
    if (m != NULL)
    {
        display_map(m);
        flock(fd, LOCK_UN);
        map_free(m);
    }
    close(fd);
}

void game_stop(const char *state_path)
{
    int  fd = open(state_path, O_RDWR);
    Map *m;

    if (fd < 0)
    {
        return;
    }
    m = lock_and_load(fd);
    if (m != NULL)
    {
        int changed = 0;
        if (m->state == STATE_RUNNING)
        {
            m->state = STATE_QUIT;
            changed  = 1;
        }
        commit_and_unlock(fd, m, changed);
        map_free(m);
    }
    close(fd);
}
