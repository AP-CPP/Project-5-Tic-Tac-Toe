// Importaing libraries 
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
