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
  .dma_buf_len = 64,
  .tx_desc_auto_clear = true
  // .use_apll = true,
  // .fixed_mclk = 0,
};

// BACKUP CONFIG (almost working, but stuttering sometimes)
// const i2s_config_t i2s_config_tx = {
//   .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_TX),
//   .sample_rate = I2S_SAMPLE_RATE,
//   .bits_per_sample = i2s_bits_per_sample_t(I2S_SAMPLE_BITS),
//   .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
//   .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
//   .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
//   .dma_buf_count = 32,
//   .dma_buf_len = 64
// };

const i2s_pin_config_t pin_config_tx = {
  .bck_io_num = I2S_BCLK,
  .ws_io_num = I2S_LRC,
  .data_out_num = I2S_DOUT,
  .data_in_num = -1,
};