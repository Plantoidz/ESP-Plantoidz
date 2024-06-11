/*
 ethernet und wifi plantoid node

- Compatible devices 
ESP32-S3 DevKit-C
OLIMEX ESP32 POE
*/
// we define if we want serial debug
bool serialDebug = true;

// time to load the libs. , configs and generic functions
#include "includes.h"      //1 this needs to be first, or it all crashes and burns...
#include "fsFunctions.h"   //2 yes, we have a filesystem
#include "config.h"        //3 configuration file
#include "wmFunctions.h"   //4 functions for the wifimanager
#include "wsSetup.h"       //5 setup of the wifi manager
#include "i2sSetup.h"      //6 setup of the I2S stuff
#include "i2sFunctions.h"  //7functions to use I2S stuff
#include "ledFunctions.h"  //8 functions linked to the LEDS
#include "wsFunctions.h"   //9 functions for the websockets thing
// #include "ethFunctions.h" @@@ETH

void setup() {
  if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)) {
    if (serialDebug) Serial.println("SPIFFS Mount Failed");
    return;
  }
  delay(300);  // power-up safety delay
  pinMode(TRIGGER_PIN, INPUT_PULLUP);
  Serial.begin(115200);
  if (serialDebug) Serial.setDebugOutput(true);

  // Load the SPIFFS data
  ReadFromFS();

  // Add a handler for network events. This is misnamed "WiFi" because the ESP32 is historically WiFi only,
  // but in our case, this will react to Ethernet events.
  if (serialDebug) Serial.print("Registering event handler for ETH events...");
  // WiFi.onEvent(WiFiEvent);   @@@ETH

  // Starth Ethernet (this does NOT start WiFi at the same time). @@@ETH
  // if (serialDebug) Serial.print("Starting ETH interface...");
  // ETH.begin();
  // if (eth_connected == false) {
  setupWm();
  // }

  setup_LEDs();
  
  connectWSServer_mic();

  set_modality(MODE_IDLE);
  LED_function = &LED_sleep;
}

void loop() {
  if (wm_nonblocking) wm.process();  // avoid delays() in loop when non-blocking and other long running code
  checkButton();
  if (client_mic.available()) { client_mic.poll(); }
  LED_loop(1);
}




void deactivate_TX() {
  if (serialDebug) Serial.println("DELETING TX MODE");
    i2s_TX_uninst();
    if (i2sampTask != NULL) {
      vTaskDelete(i2sampTask);
      i2sampTask = NULL;
    }
}

void set_modality(int m) {

  //m = MODE_IDLE 0, MODE_LISTEN 1, MODE_THINK 2, MODE_SPEAK 3
  if (serialDebug) Serial.print("analysing status for m = ");
  if (serialDebug) Serial.print(m);



  if (m == MODE_LISTEN) {
    if (serialDebug) Serial.println("Activating listening mode.");

    // first unset the SPEAK mode
    // if (serialDebug) Serial.println("DELETING TX MODE");
    // i2s_TX_uninst();
    // if (i2sampTask != NULL) {
    //   vTaskDelete(i2sampTask);
    //   i2sampTask = NULL;
    // }


    deactivate_TX();

    MODE = MODE_LISTEN;
    LED_function = &LED_listen;
    xTaskCreatePinnedToCore(micTask, "micTask", 10000, NULL, 1, &i2smicTask, 1);
  }



  if (m == MODE_SPEAK) {
    if (serialDebug) Serial.println("Activating speaking mode.");
    // first unset the LISTEN mode
              // TODO: this will need to be reactivated when we want to use the mic again
              // if (serialDebug) Serial.println("DELETING RX MODE");
              // i2s_RX_uninst();
              // if (i2smicTask != NULL) {
              //   vTaskDelete(i2smicTask);
              //   i2smicTask = NULL;
              // }
    // then activate the SPEAK mode
    MODE = MODE_SPEAK;
    LED_function = &LED_speak;
    xTaskCreatePinnedToCore(ampTask, "ampTask", 10000, NULL, 1, &i2sampTask, 1);
  }


  if (m == MODE_THINK) {
     if (serialDebug) Serial.println("Activating thinking mode.");
     MODE = MODE_THINK;
     LED_function = &LED_think;
  }



  if (m == MODE_IDLE) {
     if (serialDebug) Serial.println("Activating idle mode.");
     MODE = MODE_IDLE;
     LED_function = &LED_sleep;

    deactivate_TX();

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
        int16_t signedSample;
        uint16_t i;
        const char* cstr = message.c_str();

        // MULTIPLE by 10 to INCREASE VOLUME
        // for (i = 0; i < message.length(); i += 2) {
        //     signedSample = *((int16_t*)(cstr + i));
        //     signedSample = signedSample * 2;
        //     *((int16_t*)(cstr + i)) = signedSample;
        // }
        i2s_write_data((char*)message.c_str(), msgLength);
      } else {
        client_amp.close();
        MODE = MODE_IDLE;
        LED_function = &LED_sleep; 

        if (serialDebug) Serial.println("DELETING AND UNINSTALLING THE TX MODE");
        i2s_TX_uninst();
        vTaskDelete(NULL);
      }
    }
  }
}