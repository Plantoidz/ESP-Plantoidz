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
#include "ethFunctions.h"
void setup() {
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
    WiFi.mode(WIFI_STA);  // explicitly set mode, esp defaults to STA+AP
    delay(3000);
    Serial.println("\n Starting");

    // wm.resetSettings(); // wipe settings

    if (wm_nonblocking) wm.setConfigPortalBlocking(false);

    // add a custom input field
    int customFieldLength = 40;


    // new (&custom_field) WiFiManagerParameter("customfieldid", "Custom Field Label", "Custom Field Value", customFieldLength,"placeholder=\"Custom Field Placeholder\"");

    // test custom html input type(checkbox)
    // new (&custom_field) WiFiManagerParameter("customfieldid", "Custom Field Label", "Custom Field Value", customFieldLength,"placeholder=\"Custom Field Placeholder\" type=\"checkbox\""); // custom html type

    // test custom html(radio)
    const char* custom_radio_str = "<br/><label for='customfieldid'>Custom Field Label</label><input type='radio' name='customfieldid' value='1' checked> One<br><input type='radio' name='customfieldid' value='2'> Two<br><input type='radio' name='customfieldid' value='3'> Three";
    new (&custom_field) WiFiManagerParameter(custom_radio_str);  // custom html input

    wm.addParameter(&custom_field);
    wm.setSaveParamsCallback(saveParamCallback);

    // custom menu via array or vector
    //
    // menu tokens, "wifi","wifinoscan","info","param","close","sep","erase","restart","exit" (sep is seperator) (if param is in menu, params will not show up in wifi page!)
    // const char* menu[] = {"wifi","info","param","sep","restart","exit"};
    // wm.setMenu(menu,6);
    std::vector<const char*> menu = { "wifi", "info", "param", "sep", "restart", "exit" };
    wm.setMenu(menu);

    // set dark theme
    wm.setClass("invert");


    //set static ip
    // wm.setSTAStaticIPConfig(IPAddress(10,0,1,99), IPAddress(10,0,1,1), IPAddress(255,255,255,0)); // set static ip,gw,sn
    // wm.setShowStaticFields(true); // force show static ip fields
    // wm.setShowDnsFields(true);    // force show dns field always

    // wm.setConnectTimeout(20); // how long to try to connect for before continuing
    wm.setConfigPortalTimeout(30);  // auto close configportal after n seconds
    // wm.setCaptivePortalEnable(false); // disable captive portal redirection
    // wm.setAPClientCheck(true); // avoid timeout if client connected to softap

    // wifi scan settings
    // wm.setRemoveDuplicateAPs(false); // do not remove duplicate ap names (true)
    // wm.setMinimumSignalQuality(20);  // set min RSSI (percentage) to show in scans, null = 8%
    // wm.setShowInfoErase(false);      // do not show erase button on info page
    // wm.setScanDispPerc(true);       // show RSSI as percentage not graph icons

    // wm.setBreakAfterConfig(true);   // always exit configportal even if wifi save fails

    bool res;
    // res = wm.autoConnect(); // auto generated AP name from chipid
    // res = wm.autoConnect("AutoConnectAP"); // anonymous ap
    res = wm.autoConnect("theplant");  // password protected ap

    if (!res) {
      Serial.println("Failed to connect or hit timeout");

    } else {
      //if you get here you have connected to the WiFi
      Serial.println("connected...yeey :)");
    }
  }

  /*///////////////////////////////////////// config de wifimanager

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

  Serial.println("helooo");
  res = wm.autoConnect("unconfiguredPlantoid", apPassword);  // password protected ap

  Serial.println("hello i'm here");

  if (!res) {
    if (serialDebug) Serial.println("Failed to connect or hit timeout");
    // ESP.restart();
  } else {
    //if you get here you have connected to the WiFi
    if (serialDebug) Serial.println("Pantoid connected... by wifi :)");
  }
  */
  ///////////////////////////////////////////////////////////////////////////



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