/*
 * Module for chess GUI. Displays a chess board on the screen. Includes
 * function to update UI based on new move, sidebar, handling of chess rules
 * like passant, promotion and castling.
 *
 * Author: Ellen Xu <ellenjxu@stanford.edu>
 * Author: Javier Garcia Nieto <jgnieto@stanford.edu>
 */
#include "chess_commands.h"
#include "chess_gui.h"
#include "gl.h"
#include "gpio.h"
#include "gpio_extra.h"
#include "gpio_interrupt.h"
#include "printf.h"
#include "strings.h"
#include "timer.h"
#include "uart.h"
#include <stdint.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

#define SQUARE_SIZE (SCREEN_HEIGHT > SCREEN_WIDTH ? SCREEN_WIDTH/9 : SCREEN_HEIGHT/9)
#define PADDING 2

#define MANGO           0
#define CHESS_COM_GREEN 1
#define CHESS_COM_BLUE  2

#define THEME MANGO

#define SHOW_STATS   false
#define SHOW_LETTERS true
#define SHOW_NUMBERS true

#define PIECE_BLACK         GL_BLACK
#define PIECE_WHITE         GL_WHITE
#define PIECE_HIGHLIGHT     GL_RED
#define PIECE_BLACK_LIGHT   gl_color(96,96,96)
#define PIECE_WHITE_LIGHT   gl_color(211,211,211)

#define THIN_CURSOR     3
#define THICK_CURSOR    5

#define V_PADDING   5
#define H_PADDING   15
#define HISTORY_LINES   12

#if THEME == MANGO
#define CHESS_BLACK gl_color(188,  81, 150)
#define CHESS_WHITE gl_color(243, 216, 95)
#define SIDEBAR_FT  gl_color(243, 216, 95)
#define SIDEBAR_BG  gl_color(  0,   0,   0)
#define CURSOR_COLOR GL_RED
#elif THEME == CHESS_COM_BLUE
#define CHESS_BLACK gl_color( 84, 114, 150)
#define CHESS_WHITE gl_color(234, 233, 212)
#define SIDEBAR_FT  gl_color(255, 255, 255)
#define SIDEBAR_BG  gl_color(  0,   0,   0)
#define CURSOR_COLOR gl_color(222, 187, 11) // #debb0b lol
#else
#define CHESS_BLACK gl_color(124, 149, 93)
#define CHESS_WHITE gl_color(238, 238, 213)
#define SIDEBAR_FT  gl_color(255, 255, 255)
#define SIDEBAR_BG  gl_color(  0,   0,   0)
#define CURSOR_COLOR gl_color(222, 187, 11) // #debb0b lol
#endif


#define SIZE(x) (sizeof(x) / sizeof(*x))

static const char CHESS_GUI_PIECE_NAMES[] = { ' ', 'P', 'N', 'B', 'R', 'Q', 'K', 'p', 'n', 'b', 'r', 'q', 'k' };

// initialize
static const chess_gui_piece_t STARTING_BOARD[CHESS_SIZE][CHESS_SIZE] = {
    {BR, BN, BB, BQ, BK, BB, BN, BR},
    {BP, BP, BP, BP, BP, BP, BP, BP},
    {XX, XX, XX, XX, XX, XX, XX, XX},
    {XX, XX, XX, XX, XX, XX, XX, XX},
    {XX, XX, XX, XX, XX, XX, XX, XX},
    {XX, XX, XX, XX, XX, XX, XX, XX},
    {WP, WP, WP, WP, WP, WP, WP, WP},
    {WR, WN, WB, WQ, WK, WB, WN, WR},
};

static chess_gui_piece_t board[CHESS_SIZE][CHESS_SIZE];

static uint8_t stale[CHESS_SIZE][CHESS_SIZE];

static struct {
    int chosen_row;
    int chosen_col;
    bool has_chosen;

    int row;
    int col;
} cursor;

static struct {
    int from_col;
    int from_row;
    int to_col;
    int to_row;

    bool display;
} engine_move;

static struct {
    chess_gui_piece_t taken[4 * CHESS_SIZE];
    int taken_count;

    char W[6];
    char D[6];
    char L[6];
} sidebar;

static bool is_white(chess_gui_piece_t piece) {
    switch (piece) {
        case WP:
        case WN:
        case WB:
        case WR:
        case WQ:
        case WK:
            return true;
        default:
            return false;
    }
}

static char move_history[512][6];
static int nmoves;

/*
* Draws a border around a rectangle.
*/
static void draw_border(int x, int y, int width, int height, int thickness, color_t color) {
    gl_draw_rect(x, y, width, thickness, color);
    gl_draw_rect(x, y, thickness, height, color);
    gl_draw_rect(x + width - thickness, y, thickness, height, color);
    gl_draw_rect(x, y + height - thickness, width, thickness, color);
}

static void stale_everything(void) {
    memset(stale, 1, sizeof(stale));
}

static void gui_draw(bool after_swap) {
    // draw the chess board
    for (int row = 0; row < 8; row++) {
        int black_square = row % 2;
        for (int col = 0; col < 8; col++) {
            if (stale[row][col]) {
                gl_draw_rect(
                        SQUARE_SIZE*col,
                        SQUARE_SIZE*row,
                        SQUARE_SIZE,
                        SQUARE_SIZE,
                        black_square ? CHESS_BLACK : CHESS_WHITE
                        );

                if (row == cursor.row && col == cursor.col && cursor.has_chosen) {
                    // selected: draw yellow
                    gl_draw_char(
                            SQUARE_SIZE*col + SQUARE_SIZE / 2 - gl_get_char_width() / 2,
                            SQUARE_SIZE*row + SQUARE_SIZE / 2 - gl_get_char_height() / 2,
                            CHESS_GUI_PIECE_NAMES[board[cursor.chosen_row][cursor.chosen_col]],
                            CURSOR_COLOR
                            );
                } else if (row == cursor.chosen_row && col == cursor.chosen_col && cursor.has_chosen) {
                    // selected piece's original location draw nothing
                } else if (engine_move.display && row == engine_move.to_row && col == engine_move.to_col) {
                    // engine's latest move: highlight in red
                    gl_draw_char(
                            SQUARE_SIZE*col + SQUARE_SIZE / 2 - gl_get_char_width() / 2,
                            SQUARE_SIZE*row + SQUARE_SIZE / 2 - gl_get_char_height() / 2,
                            CHESS_GUI_PIECE_NAMES[board[row][col]],
                            PIECE_HIGHLIGHT
                            );
                } else if (engine_move.display && row == engine_move.from_row && col == engine_move.from_col) {
                    // where the piece that engine moved last was: light shade
                    gl_draw_char(
                            SQUARE_SIZE*col + SQUARE_SIZE / 2 - gl_get_char_width() / 2,
                            SQUARE_SIZE*row + SQUARE_SIZE / 2 - gl_get_char_height() / 2,
                            CHESS_GUI_PIECE_NAMES[board[row][col]],
                            is_white(board[row][col]) ? PIECE_WHITE_LIGHT : PIECE_BLACK_LIGHT
                            );
                } else {
                    gl_draw_char(
                            SQUARE_SIZE*col + SQUARE_SIZE / 2 - gl_get_char_width() / 2,
                            SQUARE_SIZE*row + SQUARE_SIZE / 2 - gl_get_char_height() / 2,
                            CHESS_GUI_PIECE_NAMES[board[row][col]],
                            is_white(board[row][col]) ? PIECE_WHITE : PIECE_BLACK
                            );
                }

#if SHOW_LETTERS
                if (row == 7) {
                    gl_draw_char(
                            SQUARE_SIZE*(col + 1) - gl_get_char_width()  - PADDING,
                            SQUARE_SIZE*(row + 1) - gl_get_char_height() - PADDING,
                            'a' + col, black_square ? CHESS_WHITE : CHESS_BLACK
                            );
                }
#endif

#if SHOW_NUMBERS
                if (col == 0) {
                    gl_draw_char(
                            SQUARE_SIZE*col + PADDING,
                            SQUARE_SIZE*row + PADDING,
                            '1' + 7 - row, black_square ? CHESS_WHITE : CHESS_BLACK
                            );
                }
#endif

                if (row == cursor.row && col == cursor.col) {
                    draw_border(
                            col * SQUARE_SIZE,
                            row * SQUARE_SIZE,
                            SQUARE_SIZE,
                            SQUARE_SIZE,
                            cursor.has_chosen ? THICK_CURSOR : THIN_CURSOR,
                            CURSOR_COLOR
                            );
                }
            }

            black_square = !black_square;

            if (after_swap)
                stale[row][col] = 0;
        }
    }
}

void chess_gui_draw(void) {
    gui_draw(false);
    gl_swap_buffer();
    gui_draw(true);
}

static void draw_text_centered(const char *text, int x, int y, int w, color_t color) {
    int char_w = gl_get_char_width();
    int text_width = char_w * strlen(text);
    
    if (text_width > w) {
        // text is too long, draw as much as possible
        text_width = w;
    }

    gl_draw_string(
            x + (w - text_width) / 2,
            y,
            text,
            color
            );
}

#if SHOW_STATS
static void draw_stat(const char *label, char *number, int line) {
    int char_h = gl_get_char_height();

    char str[64];

    str[0] = '\0';
    strlcat(str, label, sizeof(str));
    strlcat(str, sidebar.W, sizeof(str));
    strlcat(str, "%", sizeof(str));
    gl_draw_string(
            SQUARE_SIZE * 8 + H_PADDING,
            (char_h + V_PADDING) * line,
            str,
            SIDEBAR_FT
            );
}
#endif

static void sidebar_draw(void) {
    static const char *HEADERS[] = {
        "Mango Chess",
        "(Totally Legit)",
        "",
        "Javier & Ellen",
        "CS107E W2024",
        "",
    };

    gl_draw_rect(
            SQUARE_SIZE * 8,
            0,
            SCREEN_WIDTH - SQUARE_SIZE * 8,
            SCREEN_HEIGHT,
            SIDEBAR_BG
            );

    int line = 0;

    int char_h = gl_get_char_height();
    for (int i = 0; i < SIZE(HEADERS); i++) {
        draw_text_centered(
                HEADERS[i],
                SQUARE_SIZE * 8,
                (char_h + V_PADDING) * line,
                SCREEN_WIDTH - SQUARE_SIZE * 8,
                SIDEBAR_FT
                );
        line++;
    }

    draw_text_centered(
#if PLAYING == WHITE
            "Playing White",
#else
            "Playing Black",
#endif
            SQUARE_SIZE * 8,
            (char_h + V_PADDING) * line,
            SCREEN_WIDTH - SQUARE_SIZE * 8,
            SIDEBAR_FT
            );

    line ++;


#if SHOW_STATS
    line++;

    draw_text_centered(
            "Stats:",
            SQUARE_SIZE * 8,
            (char_h + V_PADDING) * line,
            SCREEN_WIDTH - SQUARE_SIZE * 8,
            SIDEBAR_FT
            );

    line++;

    draw_stat("Win:  ", sidebar.W, line++);
    draw_stat("Draw: ", sidebar.D, line++);
    draw_stat("Lose: ", sidebar.L, line++);
#endif

    line += 2;

    draw_text_centered(
            "Moves",
            SQUARE_SIZE * 8,
            (char_h + V_PADDING) * line,
            SCREEN_WIDTH - SQUARE_SIZE * 8,
            SIDEBAR_FT
            );

    line += 2;


    int i = nmoves - HISTORY_LINES;

    if (i < 0) {
        i = 0;
    } else if (i % 2 != 0) {
        i++;
    }


    for (; i < nmoves; i++) {
        gl_draw_string(
                SQUARE_SIZE * 8 + H_PADDING + (i % 2) * (SCREEN_WIDTH - SQUARE_SIZE*8) / 2,
                (char_h + V_PADDING) * line,
                move_history[i],
                SIDEBAR_FT
                );
        if (i%2 == 1) line++;
    }

}

void chess_gui_sidebar(void) {
    sidebar_draw();
    gl_swap_buffer();
    sidebar_draw();
}

void chess_gui_stats(char *W, char *D, char *L) {
#if PLAYING == BLACK
    char *tmp = W;
    W = L;
    L = tmp;
#endif

    if (W != NULL) {
        sidebar.W[0] = '\0';
        strlcat(sidebar.W, W, sizeof(sidebar.W));
    }

    if (D != NULL) {
        sidebar.D[0] = '\0';
        strlcat(sidebar.D, D, sizeof(sidebar.D));
    }

    if (L != NULL) {
        sidebar.L[0] = '\0';
        strlcat(sidebar.L, L, sizeof(sidebar.L));
    }

    chess_gui_sidebar();
}

static void draw_promote(int cursor) {
    static const char *PROMOTION_PIECES[] = {
        "Rook",
        "Knight",
        "Bishop",
        "Queen"
    };

    int char_h = gl_get_char_height();

    for (int i = 0; i < 4; i++) {
        gl_draw_string(
                SQUARE_SIZE * 8 + 5,
                SQUARE_SIZE * 6 + (char_h + 5) * (i + 1),
                PROMOTION_PIECES[i],
                cursor == i ? GL_RED : SIDEBAR_FT);
    }
}

void chess_gui_promote(int cursor) {
    if (0 <= cursor && cursor <= 3) {
        draw_promote(cursor);
        gl_swap_buffer();
        draw_promote(cursor);
    } else {
        chess_gui_sidebar();
    }
}

void chess_gui_draw_cursor(int cursor_col, int cursor_row, bool is_piece_chosen) {
    if (is_piece_chosen && !cursor.has_chosen) {
        cursor.chosen_col = cursor.col;
        cursor.chosen_row = cursor.row;
    } else if (!is_piece_chosen && cursor.has_chosen) {
        // edge case: easier to just redraw whole board
        stale_everything();
    }

    stale[cursor.row][cursor.col] = 1;

    cursor.has_chosen = is_piece_chosen;
    cursor.col = cursor_col;
    cursor.row = CHESS_SIZE - cursor_row - 1;

    stale[cursor.row][cursor.col] = 1;

    chess_gui_draw();
}

static void reset_cursor(void) {
    cursor.has_chosen = false;
    cursor.chosen_col = 0;
    cursor.chosen_row = 0;
    cursor.col = 0;

#if PLAYING == WHITE
    cursor.row = 0;
#else
    cursor.row = CHESS_SIZE - 1;
#endif
}

void chess_gui_update(const char *move, bool engine) {
    // UCI format: e2e4\n
    int col1 = move[0] - 'a';
    int col2 = move[2] - 'a';

    // row 1 in chess notation is at the bottom of our GUI, so we must invert it
    int row1 = CHESS_SIZE - (move[1] - '1') - 1;
    int row2 = CHESS_SIZE - (move[3] - '1') - 1;

    memcpy(move_history[nmoves++], move, 6);

    // keep track of taken pieces
    if (board[row2][col2] != XX) {
        sidebar.taken[sidebar.taken_count++] = board[row2][col2];
    }

    // castling
    if (board[row1][col1] == WK && col1 == 4 && row1 == 7) {
        if (col2 == 6) {
            // white short castle
            board[7][7] = XX;
            board[7][5] = WR;
        } else if (col2 == 2) {
            // white long castle
            board[7][0] = XX;
            board[7][3] = WR;
        }
    } else if (board[row1][col1] == BK && col1 == 4 && row1 == 0) {
        if (col2 == 6) {
            // black short castle
            board[0][7] = XX;
            board[0][5] = BR;
        } else if (col2 == 2) {
            // black long castle
            board[0][0] = XX;
            board[0][3] = BR;
        }
    }

    switch (move[4]) {
        case 'R':
        case 'r':
            // rook promotion
            board[row2][col2] = is_white(board[row1][col1]) ? WR : BR;
            break;

        case 'Q':
        case 'q':
            // queen promotion
            board[row2][col2] = is_white(board[row1][col1]) ? WQ : BQ;
            break;

        case 'B':
        case 'b':
            // bishop promotion
            board[row2][col2] = is_white(board[row1][col1]) ? WB : BB;
            break;

        case 'N':
        case 'n':
            // knight promotion
            board[row2][col2] = is_white(board[row1][col1]) ? WN : BN;
            break;

        default:
            // no promotion, use piece in previous position
            board[row2][col2] = board[row1][col1];
            break;
    }

    board[row1][col1] = XX;

    engine_move.display  = engine;
    engine_move.from_col = col1;
    engine_move.from_row = row1;
    engine_move.to_col   = col2;
    engine_move.to_row   = row2;

    stale_everything();
    reset_cursor();
    chess_gui_draw();
    chess_gui_sidebar();
}

void chess_gui_print(void) {
    printf("\n+---+---+---+---+---+---+---+---+\n");
    for (int i = 0; i < 8; i++) {
        printf("|");
        for (int j = 0; j < 8; j++) {
            printf(" %c |", CHESS_GUI_PIECE_NAMES[board[i][j]]);
        }
        printf("\n+---+---+---+---+---+---+---+---+\n");
    }
}

void chess_gui_init(void) {
    gl_init(SCREEN_WIDTH, SCREEN_HEIGHT, GL_DOUBLEBUFFER);
    memcpy(board, STARTING_BOARD, sizeof(STARTING_BOARD));

    sidebar.W[0] = sidebar.D[0] = sidebar.L[0] = '*';
    sidebar.W[1] = sidebar.D[1] = sidebar.L[1] = '*';
    sidebar.W[2] = sidebar.D[2] = sidebar.L[2] = '\0';

    nmoves = 0;

    stale_everything();
    reset_cursor();
    chess_gui_draw();
    chess_gui_sidebar();
}
