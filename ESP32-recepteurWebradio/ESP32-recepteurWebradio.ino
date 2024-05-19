/*
  Simple Internet Radio Demo + fastled
  !!!! don use delay() function in loop
  Uses MAX98357 I2S Amplifier Module
  Uses ESP32-audioI2S Library - https://github.com/schreibfaul1/ESP32-audioI2S


*/

// Include required libraries
#include "Arduino.h"
#include "WiFi.h"
#include "Audio.h"
#include <FastLED.h>



// Information about the LED strip itself
#define LED_PIN 4
#define NUM_LEDS 25
#define CHIPSET WS2811
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];
#define BRIGHTNESS 128

// Define I2S connections
#define I2S_DOUT 13 //this value is for olimex, for simple esp32 use :   22
#define I2S_BCLK 14 //this value is for olimex, for simple esp32 use :   26
#define I2S_LRC  15 //this value is for olimex, for simple esp32 use :   25

// Create audio object
Audio audio;

// Wifi Credentials
// String ssid = "Freebox-A5E322";
// String password = "separes7-feminin-plagiariis-divinat";
String ssid = "HackerSpace";
String password = "teamhackers";

void setup() {
  delay(100);  // power-up safety delay
  // It's important to set the color correction for your LED strip here,
  // so that colors can be more accurately rendered through the 'temperature' profiles
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalSMD5050);
  FastLED.setBrightness(BRIGHTNESS);
#define TEMPERATURE_1 Tungsten100W
#define TEMPERATURE_2 OvercastSky

// How many seconds to show each temperature before switching
#define DISPLAYTIME 1
// How many seconds to show black between switches
#define BLACKTIME 0

  // Start Serial Monitor
  Serial.begin(115200);

  // Setup WiFi in Station mode
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // WiFi Connected, print IP to serial monitor
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("");

  // Connect MAX98357 I2S Amplifier Module
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);

  // Set thevolume (0-100)
  audio.setVolume(50);

  // Connect to an Internet radio station (select one as desired)
  //audio.connecttohost("http://vis.media-ice.musicradio.com/CapitalMP3");
  //audio.connecttohost("mediaserv30.live-nect MAX98357 I2S Amplifier Module
  //audio.connecttohost("www.surfmusic.de/m3u/100-5-das-hitradio,4529.m3u");
  //audio.connecttohost("stream.1a-webradio.de/deutsch/mp3-128/vtuner-1a");
  //audio.connecttohost("www.antenne.de/webradio/antenne.m3u");
  audio.connecttohost("0n-80s.radionetz.de:8000/0n-70s.mp3");
}

void loop()

{
  // Run audio player
  audio.loop();


  // draw a generic, no-name rainbow
  static uint8_t starthue = 0;
  fill_rainbow(leds, NUM_LEDS, --starthue, 20);
  // fill_rainbow(0, NUM_LEDS - 1, --starthue, 20);
  // Choose which 'color temperature' profile to enable.
  uint8_t secs = (millis() / 1000) % (DISPLAYTIME * 2);
  if (secs < DISPLAYTIME) {
    FastLED.setTemperature(TEMPERATURE_1);  // first temperature
    leds[0] = TEMPERATURE_1;                // show indicator pixel
  } else {
    FastLED.setTemperature(TEMPERATURE_2);  // second temperature
    leds[0] = TEMPERATURE_2;                // show indicator pixel
  }

  // Black out the LEDs for a few secnds between color changes
  // to let the eyes and brains adjust
  if ((secs % DISPLAYTIME) < BLACKTIME) {
    memset8(leds, 0, NUM_LEDS * sizeof(CRGB));
  }

  FastLED.show();
  FastLED.delay(1);
}

// Audio status functions

void audio_info(const char *info) {
  Serial.print("info        ");
  Serial.println(info);
}
void audio_id3data(const char *info) {  //id3 metadata
  Serial.print("id3data     ");
  Serial.println(info);
}
void audio_eof_mp3(const char *info) {  //end of file
  Serial.print("eof_mp3     ");
  Serial.println(info);
}
void audio_showstation(const char *info) {
  Serial.print("station     ");
  Serial.println(info);
}
void audio_showstreaminfo(const char *info) {
  Serial.print("streaminfo  ");
  Serial.println(info);
}
void audio_showstreamtitle(const char *info) {
  Serial.print("streamtitle ");
  Serial.println(info);
}
void audio_bitrate(const char *info) {
  Serial.print("bitrate     ");
  Serial.println(info);
}
void audio_commercial(const char *info) {  //duration in sec
  Serial.print("commercial  ");
  Serial.println(info);
}
void audio_icyurl(const char *info) {  //homepage
  Serial.print("icyurl      ");
  Serial.println(info);
}
void audio_lasthost(const char *info) {  //stream URL played
  Serial.print("lasthost    ");
  Serial.println(info);
}
void audio_eof_speech(const char *info) {
  Serial.print("eof_speech  ");
  Serial.println(info);
}