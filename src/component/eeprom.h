#pragma once

struct eeprom_data {
    const char *contents;
    const char *data;
};

struct eeprom_data *eeprom_init(void);
