

#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>




const char *WIFI_SSID     = "Andrew";
const char *WIFI_PASSWORD = "!Pasword1234!";


const char *MQTT_HOST     = "apcpp.duckdns.org";
const uint16_t MQTT_PORT  = 1883;
const char *MQTT_USER     = "ledctl";
const char *MQTT_PASSWORD = "Andrew12345!";
const char *CLIENT_ID     = "tictactoe-esp32";



#define SDA_PIN 13
#define SCL_PIN 14
LiquidCrystal_I2C lcd(0x27, 16, 2);

const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {15, 27, 26, 25};
byte colPins[COLS] = {2, 21, 22, 23};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);


#define TOPIC_BOARD     "tictactoe/board"
#define TOPIC_STATUS    "tictactoe/status"
#define TOPIC_AVAILABLE "tictactoe/available"
#define TOPIC_MOVE      "tictactoe/move"



WiFiClient   wifiClient;
PubSubClient mqtt(wifiClient);


char g_board[10]  = "         ";   // 9 chars + null
char g_status[16] = "WAITING";
char g_avail[64]  = "";

char pendingCol = 0;


void render() {
  lcd.clear();


  lcd.setCursor(0, 0);
  if      (strcmp(g_status, "WAITING")  == 0) lcd.print("Waiting...");
  else if (strcmp(g_status, "X_TURN")   == 0) lcd.print("X's turn");
  else if (strcmp(g_status, "O_TURN")   == 0) lcd.print("YOUR TURN (O)");
  else if (strcmp(g_status, "BOT_TURN") == 0) lcd.print("Bot's turn");
  else if (strcmp(g_status, "X_WIN")    == 0) lcd.print("X wins!");
  else if (strcmp(g_status, "O_WIN")    == 0) lcd.print("O wins!");
  else if (strcmp(g_status, "DRAW")     == 0) lcd.print("Draw!");
  else if (strcmp(g_status, "INVALID")  == 0) lcd.print("Invalid move");
  else                                         lcd.print(g_status);


  lcd.setCursor(0, 1);
  if (strcmp(g_status, "O_TURN") == 0) {
    if (pendingCol == 0) {
      lcd.print("Press A/B/C");
    } else {
      lcd.print("Got ");
      lcd.print(pendingCol);
      lcd.print(", press 1-3");
    }
  } else if (strcmp(g_status, "X_TURN")   == 0 ||
             strcmp(g_status, "BOT_TURN") == 0) {
    lcd.print("Wait your turn");
  } else if (strcmp(g_status, "X_WIN") == 0 ||
             strcmp(g_status, "O_WIN") == 0 ||
             strcmp(g_status, "DRAW")  == 0) {
    lcd.print("Press # to new");
  } else {
    lcd.print(g_avail);
  }
}




void onMqttMessage(char *topic, byte *payload, unsigned int length) {
  
  char buf[128];
  if (length >= sizeof(buf)) length = sizeof(buf) - 1;
  for (unsigned int i = 0; i < length; i++) buf[i] = (char)payload[i];
  buf[length] = '\0';

  Serial.print("[");
  Serial.print(topic);
  Serial.print("] ");
  Serial.println(buf);

  if (strcmp(topic, TOPIC_BOARD) == 0) {
    
    for (int i = 0; i < 9 && i < (int)length; i++) g_board[i] = buf[i];
    g_board[9] = '\0';
  } else if (strcmp(topic, TOPIC_STATUS) == 0) {
    strncpy(g_status, buf, sizeof(g_status) - 1);
    g_status[sizeof(g_status) - 1] = '\0';
    
    pendingCol = 0;
  } else if (strcmp(topic, TOPIC_AVAILABLE) == 0) {
    strncpy(g_avail, buf, sizeof(g_avail) - 1);
    g_avail[sizeof(g_avail) - 1] = '\0';
  }

  render();
}

void connectMqtt() {
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(onMqttMessage);

  while (!mqtt.connected()) {
    Serial.print("Connecting to MQTT broker... ");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("MQTT connect...");

    if (mqtt.connect(CLIENT_ID, MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("connected.");
      mqtt.subscribe(TOPIC_BOARD);
      mqtt.subscribe(TOPIC_STATUS);
      mqtt.subscribe(TOPIC_AVAILABLE);
    } else {
      Serial.print("failed (rc=");
      Serial.print(mqtt.state());
      Serial.println("). Retrying in 3s.");
      lcd.setCursor(0, 1);
      lcd.print("Retry in 3s");
      delay(3000);
    }
  }
}


void connectWifi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi connect...");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("WiFi connected. IP: ");
  Serial.println(WiFi.localIP());

  lcd.setCursor(0, 1);
  lcd.print("OK ");
  lcd.print(WiFi.localIP().toString());
  delay(1000);
}



void handleKey(char key) {
  Serial.print("Key: ");
  Serial.println(key);


  if (key == '#') {
    Serial.println("Publishing NEW 2");
    mqtt.publish("tictactoe/control", "NEW 2");
    pendingCol = 0;
    return;
  }


  if (strcmp(g_status, "O_TURN") != 0) {
    return;
  }


  if (pendingCol == 0) {
    if (key == 'A' || key == 'B' || key == 'C') {
      pendingCol = key;
      render();
    }
    return;
  }


  if (key == '1' || key == '2' || key == '3') {
    char move[5];
    move[0] = 'O';
    move[1] = ':';
    move[2] = pendingCol;
    move[3] = key;
    move[4] = '\0';

    Serial.print("Publishing move: ");
    Serial.println(move);
    mqtt.publish(TOPIC_MOVE, move);

    pendingCol = 0;

  } else if (key == '*') {

    pendingCol = 0;
    render();
  }
}


bool i2cAddrTest(uint8_t addr) {
  Wire.beginTransmission(addr);
  return (Wire.endTransmission() == 0);
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n=== Tic-Tac-Toe ESP32 client ===");

  Wire.begin(SDA_PIN, SCL_PIN);
  if (!i2cAddrTest(0x27) && i2cAddrTest(0x3F)) {
    lcd = LiquidCrystal_I2C(0x3F, 16, 2);
  }
  lcd.init();
  lcd.backlight();
  lcd.print("Booting...");

  connectWifi();
  connectMqtt();
  render();
}

void loop() {
  
  if (!mqtt.connected()) {
    connectMqtt();
  }
  mqtt.loop();   


  char key = keypad.getKey();
  if (key) {
    handleKey(key);
  }
}
