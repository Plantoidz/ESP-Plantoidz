
#include <driver/i2s.h>

#define I2S_PORT I2S_NUM_0

#define I2S_WS 12
#define I2S_SD 32
#define I2S_SCK 14

#define bufferLen 1024


// UDP 
#include "WiFi.h"
#include "AsyncUDP.h"

const char* ssidName = "Freebox-A5E322";
const char* ssidPswd = "separes7-feminin-plagiariis-divinat";

// UDP Destination
// IPAddress udpAddress(192, 168, 0, 101);
IPAddress udpAddress(192, 168, 1, 185);
const int udpPort = 3333;
// Connection state
boolean connected = false;

//The udp library class
AsyncUDP udp;




void setupWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssidName, ssidPswd);
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.println("WiFi Failed");
        while (1) {
            delay(1000);
        }
    }
    else {
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());      
    }
}





void i2s_install() {
  // Set up I2S Processor configuration
  const i2s_config_t i2s_config = {
    .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = 44100,
    .bits_per_sample = i2s_bits_per_sample_t(16),
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_STAND_I2S ),
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
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


void setup() {
 
  // Set up Serial Monitor
  Serial.begin(115200);

  Serial.println(" * Configuring WiFi");
  setupWiFi();



  Serial.println("Setting up I2S Mic..");
 
  delay(1000);
 
  // Set up I2S
  i2s_install();
  i2s_setpin();
  i2s_start(I2S_PORT);
 
 
  delay(500);
}


// Define input buffer length
int32_t sBuffer[bufferLen];


void loop() {


      if (!connected) {
        if (udp.connect(udpAddress, udpPort)) {
            connected = true;
            Serial.println(" * Connected to host");
        
        }
    }

 // Get I2S data and place in data buffer
  size_t bytesIn = 0;
  esp_err_t result = i2s_read(I2S_PORT, &sBuffer, bufferLen, &bytesIn, portMAX_DELAY);


  if (result == ESP_OK) {



    int16_t samples_read = bytesIn / 4;

    if(samples_read > 0) {

      for(int16_t i = 0; i < samples_read; i++) {

        Serial.print(3000 * -1);
        Serial.print(" ");
        Serial.print(3000);
        Serial.print(" ");

        Serial.println(sBuffer[i]);



      char cstr[17];
      itoa(sBuffer[i], cstr, 10);
      udp.write((uint8_t*)cstr, strlen(cstr));




      }
      
    }
  }
}
