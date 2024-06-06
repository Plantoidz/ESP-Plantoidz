/*
  ESP32 I2S Microphone Sample
  esp32-i2s-mic-sample.ino
  Sample sound from I2S microphone, display on Serial Plotter
  Requires INMP441 I2S microphone
 
  DroneBot Workshop 2022
  https://dronebotworkshop.com
*/
 


// UDP 
#include "WiFi.h"
#include "AsyncUDP.h"
// #include <WiFiUdp.h>
const int block_size = 128;



// WiFi network name and password:

// const char* ssidName = "HackerSpace";
// const char* ssidPswd = "teamhackers";
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
// WiFiUDP Udp;






// Include I2S driver
#include <driver/i2s.h>
 
// Connections to INMP441 I2S microphone
#define I2S_WS 12
#define I2S_SD 32
#define I2S_SCK 14
 
// Use I2S Processor 0
#define I2S_PORT I2S_NUM_0
 
// Define input buffer length
#define bufferLen 64
int16_t sBuffer[bufferLen];
 
void i2s_install() {
  // Set up I2S Processor configuration
  const i2s_config_t i2s_config = {
    .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = 44100,
    .bits_per_sample = i2s_bits_per_sample_t(16),
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_STAND_I2S),
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




void setup() {
 
  // Set up Serial Monitor
  Serial.begin(115200);
  Serial.println(" * Configuring WiFi");
  setupWiFi();


// Udp.begin(udpPort);
  
 
  delay(1000);
 
  // Set up I2S
  Serial.println(" * Configuring I2S");

  i2s_install();
  i2s_setpin();
  i2s_start(I2S_PORT);
 
  delay(500);
}





void i2s_mic()
{
    // int num_bytes_read = i2s_read_bytes(I2S_PORT, (char*)buffer + rpt, block_size, portMAX_DELAY);

  // size_t bytesIn = 0;
  // esp_err_t result = i2s_read(I2S_PORT, (char*)buffer + rpt, block_size, &bytesIn, portMAX_DELAY);
 
   int rangelimit = 3000;


  size_t bytesIn = 0;
  esp_err_t result = i2s_read(I2S_PORT, &sBuffer, bufferLen, &bytesIn, portMAX_DELAY);
 
     int16_t samples_read = bytesIn / 8;

  if (result == ESP_OK)
  {
    // Read I2S data buffer
    if (samples_read > 0) {
      for (int16_t i = 0; i < samples_read; ++i) {

        Serial.print(rangelimit * -1);
        Serial.print(" ");
        Serial.print(rangelimit);
        Serial.print(" ");

          Serial.println(sBuffer[i]);
                          // udp.write( (uint8_t *)buffer, 1024);
                          // udp.write((uint8_t*)buffer+1024, 1024);

      uint8_t cstr[17];
      itoa(sBuffer[i], (char*)cstr, 10);
      udp.write(cstr, 1024);



      }
    }
  }
    
    // rpt = rpt + bytesIn;
    // if (rpt > 2043) rpt = 0;
}

 
 
 int32_t buffer[512];    // Buffer
 volatile uint16_t rpt = 0; // Pointer

void loop3() {
    static uint8_t state = 0; 
    i2s_mic();

    if (!connected) {
        if (udp.connect(udpAddress, udpPort)) {
            connected = true;
            Serial.println("Connected to UDP Listener");
            Serial.println("Under Linux for listener use: netcat -u -p 16500 -l | play -t s16 -r 48000 -c 2 -");
            Serial.println("Under Linux for recorder use: netcat -u -p 16500 -l | rec -t s16 -r 48000 -c 2 - file.mp3");

        }
    }
    else {
        switch (state) {
            case 0: // wait for index to pass halfway
                if (rpt > 1023) {
                state = 1;
                }
                break;
            case 1: // send the first half of the buffer
                state = 2;
                udp.write( (uint8_t *)buffer, 1024);
                break;
            case 2: // wait for index to wrap
                if (rpt < 1023) {
                    state = 3;
                }
                break;
            case 3: // send second half of the buffer
                state = 0;
                udp.write((uint8_t*)buffer+1024, 1024);
                break;
        }
    }   
}



void loop4() {
    if (!connected) {
        if (udp.connect(udpAddress, udpPort)) {
            connected = true;
            Serial.println(" * Connected to host");
        
        }
    }

        i2s_mic();



}


void loop() {

      if (!connected) {
        if (udp.connect(udpAddress, udpPort)) {
            connected = true;
            Serial.println(" * Connected to host");
        
        }
    }
 
//  else {
  // False print statements to "lock range" on serial plotter display
  // Change rangelimit value to adjust "sensitivity"
  int rangelimit = 3000;
  // Serial.print(rangelimit * -1);
  // Serial.print(" ");
  // Serial.print(rangelimit);
  // Serial.print(" ");
 
  // Get I2S data and place in data buffer
  size_t bytesIn = 0;
  esp_err_t result = i2s_read(I2S_PORT, &sBuffer, bufferLen, &bytesIn, portMAX_DELAY);
 
     int16_t samples_read = bytesIn / 8;

  if (result == ESP_OK)
  {
    // Read I2S data buffer
    if (samples_read > 0) {
      // float mean = 0;
      for (int16_t i = 0; i < samples_read; ++i) {
        // mean += (sBuffer[i]);

  int ragelimit = 3000;

  Serial.print(rangelimit * -1);
  Serial.print(" ");
  Serial.print(rangelimit);
  Serial.print(" ");

Serial.println(sBuffer[i]);


      char cstr[17];
      itoa(sBuffer[i], cstr, 10);

// Serial.println(cstr);

      // Udp.beginPacket(udpAddress, udpPort);
      udp.write((uint8_t*)cstr, strlen(cstr));
      // Udp.endPacket();


      }

      // udp.write(reinterpret_cast <uint8_t *> (&sBuffer), samples_read);



 
      // Average the data reading
      mean /= samples_read;
 
      // Print to serial plotter
     // Serial.println(mean);

      //send to udp
     // udp.write((uint8_t*)sBuffer, bufferLen);

    }
  }

//  }
}