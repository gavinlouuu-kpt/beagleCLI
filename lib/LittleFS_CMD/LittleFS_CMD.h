#ifndef LITTLEFS_CMD_H
#define LITTLEFS_CMD_H

#include <Arduino.h>
#include "FS.h"
#include <LittleFS.h>
#include <beagleCLI.h>

extern std::string cwd;
void LittleFS_CMD();

#endif // LITTLEFS_CMD_H