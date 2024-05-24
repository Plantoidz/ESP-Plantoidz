/*
 ethernet und wifi plantoid node

- Compatible devices 
ESP32-S3 DevKit-C
OLIMER ESP32 POE
*/

// time to load the libs. , configs and generic functions
#include "includes.h"  //this needs to be first, or it all crashes and burns...
#include "config.h"
#include "wsSetup.h"
#include "i2sSetup.h"
#include "i2sFunctions.h"
//#include "ledFunctions.h"
//#include "wFunctions.h"
#include "wmFunctions.h"
int MODE = 0;            //MODE_IDLE 0, MODE_LISTEN 1, MODE_THINK 2, MODE_SPEAK 3
void (*LED_function)();  // ????



void setup() {
  delay(100);           // power-up safety delay
  WiFi.mode(WIFI_STA);  // explicitly set mode, esp defaults to STA+AP
  if (serialDebug) Serial.begin(115200);
  if (serialDebug) Serial.setDebugOutput(true);
  delay(3000);
  ////////////////////////////////////////// config de wifimanager
  pinMode(TRIGGER_PIN, INPUT_PULLUP);
  if (wm_nonblocking) wm.setConfigPortalBlocking(false);
  // add a custom input field
  int customFieldLength = 40;
  // test custom html(radio)
  const char* custom_radio_str = "<br/><label for='customfieldid'>Custom Field Label</label><input type='radio' name='customfieldid' value='1' checked> One<br><input type='radio' name='customfieldid' value='2'> Two<br><input type='radio' name='customfieldid' value='3'> Three";
  new (&custom_field) WiFiManagerParameter(custom_radio_str);  // custom html input
  wm.addParameter(&custom_field);
  wm.setSaveParamsCallback(saveParamCallback);
  std::vector<const char*> menu = { "wifi", "info", "param", "sep", "restart", "exit" };
  wm.setMenu(menu);
  // set dark theme
  wm.setClass("invert");
  bool res;
  res = wm.autoConnect("unconfiguredPlantoid", apPassword);  // password protected ap

  if (!res) {
    if (serialDebug) Serial.println("Failed to connect or hit timeout");
    // ESP.restart();
  } else {
    //if you get here you have connected to the WiFi
    if (serialDebug) Serial.println("Pantoid connected... by wifi :)");
  }
////////////////////////////////////////////////////////////////////////////
  setup_LEDs();
  connectWSServer_mic();
  set_modality(MODE_IDLE);
}

void setup_LEDs() {
  // It's important to set the color correction for your LED strip here,
  // so that colors can be more accurately rendered through the 'temperature' profiles
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalSMD5050);
  FastLED.setBrightness(BRIGHTNESS);
  LED_function = LED_listen;  //put the leds in initialy in listen mode
}

void LED_listen() {
  // draw a generic, no-name rainbow
  static uint8_t starthue = 0;
  fill_rainbow(leds, NUM_LEDS, --starthue, 20);
}

void LED_speak() {
  static uint8_t hue = 0;
  if (serialDebug) Serial.print("@");
  // First slide the led in one direction
  for (int i = 0; i < NUM_LEDS; i++) {
    // Set the i'th led to red
    leds[i] = CHSV(hue++, 255, 255);
    // Show the leds
    FastLED.show();
    // now that we've shown the leds, reset the i'th led to black
    // leds[i] = CRGB::Black;
    fadeall();
    // Wait a little bit before we loop around and do it again
    FastLED.delay(10);
  }

  // Now go in the other direction.
  for (int i = (NUM_LEDS)-1; i >= 0; i--) {
    // Set the i'th led to red
    leds[i] = CHSV(hue++, 255, 255);
    // Show the leds
    FastLED.show();
    // now that we've shown the leds, reset the i'th led to black
    // leds[i] = CRGB::Black;
    fadeall();
    // Wait a little bit before we loop around and do it again
    FastLED.delay(10);
  }
}
void fadeall() {
  for (int i = 0; i < NUM_LEDS; i++) { leds[i].nscale8(250); }
}

void LED_think() {
}

void LED_loop() {
  LED_function();
  FastLED.show();
  FastLED.delay(1);
}

void loop() {
  if (wm_nonblocking) wm.process();  // avoid delays() in loop when non-blocking and other long running code
  checkButton();
  if (client_mic.available()) { client_mic.poll(); }
  LED_loop();
}

void set_modality(int m) {
  //MODE_IDLE 0, MODE_LISTEN 1, MODE_THINK 2, MODE_SPEAK 3
  if (serialDebug) Serial.print("analysing status for m = ");
  if (serialDebug) Serial.print(m);
  if (m == MODE_LISTEN) {
    if (serialDebug) Serial.println("Activating listening mode.");
    MODE = MODE_LISTEN;
    LED_function = &LED_listen;
    xTaskCreatePinnedToCore(micTask, "micTask", 10000, NULL, 1, &i2smicTask, 1);
  }

  if (m == MODE_SPEAK) {
    if (serialDebug) Serial.println("Activating speaking mode.");
    // first unset the LISTEN mode
    i2s_RX_uninst();
    if (i2smicTask != NULL) {
      vTaskDelete(i2smicTask);
      i2smicTask = NULL;
    }
    // then activate the SPEAK mode
    MODE = MODE_SPEAK;
    LED_function = &LED_speak;
    xTaskCreatePinnedToCore(ampTask, "ampTask", 10000, NULL, 1, NULL, 1);
  }
}

// WEBSOCKETS STUFF
void connectWSServer_mic() {
  client_mic.onEvent(onEventsCallback_mic);
  client_mic.onMessage(onMessageCallback_mic);
  while (!client_mic.connect(websocket_server_host, websocket_server_port_mic, "/")) {
    delay(500);
    if (serialDebug) Serial.println("waiting for the ws server");
    if (serialDebug) Serial.print(".");
    checkButton();
  }
  if (serialDebug) Serial.println("Websocket Connected to the mic server!");
  client_mic.send(String(ESP_ID));
}

void connectWSServer_amp() {
  client_amp.onEvent(onEventsCallback_amp);
  while (!client_amp.connect(websocket_server_host, websocket_server_port_amp, "/")) {
    delay(500);
    if (serialDebug) Serial.print(".");
    checkButton();
  }
  if (serialDebug) Serial.println("Websocket Connected to the amp server!");
}

void onEventsCallback_mic(WebsocketsEvent event, String data) {
  if (event == WebsocketsEvent::ConnectionOpened) {
    if (serialDebug) Serial.println("Connnection Opened for mic");
    isWebSocketConnected_mic = true;
  } else if (event == WebsocketsEvent::ConnectionClosed) {
    if (serialDebug) Serial.println("Connnection Closed for mic");
    isWebSocketConnected_mic = false;
  } else if (event == WebsocketsEvent::GotPing) {
    if (serialDebug) Serial.println("Got a Ping!");
  } else if (event == WebsocketsEvent::GotPong) {
    if (serialDebug) Serial.println("Got a Pong!");
  }
}

void onEventsCallback_amp(WebsocketsEvent event, String data) {
  if (event == WebsocketsEvent::ConnectionOpened) {
    if (serialDebug) Serial.println("Connnection Opened for amp");
    isWebSocketConnected_amp = true;
  } else if (event == WebsocketsEvent::ConnectionClosed) {
    if (serialDebug) Serial.println("Connnection Closed for amp");
    isWebSocketConnected_amp = false;
  } else if (event == WebsocketsEvent::GotPing) {
    if (serialDebug) Serial.println("Got a Ping!");
  } else if (event == WebsocketsEvent::GotPong) {
    if (serialDebug) Serial.println("Got a Pong!");
  }
}

void onMessageCallback_mic(WebsocketsMessage msg) {
  if (serialDebug) Serial.print("Got message: ");
  if (serialDebug) Serial.println(msg.data());
  String data = msg.data();
  int m = data.toInt();
  set_modality(m);
  if (serialDebug) Serial.print("current value of MODE == ");
  if (serialDebug) Serial.println(MODE);
}

void onMessageCallback_amp(WebsocketsMessage message) {
  if (serialDebug) Serial.println("Receiving data stream for AMP............");
  int msgLength = message.length();
  if (message.type() == MessageType::Binary) {
    if (msgLength > 0) {
      i2s_write_data((char*)message.c_str(), msgLength);
    }
  }
}



void micTask(void* parameter) {
  i2s_RX_init(I2S_PORT_RX);
  i2s_start(I2S_PORT_RX);
  size_t bytesIn = 0;

  while (1) {
    esp_err_t result = i2s_read(I2S_PORT_RX, &sBuffer, bufferLen, &bytesIn, portMAX_DELAY);
    if (result == ESP_OK && isWebSocketConnected_mic) {
      client_mic.sendBinary((const char*)sBuffer, bytesIn);
    }
  }
}

void ampTask(void* parameter) {
  i2s_TX_init(I2S_PORT_TX);
  i2s_start(I2S_PORT_TX);
  connectWSServer_amp();
  while (1) {
    WebsocketsMessage message = client_amp.readBlocking();
    int msgLength = message.length();
    if (message.type() == MessageType::Binary) {
      if (msgLength > 0) {
        i2s_write_data((char*)message.c_str(), msgLength);
      } else {
        i2s_TX_uninst();
        vTaskDelete(NULL);
      }
    }
  }
}



