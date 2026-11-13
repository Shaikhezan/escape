/*
 * terminal.h - Terminal IO (raw key input) and drawing the game screen
 */
#ifndef TERMINAL_H
#define TERMINAL_H

#include <termios.h>

#include "map.h"

/* Switch stdin to "single key, no echo" mode. 'saved' receives the old
 * settings. Returns 0 on success, -1 if stdin is not a terminal. */
int  term_raw_on(struct termios *saved);

/* Put the terminal back the way it was (and show the cursor again) */
void term_restore(const struct termios *saved);

/* Clear the whole screen and move the cursor to the top-left corner */
void term_clear(void);

/* Draw the whole interface (border, objects, instructions, status line)
 * with a single write() call. */
void display_map(const Map *m);

#endif
