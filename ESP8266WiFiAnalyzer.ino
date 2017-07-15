/*
 * ESP8266 WiFi Analyzer
 * Revise from ESP8266WiFi WiFiScan example.
 * Require ESP8266 board support, Adafruit GFX and ILI9341 library.
 */
// Uncomment only the TFT model you are using
#define ILI9341
//#define ST7735_18GREENTAB
//#define ST7735_18REDTAB
//#define ST7735_18GBLACKTAB
//#define ST7735_144GREENTAB

//POWER SAVING SETTING
#define SCAN_COUNT_SLEEP 5
#define LCD_PWR_PIN D2
#define LED_PWR_PIN D4

#include "ESP8266WiFi.h"

#include <SPI.h>
#include <Adafruit_GFX.h>    // Core graphics library
// Hardware-specific library
#if defined(ST7735_18GREENTAB) || defined(ST7735_18REDTAB) || defined(ST7735_18GBLACKTAB) || defined(ST7735_144GREENTAB)
#include <Adafruit_ST7735.h>
#elif defined(ILI9341)
#include <Adafruit_ILI9341.h>
#endif

#define TFT_DC     D1
#define TFT_CS     D8

#if defined(ST7735_18GREENTAB) || defined(ST7735_18REDTAB) || defined(ST7735_18GBLACKTAB) || defined(ST7735_144GREENTAB)
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS,  TFT_DC, 0 /* RST */);
#elif defined(ILI9341)
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
#endif

// Graph constant
#if defined(ST7735_18GREENTAB) || defined(ST7735_18REDTAB) || defined(ST7735_18GBLACKTAB)
#define WIDTH 160
#define HEIGHT 128
#define BANNER_HEIGHT 8
#elif defined(ST7735_144GREENTAB)
#define WIDTH 128
#define HEIGHT 128
#define BANNER_HEIGHT 8
#elif defined(ILI9341)
#define WIDTH 320
#define HEIGHT 240
#define BANNER_HEIGHT 16
#endif
#define GRAPH_BASELINE (HEIGHT - 18)
#define GRAPH_HEIGHT (HEIGHT - 52)
#define CHANNEL_WIDTH (WIDTH / 16)

// RSSI RANGE
#define RSSI_CEILING -40
#define RSSI_FLOOR -100
#define NEAR_CHANNEL_RSSI_ALLOW -70

// define color
#if defined(ST7735_18GREENTAB) || defined(ST7735_18REDTAB) || defined(ST7735_18GBLACKTAB) || defined(ST7735_144GREENTAB)
#define TFT_WHITE   ST7735_WHITE    /* 255, 255, 255 */
#define TFT_BLACK   ST7735_BLACK    /*   0,   0,   0 */
#define TFT_RED     ST7735_RED      /* 255,   0,   0 */
#define TFT_ORANGE  0xFD20          /* 255, 165,   0 */
#define TFT_YELLOW  ST7735_YELLOW   /* 255, 255,   0 */
#define TFT_GREEN   ST7735_GREEN    /*   0, 255,   0 */
#define TFT_CYAN    ST7735_CYAN     /*   0, 255, 255 */
#define TFT_BLUE    ST7735_BLUE     /*   0,   0, 255 */
#define TFT_MAGENTA ST7735_MAGENTA  /* 255,   0, 255 */
#else
#define TFT_WHITE   ILI9341_WHITE   /* 255, 255, 255 */
#define TFT_BLACK   ILI9341_BLACK   /*   0,   0,   0 */
#define TFT_RED     ILI9341_RED     /* 255,   0,   0 */
#define TFT_ORANGE  ILI9341_ORANGE  /* 255, 165,   0 */
#define TFT_YELLOW  ILI9341_YELLOW  /* 255, 255,   0 */
#define TFT_GREEN   ILI9341_GREEN   /*   0, 255,   0 */
#define TFT_CYAN    ILI9341_CYAN    /*   0, 255, 255 */
#define TFT_BLUE    ILI9341_BLUE    /*   0,   0, 255 */
#define TFT_MAGENTA ILI9341_MAGENTA /* 255,   0, 255 */
#endif

// Channel color mapping from channel 1 to 14
uint16_t channel_color[] = {
  TFT_RED, TFT_ORANGE, TFT_YELLOW, TFT_GREEN, TFT_CYAN, TFT_MAGENTA,
  TFT_RED, TFT_ORANGE, TFT_YELLOW, TFT_GREEN, TFT_CYAN, TFT_MAGENTA,
  TFT_RED, TFT_ORANGE
};

uint8_t scan_count = 0;

void setup() {
  pinMode(LCD_PWR_PIN, OUTPUT);   // sets the pin as output
  pinMode(LED_PWR_PIN, OUTPUT);   // sets the pin as output
  digitalWrite(LCD_PWR_PIN, HIGH);
  digitalWrite(LED_PWR_PIN, HIGH);

  // init LCD
#if defined(ST7735_18GREENTAB)
  tft.initR(INITR_18GREENTAB);
#elif defined(ST7735_18REDTAB)
  tft.initR(INITR_18REDTAB);
#elif defined(ST7735_18GBLACKTAB)
  tft.initR(INITR_18BLACKTAB);
#elif defined(ST7735_144GREENTAB)
  tft.initR(INITR_144GREENTAB);
#else
  tft.begin();
#endif

  tft.setRotation(3);

  // init banner
#ifdef ILI9341
  tft.setTextSize(2);
#endif
  tft.fillScreen(TFT_BLUE);
  tft.setTextColor(TFT_WHITE, TFT_RED);
  tft.setCursor(0, 0);
#if defined(ST7735_144GREENTAB)
  tft.print(" ESP");
  tft.setTextColor(TFT_WHITE, TFT_ORANGE);
  tft.print("8266");
  tft.setTextColor(TFT_WHITE, TFT_GREEN);
  tft.print("WiFi");
  tft.setTextColor(TFT_WHITE, TFT_BLUE);
  tft.print("Analyzer");
#else
  tft.print(" ESP ");
  tft.setTextColor(TFT_WHITE, TFT_ORANGE);
  tft.print(" 8266 ");
  tft.setTextColor(TFT_WHITE, TFT_GREEN);
  tft.print(" WiFi ");
  tft.setTextColor(TFT_WHITE, TFT_BLUE);
  tft.print(" Analyzer");
#endif

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
  tft.fillRect(0, BANNER_HEIGHT, 320, 224, TFT_BLACK);
  tft.setTextSize(1);

  if (n == 0) {
    tft.setTextColor(TFT_BLACK);
    tft.setCursor(0, BANNER_HEIGHT);
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
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(0, BANNER_HEIGHT);
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
  tft.drawFastHLine(0, GRAPH_BASELINE, 320, TFT_WHITE);
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

  //POWER SAVING
  if (++scan_count >= SCAN_COUNT_SLEEP) {
    digitalWrite(LCD_PWR_PIN, LOW);
    digitalWrite(LED_PWR_PIN, LOW);
    ESP.deepSleep(0);
  }
}

