#include <WiFi.h>
#include <WiFiUdp.h>
#include <FastLED.h>
#define DATA_PIN 15
#define NUM_LEDS 174
#define UDP_PORT 3000

const char * ssid = "";
const char * password = "";
 
CRGB leds[NUM_LEDS];
WiFiUDP Udp;

void setup() {
  Serial.begin(115200);
  Serial.println();
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.show();
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Udp.begin(UDP_PORT);
  
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  delay(1000);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000); 
}

uint8_t a2iRes = 0;

void a2i(char c) {
  a2iRes *= 10;
  a2iRes += (uint8_t) c - '0';
}

uint16_t c2iRes = 0;

void c2i(char c) {
  c2iRes *= 255;
  c2iRes += (uint8_t) c - 1;
}

void loop() {
  char c;
  uint8_t state = 0;
  uint16_t packetSize = Udp.parsePacket();
  uint16_t led = 0;
  uint8_t colors[3] = {0, 0, 0};
  if (packetSize) {
    Serial.print("Received packet of size ");
    Serial.print(packetSize);
    Serial.print(" from ");
    IPAddress remoteIp = Udp.remoteIP();
    Serial.println(remoteIp);
	
    while (Udp.available() ) {
      c = Udp.read();
      switch (state) {
        case 0: //Z0 - Interpret ordinal number as brightness for led
          switch (c) {
            case '\0':
              state = 1;
              break;
            default:
              colors[led % 3] = (uint8_t) c - 1;
              if (led % 3 == 2) {
                leds[led / 3] = CRGB(colors[0], colors[1], colors[2]);
              }
              led++;
              break;
          }
          break;
        case 1: //Z1 - Command-Mode - Change reading-mode, start macros, ..
          switch (c) {
            case '\0':
              state = 0;
              FastLED.show();
              led = 0;
              break;
            case 'b':
              state = 0;
                for (int i = 0; i < NUM_LEDS; i++) leds[i] = CRGB(0, 0, 0);
              break;
            case '#':
              c2iRes = 0;
              state = 3;
              break;
            default:
              if ('0' <= c && c <= '9') {
                state = 2;
                a2iRes = 0;
                a2i(c);
              }
          }
          break; //Set led to value encoded in ordinal numbers
        case 3:
          if (c == '\0') {
            led = 3 * c2iRes;
            colors[0] = colors[1] = colors[2] = 0;
            state = 0;
          } else {
            c2i(c);
          }
          break;
        case 2: //set led to value encoded as decimals
          if (c == ':') {
            led = 3 * a2iRes;
            colors[0] = colors[1] = colors[2] = 0;
            state = 0;
          } else {
            a2i(c);
          }
          break;
      }
    }
    Udp.flush();
  }
}