#define ENABLE_WIFI 1
#define ENABLE_SONAR 0
#define ENABLE_BLUE_TRACE 0
#define ENABLE_EFFECTS 1

#include "config.h"
#include "effects.h"

#include <ESP8266WiFi.h>

#if ENABLE_WIFI 
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>

//#include <ESP8266WiFi.h>          //ESP8266 Core WiFi Library (you most likely already have this in your sketch)
//#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
//#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#endif

#include <NeoPixelBus.h>
#if ENABLE_SONAR
#include <ESP8266Ultrasonic.h>
#endif
#include "timer.h"

unsigned loop_timings[32];
unsigned loop_timing_index;

enum {
  PIN_LED_STRIP = 2,
  PIN_SONAR_TRIG = 15, //D8,
  PIN_SONAR_ECHO = 13, //D7,
};

// somehow, decreasing kLEDCount to 200 causes bottom LEDs to stop doing alpha blending (WTF)
#define kLEDCount 300
#define kActualLEDCount 200

#define colorSaturation 255

NeoPixelBus<NeoGrbFeature, NeoEsp8266Dma800KbpsMethod> strip(kLEDCount); // GPI02 aka D4
#if ENABLE_SONAR
ESP8266Ultrasonic sonar(PIN_SONAR_TRIG, PIN_SONAR_ECHO);
#endif

RgbColor red(colorSaturation, 0, 0);
RgbColor green(0, colorSaturation, 0);
RgbColor yellow(colorSaturation, colorSaturation, 0);
RgbColor blue(0, 0, colorSaturation);
RgbColor white(colorSaturation);
RgbColor black(0);

#define HexColor(hex) RgbColor(((uint32_t)(hex) >> 16) & 0xFF, ((uint32_t)(hex) >> 8) & 0xFF, (uint32_t)(hex) & 0xFF)

//RgbColor colors[] = {black, red, yellow, blue, green};
RgbColor colors[] = {
  HexColor(0x000000),
  HexColor(0x1a5e50),
  HexColor(0xe7878c),
  HexColor(0xe7d120),
  HexColor(0x278330)
};

Timer<150> rotate_timer;
Timer<500> watchdog_timer;
#if ENABLE_EFFECTS
Timer<15> frame_timer;
#endif

#if ENABLE_BLUE_TRACE
int blue_trace_pos = 150;
Timer<200> blue_trace_timer;
#endif

#if ENABLE_WIFI 
//WiFiManager wifiManager;
ESP8266WebServer webServer(80);
#endif

#if ENABLE_EFFECTS
uint8_t pixels[kLEDCount];
uint8_t next_pixels[kLEDCount];
PARAMS params;
unsigned long cur_tick_time = 0;
unsigned long next_tick_time = 0;
#endif

#if !ENABLE_EFFECTS
struct Mover {
  RgbColor color;
  int pos;
  int dir;
  int step_ms;
  bool bounce;

  FlexTimer timer_;

  void start() {
    timer_.restart(step_ms);
  }

  void draw() {
    strip.SetPixelColor(pos, color);
  }

  void move(unsigned long now) {
    if (timer_.fired(now)) {
      step();
    }
  }

  void step() {
    if (bounce) {
      if (dir > 0) {
        if (pos == kLEDCount - 1) {
          dir = -dir;
        }
      } else {
        if (pos == 0) {
          dir = -dir;
        }
      }
    }

    pos += dir;
    if (!bounce) {
      if (dir > 0) {
        pos = pos % kLEDCount;
      } else {
        while (pos < 0) {
          pos += kLEDCount;
        }
      }
    }
  }
};

Mover movers[] = {
  {red,               0, +1, 20, true},
  {green,   kLEDCount-1, -1, 10, true},
  {blue,            100, +1, 30, false},
  {red,              10, +1,  2, true},
  {green,  kLEDCount-11, +1,  8, true},
  {blue,            150, -1,  1, false},
};
#endif

#if ENABLE_WIFI 
void handleRoot(void) {
  unsigned long timings_sum = 0;
  int timings_count = sizeof(loop_timings) / sizeof(loop_timings[0]);
  for (int i = 0; i < timings_count; i++) {
    timings_sum += loop_timings[i];
  }
  unsigned timings_avg = (unsigned)((timings_sum + timings_count/2) / timings_count);
  
  char temp[400];
  snprintf(temp, sizeof(temp), 
"<!DOCTYPE html><html><head><meta http-equiv='refresh' content='2' />"
"<style>* {margin: 0; padding: 0} body {font: 16px Helvetica, Arial, sans-serif; margin: 2em 4em; } p {margin-top: 0.5em;}</style>"
"<body>"
"<p>LED count: %d"
"<p>Effect: %02d (step %02d)"
"<p>Loop time (ms): %d"
"</html>", kActualLEDCount, params.effect, params.step, timings_avg);
  temp[sizeof(temp)-1] = 0;
  
  webServer.send (200, "text/html", temp);
}

void handleNotFound(void) {
  webServer.send (404, "text/plain", "Not found");
}
#endif

void setup()
{
  Serial.begin(115200);
  //while (!Serial); // wait for serial attach
  Serial.println("Booting");

#if ENABLE_WIFI 
//  {
//    String ssid = "LEDs " + String(ESP.getChipId());
//    wifiManager.autoConnect(ssid.c_str(), NULL);
//  }
  WiFi.mode(WIFI_STA);
  WiFi.begin(kWifiNetwork, kWifiPassword);
//  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
//    Serial.println("Connection Failed! Rebooting...");
//    delay(5000);
//    ESP.restart();
//  }

  webServer.on("/", handleRoot);
  webServer.onNotFound(handleNotFound);
#else
  WiFi.forceSleepBegin();
#endif
  
  pinMode(LED_BUILTIN, OUTPUT);
  

    Serial.println();
    Serial.println("Initializing...");
    Serial.flush();

    // this resets all the neopixels to an off state
    strip.Begin();
    strip.Show();

    Serial.println();
    Serial.println("Running...");

    //strip.SetPixelColor(0, red);
    //strip.SetPixelColor(1, green);
    //strip.SetPixelColor(2, blue);

    rotate_timer.start();
    watchdog_timer.start();
#if ENABLE_BLUE_TRACE
    blue_trace_timer.start();
#endif

#if ENABLE_EFFECTS
    effects_reset(kActualLEDCount);
#else
    for (int i = 0; i < sizeof(movers)/sizeof(movers[0]); i++) {
      movers[i].start();
    }
#endif

    bzero(next_pixels, kLEDCount * sizeof(pixels[0]));
#if ENABLE_EFFECTS
    frame_timer.start();
#endif
}

bool state = false;
bool was_connected = false;

void loop()
{
  unsigned long now = millis();

#if ENABLE_WIFI 
  if (WiFi.status() == WL_CONNECTED) {
    if (!was_connected) {
      was_connected = true;
      Serial.println("Wi-Fi connected.");
      Serial.println(WiFi.localIP());
      webServer.begin();
      Serial.println ("HTTP server started");
    }
  } else {
    if (was_connected) {
      was_connected = false;
      Serial.println("Wi-Fi disconnected.");
    }
  }

  webServer.handleClient();
#endif

#if ENABLE_SONAR
  if (sonar.update()) {
    //Serial.println("Got sonar update.");
  }

  const int num_leds = kLEDCount-2;
  const int cm_cutoff = 30;

  int leds = num_leds - (sonar.distance_mm() / 10 /* mm to cm */ * num_leds / cm_cutoff);
  int brightness = colorSaturation - (sonar.distance_mm() / 10 /* mm to cm */ * colorSaturation / cm_cutoff);
  if (leds < 0) {
    leds = 0;
  }
  if (brightness < 0) brightness = 0;
  if (brightness > colorSaturation) brightness = colorSaturation;

  strip.SetPixelColor(0, red);
  RgbColor color (0, brightness, 0);
  for (int i = 1; i <= num_leds; i++) {
    //strip.SetPixelColor(i, (i <= leds ? color : black));
    strip.SetPixelColor(i, color);
  }
  strip.SetPixelColor(num_leds+1, blue);
#elif ENABLE_EFFECTS
  if (next_tick_time == 0 || now >= next_tick_time) {
    memcpy(pixels, next_pixels, kLEDCount * sizeof(pixels[0]));
    effects_tick(next_pixels, &params);
    cur_tick_time = now;
    next_tick_time = now + params.next_tick_delay_ms;
  }

  if (frame_timer.fired(now)) {
    float progress = (now - cur_tick_time) / (float)(next_tick_time - cur_tick_time);
    for (int i = 0; i < kLEDCount; i++) {
      RgbColor old_color = colors[pixels[i]];
      RgbColor new_color = colors[next_pixels[i]];
      strip.SetPixelColor(i, RgbColor::LinearBlend(old_color, new_color, progress));
//      strip.SetPixelColor(i, new_color);
    }
  }

#else
  for (int i = 0; i < kLEDCount; i++) {
    strip.SetPixelColor(i, black);
  }
  for (int i = 0; i < sizeof(movers)/sizeof(movers[0]); i++) {
    movers[i].draw();
    movers[i].move(now);
  }
  //strip.SetPixelColor(0, red);
  //strip.SetPixelColor(kLEDCount-1, green);
#endif

#if ENABLE_BLUE_TRACE
  if (blue_trace_timer.fired(now)) {
    blue_trace_pos = (blue_trace_pos + 1) % kActualLEDCount;
    Serial.printf("blue at %d\n", blue_trace_pos);
  }
  strip.SetPixelColor(blue_trace_pos, blue);
#endif
  
  strip.Show();

  if (rotate_timer.fired(now)) {
    //strip.RotateLeft(1);  
  }
  if (watchdog_timer.fired(now)) {
#if ENABLE_EFFECTS
    Serial.printf("Effect %02d step %02d\n", params.effect, params.step);
#endif
    digitalWrite(LED_BUILTIN, (state ? HIGH: LOW));
    state = !state;
  }

  unsigned loop_timing = millis() - now;
  loop_timings[loop_timing_index] = loop_timing;
  loop_timing_index = (loop_timing_index+1) % (sizeof(loop_timings) / sizeof(loop_timings[0]));
}

