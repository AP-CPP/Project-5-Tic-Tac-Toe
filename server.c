//  (void)obj; is used to ignore warnings

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
    char player; // player x or o
    char pos[3]; // postion of board
    int idx = -1;
    // Fixed if statements and changed it to else if
    const char *error_msg = NULL;

    // Checks if a game is being played
    if (!g_active) {
        error_msg = "no game in progress";
    // Checks if move is formated correct like with : 
    } else if (parse_move(payload, &player, pos) != 0) {
        error_msg = "bad move format";
    } else if (player != g_current) {
        error_msg = "wrong player turn";
    // Checks to see if the position is open
    } else {
        idx = label_to_index(pos);
        if (idx < 0 || g_board[idx] != EMPTY) {
            error_msg = "invalid or occupied position";
        }
    }
    // If any fail then it tells the other clients that the last attempt was "Invaild"
    if (error_msg) {
        publish_retained(m, TOPIC_STATUS, "INVALID");
        printf("Move ignored: %s. (Payload: %s)\n", error_msg, payload);
        return;
    }
    g_board[idx] = player; // move
    
    GameResult r = check_result(g_board); // checking the current board to see if there has been a win
    // If there is a win / if there is still a game
    if (r != RESULT_NONE) {
        const char *results[] = { [RESULT_X_WIN]="X_WIN", [RESULT_O_WIN]="O_WIN", [RESULT_DRAW]="DRAW" }; // results 
        end_game(m, results[r]); // winner and ends game
        return; 
    }
    g_current = (g_current == 'X') ? 'O' : 'X'; // Will change from x to o and vice versa after a game
    broadcast_state(m, turn_status());
}


static void on_connect(struct mosquitto *m, void *obj, int rc) {
    (void)obj;
    // Checking to see if connection has been established
    if (rc != 0) {
        fprintf(stderr, "Connect failed: %s\n", mosquitto_connack_string(rc));
        return;
    }
    // Monitorning / subing to topics
    printf("Connected to broker.\n");
    mosquitto_subscribe(m, NULL, TOPIC_CONTROL, 1);
    mosquitto_subscribe(m, NULL, TOPIC_MOVE,    1);
    publish_retained(m, TOPIC_STATUS, "WAITING");
}

static void on_message(struct mosquitto *m, void *obj, const struct mosquitto_message *msg) {
    (void)obj;
    // Handles payload message
    char payload[64] = {0}; 
    int n = msg->payloadlen < (int)sizeof(payload) - 1
            ? msg->payloadlen : (int)sizeof(payload) - 1;
// Explained previously similar idea
    memcpy(payload, msg->payload, n);
// See if the player wants to play alone or multiplayer
    if (strcmp(msg->topic, TOPIC_CONTROL) == 0) {
        if      (strncmp(payload, "NEW 1", 5) == 0) start_game(m, MODE_1P);
        else if (strncmp(payload, "NEW 2", 5) == 0) start_game(m, MODE_2P);
        // Error handle
        else printf("Unknown control: %s\n", payload);
    } else if (strcmp(msg->topic, TOPIC_MOVE) == 0) {
        handle_move(m, payload);
    }


int main(void) {
    // Password for mqtt
    const char *password = getenv("MQTT_PASS");
    if (!password) {
        fprintf(stderr, "Set MQTT_PASS env var with the broker password.\n");
        return 1;
    }
    // Explained previously 
    memset(g_board, EMPTY, BOARD_SIZE);

    // Mosquitto lib
    mosquitto_lib_init();
    struct mosquitto *m = mosquitto_new("tictactoe-server", true, NULL);
    if (!m) { fprintf(stderr, "mosquitto_new failed\n"); return 1; }

    // Config
    mosquitto_username_pw_set(m, MQTT_USER, password);
    mosquitto_connect_callback_set(m, on_connect);
    mosquitto_message_callback_set(m, on_message);

    // Broker connection
    if (mosquitto_connect(m, BROKER_HOST, BROKER_PORT, 60) != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "Cannot connect to %s:%d\n", BROKER_HOST, BROKER_PORT);
        return 1;
    }
    
    printf("Server starting. Ctrl+C to exit.\n");
    mosquitto_loop_forever(m, -1, 1);

    mosquitto_destroy(m);
    mosquitto_lib_cleanup();
    return 0;
}
