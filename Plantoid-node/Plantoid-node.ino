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
//#include "i2sSetup.h"
//#include "i2sFunctions.h"
#include "ledFunctions.h"
#include "wsFunctions.h"


#define I2S_PORT_TX I2S_NUM_0
#define I2S_PORT_RX I2S_NUM_0


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

void i2s_RX_uninst() {
  i2s_driver_uninstall(I2S_PORT_RX);
}

void i2s_TX_uninst() {
  i2s_driver_uninstall(I2S_PORT_TX);
}

void i2s_write_data(char* buf_ptr, int buf_size) {
  size_t i2s_bytes_write = 0;
  i2s_write(I2S_PORT_TX, buf_ptr, buf_size, &i2s_bytes_write, portMAX_DELAY);
}

void setup() {
  delay(100);           // power-up safety delay
  WiFi.mode(WIFI_STA);  // explicitly set mode, esp defaults to STA+AP
  if (serialDebug) Serial.begin(115200);
  if (serialDebug) Serial.setDebugOutput(true);
  delay(3000);
  ////////////////////////////////////////// config de wifimanager
  pinMode(TRIGGER_PIN, INPUT_PULLUP);
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
  ////////////////////////////////////////////////////////////////////////////
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
    MODE = MODE_LISTEN;
    LED_function = &LED_listen;
    xTaskCreatePinnedToCore(micTask, "micTask", 10000, NULL, 1, &i2smicTask, 1);
  }

  if (m == MODE_SPEAK) {
    if (serialDebug) Serial.println("Activating speaking mode.");
    // first unset the LISTEN mode
    i2s_RX_uninst();
    if (i2smicTask != NULL) {
      vTaskDelete(i2smicTask);
      i2smicTask = NULL;
    }
    // then activate the SPEAK mode
    MODE = MODE_SPEAK;
    LED_function = &LED_speak;
    xTaskCreatePinnedToCore(ampTask, "ampTask", 10000, NULL, 1, NULL, 1);
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


// #define MINIMP3_ONLY_MP3
// // #define MINIMP3_IMPLEMENTATION
// #include "minimp3.h"

// void ampTask2(void* parameter) {
//   i2s_TX_init(I2S_PORT_TX);
//   i2s_start(I2S_PORT_TX);
//   connectWSServer_amp();

// static mp3dec_t mp3d;
// mp3dec_init(&mp3d);
// mp3dec_frame_info_t info;
// short pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];
// /*unsigned char *input_buf; - input byte stream*/
// // samples = mp3dec_decode_frame(&mp3d, input_buf, buf_size, pcm, &info);  

//   while (1) {
//     WebsocketsMessage message = client_amp.readBlocking();
//     int msgLength = message.length();
//     if (message.type() == MessageType::Binary) {
//       if (msgLength > 0) {

//         int samples = mp3dec_decode_frame(&mp3d, (const uint8_t*)message.c_str(), /*msgLength*/ MINIMP3_MAX_SAMPLES_PER_FRAME, pcm, &info);
//         i2s_write_data((char*)pcm, samples);

//  //       i2s_write_data((char*)message.c_str(), msgLength);
//       } else {
//         i2s_TX_uninst();
//         vTaskDelete(NULL);
//       }
//     }
//   }
// }


void ampTask(void* parameter) {
  i2s_TX_init(I2S_PORT_TX);
  i2s_start(I2S_PORT_TX);
  connectWSServer_amp();
  while (1) {
    WebsocketsMessage message = client_amp.readBlocking();
    int msgLength = message.length();
    if (message.type() == MessageType::Binary) {
      if (msgLength > 0) {
        i2s_write_data((char*)message.c_str(), msgLength);
      } else {
        i2s_TX_uninst();
        vTaskDelete(NULL);
      }
    }
  }
}