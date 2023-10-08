#include <stdint.h>
#include "ps2.h"
#include "io.h"
#include "uuid.h"
#include "keyboard.h"
#include "api/computer.h"
#include "api/component.h"

static const uint8_t unshifted[] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, '\t', '`', 0,
    0, 0, 0, 0, 0, 'q', '1', 0,
    0, 0, 'z', 's', 'a', 'w', '2', 0,
    0, 'c', 'x', 'd', 'e', '4', '3', 0,
    0, ' ', 'v', 'f', 't', 'r', '5', 0,
    0, 'n', 'b', 'h', 'g', 'y', '6', 0,
    0, 0, 'm', 'j', 'u', '7', '8', 0,
    0, ',', 'k', 'i', 'o', '0', '9', 0,
    0, '.', '/', 'l', ';', 'p', '-', 0,
    0, 0, '\'', 0, '[', '=', 0, 0,
    0, 0, '\r', ']', 0, '\\', 0, 0,
    0, 0, 0, 0, 0, 0, '\b', 0,
    0, '1', 0, '4', '7', 0, 0, 0,
    '0', '.', '2', '5', '6', '8', 27, 0,
    0, '+', '3', '-', '*', '9', 0, 0
};

static const uint8_t shifted[] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, '\t', '~', 0,
    0, 0, 0, 0, 0, 'Q', '!', 0,
    0, 0, 'Z', 'S', 'A', 'W', '@', 0,
    0, 'C', 'X', 'D', 'E', '$', '#', 0,
    0, ' ', 'V', 'F', 'T', 'R', '%', 0,
    0, 'N', 'B', 'H', 'G', 'Y', '^', 0,
    0, 0, 'M', 'J', 'U', '&', '*', 0,
    0, '<', 'K', 'I', 'O', ')', '(', 0,
    0, '>', '?', 'L', ':', 'P', '_', 0,
    0, 0, '"', 0, '{', '+', 0, 0,
    0, 0, '\r', '}', 0, '|', 0, 0,
    0, 0, 0, 0, 0, 0, '\b', 0,
    0, '1', 0, '4', '7', 0, 0, 0,
    '0', '.', '2', '5', '6', '8', 27, 0,
    0, '+', '3', '-', '*', '9', 0, 0
};

static const uint8_t keycodes[] = {
    KEY_NONE,
    KEY_F9,
    KEY_NONE,
    KEY_F5,
    KEY_F3,
    KEY_F1,
    KEY_F2,
    KEY_F12,
    KEY_NONE,
    KEY_F10,
    KEY_F8,
    KEY_F6,
    KEY_F4,
    KEY_TAB,
    KEY_GRAVE,
    KEY_NONE,
    KEY_NONE,
    KEY_LMENU,
    KEY_LSHIFT,
    KEY_NONE,
    KEY_LCONTROL,
    KEY_Q,
    KEY_1,
    KEY_NONE,
    KEY_NONE,
    KEY_NONE,
    KEY_Z,
    KEY_S,
    KEY_A,
    KEY_W,
    KEY_2,
    KEY_NONE,
    KEY_NONE,
    KEY_C,
    KEY_X,
    KEY_D,
    KEY_E,
    KEY_4,
    KEY_3,
    KEY_NONE,
    KEY_NONE,
    KEY_SPACE,
    KEY_V,
    KEY_F,
    KEY_T,
    KEY_R,
    KEY_5,
    KEY_NONE,
    KEY_NONE,
    KEY_N,
    KEY_B,
    KEY_H,
    KEY_G,
    KEY_Y,
    KEY_6,
    KEY_NONE,
    KEY_NONE,
    KEY_NONE,
    KEY_M,
    KEY_J,
    KEY_U,
    KEY_7,
    KEY_8,
    KEY_NONE,
    KEY_NONE,
    KEY_COMMA,
    KEY_K,
    KEY_I,
    KEY_O,
    KEY_0,
    KEY_9,
    KEY_NONE,
    KEY_NONE,
    KEY_PERIOD,
    KEY_SLASH,
    KEY_L,
    KEY_SEMICOLON,
    KEY_P,
    KEY_MINUS,
    KEY_NONE,
    KEY_NONE,
    KEY_NONE,
    KEY_NONE,
    KEY_NONE,
    KEY_LBRACKET,
    KEY_EQUALS,
    KEY_NONE,
    KEY_NONE,
    KEY_CAPITAL,
    KEY_RSHIFT,
    KEY_RETURN,
    KEY_RBRACKET,
    KEY_NONE,
    KEY_BACKSLASH,
    KEY_NONE,
    KEY_NONE,
    KEY_NONE,
    KEY_NONE,
    KEY_NONE,
    KEY_NONE,
    KEY_NONE,
    KEY_NONE,
    KEY_BACK,
    KEY_NONE,
    KEY_NONE,
    KEY_1,
    KEY_NONE,
    KEY_4,
    KEY_7,
    KEY_NONE,
    KEY_NONE,
    KEY_NONE,
    KEY_0,
    KEY_DECIMAL,
    KEY_2,
    KEY_5,
    KEY_6,
    KEY_8,
    KEY_ESCAPE,
    KEY_NUMLOCK,
    KEY_F11,
    KEY_ADD,
    KEY_3,
    KEY_SUBTRACT,
    KEY_MULTIPLY,
    KEY_9,
    KEY_SCROLL,
    KEY_NONE,
    KEY_NONE,
    KEY_NONE,
    KEY_NONE,
    KEY_F7
};

static const uint16_t keycodes_sparse[][2] = {
    {0x111, KEY_RMENU},
    {0x114, KEY_RCONTROL},
    {0x11f, KEY_LMETA},
    {0x127, KEY_RMETA},
    {0x12f, KEY_APPS},
    {0x14a, KEY_DIVIDE},
    {0x15a, KEY_NUMPADENTER},
    {0x169, KEY_END},
    {0x16b, KEY_LEFT},
    {0x16c, KEY_HOME},
    {0x170, KEY_INSERT},
    {0x171, KEY_DELETE},
    {0x172, KEY_DOWN},
    {0x174, KEY_RIGHT},
    {0x175, KEY_UP},
    {0x17a, KEY_NEXT},
    {0x17d, KEY_PRIOR},
    {0, 0}
};

const char *keyboard_address = NULL;

void ps2_init(void) {
    keyboard_address = new_uuid();

    struct component *keyboard = new_component("keyboard", keyboard_address, NULL);
    add_component(keyboard);

    // read ps/2 controller config byte
    __asm__ __volatile__ ("cli");
    outb(0x64, 0x20);
    while (!(inb(0x64) & 1));
    uint8_t config = inb(0x60);
    __asm__ __volatile__ ("sti");

    config |= 1 << 0; // enable interrupt for 1st ps/2 port
    config &= ~(1 << 6); // disable translation for 1st ps/2 port

    // write ps/2 controller config byte
    __asm__ __volatile__ ("cli");
    outb(0x64, 0x60);
    while (inb(0x64) & 2);
    outb(0x60, config);
    __asm__ __volatile__ ("sti");
}

#define FLAG_EXT 1
#define FLAG_BREAK 2
#define FLAG_SHIFT 4
#define FLAG_ALT 8
#define FLAG_CTRL 16

void queue_key_signal(uint16_t raw_code, uint8_t flags) {
    int character = 0;
    if (!(flags & (FLAG_CTRL | FLAG_ALT)))
        character = (flags & FLAG_SHIFT) ? shifted[raw_code & 0xff] : unshifted[raw_code & 0xff];

    int code = 0;
    if (raw_code < sizeof(keycodes))
        code = keycodes[raw_code];
    else
        for (int i = 0; keycodes_sparse[i][0]; i++)
            if (keycodes_sparse[i][0] == raw_code) {
                code = keycodes_sparse[i][1];
                break;
            }

    if (character == 0 && code == 0)
        return;

    struct signal signal = {
        .name = (flags & FLAG_BREAK) ? "key_up" : "key_down",
        .kind = SIG_KEYBOARD,
        .data = {
            .keyboard = {
                .address = keyboard_address,
                .character = character,
                .code = code,
            }
        }
    };
    queue_signal(&signal);
}

void ps2_interrupt(void) {
    static uint8_t flags = 0;
    uint16_t received = inb(0x60);

    if (received == 0xf0)
        flags |= FLAG_BREAK;
    else if (received == 0xe0)
        flags |= FLAG_EXT;
    else {
        received |= (flags & FLAG_EXT) << 8;

        switch (received) {
            case 0x12:
            case 0x59:
                if (flags & FLAG_BREAK)
                    flags &= ~FLAG_SHIFT;
                else
                    flags |= FLAG_SHIFT;
                break;
            case 0x14:
            case 0x14 | (FLAG_EXT << 8):
                if (flags & FLAG_BREAK)
                    flags &= ~FLAG_CTRL;
                else
                    flags |= FLAG_CTRL;
                break;
            case 0x11:
            case 0x11 | (FLAG_EXT << 8):
                if (flags & FLAG_BREAK)
                    flags &= ~FLAG_ALT;
                else
                    flags |= FLAG_ALT;
                break;
        }

        if (keyboard_address != NULL)
            queue_key_signal(received, flags);

        flags &= ~(FLAG_EXT | FLAG_BREAK);
    }
}
