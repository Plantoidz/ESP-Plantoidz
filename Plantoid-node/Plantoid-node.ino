/*
 ethernet und wifi plantoid node

- Compatible devices 
ESP32-S3 DevKit-C
OLIMEX ESP32 POE
*/

// time to load the libs. , configs and generic functions
#include "includes.h"     //this needs to be first, or it all crashes and burns...
#include "fsFunctions.h"  // yes, we have a filesystem
#include "config.h"
#include "wmFunctions.h"
#include "wsSetup.h"
#include "i2sSetup.h"
#include "i2sFunctions.h"
#include "ledFunctions.h"
#include "wsFunctions.h"
#include "ethFunctions.h"

void setup() {
  if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)) {
    Serial.println("SPIFFS Mount Failed");
    return;
  }
  delay(500);  // power-up safety delay
  pinMode(TRIGGER_PIN, INPUT_PULLUP);
  Serial.begin(115200);
  if (serialDebug) Serial.setDebugOutput(true);

  // Add a handler for network events. This is misnamed "WiFi" because the ESP32 is historically WiFi only,
  // but in our case, this will react to Ethernet events.
  Serial.print("Registering event handler for ETH events...");
  WiFi.onEvent(WiFiEvent);

  // Starth Ethernet (this does NOT start WiFi at the same time)
  Serial.print("Starting ETH interface...");
  ETH.begin();
  if (eth_connected == false) {
    setupWm();
  }
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

    // first unset the SPEAK mode
    Serial.println("DELETING TX MODE");
    i2s_TX_uninst();
    if (i2sampTask != NULL) {
      vTaskDelete(i2sampTask);
      i2sampTask = NULL;
    }

    MODE = MODE_LISTEN;
    LED_function = &LED_listen;
    xTaskCreatePinnedToCore(micTask, "micTask", 10000, NULL, 1, &i2smicTask, 1);
  }

  if (m == MODE_SPEAK) {
    if (serialDebug) Serial.println("Activating speaking mode.");
    // first unset the LISTEN mode
    Serial.println("DELETING RX MODE");
    i2s_RX_uninst();
    if (i2smicTask != NULL) {
      vTaskDelete(i2smicTask);
      i2smicTask = NULL;
    }
    // then activate the SPEAK mode
    MODE = MODE_SPEAK;
    LED_function = &LED_speak;
    xTaskCreatePinnedToCore(ampTask, "ampTask", 10000, NULL, 1, &i2sampTask, 1);
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
        //message.c_str()=*2
        i2s_write_data((char*)message.c_str(), msgLength);
      } else {
        Serial.println("DELETING AND UNINSTALLING THE TX MODE");
        i2s_TX_uninst();
        vTaskDelete(NULL);
      }
    }
  }
}