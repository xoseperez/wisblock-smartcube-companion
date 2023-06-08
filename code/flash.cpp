#include <Arduino.h>
#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>

#include "config.h"
#include "utils.h"
#include "flash.h"

//#define DO_NOT_SAVE

using namespace Adafruit_LittleFS_Namespace;
const char _settings_file_name[] = "rakcube.bin";
File _settings_file(InternalFS);

void flash_dump() {

    #if DEBUG > 1
    
        Serial.println("[FLASH] Settings dump");

        char buffer[30] = {0};

        for (uint8_t puzzle=0; puzzle<6; puzzle++) {

            for (uint8_t user=0; user<4; user++) {
                
                // Header
                Serial.println("[FLASH] ----------------------------------");
                Serial.printf ("[FLASH] User %d - Puzzle %d\n", user+1, puzzle);
                Serial.println("[FLASH] ----------------------------------");

                // Best
                utils_time_to_text(g_settings.puzzle[puzzle].user[user].best.time, buffer, false);
                Serial.printf ("[FLASH] BEST    %s            %5.2f\n", buffer, g_settings.puzzle[puzzle].user[user].best.tps / 100.0);

                // Ao5
                utils_time_to_text(g_settings.puzzle[puzzle].user[user].ao5.time, buffer, false);
                Serial.printf ("[FLASH]  Ao5    %s    %4d    %5.2f\n", buffer, g_settings.puzzle[puzzle].user[user].ao5.turns, g_settings.puzzle[puzzle].user[user].ao5.tps / 100.0);

                // Ao12
                utils_time_to_text(g_settings.puzzle[puzzle].user[user].ao12.time, buffer, false);
                Serial.printf ("[FLASH] Ao12    %s    %4d    %5.2f\n", buffer, g_settings.puzzle[puzzle].user[user].ao12.turns, g_settings.puzzle[puzzle].user[user].ao12.tps / 100.0);

                // Last 12
                for (uint8_t i=0; i<12; i++) {
                    utils_time_to_text(g_settings.puzzle[puzzle].user[user].solve[i].time, buffer, false);
                    Serial.printf ("[FLASH] %4d    %s    %4d    %5.2f\n", i+1, buffer, g_settings.puzzle[puzzle].user[user].solve[i].turns, g_settings.puzzle[puzzle].user[user].solve[i].tps / 100.0);
                }


            }
        
        }
    
        Serial.println("[FLASH] ----------------------------------");
    
    #endif

}

void flash_reset() {
    
    #ifndef DO_NOT_SAVE
    
        InternalFS.format();
        if (_settings_file.open(_settings_file_name, FILE_O_WRITE))	{
            s_settings default_settings;
            _settings_file.write((uint8_t *)&default_settings, sizeof(s_settings));
            _settings_file.flush();
            _settings_file.close();
        }
    
    #endif

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
    if (markers[0] != FLASH_MAGIC_NUMBER) {
        #if DEBUG > 0
            Serial.println("[FLASH] Wrong file format, reformatting");
        #endif
        flash_reset();
        _settings_file.open(_settings_file_name, FILE_O_READ);
        _settings_file.read(markers, 2);
    }

    // Read data
    _settings_file.read((uint8_t *) &g_settings, sizeof(s_settings));
    _settings_file.close();
    #if DEBUG > 0
        Serial.println("[FLASH] Data loaded");
    #endif

    // Migrate from v1 to v2
    // v2 introduces 3 more puzzles beyond 3x3x3
    // so it requires to reset the values for all puzzles except the first one
    if (markers[1] == 1) {
        #if DEBUG > 0
            Serial.println("[FLASH] Updating from version 1 to 2");
        #endif
        for (uint8_t i=1; i<6; i++) {
            memset((void *)&g_settings.puzzle[i], 0, sizeof(s_puzzle));
        }
    }

    //flash_dump();

}

bool flash_save() {

    uint8_t markers[2] = {0};

    // Open file
    _settings_file.open(_settings_file_name, FILE_O_READ);
    if (!_settings_file) {
        #if DEBUG > 0
            Serial.println("[FLASH] Settings file does not exist, force format");
        #endif
        flash_reset();
        _settings_file.open(_settings_file_name, FILE_O_READ);
    }

    // Jump over markers
    _settings_file.read(markers, 2);

    // Compare contents
	s_settings flash_content;
    _settings_file.read((uint8_t *)&flash_content, sizeof(s_settings));
	_settings_file.close();    

    if (memcmp((void *)&flash_content, (void *)&g_settings, sizeof(s_settings)) != 0) {
		
        #if DEBUG > 0
            Serial.println("[FLASH] Flash content changed, writing new data");
		    delay(100);
        #endif

        #ifndef DO_NOT_SAVE

            InternalFS.remove(_settings_file_name);

            if (!_settings_file.open(_settings_file_name, FILE_O_WRITE)) {
                return false;
            }

            uint8_t markers[2] = { FLASH_MAGIC_NUMBER, FLASH_VERSION};
            _settings_file.write(markers, 2);
            _settings_file.write((uint8_t *)&g_settings, sizeof(s_settings));
            _settings_file.flush();
            _settings_file.close();
        
        #endif

	}

    flash_dump();
    return true;

}

void flash_setup() {
    InternalFS.begin();
}