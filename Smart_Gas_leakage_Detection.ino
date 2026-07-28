//Gas detection Telegram alert
//Status, Track, Msg
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

#define WIFI_SSID "YOUR_WIFI_NAME"
#define WIFI_PASS "YOUR_WIFI_PASSWORD"

#define BOT_TOKEN "YOUR_TELEGRAM_BOT_TOKEN"
#define CHAT_ID "YOUR_CHAT_ID"

#define MQ135_PIN 34
#define LED_PIN 2
#define BUZZER 4

WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

int threshold = 1000;
bool alertSent = false;
bool onlineMsgSent = false;
bool ledStatus = true;

unsigned long lastBotCheck = 0;
int botDelay = 1000;

// 🔹 Tracking flags
bool isTracking = false;
unsigned long lastTrackTime = 0;

void setup() {
  Serial.begin(115200);

  pinMode(BUZZER, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  digitalWrite(LED_PIN, HIGH);

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connecting to WiFi");

  client.setInsecure();

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected!");

  if (!onlineMsgSent) {
    bot.sendMessage(CHAT_ID, "🟢 *ESP32 is Online!* \nGas Monitoring System Started.", "Markdown");
    onlineMsgSent = true;
  }
}

void sendLiveStatus() {
  int gasValue = analogRead(MQ135_PIN);

  String ledText = (ledStatus) ? "🟢 LED ON" : "🔴 LED OFF";
  String buzzerText = (digitalRead(BUZZER) == HIGH) ? "🔊 BUZZER ON" : "🔇 BUZZER OFF";

  String msg =
    "📡 *LIVE TRACKING DATA*\n\n"
    "🌫 Gas Value: *" + String(gasValue) + "*\n" +
    ledText + "\n" +
    buzzerText + "\n";

  bot.sendMessage(CHAT_ID, msg, "Markdown");
}

void handleNewMessages(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {

    String text = bot.messages[i].text;
    String chat_id = bot.messages[i].chat_id;

    // 🔹 // Handle /status command
    if (text == "/status") {
      sendLiveStatus();
    }

    // 🔹 Start tracking
    if (text == "/track") {
      isTracking = true;
      bot.sendMessage(chat_id, "📡 *Tracking Started*\nSending updates every 10 seconds.", "Markdown");
    }

    // 🔹 Stop tracking
    if (text == "/stop") {
      isTracking = false;
      bot.sendMessage(chat_id, "🛑 *Tracking Stopped*", "Markdown");
    }
  }
}

void loop() {

  // 🔹 Check for Telegram commands
  if (millis() - lastBotCheck > botDelay) {
    int newMessages = bot.getUpdates(bot.last_message_received + 1);
    if (newMessages) {
      handleNewMessages(newMessages);
    }
    lastBotCheck = millis();
  }

  // 🔹 Tracking sends live data every 10 sec
  if (isTracking && millis() - lastTrackTime > 10000) {
    sendLiveStatus();
    lastTrackTime = millis();
  }

  // Gas leakage detection logic
  int gasValue = analogRead(MQ135_PIN);
  Serial.println(gasValue);

  if (gasValue > threshold) {
    digitalWrite(BUZZER, HIGH);
    digitalWrite(LED_PIN, LOW);

    if (ledStatus == true) {
      bot.sendMessage(CHAT_ID, "🔴 *POWER OFF* – Gas Detected!", "Markdown");
      ledStatus = false;
    }

    if (!alertSent) {
      bot.sendMessage(CHAT_ID, "⚠️ *Gas Alert!* \nMQ135 Value: " + String(gasValue), "Markdown");
      alertSent = true;
    }

  } else {
    digitalWrite(BUZZER, LOW);
    digitalWrite(LED_PIN, HIGH);

    if (ledStatus == false) {
      bot.sendMessage(CHAT_ID, "🟢 *POWER ON* – Gas Levels Normal.", "Markdown");
      ledStatus = true;
    }

    alertSent = false;
  }

  delay(1000);
}
