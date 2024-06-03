void WiFiEvent(WiFiEvent_t event) {
  switch (event) {

    case ARDUINO_EVENT_ETH_START:
      // This will happen during setup, when the Ethernet service starts
      if (serialDebug) Serial.println("ETH Started");
      //set eth hostname here
      ETH.setHostname("esp32-ethernet"); 
      break;

    case ARDUINO_EVENT_ETH_CONNECTED:
      // This will happen when the Ethernet cable is plugged
      if (serialDebug) Serial.println("ETH Connected");
      break;

    case ARDUINO_EVENT_ETH_GOT_IP:
      // This will happen when we obtain an IP address through DHCP:
      if (serialDebug) Serial.print("Got an IP Address for ETH MAC: ");
      if (serialDebug) Serial.print(ETH.macAddress());
      if (serialDebug) Serial.print(", IPv4: ");
      if (serialDebug) Serial.print(ETH.localIP());
      if (ETH.fullDuplex()) {
        if (serialDebug) Serial.print(", FULL_DUPLEX");
      }
      if (serialDebug) Serial.print(", ");
      if (serialDebug) Serial.print(ETH.linkSpeed());
      if (serialDebug) Serial.println("Mbps");
      eth_connected = true;
      break;

    case ARDUINO_EVENT_ETH_DISCONNECTED:
      // This will happen when the Ethernet cable is unplugged
      if (serialDebug) Serial.println("ETH Disconnected");
      eth_connected = false;
      ESP.restart();
      break;

    case ARDUINO_EVENT_ETH_STOP:
      // This will happen when the ETH interface is stopped but this never happens
      if (serialDebug) Serial.println("ETH Stopped");
      eth_connected = false;
      break;

    default:
      break;
  }
}