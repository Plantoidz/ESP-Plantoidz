//I2S STUFF


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