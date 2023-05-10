#ifndef _FLASH_H
#define _FLASH_H

#define FLASH_MAGIC_NUMBER  0xA5
#define FLASH_VERSION       1

// A single solve data (8 bytes)
struct s_solve {
	uint32_t time = 0;
    uint16_t turns = 0;
    uint16_t tps = 0;
};

// User data: best, av5 and av12 plus 12 last solves (15 * 8 = 120 bytes)
struct s_user {
    s_solve best;
    s_solve av5;
    s_solve av12;
    s_solve solve[12];
};

// Users (120 * 4 + 2 = 482 bytes)
struct s_settings {
    s_user user[4];
};

extern s_settings g_settings;

void flash_load();
bool flash_save();
void flash_setup();

#endif // _FLASH_H