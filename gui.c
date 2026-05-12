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
