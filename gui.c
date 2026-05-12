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

