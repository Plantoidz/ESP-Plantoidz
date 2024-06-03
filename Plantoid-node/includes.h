// we define if we want serial debug
bool serialDebug = true;
// time to load the libs.
#include <FS.h>  //this needs to be first, or it all crashes and burns...
#include <SPIFFS.h>
#include <Arduino.h>
#include <FastLED.h>
#include <driver/i2s.h>
// #include <ETH.h>  @@@ETH
#include <ArduinoWebsockets.h>
#include <WiFiManager.h>  // https://github.com/tzapu/WiFiManager
