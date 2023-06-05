#ifndef _FLASH_H
#define _FLASH_H

#define FLASH_MAGIC_NUMBER  0xA5
#define FLASH_VERSION       2

// A single solve data (8 bytes)
struct s_solve {
	uint32_t time = 0;
    uint16_t turns = 0;
    uint16_t tps = 0;
};

// User data: best, ao5 and ao12 plus 12 last solves (15 * 8 = 120 bytes)
struct s_user {
    s_solve best;
    s_solve ao5;
    s_solve ao12;
    s_solve solve[12];
};

// Users (120 * 4 = 480 bytes)
struct s_puzzle {
    s_user user[4];
};

// Puzzles (480 * 1 = 480 bytes)
struct s_settings {
    s_puzzle puzzle[4];
};

extern s_settings g_settings;

void flash_load();
bool flash_save();
void flash_setup();

#endif // _FLASH_H