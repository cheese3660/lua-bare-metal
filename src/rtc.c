#include <stdbool.h>
#include <time.h>
#include "rtc.h"
#include "io.h"

static bool is_24h = false;
static bool is_bcd = false;
volatile uint16_t jiffies_frac = 0;
volatile uint64_t epoch_time = 0;
volatile uint64_t uptime = 0;

void rtc_init(void) {
    outb(0x70, 0x0c);
    inb(0x71);

    uint8_t status_b = rtc_read(0x0b);
    is_24h = !(status_b >> 1);
    is_bcd = !(status_b >> 2);

    get_time();

    /* https://wiki.osdev.org/RTC */
    __asm__ __volatile__ ("cli");
    outb(0x70, 0x8b);           // select register B, and disable NMI
    char prev = inb(0x71);      // read the current value of register B
    outb(0x70, 0x8b);           // set the index again (a read will reset the index to register D)
    outb(0x71, prev | 0x40);    // write the previous value ORed with 0x40. This turns on bit 6 of register B
    __asm__ __volatile__ ("sti");
}

uint8_t rtc_read(uint8_t index) {
    __asm__ __volatile__ ("cli");
    outb(0x70, index);
    for (int i = 0; i < 1048576; i++);
    uint8_t value = inb(0x71);
    __asm__ __volatile__ ("sti");
    return value;
}

void rtc_write(uint8_t index, uint8_t value) {
    __asm__ __volatile__ ("cli");
    outb(0x70, index);
    for (int i = 0; i < 1048576; i++);
    outb(0x71, value);
    __asm__ __volatile__ ("sti");
}

uint8_t convert_bcd(uint8_t value) {
    if (is_bcd)
        return ((value & 0xf0) >> 1) + ((value & 0xf0) >> 3) + (value & 0xf);
    else
        return value;
}

static uint16_t days_until_month[] = {
    0, // january
    31, // february
    59, // march
    90, // april
    120, // may
    151, // june
    181, // july
    212, // august
    243, // september
    273, // october
    304, // november
    334, // december
};

time_t mktime(struct tm *date) {
    time_t time = -2208988800; // 1900/01/01

    time += date->tm_sec;
    time += (time_t) date->tm_min * 60;
    time += (time_t) date->tm_hour * 60 * 60;

    time += (time_t) date->tm_year * 60 * 60 * 24 * 365; // year
    time += ((time_t) date->tm_year / 4 + 1) * 60 * 60 * 24; // account for leap years
    time -= (((time_t) date->tm_year + 300) / 400) * 60 * 60 * 24;

    time += (time_t) days_until_month[date->tm_mon] * 60 * 60 * 24;
    if (date->tm_year % 4 == 0 && date->tm_mon > 2)
        time += 60 * 60 * 24; // add leap day

    time += (time_t) (date->tm_mday - 1) * 60 * 60 * 24; // day

    if (date->tm_isdst)
        time += 60 * 60; // add an additional hour if DST is specified

    return time;
}

static uint64_t get_time_unchecked(void) {
    struct tm date = {
        .tm_sec = convert_bcd(rtc_read(0x00)),
        .tm_min = convert_bcd(rtc_read(0x02)),
        .tm_mday = convert_bcd(rtc_read(0x07)),
        .tm_mon = convert_bcd(rtc_read(0x08)) - 1,
        .tm_year = convert_bcd(rtc_read(0x09)) + 100,
    };

    if (is_24h)
        date.tm_hour = convert_bcd(rtc_read(0x04));
    else {
        uint8_t hours = rtc_read(0x04);

        if (hours & 0x80) // PM
            date.tm_hour = (convert_bcd(hours & ~0x80) % 12) + 12;
        else // AM
            date.tm_hour = convert_bcd(hours) % 12;
    }

    return epoch_time = mktime(&date);
}

uint64_t get_time(void) {
    while (1) {
        while (rtc_read(0x0b) & (1 << 7));
        uint64_t time_a = get_time_unchecked();
        while (rtc_read(0x0b) & (1 << 7));
        uint64_t time_b = get_time_unchecked();

        if (time_a == time_b)
            return epoch_time = time_a;
    }
}

void timer_tick(void) {
    jiffies_frac++;

    if (jiffies_frac > 1024) {
        jiffies_frac = 0;
        epoch_time++;
        uptime++;
    }

    outb(0x70, 0x0c);
    inb(0x71);
}
