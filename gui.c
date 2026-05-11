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


typedef enum {
    SCREEN_CONNECT,
    SCREEN_MODE,
    SCREEN_GAME
} Screen;
