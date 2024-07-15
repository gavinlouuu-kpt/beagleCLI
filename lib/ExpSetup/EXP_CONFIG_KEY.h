#ifndef EXP_CONFIG_KEY_H
#define EXP_CONFIG_KEY_H
#include <Arduino.h>

String EXP_NAME = "/exp_name";
String HEATER_SETTING = "/heater_settings";
String HEATING_TIME = "/heating_time(ms)";
String EXP_TIME = "/exp_time(ms)";
String EXPOSE_DURATION = "/duration(s)"; // expusre duration
String REPEAT = "/repeat";
String CHANNEL = "/channel";
const char *SETUP_NAME = "/expSetup.json";

#endif // EXP_CONFIG_KEY_H