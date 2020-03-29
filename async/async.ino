#include "WiFi.h"
#include "AsyncUDP.h"
#include "FastLED.h"

#define NUM_LEDS 174
#define DATA_PIN 15
#define UDP_PORT 3000

const char * ssid = "";
const char * password = "";
char c;
uint8_t state = 0;
uint16_t led = 0;
uint8_t colors[3] = {0, 0, 0};

CRGB leds[NUM_LEDS];
AsyncUDP udp;

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

void setup() {
  Serial.begin(115200);
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

  pinMode(2, OUTPUT);
  digitalWrite(2, LOW);
  delay(1000);
  digitalWrite(2, HIGH);
  delay(1000);

  if (udp.listen(UDP_PORT)) {
    Serial.print("UDP Listening on IP: ");
    Serial.println(WiFi.localIP());
    udp.onPacket([](AsyncUDPPacket packet) {
//      Serial.print("UDP Packet Type: ");
//      Serial.print(packet.isBroadcast() ? "Broadcast" : packet.isMulticast() ? "Multicast" : "Unicast");
//      Serial.print(", From: ");
//      Serial.print(packet.remoteIP());
//      Serial.print(":");
//      Serial.print(packet.remotePort());
//      Serial.print(", To: ");
//      Serial.print(packet.localIP());
//      Serial.print(":");
//      Serial.print(packet.localPort());
//      Serial.print(", Length: ");
//      Serial.print(packet.length());
//      Serial.print(", Data: ");
//      Serial.write(packet.data(), packet.length());
//      Serial.println();
      while (packet.available() ) {
        c = packet.read();
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
                  //                strip.setPixelColor(led / 3, colors[0], colors[1], colors[2]);
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
                //              strip.show();
                led = 0;
                break;
              case 'b':
                state = 0;
                //              for (int i = 0; i < NUM_LEDS; i++) strip.setPixelColor(i, 0, 0, 0);
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
      //reply to the client
      //          packet.printf("Got %u bytes of data", packet.length());
    });
  }
}

void loop() {}
