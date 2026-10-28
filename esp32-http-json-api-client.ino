/*
 * ESP32 HTTPS JSON API Client
 * ---------------------------
 * Fetches JSON from a public HTTPS API, validates the server with a root CA,
 * and parses only the fields you need using an ArduinoJson filter and a
 * streaming parse (low RAM, no big String buffers).
 *
 * Default API: Open-Meteo current weather (free, no key).
 *
 * Required libraries:
 *   - ArduinoJson (Benoit Blanchon) - install via Library Manager
 *   - WiFi.h, HTTPClient.h, WiFiClientSecure.h ship with the ESP32 core
 *
 * Board: any ESP32 dev board. No external wiring.
 *
 * Setup:
 *   1. Copy config.h.example to config.h
 *   2. Edit WiFi creds, API URL, and paste the API's root CA
 *   3. Flash. Open Serial Monitor at 115200.
 *
 * (c) 2026 Fluidwire. Released under the MIT License.
 * https://github.com/fluidwire
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "config.h"

uint32_t lastPoll = 0;
bool firstPoll = true;

void connectWiFi() {
  Serial.printf("WiFi: connecting to %s ", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis() - start > 30000) {
      Serial.println(" timeout, retrying");
      WiFi.disconnect();
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      start = millis();
    }
  }
  Serial.printf(" OK, ip=%s\n", WiFi.localIP().toString().c_str());
}

bool fetchAndParse() {
  WiFiClientSecure client;
#if ALLOW_INSECURE_TLS
  client.setInsecure();
  Serial.println("WARNING: TLS validation disabled (testing only)");
#else
  client.setCACert(API_ROOT_CA);
#endif

  HTTPClient http;
  http.setTimeout(10000);
  if (!http.begin(client, API_URL)) {
    Serial.println("HTTP: begin() failed (bad URL?)");
    return false;
  }
  http.addHeader("Accept", "application/json");

  int code = http.GET();
  if (code != HTTP_CODE_OK) {
    if (code > 0) Serial.printf("HTTP GET -> %d\n", code);
    else Serial.printf("HTTP GET failed: %s\n", http.errorToString(code).c_str());
    http.end();
    return false;
  }

  // Keep only the fields we care about; everything else is discarded while parsing.
  JsonDocument filter;
  filter["current"]["time"] = true;
  filter["current"]["temperature_2m"] = true;
  filter["current"]["relative_humidity_2m"] = true;
  filter["current"]["wind_speed_10m"] = true;

  JsonDocument doc;
  DeserializationError err =
      deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter));
  http.end();

  if (err) {
    Serial.printf("JSON parse failed: %s\n", err.c_str());
    return false;
  }

  JsonObject cur = doc["current"];
  if (cur.isNull()) {
    Serial.println("JSON: no 'current' object in response");
    return false;
  }

  const char *ts = cur["time"] | "?";
  float temp = cur["temperature_2m"] | NAN;
  float hum = cur["relative_humidity_2m"] | NAN;
  float wind = cur["wind_speed_10m"] | NAN;
  Serial.printf("[%s] temp=%.1f C  humidity=%.0f %%  wind=%.1f km/h\n", ts, temp, hum, wind);
  return true;
}

void setup() {
  Serial.begin(115200);
  delay(200);
  connectWiFi();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) connectWiFi();

  uint32_t now = millis();
  if (!firstPoll && now - lastPoll < POLL_INTERVAL_MS) {
    delay(50);
    return;
  }
  firstPoll = false;
  lastPoll = now;

  if (!fetchAndParse()) {
    Serial.println("Fetch failed; will retry next interval");
  }
}
