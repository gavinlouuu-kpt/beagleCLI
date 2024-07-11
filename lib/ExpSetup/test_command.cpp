#include <exp_setup.h>
#include <zsrelay.h>
#include <pinConfig.h>
#include <Arduino.h>
#include <beagleCLI.h>

void sector_1_on()
{
    relayRange(0, 7, 1);
}

void sector_1_off()
{
    relayRange(0, 7, 0);
}

void sector_2_on()
{
    relayRange(8, 15, 1);
}

void sector_2_off()
{
    relayRange(8, 15, 0);
}

void sector_3_on()
{
    relayRange(16, 23, 1);
}

void sector_3_off()
{
    relayRange(16, 23, 0);
}

void sector_4_on()
{
    relayRange(24, 31, 1);
}

void sector_4_off()
{
    relayRange(24, 31, 0);
}

void sector_5_on()
{
    relayRange(32, 39, 1);
}

void sector_5_off()
{
    relayRange(32, 39, 0);
}

void s1pOn()
{
    std::vector<uint8_t> pump_on = switchCommand(1, clean_1r, 1);
    Serial2.write(pump_on.data(), pump_on.size());
}

void s1pOff()
{
    std::vector<uint8_t> pump_on = switchCommand(1, clean_1r, 0);
    Serial2.write(pump_on.data(), pump_on.size());
}

void s2pOn()
{
    std::vector<uint8_t> pump_on = switchCommand(1, clean_2r, 1);
    Serial2.write(pump_on.data(), pump_on.size());
}

void s2pOff()
{
    std::vector<uint8_t> pump_on = switchCommand(1, clean_2r, 0);
    Serial2.write(pump_on.data(), pump_on.size());
}

void s3pOn()
{
    std::vector<uint8_t> pump_on = switchCommand(1, clean_3r, 1);
    Serial2.write(pump_on.data(), pump_on.size());
}

void s3pOff()
{
    std::vector<uint8_t> pump_on = switchCommand(1, clean_3r, 0);
    Serial2.write(pump_on.data(), pump_on.size());
}

void s4pOn()
{
    std::vector<uint8_t> pump_on = switchCommand(1, clean_4r, 1);
    Serial2.write(pump_on.data(), pump_on.size());
}

void s4pOff()
{
    std::vector<uint8_t> pump_on = switchCommand(1, clean_4r, 0);
    Serial2.write(pump_on.data(), pump_on.size());
}

void s5pOn()
{
    std::vector<uint8_t> pump_on = switchCommand(1, clean_5r, 1);
    Serial2.write(pump_on.data(), pump_on.size());
}

void s5pOff()
{
    std::vector<uint8_t> pump_on = switchCommand(1, clean_5r, 0);
    Serial2.write(pump_on.data(), pump_on.size());
}

void test_CMD()
{
    commandMap["sector1On"] = []()
    { sector_1_on(); };
    commandMap["sector1Off"] = []()
    { sector_1_off(); };
    commandMap["sector2On"] = []()
    { sector_2_on(); };
    commandMap["sector2Off"] = []()
    { sector_2_off(); };
    commandMap["sector3On"] = []()
    { sector_3_on(); };
    commandMap["sector3Off"] = []()
    { sector_3_off(); };
    commandMap["sector4On"] = []()
    { sector_4_on(); };
    commandMap["sector4Off"] = []()
    { sector_4_off(); };
    commandMap["sector5On"] = []()
    { sector_5_on(); };
    commandMap["sector5Off"] = []()
    { sector_5_off(); };
    commandMap["s1pOn"] = []()
    { s1pOn(); };
    commandMap["s1pOff"] = []()
    { s1pOff(); };
    commandMap["s2pOn"] = []()
    { s2pOn(); };
    commandMap["s2pOff"] = []()
    { s2pOff(); };
    commandMap["s3pOn"] = []()
    { s3pOn(); };
    commandMap["s3pOff"] = []()
    { s3pOff(); };
    commandMap["s4pOn"] = []()
    { s4pOn(); };
    commandMap["s4pOff"] = []()
    { s4pOff(); };
    commandMap["s5pOn"] = []()
    { s5pOn(); };
    commandMap["s5pOff"] = []()
    { s5pOff(); };
}