/*
 * ESP8266 WiFi Analyzer
 * Revise from ESP8266WiFi WiFiScan example.
 * Require ESP8266 board support, Adafruit GFX and ILI9341 library.
 */
#include "ESP8266WiFi.h"

#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"

// For the Adafruit shield, these are the default.
#define TFT_DC D1
#define TFT_CS D8

// Use hardware SPI (on Uno, #13, #12, #11) and the above for CS/DC
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// Graph constant
#define RSSI_CEILING -40
#define RSSI_FLOOR -100
#define GRAPH_BASELINE 222
#define GRAPH_HEIGHT 188
#define CHANNEL_WIDTH 20
#define NEAR_CHANNEL_RSSI_ALLOW -70

// Channel color mapping from channel 1 to 14
uint16_t channel_color[] = {
  ILI9341_RED,         /* 255,   0,   0 */
  ILI9341_ORANGE,      /* 255, 165,   0 */
  ILI9341_YELLOW,      /* 255, 255,   0 */
  ILI9341_GREEN,       /*   0, 255,   0 */
  ILI9341_CYAN,        /*   0, 255, 255 */
  ILI9341_MAGENTA,     /* 255,   0, 255 */
  ILI9341_RED,         /* 255,   0,   0 */
  ILI9341_ORANGE,      /* 255, 165,   0 */
  ILI9341_YELLOW,      /* 255, 255,   0 */
  ILI9341_GREEN,       /*   0, 255,   0 */
  ILI9341_CYAN,        /*   0, 255, 255 */
  ILI9341_MAGENTA,     /* 255,   0, 255 */
  ILI9341_RED,         /* 255,   0,   0 */
  ILI9341_ORANGE      /* 255, 165,   0 */
};

void setup() {
  // init LCD
  tft.begin();
  tft.setRotation(3);

  // init banner
  tft.fillScreen(ILI9341_BLUE);
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE, ILI9341_RED);
  tft.setCursor(0, 0);
  tft.print("  ESP");
  tft.setTextColor(ILI9341_WHITE, ILI9341_ORANGE);
  tft.print("8266 ");
  tft.setTextColor(ILI9341_WHITE, ILI9341_GREEN);
  tft.print(" WiFi ");
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLUE);
  tft.print(" Analyzer");

  // Set WiFi to station mode and disconnect from an AP if it was previously connected
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // rest for WiFi routine?
  delay(100);
}

void loop() { 
  uint8_t ap_count[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  int32_t max_rssi[] = {-100, -100, -100, -100, -100, -100, -100, -100, -100, -100, -100, -100, -100, -100};

  // WiFi.scanNetworks will return the number of networks found
  int n = WiFi.scanNetworks();

  // clear old graph
  tft.fillRect(0, 16, 320, 224, ILI9341_BLACK);
  tft.setTextSize(1);

  if (n == 0) {
    tft.setTextColor(ILI9341_BLACK);
    tft.setCursor(0, 16);
    tft.println("no networks found");
  } else {
    // plot found WiFi info
    for (int i = 0; i < n; i++) {
      int32_t channel = WiFi.channel(i);
      int32_t rssi = WiFi.RSSI(i);
      uint16_t color = channel_color[channel - 1];
      int height = constrain(map(rssi, RSSI_FLOOR, RSSI_CEILING, 1, GRAPH_HEIGHT), 1, GRAPH_HEIGHT);

      // channel stat
      ap_count[channel - 1]++;
      if (rssi > max_rssi[channel - 1]) {
        max_rssi[channel - 1] = rssi;
      }

      tft.drawLine(channel * CHANNEL_WIDTH, GRAPH_BASELINE - height, (channel - 1) * CHANNEL_WIDTH, GRAPH_BASELINE + 1, color);
      tft.drawLine(channel * CHANNEL_WIDTH, GRAPH_BASELINE - height, (channel + 1) * CHANNEL_WIDTH, GRAPH_BASELINE + 1, color);

      // Print SSID, signal strengh and if not encrypted
      tft.setTextColor(color);
      tft.setCursor((channel - 1) * CHANNEL_WIDTH, GRAPH_BASELINE - 10 - height);
      tft.print(WiFi.SSID(i));
      tft.print('(');
      tft.print(rssi);
      tft.print(')');
      if (WiFi.encryptionType(i) == ENC_TYPE_NONE) {
        tft.print('*');
      }

      // rest for WiFi routine?
      delay(10);
    }
  }

  // print WiFi stat
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(0, 16);
  tft.print(n);
  tft.print(" networks found, suggested channels: ");
  bool listed_first_channel = false;
  for (int i = 1; i <= 11; i++) { // channels 12-14 may not available
    if ((i == 1) || (max_rssi[i - 2] < NEAR_CHANNEL_RSSI_ALLOW)) { // check previous channel signal strengh
      if ((i == sizeof(channel_color)) || (max_rssi[i] < NEAR_CHANNEL_RSSI_ALLOW)) { // check next channel signal strengh
        if (ap_count[i - 1] == 0) { // check no AP exists in same channel
          if (!listed_first_channel) {
            listed_first_channel = true;
          } else {
            tft.print(", ");
          }
          tft.print(i);
        }
      }
    }
  }

  // draw graph base axle
  tft.drawFastHLine(0, GRAPH_BASELINE, 320, ILI9341_WHITE);
  for (int i = 1; i <= 14; i++) {
    tft.setTextColor(channel_color[i - 1]);
    tft.setCursor((i * CHANNEL_WIDTH) - ((i < 10)?3:6), GRAPH_BASELINE + 2);
    tft.print(i);
    if (ap_count[i - 1] > 0) {
      tft.setCursor((i * CHANNEL_WIDTH) - ((ap_count[i - 1] < 10)?9:12), GRAPH_BASELINE + 11);
      tft.print('(');
      tft.print(ap_count[i - 1]);
      tft.print(')');
    }
  }

  // Wait a bit before scanning again
  delay(5000);
}

