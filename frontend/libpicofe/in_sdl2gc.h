
#pragma once


int in_sdl2gc_init(const struct in_pdata *pdata, void (*handler)(void *event));

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0')

enum {
    SDL2GC_DPAD_UP = 0,
    SDL2GC_DPAD_DOWN,
    SDL2GC_DPAD_LEFT,
    SDL2GC_DPAD_RIGHT,
    SDL2GC_BTN_TRIANGLE,
    SDL2GC_BTN_CIRCLE,
    SDL2GC_BTN_SQUARE,
    SDL2GC_BTN_CROSS,
    SDL2GC_BTN_SELECT,
    SDL2GC_BTN_START,
    SDL2GC_BTN_L1,
    SDL2GC_BTN_L2,
    SDL2GC_BTN_R1,
    SDL2GC_BTN_R2,
    SDL2GC_BTN_PS, // for menu open if available
    SDL2GC_BTN_L3,
    SDL2GC_BTN_R3
};



