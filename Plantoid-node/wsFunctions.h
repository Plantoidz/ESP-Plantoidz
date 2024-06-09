// WEBSOCKETS STUFF

void ampTask(void* parameter);
void micTask(void* parameter);
void onEventsCallback_amp(WebsocketsEvent event, String data);
void onEventsCallback_mic(WebsocketsEvent event, String data);
void set_modality(int m);
void i2s_write_data(char* buf_ptr, int buf_size);

void connectWSServer_amp() {

  client_amp.onEvent(onEventsCallback_amp);
  while (!client_amp.connect(websocket_server_host, atoi(websocket_server_port_amp), "/")) {
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
    ESP.restart();
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
    // go back to IDLE mode
        MODE = MODE_IDLE;
        LED_function = &LED_sleep;     
    // then unset the SPEAK mode
    if (serialDebug) Serial.println("DELETING TX MODE");
    i2s_TX_uninst();
    if (i2sampTask != NULL) {
      vTaskDelete(i2sampTask);
      i2sampTask = NULL;
    }
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

void connectWSServer_mic() {

  if (serialDebug) {
    Serial.print("connecting to IP server: ");
    Serial.print(websocket_server_host);
    Serial.print(" on port: ");
    Serial.println(websocket_server_port_mic);
    Serial.print("original file content: ");
    Serial.println(returnFile(SPIFFS, "/serverip.txt").c_str());
  }

  client_mic.onEvent(onEventsCallback_mic);
  client_mic.onMessage(onMessageCallback_mic);
  if (serialDebug) Serial.println("waiting for the ws server");
  while (!client_mic.connect(websocket_server_host, atoi(websocket_server_port_mic), "/")) {
    delay(500);
    if (serialDebug) Serial.print(".");
    checkButton();
  }
  if (serialDebug) Serial.println("Websocket Connected to the mic server!");
  client_mic.send(String(ESP_ID));
}
