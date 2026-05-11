// Importing libraries 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mosquitto.h>
#include "game.h"

//Connection settings 
#define BROKER_HOST "localhost"
#define BROKER_PORT 1883
#define MQTT_USER   "ledctl"

// Subscriber topic names
#define TOPIC_CONTROL   "tictactoe/control"
#define TOPIC_MOVE      "tictactoe/move"
#define TOPIC_BOARD     "tictactoe/board"
#define TOPIC_STATUS    "tictactoe/status"
#define TOPIC_AVAILABLE "tictactoe/available"


// Game mode 
typedef enum { MODE_NONE, MODE_1P, MODE_2P } GameMode;

static char g_board[BOARD_SIZE]; // current 9-character board
static char g_current = 'X'; // whose turn it is
static GameMode g_mode = MODE_NONE; // current mode (or none if no game)
static int g_active  = 0; // 1 if a game is in progress

static void publish_retained(struct mosquitto *m, const char *topic, const char *payload) {
    mosquitto_publish(m, NULL, topic, (int)strlen(payload), payload, 1, true);
}


static const char *turn_status(void) {
    if (g_mode == MODE_1P && g_current == 'O') return "BOT_TURN";
    return (g_current == 'X') ? "X_TURN" : "O_TURN";
}

static void broadcast_state(struct mosquitto *m, const char *status) {
publish_retained(m, TOPIC_BOARD, g_board);

    /* Creating buffer * on stack for the board
    Again give it more bytes than needed*/
    char pretty[256];
    // Called from tictactoe.c to fill it with strings  X  | B1 | C1 and etc
    format_board(g_board, pretty);

    // publishing to broker with a different topic 
    publish_retained(m, TOPIC_PRETTY_BOARD, pretty);

    // Just used of holding the postions
    char avail[64];
    // Calling the avaible positions on the board from tictactoe
    available_positions(g_board, avail);
    
    // Publishing the positions
    publish_retained(m, TOPIC_AVAILABLE, avail);
    publish_retained(m, TOPIC_STATUS, status);

    
    printf("\n=== status: %s ===\n", status);
    printf("%s", pretty);
    printf("Available: %s\n\n", avail);
}

static void start_game(struct mosquitto *m, GameMode mode) {
    /* Clearing any previous game 
    Basically resting all the squares 
    memset is being called from the string.h lib
    a for loop code also be used but decided to try something new*/
    memset(g_board, EMPTY, BOARD_SIZE);
    // Setting the players turn
    g_current = 'X';
    g_mode = mode;
    // Used to set the game status as active
    g_active = 1;
    
    printf("New game (mode=%s).\n", mode == MODE_1P ? "1P" : "2P");
    broadcast_state(m, turn_status());
}

static void end_game(struct mosquitto *m, const char *status) {
    // Used to set the game status as inactive
    g_active = 0;
    g_mode = MODE_NONE;
    broadcast_state(m, status);
    printf("Game over: %s\n", status);
}

// Used for getting the data from payload 
static int parse_move(const char *payload, char *player, char *pos) {
    // First parts are used for input validation second part is for "case correction"
    // Checking to see if the player entered a vaild square and "x"
    if (strlen(payload) < 4) return -1;
    // Checking to see if the seperator to chose square is a :
    if (payload[1] != ':')   return -1;
    // Getting the first letter
    char p = payload[0];

    
    // Ascii converision for upper case
    if (p >= 'a' && p <= 'z') p -= 32;
    // Only allows x or o
    if (p != 'X' && p != 'O') return -1;
    *player = p;
    pos[0] = payload[2];
    pos[1] = payload[3];
    pos[2] = '\0';
    return 0;
}

// Used to handle game moves
static void handle_move(struct mosquitto *m, const char *payload) {
    // First looking to see if there is an active game
    if (!g_active) {
        //
        publish_retained(m, TOPIC_STATUS, "INVALID");
        printf("Move ignored: no game in progress.\n");
        return;
    }
    char player; char pos[3];
    if (parse_move(payload, &player, pos) != 0) {
        publish_retained(m, TOPIC_STATUS, "INVALID");
        printf("Bad move format: %s\n", payload);
        return;
    }
    if (player != g_current) {
        publish_retained(m, TOPIC_STATUS, "INVALID");
        printf("Not %c's turn (it's %c's).\n", player, g_current);
        return;
    }
    int idx = label_to_index(pos);
    if (idx < 0 || g_board[idx] != EMPTY) {
        publish_retained(m, TOPIC_STATUS, "INVALID");
        printf("Bad position: %s\n", pos);
        return;
    }

    g_board[idx] = player;
    GameResult r = check_result(g_board);
    if (r == RESULT_X_WIN) { end_game(m, "X_WIN"); return; }
    if (r == RESULT_O_WIN) { end_game(m, "O_WIN"); return; }
    if (r == RESULT_DRAW)  { end_game(m, "DRAW");  return; }

    g_current = (g_current == 'X') ? 'O' : 'X';
    broadcast_state(m, turn_status());
}
