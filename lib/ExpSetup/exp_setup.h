#ifndef EXP_SETUP_H
#define EXP_SETUP_H

#include <Arduino.h>
#include <string>
#include <vector>
#include <unordered_map>

void readConfigCMD();
// void read_number_of_setups();
// void load_sd_json(const char *filename, String &configData);
void M5_SD_JSON(const char *filename, String &configData);
void BMEsampleTask();
void ADSsampleTask();
void expMutexSetup();
int sensorCheck();
void relayRange(int start, int end, int state);
void relayCMD();
void purgeAll();
void purgeOff();
int purgeChannel(int channel);

#endif // EXP_SETUP_H
