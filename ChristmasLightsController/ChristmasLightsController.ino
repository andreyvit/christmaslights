#include <Arduino.h>
#include "config.h"
#if ENABLE_EFFECTS
#include "effects.h"
#endif

#if DEVICE_ESP8266
#include <ESP8266WiFi.h>
#endif

#if ENABLE_WIFI
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#endif

#include <NeoPixelBus.h>
#if ENABLE_SONAR
#include <ESP8266Ultrasonic.h>
#endif
#include "timer.h"

#define ENABLE_AUTO_ROTATE 0
#define ENABLE_VERBOSE_LOGGING 1

unsigned loop_timings[32];
unsigned loop_timing_index;
char deviceID[64];

enum {
  PIN_LED_STRIP = 2,
  PIN_SONAR_TRIG = 15, //D8,
  PIN_SONAR_ECHO = 13, //D7,
};

#define colorSaturation 255

#if DEVICE_ESP8266
NeoPixelBus<NeoGrbFeature, NeoEsp8266Dma800KbpsMethod> strip(kLEDCount); // GPI02 aka D4
#else
NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(kLEDCount, PIN_LED_STRIP);
#endif

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

#if ENABLE_AUTO_ROTATE
Timer<150> rotate_timer;
#endif
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
ESP8266HTTPUpdateServer updateServer;
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

unsigned compute_loop_timing_avg_ms() {
  unsigned long timings_sum = 0;
  int timings_count = sizeof(loop_timings) / sizeof(loop_timings[0]);
  for (int i = 0; i < timings_count; i++) {
    timings_sum += loop_timings[i];
  }
  unsigned timings_avg = (unsigned)((timings_sum + timings_count/2) / timings_count);  
}

#if ENABLE_WIFI
void handleRoot(void) {
  unsigned timings_avg = compute_loop_timing_avg_ms();

  char temp[400];
  snprintf(temp, sizeof(temp),
"<!DOCTYPE html><html><head><meta http-equiv='refresh' content='2' />"
"<style>* {margin: 0; padding: 0} body {font: 16px Helvetica, Arial, sans-serif; margin: 2em 4em; } p {margin-top: 0.5em;}</style>"
"<body>"
"<p>Device ID: %s"
"<p>LED count: %d"
"<p>Effect: %02d (step %02d)"
"<p>Loop time (ms): %d"
"<p><a href='/update'>Update Firmware</a>"
"</html>", deviceID, kActualLEDCount, params.effect, params.step, timings_avg);
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

#if DEVICE_ESP8266
  sprintf(deviceID, "LED_%08X", ESP.getChipId());
  Serial.printf("Booting %s\n", deviceID);
#else
  Serial.println(F("BOOT")); Serial.flush();
#endif

#if ENABLE_WIFI
//  {
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
  updateServer.setup(&webServer); // "/update"

  ArduinoOTA.setHostname(deviceID);
  ArduinoOTA.onStart([]() {
    // ArduinoOTA.getCommand() == U_FLASH
    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("OTA starting");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nOTA end");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("OTA progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA error: %u ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
    else Serial.println("Unknown");
  });

  MDNS.begin(deviceID);
  MDNS.addService("http", "tcp", 80);
#elif DEVICE_ESP8266
  WiFi.forceSleepBegin();
#endif

  pinMode(LED_BUILTIN, OUTPUT);
#if !DEVICE_ESP8266
  pinMode(PIN_LED_STRIP, OUTPUT);
#endif

  Serial.println(F("INIT")); Serial.flush();

    // this resets all the neopixels to an off state
    strip.Begin();
    strip.Show();

#if ENABLE_AUTO_ROTATE
    rotate_timer.start();
#endif
    watchdog_timer.start();
#if ENABLE_BLUE_TRACE
    blue_trace_timer.start();
#endif

    Serial.println("SETUP");
#if ENABLE_EFFECTS
    effects_reset(kActualLEDCount);
#else
    for (int i = 0; i < sizeof(movers)/sizeof(movers[0]); i++) {
      movers[i].start();
    }
#endif

#if ENABLE_EFFECTS
#if DEVICE_ESP8266
    bzero(next_pixels, kLEDCount * sizeof(next_pixels[0]));
#else
    for (int i = 0; i < kLEDCount; i++) {
      next_pixels[i] = 0;
    }
#endif
    frame_timer.start();
#endif
    Serial.println(F("RUNNING")); Serial.flush();
}

bool was_connected = false;

bool activity_indicator_state = false;
void toggle_activity_indicator() {
  digitalWrite(LED_BUILTIN, (activity_indicator_state ? HIGH: LOW));
  activity_indicator_state = !activity_indicator_state;
}

void log_statistics() {
  Serial.print(F(" LOOP="));
  Serial.print(compute_loop_timing_avg_ms());
}

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
      ArduinoOTA.begin();
    }
  } else {
    if (was_connected) {
      was_connected = false;
      Serial.println("Wi-Fi disconnected.");
    }
  }

  webServer.handleClient();

  ArduinoOTA.handle();
  MDNS.update();
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
    // Serial.println("TICKING"); Serial.flush();
    effects_tick(next_pixels, &params);
    toggle_activity_indicator();
    Serial.print("E"); Serial.print(params.effect);
    Serial.print(" S"); Serial.print(params.step);
    log_statistics();
#if TRACE_FIRST_N_PIXELS > 0
    Serial.print(F(" PX>"));
    for (int i = 0; i < LOG_PIXELS; i++) {
      uint8_t c = next_pixels[i];
      Serial.print(" ");
      Serial.print(c);
    }
#endif
    Serial.println();

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
    Serial.print(F("BLUE ")); Serial.println(blue_trace_pos);
  }
  strip.SetPixelColor(blue_trace_pos, blue);
#endif

  strip.Show();

#if ENABLE_AUTO_ROTATE
  if (rotate_timer.fired(now)) {
    strip.RotateLeft(1);
  }
#endif

#if !ENABLE_EFFECTS
  if (watchdog_timer.fired(now)) {
    toggle_activity_indicator();
    Serial.print("ALIVE");
    log_statistics();
    Serial.println();
  }
#endif

  unsigned loop_timing = millis() - now;
  loop_timings[loop_timing_index] = loop_timing;
  loop_timing_index = (loop_timing_index+1) % (sizeof(loop_timings) / sizeof(loop_timings[0]));
}

extern "C" {
  void effects_log_string(const char *message) {
    Serial.print(message); Serial.flush();
  }
  void effects_log_int(int value) {
    Serial.print(value); Serial.flush();
  }
}
