#pragma once
typedef int ota_error_t;
class ArduinoOTAClass {
 public:
  void setHostname(const char *) {}
  void setPassword(const char *) {}
  template <typename F> void onStart(F) {}
  void begin() {}
  void handle() {}
};
inline ArduinoOTAClass ArduinoOTA;
