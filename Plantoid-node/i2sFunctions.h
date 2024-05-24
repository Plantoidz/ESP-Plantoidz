//I2S STUFF

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