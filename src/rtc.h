#pragma once

#include <stdint.h>

extern uint16_t jiffies_frac;
extern uint64_t epoch_time;
extern uint64_t uptime;

void rtc_init(void);
uint8_t rtc_read(uint8_t index);
void rtc_write(uint8_t index, uint8_t value);
uint64_t get_time(void);
void timer_tick(void);
