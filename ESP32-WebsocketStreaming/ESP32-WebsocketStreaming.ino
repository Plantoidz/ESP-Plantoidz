#include <WiFi.h>
#include <ArduinoWebsockets.h>
#include "driver/i2s.h"

using namespace websockets;
WebsocketsClient client;

// const char* ssid = "HackerSpace";
// const char* password = "teamhackers";
// const char* websockets_server_host =  "192.168.0.102";


const char* ssid = "Freebox-A5E322";
const char* password = "separes7-feminin-plagiariis-divinat";
const char* websockets_server_host =  "192.168.1.185";

const uint16_t websockets_server_port = 7777;

// AMP I2S CONNECTIONS
#define I2S_DOUT 13 
#define I2S_BCLK 14 // this should be the same as I2S_SCK for the mic
#define I2S_LRC  15 // this should be the same as I2S_WS for the mic

#define I2S_SAMPLE_RATE   (16000)
#define I2S_SAMPLE_BITS   (16)
#define I2S_READ_LEN      (1024)

#define I2S_PORT_TX I2S_NUM_0




const i2s_config_t i2s_config_tx = {
  .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_TX),
  .sample_rate = I2S_SAMPLE_RATE,
  .bits_per_sample = i2s_bits_per_sample_t(I2S_SAMPLE_BITS),
  .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
  .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
  .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
  .dma_buf_count = 32,
  .dma_buf_len = 64,
};

const i2s_pin_config_t pin_config_tx = {
  .bck_io_num = I2S_BCLK,
  .ws_io_num = I2S_LRC,
  .data_out_num = I2S_DOUT,
  .data_in_num = -1,
};

void i2s_TX_init() {
  i2s_driver_install(I2S_PORT_TX, &i2s_config_tx, 0, NULL);
  i2s_set_pin(I2S_PORT_TX, &pin_config_tx);
}




void setup() {
  Serial.begin(115200);
  
  i2s_TX_init();
  connectWiFi();

  connectWS();
  //   xTaskCreate(ping_task, "ping_task", 2048, NULL, 1, NULL);
}


void loop() {

 if (client.available()) {
    client.poll();
  }
}


void onMessageCallback(WebsocketsMessage message) {
    int msgLength = message.length();
    if(message.type() == MessageType::Binary){
      if(msgLength > 0){
        i2s_write_data((char*)message.c_str(), msgLength);
      }
    }
}

void i2s_write_data(char *buf_ptr, int buf_size){

 // i2s_write_bytes(I2S_PORT_TX, buf_ptr, buf_size, portMAX_DELAY);

 size_t i2s_bytes_write = 0;
 i2s_write(I2S_PORT_TX, buf_ptr, buf_size, &i2s_bytes_write, portMAX_DELAY);

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

void connectWS() {
  client.onMessage(onMessageCallback);
//  client.onEvent(onEventsCallback);
  while(!client.connect(websockets_server_host, websockets_server_port, "/")){
    delay(500);
    Serial.print(".");
  }
  Serial.println("Socket connected");
}


