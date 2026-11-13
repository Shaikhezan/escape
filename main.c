/*
 * main.c - "escape": ASCII terminal game using fork() and flock()
 *
 * Usage: ./escape <mapfile>
 *
 *   parent  : prompts the user (Terminal IO) and moves the Player
 *   child 1 : moves the Snake  (every 2 s)
 *   child 2 : moves the Wolf   (every 1 s)
 *
 * The three processes talk only through a shared "state file" which is a
 * copy of the map file.
 */
#include <fcntl.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#include "game.h"
#include "map.h"
#include "terminal.h"
#include "util.h"

#define STATE_FILE "state.txt"

int main(int argc, char *argv[])
{
    struct termios saved;
    int            fd;
    Map           *m;
    pid_t          snake_pid, wolf_pid;
    int            forked_ok = 1;

    /* ---- command line check ---- */
    if (argc != 2)
    {
        write_str(STDERR_FILENO, "Error: wrong number of arguments.\n");
        write_str(STDERR_FILENO, "Usage: ./escape <filename>\n");
        return 1;
    }

    /* ---- open + read the map file ---- */
    fd = open(argv[1], O_RDONLY);
    if (fd < 0)
    {
        write_str(STDERR_FILENO, "Error: cannot open map file '");
        write_str(STDERR_FILENO, argv[1]);
        write_str(STDERR_FILENO, "'.\n");
        return 1;
    }
    m = map_load(fd);
    close(fd);
    if (m == NULL)
    {
        write_str(STDERR_FILENO, "Error: map file has an invalid format.\n");
        return 1;
    }

    /* ---- create the shared state file (a copy of the map) ---- */
    m->state = STATE_RUNNING;
    fd = open(STATE_FILE, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd < 0)
    {
        write_str(STDERR_FILENO, "Error: cannot create the state file.\n");
        map_free(m);
        return 1;
    }
    map_save(fd, m);
    close(fd);
    map_free(m);

    /* ---- terminal setup ---- */
    if (term_raw_on(&saved) < 0)
    {
        write_str(STDERR_FILENO, "Error: stdin must be a terminal.\n");
        unlink(STATE_FILE);
        return 1;
    }
    term_clear();
    game_show(STATE_FILE);

    /* ---- create the Snake and Wolf processes ---- */
    snake_pid = fork();
    if (snake_pid == 0)
    {
        enemy_loop(STATE_FILE, SNAKE);
        exit(0);
    }
    if (snake_pid < 0)
    {
        forked_ok = 0;
    }

    wolf_pid = -1;
    if (forked_ok)
    {
        wolf_pid = fork();
        if (wolf_pid == 0)
        {
            enemy_loop(STATE_FILE, WOLF);
            exit(0);
        }
        if (wolf_pid < 0)
        {
            forked_ok = 0;
        }
    }

    if (forked_ok)
    {
        player_loop(STATE_FILE);        /* returns when the game is over */
    }
    else
    {
        game_stop(STATE_FILE);          /* tell any started child to stop */
    }

    /* ---- wait for the children, then clean up ---- */
    while (wait(NULL) > 0)
    {
        /* keep waiting until there are no children left */
    }
    term_restore(&saved);
    unlink(STATE_FILE);

    if (!forked_ok)
    {
        write_str(STDERR_FILENO, "Error: fork() failed.\n");
        return 1;
    }
    return 0;
}
