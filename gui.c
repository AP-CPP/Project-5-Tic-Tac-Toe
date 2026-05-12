#include "raylib.h"
#include <mosquitto.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WIN_W 640
#define WIN_H 540

#define MQTT_USER "ledctl"
#define MQTT_PORT 1883


#define TOPIC_CONTROL "tictactoe/control"
#define TOPIC_MOVE "tictactoe/move"
#define TOPIC_BOARD "tictactoe/board"
#define TOPIC_STATUS "tictactoe/status"
#define TOPIC_AVAILABLE "tictactoe/available"
#define GRID_X      170
#define GRID_Y      120
#define CELL_SIZE   100
#define GRID_SIZE   (CELL_SIZE * 3)

typedef enum {
    SCREEN_CONNECT,
    SCREEN_MODE,
    SCREEN_GAME
} Screen;

static char g_url[128] = "apcpp.duckdns.org";
static int  g_url_len  = 17; 
static const char *g_mqtt_pass = NULL;
static struct mosquitto *g_mosq = NULL;
static int g_mqtt_connected = 0;
static char g_status_text[64] = "";


static void on_message(struct mosquitto *m, void *obj, const struct mosquitto_message *msg) {
    (void)m; (void)obj;
char buf[128];
    int n = msg->payloadlen < (int)sizeof(buf) - 1 ? msg->payloadlen : (int)sizeof(buf) - 1;
    memcpy(buf, msg->payload, n);
    buf[n] = '\0';

 if (strcmp(msg->topic, TOPIC_BOARD) == 0) {
        
        for (int i = 0; i < 9 && i < n; i++) g_board[i] = buf[i];
        g_board[9] = '\0';
    } else if (strcmp(msg->topic, TOPIC_STATUS) == 0) {
        strncpy(g_status, buf, sizeof(g_status) - 1);
        g_status[sizeof(g_status) - 1] = '\0';
    } else if (strcmp(msg->topic, TOPIC_AVAILABLE) == 0) {
        strncpy(g_avail, buf, sizeof(g_avail) - 1);
        g_avail[sizeof(g_avail) - 1] = '\0';
    }
}

static void on_connect(struct mosquitto *m, void *obj, int rc) {
    (void)obj;
    if (rc != 0) {
        snprintf(g_status_text, sizeof(g_status_text),
                 "Connect failed: %s", mosquitto_connack_string(rc));
        g_mqtt_connected = 0;
        return;
    }
 g_mqtt_connected = 1;
    snprintf(g_status_text, sizeof(g_status_text), "Connected.");
    mosquitto_subscribe(m, NULL, TOPIC_BOARD,     1);
    mosquitto_subscribe(m, NULL, TOPIC_STATUS,    1);
    mosquitto_subscribe(m, NULL, TOPIC_AVAILABLE, 1);
}
static int try_connect(void) {
    if (!g_mqtt_pass) {
        snprintf(g_status_text, sizeof(g_status_text),
                 "Set MQTT_PASS env var");
        return -1;
    }
 if (g_mosq) {
        mosquitto_loop_stop(g_mosq, true);
        mosquitto_destroy(g_mosq);
        g_mosq = NULL;
    }

    mosquitto_lib_init();
    g_mosq = mosquitto_new("tictactoe-gui", true, NULL);
    if (!g_mosq) {
        snprintf(g_status_text, sizeof(g_status_text), "mosquitto_new failed");
        return -1;
    }

    mosquitto_username_pw_set(g_mosq, MQTT_USER, g_mqtt_pass);
    mosquitto_connect_callback_set(g_mosq, on_connect);
    mosquitto_message_callback_set(g_mosq, on_message);

    snprintf(g_status_text, sizeof(g_status_text), "Connecting to %s...", g_url);

    int rc = mosquitto_connect(g_mosq, g_url, MQTT_PORT, 60);
    if (rc != MOSQ_ERR_SUCCESS) {
        snprintf(g_status_text, sizeof(g_status_text),
                 "Cannot connect: %s", mosquitto_strerror(rc));
        return -1;
    }

   
    mosquitto_loop_start(g_mosq);
    return 0;
}
 if (g_mosq) {
        mosquitto_loop_stop(g_mosq, true);
        mosquitto_destroy(g_mosq);
        g_mosq = NULL;
    }

    mosquitto_lib_init();
    g_mosq = mosquitto_new("tictactoe-gui", true, NULL);
    if (!g_mosq) {
        snprintf(g_status_text, sizeof(g_status_text), "mosquitto_new failed");
        return -1;
    }

    mosquitto_username_pw_set(g_mosq, MQTT_USER, g_mqtt_pass);
    mosquitto_connect_callback_set(g_mosq, on_connect);
    mosquitto_message_callback_set(g_mosq, on_message);

    snprintf(g_status_text, sizeof(g_status_text), "Connecting to %s...", g_url);

    int rc = mosquitto_connect(g_mosq, g_url, MQTT_PORT, 60);
    if (rc != MOSQ_ERR_SUCCESS) {
        snprintf(g_status_text, sizeof(g_status_text),
                 "Cannot connect: %s", mosquitto_strerror(rc));
        return -1;
    }

   
    mosquitto_loop_start(g_mosq);
    return 0;
}
static int button(int x, int y, int w, int h, const char *label) {
    Rectangle r = { (float)x, (float)y, (float)w, (float)h };
    Vector2 mp = GetMousePosition();
    int hover = CheckCollisionPointRec(mp, r);

    Color fill = hover ? LIGHTGRAY : RAYWHITE;
    DrawRectangleRec(r, fill);
    DrawRectangleLinesEx(r, 2, DARKGRAY);

    int tw = MeasureText(label, 22);
    DrawText(label, x + (w - tw) / 2, y + (h - 22) / 2, 22, DARKGRAY);

    return hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}
static void draw_connect_screen(void) {
    DrawText("Tic-Tac-Toe", WIN_W / 2 - MeasureText("Tic-Tac-Toe", 36) / 2,
             60, 36, DARKGRAY);
    DrawText("Broker URL:", 80, 160, 22, DARKGRAY);

    
    Rectangle box = { 80, 200, WIN_W - 160, 40 };
    DrawRectangleRec(box, RAYWHITE);
    DrawRectangleLinesEx(box, 2, DARKGRAY);
    DrawText(g_url, box.x + 8, box.y + 10, 20, BLACK);

    if (((int)(GetTime() * 2)) % 2 == 0) {
        int cx = box.x + 8 + MeasureText(g_url, 20);
        DrawLine(cx, box.y + 8, cx, box.y + box.height - 8, BLACK);
    }

    DrawText(g_status_text, 80, 260, 18, DARKGRAY);

    if (button(WIN_W / 2 - 75, 320, 150, 50, "Connect")) {
        if (try_connect() == 0) {
        }
    }

static void update_connect_screen(void) {
    int ch = GetCharPressed();
    while (ch > 0) {
        if (ch >= 32 && ch < 127 && g_url_len < (int)sizeof(g_url) - 1) {
            g_url[g_url_len++] = (char)ch;
            g_url[g_url_len]   = '\0';
        }
        ch = GetCharPressed();
    }
    if (IsKeyPressed(KEY_BACKSPACE) && g_url_len > 0) {
        g_url[--g_url_len] = '\0';
    }
    if (IsKeyPressed(KEY_ENTER)) {
        try_connect();
    }
}

static void draw_mode_screen(void) {
    DrawText("Choose a mode:",
             WIN_W / 2 - MeasureText("Choose a mode:", 30) / 2,
             80, 30, DARKGRAY);

    if (button(WIN_W / 2 - 150, 180, 300, 60, "1-Player (vs Bot)")) {
        publish_string(TOPIC_CONTROL, "NEW 1");
        g_screen = SCREEN_GAME;
    }
    if (button(WIN_W / 2 - 150, 280, 300, 60, "2-Player (vs ESP32)")) {
        publish_string(TOPIC_CONTROL, "NEW 2");
        g_screen = SCREEN_GAME;
    }
}




static void index_to_label(int idx, char *out) {
    out[0] = 'A' + (idx % 3);
    out[1] = '1' + (idx / 3);
    out[2] = '\0';
}

static const char *friendly_status(void) {
    if (strcmp(g_status, "WAITING")  == 0) return "Waiting for a game";
    if (strcmp(g_status, "X_TURN")   == 0) return "YOUR TURN (X)";
    if (strcmp(g_status, "O_TURN")   == 0) return "Waiting for O";
    if (strcmp(g_status, "BOT_TURN") == 0) return "Bot is thinking...";
    if (strcmp(g_status, "X_WIN")    == 0) return "X wins!";
    if (strcmp(g_status, "O_WIN")    == 0) return "O wins!";
    if (strcmp(g_status, "DRAW")     == 0) return "It's a draw";
    if (strcmp(g_status, "INVALID")  == 0) return "Invalid move - try again";
    return g_status;
}

static void draw_game_screen(void) {
    const char *fs = friendly_status();
    DrawText(fs, WIN_W / 2 - MeasureText(fs, 24) / 2, 50, 24, DARKGRAY);


    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 3; col++) {
            int x = GRID_X + col * CELL_SIZE;
            int y = GRID_Y + row * CELL_SIZE;
            int idx = row * 3 + col;
            char c = g_board[idx];

            Rectangle cell = { (float)x, (float)y, CELL_SIZE, CELL_SIZE };

            Vector2 mp = GetMousePosition();
            int hover = (c == ' ' && strcmp(g_status, "X_TURN") == 0
                         && CheckCollisionPointRec(mp, cell));

            DrawRectangleRec(cell, hover ? LIGHTGRAY : RAYWHITE);
            DrawRectangleLinesEx(cell, 2, DARKGRAY);

            if (c == 'X') {
                DrawText("X", x + CELL_SIZE / 2 - 18, y + CELL_SIZE / 2 - 30,
                         60, MAROON);
            } else if (c == 'O') {
                DrawText("O", x + CELL_SIZE / 2 - 18, y + CELL_SIZE / 2 - 30,
                         60, DARKBLUE);
            } else {
                // Show the cell label (A1, B2, etc.) in light gray as a hint.
                char label[3];
                index_to_label(idx, label);
                DrawText(label, x + 8, y + 6, 14, LIGHTGRAY);
            }
        }
    }

    if (button(WIN_W / 2 - 75, GRID_Y + GRID_SIZE + 30, 150, 40, "New Game")) {
        g_screen = SCREEN_MODE;
    }
}

static void update_game_screen(void) {
    if (strcmp(g_status, "X_TURN") != 0) return;
    if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) return;

    Vector2 mp = GetMousePosition();
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 3; col++) {
            Rectangle cell = {
                (float)(GRID_X + col * CELL_SIZE),
                (float)(GRID_Y + row * CELL_SIZE),
                CELL_SIZE, CELL_SIZE
            };
            int idx = row * 3 + col;
            if (CheckCollisionPointRec(mp, cell) && g_board[idx] == ' ') {
                char move[5];
                move[0] = 'X';
                move[1] = ':';
                move[2] = 'A' + col;
                move[3] = '1' + row;
                move[4] = '\0';
                publish_string(TOPIC_MOVE, move);
                return;
            }
        }
    }
}
int main(void) {
    g_mqtt_pass = getenv("MQTT_PASS");
    if (!g_mqtt_pass) {
        fprintf(stderr, "Set the MQTT_PASS env var with the broker password.\n");
        return 1;
    }

    InitWindow(WIN_W, WIN_H, "Tic-Tac-Toe");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        
        if (g_screen == SCREEN_CONNECT) update_connect_screen();
        if (g_screen == SCREEN_GAME)    update_game_screen();

      
        BeginDrawing();
        ClearBackground(RAYWHITE);

        if      (g_screen == SCREEN_CONNECT) draw_connect_screen();
        else if (g_screen == SCREEN_MODE)    draw_mode_screen();
        else if (g_screen == SCREEN_GAME)    draw_game_screen();

        EndDrawing();
    }

    if (g_mosq) {
        mosquitto_loop_stop(g_mosq, true);
        mosquitto_disconnect(g_mosq);
        mosquitto_destroy(g_mosq);
    }
    mosquitto_lib_cleanup();
    CloseWindow();
    return 0;
}
