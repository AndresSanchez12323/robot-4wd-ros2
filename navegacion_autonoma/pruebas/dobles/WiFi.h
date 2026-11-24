#pragma once
enum wl_status_t {
  WL_IDLE_STATUS = 0, WL_NO_SSID_AVAIL, WL_SCAN_COMPLETED, WL_CONNECTED,
  WL_CONNECT_FAILED, WL_CONNECTION_LOST, WL_DISCONNECTED
};
#define WIFI_STA 1
#define WIFI_AP 2
class WiFiClass {
 public:
  void persistent(bool) {}
  void mode(int) {}
  void setAutoReconnect(bool) {}
  void setSleep(bool) {}
  void begin(const char *, const char * = nullptr) {}
  void disconnect() {}
  wl_status_t status() { return WL_CONNECTED; }
};
inline WiFiClass WiFi;
