//WIFIMANAGER STUFF

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
        if (serialDebug) Serial.println("connected...yeey :)");
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
  if (serialDebug) Serial.println("PARAM customfieldid = " + getParam("customfieldid"));
}