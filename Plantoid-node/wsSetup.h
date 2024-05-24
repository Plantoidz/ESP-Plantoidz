// WS SETUP
using namespace websockets;
WebsocketsClient client_mic;
WebsocketsClient client_amp;
bool isWebSocketConnected_mic;
bool isWebSocketConnected_amp;
TaskHandle_t i2smicTask;