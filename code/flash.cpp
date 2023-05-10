#include <Arduino.h>
#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>

#include "config.h"
#include "utils.h"
#include "flash.h"

using namespace Adafruit_LittleFS_Namespace;

const char _settings_file_name[] = "rakcube.bin";
File _settings_file(InternalFS);

void flash_dump() {

    #if DEBUG>1
    
        Serial.println("[FLASH] Settings dump");
        Serial.printf ("[FLASH] Version: %d\n", g_settings.version);

        char buffer[30] = {0};

        for (uint8_t user=0; user<4; user++) {
            
            // Header
            Serial.println("[FLASH] ---------------------------------");
            Serial.printf ("[FLASH] User %d\n", user+1);
            Serial.println("[FLASH] ---------------------------------");

            // Best
            utils_time_to_text(g_settings.user[user].best.time, buffer);
            Serial.printf ("[FLASH] BEST    %s            %.2f\n", buffer, g_settings.user[user].best.tps / 100.0);

            // AV5
            utils_time_to_text(g_settings.user[user].av5.time, buffer);
            Serial.printf ("[FLASH]  AV5    %s    %4d    %.2f\n", buffer, g_settings.user[user].av5.turns, g_settings.user[user].av5.tps / 100.0);

            // AV12
            utils_time_to_text(g_settings.user[user].av12.time, buffer);
            Serial.printf ("[FLASH] AV12    %s    %4d    %.2f\n", buffer, g_settings.user[user].av12.turns, g_settings.user[user].av12.tps / 100.0);

            // Last 12
            for (uint8_t i=0; i<12; i++) {
                utils_time_to_text(g_settings.user[user].solve[i].time, buffer);
                Serial.printf ("[FLASH] %4d    %s    %4d    %.2f\n", i+1, buffer, g_settings.user[user].solve[i].turns, g_settings.user[user].solve[i].tps / 100.0);
            }


        }
    
        Serial.println("[FLASH] ---------------------------------");
    
    #endif

}

void flash_reset() {
    InternalFS.format();
    if (_settings_file.open(_settings_file_name, FILE_O_WRITE))	{
		s_settings default_settings;
		_settings_file.write((uint8_t *)&default_settings, sizeof(s_settings));
		_settings_file.flush();
		_settings_file.close();
	}
}

void flash_load() {
    
    //flash_reset();

    // Open file
    _settings_file.open(_settings_file_name, FILE_O_READ);
    if (!_settings_file) {
        #if DEBUG > 0
            Serial.println("[FLASH] Settings file does not exist, force format");
        #endif
        flash_reset();
        _settings_file.open(_settings_file_name, FILE_O_READ);
    }

    // Check version
    uint8_t markers[2] = {0};
    _settings_file.read(markers, 2);
    if ((markers[0] != FLASH_MAGIC_NUMBER) || (markers[1] != FLASH_VERSION)) {
        #if DEBUG > 0
            Serial.println("[FLASH] Wrong file format or version, reformatting");
        #endif
        flash_reset();
        _settings_file.open(_settings_file_name, FILE_O_READ);
    }

    // Read data
    _settings_file.read((uint8_t *) &g_settings, sizeof(s_settings));
    _settings_file.close();
    #if DEBUG > 0
        Serial.println("[FLASH] Data loaded");
    #endif

    flash_dump();

}

bool flash_save() {

    // Open file
    _settings_file.open(_settings_file_name, FILE_O_READ);
    if (!_settings_file) {
        #if DEBUG > 0
            Serial.println("[FLASH] Settings file does not exist, force format");
        #endif
        flash_reset();
        _settings_file.open(_settings_file_name, FILE_O_READ);
    }

    // Compare contents
	s_settings flash_content;
    _settings_file.read((uint8_t *)&flash_content, sizeof(s_settings));
	_settings_file.close();    

    if (memcmp((void *)&flash_content, (void *)&g_settings, sizeof(s_settings)) != 0) {
		
        #if DEBUG > 0
            Serial.println("[FLASH] Flash content changed, writing new data");
		    delay(100);
        #endif

		InternalFS.remove(_settings_file_name);

		if (!_settings_file.open(_settings_file_name, FILE_O_WRITE)) {
            return false;
        }

        _settings_file.write((uint8_t *)&g_settings, sizeof(s_settings));
        _settings_file.flush();
        _settings_file.close();

	}

    flash_dump();
    return true;

}

void flash_setup() {
    InternalFS.begin();
}