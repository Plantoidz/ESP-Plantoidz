/////////////////////////////////////////////////////////////////
/*
  Broadcasting Your Voice with ESP32-S3 & INMP441
  For More Information: https://youtu.be/qq2FRv0lCPw
  Created by Eric N. (ThatProject)
*/
/////////////////////////////////////////////////////////////////

/*
- Device
ESP32-S3 DevKit-C
https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/hw-reference/esp32s3/user-guide-devkitc-1.html

- Required Library
Arduino ESP32: 2.0.9

Arduino Websockets: 0.5.3
https://github.com/gilmaimon/ArduinoWebsockets
*/

#define ESP_ID 4

#include <FastLED.h>
// Information about the LED strip itself
#define LED_PIN 4
#define NUM_LEDS 25
#define CHIPSET WS2811
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];
#define BRIGHTNESS 128


#include <driver/i2s.h>
#include <WiFi.h>
#include <ArduinoWebsockets.h>

#define I2S_PORT_TX I2S_NUM_0
#define I2S_PORT_RX I2S_NUM_0


// AMP I2S CONNECTIONS
#define I2S_DOUT 13 
#define I2S_BCLK 14 // this should be the same as I2S_SCK for the mic
#define I2S_LRC  15 // this should be the same as I2S_WS for the mic

// MIC I2S CONNECTIONS
#define I2S_SD 2
#define I2S_WS 15
#define I2S_SCK 14

#define I2S_SAMPLE_RATE   (16000)
#define I2S_SAMPLE_BITS   (16)
#define I2S_READ_LEN      (1024)

#define bufferCnt 10
#define bufferLen 1024
int16_t sBuffer[bufferLen];

#define MODE_IDLE   0
#define MODE_LISTEN 1
#define MODE_THINK  2
#define MODE_SPEAK  3

const char* ssid = "Freebox-A5E322";
const char* password = "separes7-feminin-plagiariis-divinat";
const char* websocket_server_host = "192.168.1.185";


// const char* ssid = "HackerSpace";
// const char* password = "teamhackers";
// const char* websocket_server_host = "192.168.0.102";

const uint16_t websocket_server_port_mic = 8888;  // <WEBSOCKET_SERVER_PORT> for the mic streaming
const uint16_t websocket_server_port_amp = 7777;  // <WEBSOCKET_SERVER_PORT> for the sound streaming

using namespace websockets;
WebsocketsClient client_mic;
WebsocketsClient client_amp;
bool isWebSocketConnected_mic;
bool isWebSocketConnected_amp;


TaskHandle_t i2smicTask;
int MODE = 0;

void (*LED_function)();


// I2S SETUPS

  const i2s_config_t i2s_config_rx = {
    .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),
   // .sample_rate = 44100,
    .sample_rate = 16000,
    .bits_per_sample = i2s_bits_per_sample_t(16),
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_STAND_I2S),
    .intr_alloc_flags = 0,
    .dma_buf_count = bufferCnt,
    .dma_buf_len = bufferLen,
    .use_apll = false
  };
  const i2s_pin_config_t pin_config_rx = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = -1,
    .data_in_num = I2S_SD
  };


const i2s_config_t i2s_config_tx = {
  .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_TX),
  .sample_rate = I2S_SAMPLE_RATE,
  .bits_per_sample = i2s_bits_per_sample_t(I2S_SAMPLE_BITS),
  .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
  .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
  .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
  .dma_buf_count = 32,
  .dma_buf_len = 64
};
const i2s_pin_config_t pin_config_tx = {
  .bck_io_num = I2S_BCLK,
  .ws_io_num = I2S_LRC,
  .data_out_num = I2S_DOUT,
  .data_in_num = -1,
};

void i2s_TX_init(i2s_port_t i2sport) {
  i2s_driver_install(i2sport, &i2s_config_tx, 0, NULL);
  i2s_set_pin(i2sport, &pin_config_tx);
}

void i2s_RX_init(i2s_port_t i2sport) {
  i2s_driver_install(i2sport, &i2s_config_rx, 0, NULL);
  i2s_set_pin(i2sport, &pin_config_rx);
}

void i2s_RX_uninst(){
  i2s_driver_uninstall(I2S_PORT_RX);
}

void i2s_TX_uninst(){
  i2s_driver_uninstall(I2S_PORT_TX);
}

// void i2s_buff_init(){
//   i2s_read_buff = (char*) calloc(I2S_READ_LEN, sizeof(char));
//   flash_write_buff = (uint8_t*) calloc(I2S_READ_LEN, sizeof(char));
// }












void LED_loop() {
  LED_function(); 
  FastLED.show();
  FastLED.delay(1);
}

void loop() {


  if(client_mic.available()) { client_mic.poll(); }
  // if(client_amp.available()) { client_amp.poll(); }

  LED_loop();

}


void set_modality(int m) {

// MODE_LISTEN 1, MODE_THINK 2, MODE_SPEAK 3

  Serial.print("analysing status for m = ");
  Serial.print(m);

  if(m == MODE_LISTEN) {

    Serial.println("Activating listening mode.");

    MODE = MODE_LISTEN;
    LED_function = &LED_listen;
    xTaskCreatePinnedToCore(micTask, "micTask", 10000, NULL, 1, &i2smicTask, 1);

  }

  if(m == MODE_SPEAK) {

    Serial.println("Activating speaking mode.");


    // first unset the LISTEN mode
    i2s_RX_uninst();
    if(i2smicTask != NULL) { vTaskDelete(i2smicTask); i2smicTask = NULL; }


    // then activate the SPEAK mode
    MODE = MODE_SPEAK;
    LED_function = &LED_speak;
    xTaskCreatePinnedToCore(ampTask, "ampTask", 10000, NULL, 1, NULL, 1);

  }
}





void connectWiFi() {
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("");
}



// WEBSOCKETS STUFF


void connectWSServer_mic() {
  client_mic.onEvent(onEventsCallback_mic);
  client_mic.onMessage(onMessageCallback_mic);

  while (!client_mic.connect(websocket_server_host, websocket_server_port_mic, "/")) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Websocket Connected to the mic server!");

  client_mic.send(String(ESP_ID));
}

void connectWSServer_amp() {
  client_amp.onEvent(onEventsCallback_amp);
  // client_amp.onMessage(onMessageCallback_amp);

  while (!client_amp.connect(websocket_server_host, websocket_server_port_amp, "/")) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Websocket Connected to the amp server!");
}


// WEBSOCKET STUFF

void onEventsCallback_mic(WebsocketsEvent event, String data) {
  if (event == WebsocketsEvent::ConnectionOpened) {
    Serial.println("Connnection Opened for mic");
    isWebSocketConnected_mic = true;
  } else if (event == WebsocketsEvent::ConnectionClosed) {
    Serial.println("Connnection Closed for mic");
    isWebSocketConnected_mic = false;
  } else if (event == WebsocketsEvent::GotPing) {
    Serial.println("Got a Ping!");
  } else if (event == WebsocketsEvent::GotPong) {
    Serial.println("Got a Pong!");
  }
}

void onEventsCallback_amp(WebsocketsEvent event, String data) {
  if (event == WebsocketsEvent::ConnectionOpened) {
    Serial.println("Connnection Opened for amp");
    isWebSocketConnected_amp = true;
  } else if (event == WebsocketsEvent::ConnectionClosed) {
    Serial.println("Connnection Closed for amp");
    isWebSocketConnected_amp = false;
  } else if (event == WebsocketsEvent::GotPing) {
    Serial.println("Got a Ping!");
  } else if (event == WebsocketsEvent::GotPong) {
    Serial.println("Got a Pong!");
  }
}

void onMessageCallback_mic(WebsocketsMessage msg) {
  Serial.print("Got message: ");
  Serial.println(msg.data());

  String data = msg.data();
  int m = data.toInt();
  set_modality(m);
  
  Serial.print("current value of MODE == ");
  Serial.println(MODE);
}

void onMessageCallback_amp(WebsocketsMessage message) {
    Serial.println("Receiving data stream for AMP............");
    int msgLength = message.length();
    if(message.type() == MessageType::Binary){
      if(msgLength > 0){
        i2s_write_data((char*)message.c_str(), msgLength);
      }
    }
}

void i2s_write_data(char *buf_ptr, int buf_size){

 size_t i2s_bytes_write = 0;
 i2s_write(I2S_PORT_TX, buf_ptr, buf_size, &i2s_bytes_write, portMAX_DELAY);

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


    while(1) {
      
      WebsocketsMessage message = client_amp.readBlocking();
      
      int msgLength = message.length();
      if(message.type() == MessageType::Binary){
          if(msgLength > 0){
              i2s_write_data((char*)message.c_str(), msgLength);
         }
         else {
           i2s_TX_uninst();
           vTaskDelete(NULL);
         }
      }


    }

    

}





// LED STUFF

void setup_LEDs() {
   // It's important to set the color correction for your LED strip here,
  // so that colors can be more accurately rendered through the 'temperature' profiles
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalSMD5050);
  FastLED.setBrightness(BRIGHTNESS);
  #define TEMPERATURE_1 Tungsten100W
  #define TEMPERATURE_2 OvercastSky
  // How many seconds to show each temperature before switching
  #define DISPLAYTIME 1
// How many seconds to show black between switches
  #define BLACKTIME 0

  LED_function = LED_listen;

}

void LED_listen() {
    // draw a generic, no-name rainbow
  static uint8_t starthue = 0;
  fill_rainbow(leds, NUM_LEDS, --starthue, 20);
}


void LED_speak() {

  static uint8_t hue = 0;
  Serial.print("@");

    // First slide the led in one direction
    for(int i = 0; i < NUM_LEDS; i++) {
        // Set the i'th led to red 
        leds[i] = CHSV(hue++, 255, 255);
        // Show the leds
        FastLED.show(); 
        // now that we've shown the leds, reset the i'th led to black
        // leds[i] = CRGB::Black;
        fadeall();
        // Wait a little bit before we loop around and do it again
        delay(10);
    }
 
    // Now go in the other direction.  
    for(int i = (NUM_LEDS)-1; i >= 0; i--) {
        // Set the i'th led to red 
        leds[i] = CHSV(hue++, 255, 255);
        // Show the leds
        FastLED.show();
        // now that we've shown the leds, reset the i'th led to black
        // leds[i] = CRGB::Black;
        fadeall();
        // Wait a little bit before we loop around and do it again
        delay(10);
    }

}
void fadeall() { for(int i = 0; i < NUM_LEDS; i++) { leds[i].nscale8(250); } }




void LED_think() {

}







void setup() {
  delay(100);  // power-up safety delay

  Serial.begin(115200);

  setup_LEDs();

  connectWiFi();
  connectWSServer_mic();
  // connectWSServer_amp();

  set_modality(MODE_IDLE);

}

