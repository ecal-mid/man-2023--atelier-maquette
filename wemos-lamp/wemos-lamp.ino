#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h> // Include the mDNS library

#include <ArduinoJson.h>

#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>

#include <Adafruit_NeoPixel.h>

#include <Preferences.h>

#include <DoubleResetDetector.h>

#include <ESPAsyncWiFiManager.h> //https://github.com/tzapu/WiFiManager

DoubleResetDetector drd(10, 0);

#define NUM_PIXELS 7 // Number of NeoPixels in the Jewel
#define DATA_PIN D3  // Arduino digital pin connected to the DIN of the Jewel

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUM_PIXELS, DATA_PIN, NEO_GRBW + NEO_KHZ800);
Preferences preferences;

DNSServer dns;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

uint32_t mainColor = pixels.Color(0, 0, 0, 0);

void setup()
{
  Serial.begin(115200);

  preferences.begin("lamp");

  AsyncWiFiManager wm(&server, &dns);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  if (drd.detectDoubleReset())
  {
    // preferences.clear();
    // wm.resetSettings();
  }

  char name[32]; // Create a character array to hold the username

  if (preferences.isKey("name"))
  {
    preferences.getString("name", name, sizeof(name));
  }
  else
  {
    snprintf(name, sizeof(name), "%s", String(ESP.getChipId()).c_str());
    preferences.putString("name", name);
  }

  if (wm.autoConnect(name))
  {
    Serial.println("Connected");
    digitalWrite(LED_BUILTIN, HIGH);
  }
  else
  {
    Serial.println("Failed to connect");
    // ESP.restart();
  }

  // MDNS.addService("http", "tcp", 80);

  if (MDNS.begin(name))
  { // Start the mDNS responder for esp8266.local
    Serial.println("mDNS responder started");
  }
  else
  {
    Serial.println("Error setting up MDNS responder!");
  }

  // serve static

  if (!LittleFS.begin())
  {
    Serial.println("An error has occurred while mounting LittleFS");
  }
  Serial.println("LittleFS mounted successfully");

  // WebSocket route
  ws.onEvent(onWebSocketEvent);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/index.html", String(), false, resolve); });

  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

  server.onNotFound(handleNotFound);
  server.addHandler(&ws);
  server.begin();

  pixels.begin(); // Initialize the NeoPixel library
  pixels.show();  // Turn all pixels off at the start
  setColor(preferences.getUInt("color", mainColor));
}

String resolve(String var)
{
  if (var == "LAMP_NAME")
  {
    return preferences.getString("name");
  }
  else
  {
    return String();
  }
}

void handleNotFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Not found");
}

void loop()
{
  drd.loop();
  MDNS.update();
}

void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  if (type == WS_EVT_CONNECT)
  {
    Serial.println("WebSocket client connected");
  }
  else if (type == WS_EVT_DISCONNECT)
  {
    Serial.println("WebSocket client disconnected");
  }
  else if (type == WS_EVT_DATA)
  {
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    String msg = "";

    if (info->final && info->index == 0 && info->len == len)
    {
      // the whole message is in a single frame and we got all of it's data
      //  Serial.printf("ws[%s][%u] %s-message[%llu]: ", server->url(), client->id(), (info->opcode == WS_TEXT)?"text":"binary", info->len);

      if (info->opcode == WS_TEXT)
      {
        for (size_t i = 0; i < info->len; i++)
        {
          msg += (char)data[i];
        }
      }

      // Serial.printf("%s\n",msg.c_str());
      handleCommand(msg, client);
    }
  }
}

void sendColor(uint32_t color, AsyncWebSocketClient *client)
{

  int r = (color >> 16) & 0xFF;
  int g = (color >> 8) & 0xFF;
  int b = color & 0xFF;
  int w = (color >> 24) & 0xFF;
  // Create a JSON object to send
  StaticJsonDocument<1000> doc;
  JsonObject json = doc.to<JsonObject>();

  json["action"] = "color";
  json["data"]["r"] = r;
  json["data"]["g"] = g;
  json["data"]["b"] = b;
  json["data"]["w"] = w;

  // Serialize the JSON object to a string
  String jsonString;
  serializeJson(doc, jsonString);

  // Send the JSON string to the WebSocket client
  client->text(jsonString);
}

void handleCommand(String &msg, AsyncWebSocketClient *client)
{
  // Parse the incoming JSON data
  DynamicJsonDocument json(2048);
  DeserializationError err = deserializeJson(json, msg);

  if (err)
  {
    Serial.print(F("deserializeJson() failed with code "));
    Serial.println(err.c_str());
    return;
  }

  const char *action = json["action"];

  if (strcmp(action, "color") == 0)
  {
    const unsigned int r = json["data"]["r"];
    const unsigned int g = json["data"]["g"];
    const unsigned int b = json["data"]["b"];
    const unsigned int w = json["data"]["w"];

    uint32_t color = pixels.Color(r, g, b, w);
    setColor(color);
    preferences.putUInt("color", color);

    // sendExcept(msg, client); // BUGGY
  }
  else if (strcmp(action, "get") == 0)
  {
    sendColor(mainColor, client);
  }
}

void sendExcept(String &msg, AsyncWebSocketClient *client)
{
  for (size_t i = 0; i < ws.count(); i++)
  {
    AsyncWebSocketClient *c = ws.client(i);
    if (c != client)
      c->text(msg);
  }

  // textAll
  // ws.textAll(msg);
}

void setColor(uint32_t color)
{

  mainColor = color;

  for (int i = 0; i < NUM_PIXELS; i++)
  {
    pixels.setPixelColor(i, color);
  }

  pixels.show();
}