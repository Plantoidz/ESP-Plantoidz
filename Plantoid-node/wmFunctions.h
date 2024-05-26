//WIFI MANAGER STUFF

void checkButton() {
  // check for button press
  if (digitalRead(TRIGGER_PIN) == LOW) {
    // poor mans debounce/press-hold, code not ideal for production
    delay(50);
    if (digitalRead(TRIGGER_PIN) == LOW) {
      if (serialDebug) Serial.println("Button Pressed");
      // still holding button for 3000 ms, reset settings, code not ideaa for production
      delay(3000);  // reset delay hold
      if (digitalRead(TRIGGER_PIN) == LOW) {
        if (serialDebug) Serial.println("Button Held");
        if (serialDebug) Serial.println("Erasing Config, restarting");
        wm.resetSettings();
        ESP.restart();
      }
      // start portal w delay
      if (serialDebug) Serial.println("Starting config portal");
      wm.setConfigPortalTimeout(portalDelay);
      if (!wm.startConfigPortal("UnconfiguredPlantoid", apPassword)) {
        if (serialDebug) Serial.println("failed to connect or hit timeout");
        delay(3000);
      } else {
        //if you get here you have connected to the WiFi
        if (serialDebug) Serial.println("Pantoid connected... by wifi :)");
      }
    }
  }
}

String getParam(String name) {
  //read parameter from server, for customhmtl input
  String value;
  if (wm.server->hasArg(name)) {
    value = wm.server->arg(name);
  }
  return value;
}

void saveParamCallback() {
  if (serialDebug) Serial.println("[CALLBACK] saveParamCallback fired");
  if (serialDebug) Serial.println("PARAM plantid = " + getParam("plantid"));
  if (serialDebug) Serial.println("PARAM serverip = " + getParam("serverip"));
  if (serialDebug) Serial.println("PARAM port1 = " + getParam("port1"));
  if (serialDebug) Serial.println("PARAM port2 = " + getParam("port2"));
  // now we write data into FS
  deleteFile(SPIFFS, "/plantid.txt");
  writeFile(SPIFFS, "/plantid.txt", getParam("plantid").c_str());

  deleteFile(SPIFFS, "/serverip.txt");
  writeFile(SPIFFS, "/serverip.txt", getParam("serverip").c_str());

  deleteFile(SPIFFS, "/port1.txt");
  writeFile(SPIFFS, "/port1.txt", getParam("port1").c_str());

  deleteFile(SPIFFS, "/port2.txt");
  writeFile(SPIFFS, "/port2.txt", getParam("port2").c_str());

  readFile(SPIFFS, "/plantid.txt");
  readFile(SPIFFS, "/serverip.txt");
  readFile(SPIFFS, "/port1.txt");
  readFile(SPIFFS, "/port2.txt");
  listDir(SPIFFS, "/", 0);
}


void setupWm() {
  WiFi.mode(WIFI_STA);  // explicitly set mode, esp defaults to STA+AP
  delay(3000);
  Serial.println("\n Starting");
    if (wm_nonblocking) wm.setConfigPortalBlocking(false);

  // add custom input fields
  int plantidFieldLength = 5;
  int serverIpFieldLength = 21;
  int serverportsFieldLength = 4;

  new (&custom_field) WiFiManagerParameter("plantid", "Plant id:", ESP_ID, plantidFieldLength, " placeholder=\'XXXXX'");
  new (&custom_field2) WiFiManagerParameter("serverip", "server ip:", websocket_server_host, serverIpFieldLength, "' placeholder=\'XXX.XXX.XXX.XXX'");
  new (&custom_field3) WiFiManagerParameter("port1", "Port 1:", websocket_server_port_mic, serverportsFieldLength, "' placeholder=\'XXXX'");
  new (&custom_field4) WiFiManagerParameter("port2", "Port 2:", websocket_server_port_amp, serverportsFieldLength, "' placeholder=\'XXXX'");
  wm.addParameter(&custom_field);
  wm.addParameter(&custom_field2);
  wm.addParameter(&custom_field3);
  wm.addParameter(&custom_field4);
  wm.setSaveParamsCallback(saveParamCallback);
  std::vector<const char*> menu = { "wifi", "info", "param", "sep", "restart", "exit" };  // custom menu via vector
  wm.setMenu(menu);
  wm.setClass("invert");          // set dark theme
  bool res;
  res = wm.autoConnect("UnconfiguredPlantoid", apPassword);  // password protected ap
  if (!res) {
    Serial.println("Failed to connect or hit timeout");
  } else {
    //if you get here you have connected to the WiFi
    Serial.println("Pantoid connected... by wifi :)");
  }
}
