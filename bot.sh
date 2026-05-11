#!/bin/bash


set -u

# Connection 
BROKER_HOST="localhost"
BROKER_PORT=1883
MQTT_USER="ledctl"


# Password verification
if [ -z "${MQTT_PASS:-}" ]; then
    echo "Error: set MQTT_PASS env var with the broker password." >&2
    exit 1
fi


# Getting topic and then seeing if message was recieved also made it wait two seconds
# Used for delay
get_topic() {
    local topic=$1
    mosquitto_sub -h "$BROKER_HOST" -p "$BROKER_PORT" \
        -u "$MQTT_USER" -P "$MQTT_PASS" \
        -t "$topic" -C 1 -W 2 2>/dev/null 
}


# Used for publishing payload to the topic
publish() {
    local topic=$1
    local payload=$2
    mosquitto_pub -h "$BROKER_HOST" -p "$BROKER_PORT" \
        -u "$MQTT_USER" -P "$MQTT_PASS" \
        -t "$topic" -m "$payload"
}


# Picking random moves that are vaild and available 
# Getting available moves
# If no moves are aviable
make_move() {

    local avail
    avail=$(get_topic "tictactoe/available")

    if [ -z "$avail" ]; then
        echo "[bot] No available positions; skipping."
        return
    fi

  # Here I'm creating a new array based on the commas 
  # Getting the position / index and count of each item in the array
  # ALso this is all affecting only local vars not global 
  # ALso this is all affecting only local vars not global 
  # Just getting the position of the index again for storing the random choice
    IFS=',' read -ra positions <<< "$avail"
    local count=${#positions[@]}
    local idx=$((RANDOM % count))
    local choice=${positions[$idx]}

    echo "[bot] Playing O:$choice (from $count options)"
    publish "tictactoe/move" "O:$choice"
}

echo "[bot] Starting. Listening for BOT_TURN on tictactoe/status..."

# Subscribe to status and react to BOT_TURN events.
mosquitto_sub -h "$BROKER_HOST" -p "$BROKER_PORT" \
    -u "$MQTT_USER" -P "$MQTT_PASS" \
    -t "tictactoe/status" |
while read -r status; do
    echo "[bot] status -> $status"
    if [ "$status" = "BOT_TURN" ]; then
        sleep 1
        make_move
    fi
done
