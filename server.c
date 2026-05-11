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

static char     g_board[BOARD_SIZE];   // current 9-character board
static char     g_current = 'X';       // whose turn it is
static GameMode g_mode    = MODE_NONE; // current mode (or none if no game)
static int      g_active  = 0;         // 1 if a game is in progress

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
    Basically resting all the squares */
    memset(g_board, EMPTY, BOARD_SIZE);
    // Setting the players turn
    g_current = 'X';
    g_mode    = mode;
    // Used to set the game status as active
    g_active  = 1;
    
    printf("New game (mode=%s).\n", mode == MODE_1P ? "1P" : "2P");
    broadcast_state(m, turn_status());
}

static void end_game(struct mosquitto *m, const char *status) {
    g_active = 0;
    g_mode   = MODE_NONE;
    broadcast_state(m, status);
    printf("Game over: %s\n", status);
}

