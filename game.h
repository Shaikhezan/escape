/*
 * game.h - game logic and the per-process loops (player / snake / wolf)
 */
#ifndef GAME_H
#define GAME_H

/* Draw the current contents of the state file (used once at start-up) */
void game_show(const char *state_path);

/* Force the game to end (state = QUIT). Used if fork() fails. */
void game_stop(const char *state_path);

/* Parent process: read keys, move the Player. Returns when the game ends. */
void player_loop(const char *state_path);

/* Child process: move the Snake (who = SNAKE) or Wolf (who = WOLF) on its
 * own timer. Returns when the game ends. */
void enemy_loop(const char *state_path, char who);

#endif
