/*
 * terminal.c - Terminal IO with termios + ANSI escape codes
 */
#include <unistd.h>

#include "terminal.h"
#include "util.h"

#define ANSI_CLEAR      "\033[2J\033[H"
#define ANSI_HOME       "\033[H"
#define ANSI_ERASE_LINE "\033[K"
#define ANSI_ERASE_DOWN "\033[J"
#define ANSI_WHITE_BG   "\033[47m"
#define ANSI_RESET      "\033[0m"
#define ANSI_HIDE_CUR   "\033[?25l"
#define ANSI_SHOW_CUR   "\033[?25h"

int term_raw_on(struct termios *saved)
{
    struct termios raw;

    if (tcgetattr(STDIN_FILENO, saved) < 0)
    {
        return -1;
    }
    raw = *saved;
    raw.c_lflag &= ~(tcflag_t)(ICANON | ECHO);  /* no line buffering, no echo */
    raw.c_cc[VMIN]  = 0;   /* read() may return with no key ...           */
    raw.c_cc[VTIME] = 1;   /* ... after 0.1 s, so the game can poll state */
    if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) < 0)
    {
        return -1;
    }
    write_str(STDOUT_FILENO, ANSI_HIDE_CUR);
    return 0;
}

void term_restore(const struct termios *saved)
{
    write_str(STDOUT_FILENO, ANSI_SHOW_CUR);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, saved);
}

void term_clear(void)
{
    write_str(STDOUT_FILENO, ANSI_CLEAR);
}

static void draw_border(OutBuf *b, int cols)
{
    int c;
    for (c = 0; c < cols + 2; c++)
    {
        ob_char(b, '*');
    }
    ob_str(b, ANSI_ERASE_LINE "\n");
}

void display_map(const Map *m)
{
    OutBuf b;
    int    r, c;
    int    in_wall;

    ob_init(&b);
    ob_str(&b, ANSI_HOME);          /* redraw in place: no flicker */

    draw_border(&b, m->cols);
    for (r = 0; r < m->rows; r++)
    {
        ob_char(&b, '*');
        in_wall = 0;
        for (c = 0; c < m->cols; c++)
        {
            char cell = m->grid[r][c];

            if (cell == WALL && !in_wall)
            {
                ob_str(&b, ANSI_WHITE_BG);
                in_wall = 1;
            }
            else if (cell != WALL && in_wall)
            {
                ob_str(&b, ANSI_RESET);
                in_wall = 0;
            }

            switch (cell)
            {
                case PLAYER: ob_char(&b, 'P'); break;
                case GOAL:   ob_char(&b, 'G'); break;
                case SNAKE:  ob_char(&b, '~'); break;
                case WOLF:   ob_char(&b, 'W'); break;
                default:     ob_char(&b, ' '); break;   /* empty and wall */
            }
        }
        if (in_wall)
        {
            ob_str(&b, ANSI_RESET);
        }
        ob_char(&b, '*');
        ob_str(&b, ANSI_ERASE_LINE "\n");
    }
    draw_border(&b, m->cols);

    ob_str(&b, "Press w to move UP"    ANSI_ERASE_LINE "\n");
    ob_str(&b, "Press s to move DOWN"  ANSI_ERASE_LINE "\n");
    ob_str(&b, "Press a to move LEFT"  ANSI_ERASE_LINE "\n");
    ob_str(&b, "Press d to move RIGHT" ANSI_ERASE_LINE "\n");

    switch (m->state)
    {
        case STATE_WIN:
            ob_str(&b, "You reached the Goal. YOU WIN!");
            break;
        case STATE_LOSE:
            ob_str(&b, "An enemy caught you. YOU LOSE!");
            break;
        case STATE_QUIT:
            ob_str(&b, "Game quit.");
            break;
        default:
            break;
    }
    ob_str(&b, ANSI_ERASE_LINE "\n" ANSI_ERASE_DOWN);

    write_all(STDOUT_FILENO, b.data, b.len);   /* ONE write per frame */
}
