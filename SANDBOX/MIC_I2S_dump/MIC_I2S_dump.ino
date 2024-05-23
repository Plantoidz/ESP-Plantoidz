#include <driver/i2s.h>

// you shouldn't need to change these settings
#define SAMPLE_BUFFER_SIZE 64
#define SAMPLE_RATE 44100
// most microphones will probably default to left channel but you may need to tie the L/R pin low
#define I2S_MIC_CHANNEL I2S_CHANNEL_FMT_ONLY_LEFT
// either wire your microphone to the same pins or change these to match your wiring
#define I2S_MIC_SERIAL_CLOCK 14
#define I2S_MIC_LEFT_RIGHT_CLOCK 12
#define I2S_MIC_SERIAL_DATA 32

// don't mess around with this
i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = i2s_bits_per_sample_t(16),
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_STAND_I2S),
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll = false
    // .tx_desc_auto_clear = false,
    // .fixed_mclk = 0};
};

// and don't mess around with this
i2s_pin_config_t i2s_mic_pins = {
    .bck_io_num = I2S_MIC_SERIAL_CLOCK,
    .ws_io_num = I2S_MIC_LEFT_RIGHT_CLOCK,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_MIC_SERIAL_DATA};



void setup()
{
  // we need serial output for the plotter
  Serial.begin(115200);
  // start up the I2S peripheral
  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &i2s_mic_pins);
}

int16_t raw_samples[SAMPLE_BUFFER_SIZE];

void loop()
{


 int rangelimit = 3000;
 

  // read from the I2S device
  size_t bytes_read = 0;
  i2s_read(I2S_NUM_0, &raw_samples, /* sizeof(int16_t) * */ SAMPLE_BUFFER_SIZE, &bytes_read, portMAX_DELAY);

  //int samples_read = bytes_read / sizeof(int16_t);
  int16_t samples_read = bytes_read / 8;

  // dump the samples out to the serial channel.
  for (int16_t i = 0; i < samples_read; i++)
  {
     Serial.print(rangelimit * -1);
    Serial.print(" ");
    Serial.print(rangelimit);
    Serial.print(" ");

    Serial.printf("%ld\n", raw_samples[i]);
  }
}