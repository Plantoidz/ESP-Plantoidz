/*
 ethernet und wifi plantoid node

- Compatible devices 
ESP32-S3 DevKit-C
OLIMER ESP32 POE
*/

// time to load the libs. , configs and generic functions
#include "includes.h"  //this needs to be first, or it all crashes and burns...
#include "config.h"
#include "wmFunctions.h"
#include "wsSetup.h"
#include "i2sSetup.h"
#include "i2sFunctions.h"
#include "ledFunctions.h"
#include "wsFunctions.h"


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