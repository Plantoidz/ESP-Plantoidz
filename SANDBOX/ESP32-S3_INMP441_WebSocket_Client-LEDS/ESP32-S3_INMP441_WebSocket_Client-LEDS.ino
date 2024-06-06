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


#define I2S_SD 2
#define I2S_WS 15
#define I2S_SCK 14
#define I2S_PORT I2S_NUM_0

#define bufferCnt 10
#define bufferLen 1024
int16_t sBuffer[bufferLen];

const char* ssid = "Freebox-A5E322";
const char* password = "separes7-feminin-plagiariis-divinat";
const char* websocket_server_host =  "192.168.1.185";


// const char* ssid = "HackerSpace";
// const char* password = "teamhackers";
// const char* websocket_server_host = "192.168.0.102";

const uint16_t websocket_server_port = 8888;  // <WEBSOCKET_SERVER_PORT>

using namespace websockets;
WebsocketsClient client;
bool isWebSocketConnected;

TaskHandle_t micStream;
int mic_streaming = 0;

void (*LED_function)() = &LED_listen;



void onEventsCallback(WebsocketsEvent event, String data) {
  if (event == WebsocketsEvent::ConnectionOpened) {
    Serial.println("Connnection Opened");
    isWebSocketConnected = true;
  } else if (event == WebsocketsEvent::ConnectionClosed) {
    Serial.println("Connnection Closed");
    isWebSocketConnected = false;
  } else if (event == WebsocketsEvent::GotPing) {
    Serial.println("Got a Ping!");
  } else if (event == WebsocketsEvent::GotPong) {
    Serial.println("Got a Pong!");
  }
}

void onMessageCallback(WebsocketsMessage msg) {
  Serial.print("Got message: ");
  Serial.println(msg.data());

  if(mic_streaming == 1) { 
    mic_streaming = 0 ;
    LED_function = &LED_asleep;
  } else { 
    mic_streaming = 1; 
    LED_function = &LED_listen;
  }
  
  Serial.print("current value of mic_streaming == ");
  Serial.println(mic_streaming);
}

void i2s_install() {
  // Set up I2S Processor configuration
  const i2s_config_t i2s_config = {
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

  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
}

void i2s_setpin() {
  // Set I2S pin configuration
  const i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = -1,
    .data_in_num = I2S_SD
  };

  i2s_set_pin(I2S_PORT, &pin_config);
}


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

}

void setup() {
  Serial.begin(115200);

  setup_LEDs();
  connectWiFi();
  connectWSServer();
  xTaskCreatePinnedToCore(micTask, "micTask", 10000, NULL, 1, NULL, 1);
}




void LED_listen() {
    // draw a generic, no-name rainbow
  static uint8_t starthue = 0;
  fill_rainbow(leds, NUM_LEDS, --starthue, 20);
}




void LED_asleep() {

  static uint8_t hue = 0;

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




void LED_speak() {

}

void loop() {

  // static uint8_t starthue = 0;
  // fill_rainbow(leds, NUM_LEDS, --starthue, 20);
  LED_listen();
  
  FastLED.show();
  FastLED.delay(1);

  client.poll();
  

}


void connectWiFi() {
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
}

void connectWSServer() {
  client.onEvent(onEventsCallback);
  client.onMessage(onMessageCallback);

  while (!client.connect(websocket_server_host, websocket_server_port, "/")) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Websocket Connected!");
}


void micTask(void* parameter) {

  i2s_install();
  i2s_setpin();
  i2s_start(I2S_PORT);

  size_t bytesIn = 0;

  while (1) {

    // if(mic_streaming == 1) {
     if(true) {

          esp_err_t result = i2s_read(I2S_PORT, &sBuffer, bufferLen, &bytesIn, portMAX_DELAY);
          if (result == ESP_OK && isWebSocketConnected) {
            client.sendBinary((const char*)sBuffer, bytesIn);
          }
    }
    else {
      // Serial.print("x");
    }

  }
}