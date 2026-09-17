/*
 * (C) Gražvydas "notaz" Ignotas, 2012
 * (C) Artur "screemer" Jakubowicz, 2020
 *
 * This work is licensed under the terms of any of these licenses
 * (at your option):
 *  - GNU GPL, version 2 or later.
 *  - GNU LGPL, version 2.1 or later.
 *  - MAME license.
 *
 *  NOTE: Removed support for joysticks - GameController API implemented in in_sdl2gc.c
 * See the COPYING file in the top-level directory.
 */

#include <stdio.h>
#include <SDL2/SDL.h>


#include "input.h"
#include "in_sdl.h"



#include "keysym.h"

typedef int bool;
#define true 1
#define false 0
//#define CONFIG_DEBUG


#define IN_SDL_PREFIX "sdl:"
/* should be machine word for best performace */
typedef unsigned long keybits_t;
#define KEYBITS_WORD_BITS (sizeof(keybits_t) * 8)

struct in_sdl_state {
    const in_drv_t *drv;
    keybits_t keystate[SDLK_LAST / KEYBITS_WORD_BITS + 1];
};

static void (*ext_event_handler)(void *event);

SDLK_to_MY_SDLK SDLK_to_My_SDLK_ARRAY[] = {
        {SDLK_UNKNOWN,            MY_SDLK_UNKNOWN},

        {SDLK_RETURN,             MY_SDLK_RETURN},
        {SDLK_ESCAPE,             MY_SDLK_ESCAPE},
        {SDLK_BACKSPACE,          MY_SDLK_BACKSPACE},
        {SDLK_TAB,                MY_SDLK_TAB},
        {SDLK_SPACE,              MY_SDLK_SPACE},
        {SDLK_EXCLAIM,            MY_SDLK_EXCLAIM},
        {SDLK_QUOTEDBL,           MY_SDLK_QUOTEDBL},
        {SDLK_HASH,               MY_SDLK_HASH},
        {SDLK_PERCENT,            MY_SDLK_PERCENT},
        {SDLK_DOLLAR,             MY_SDLK_DOLLAR},
        {SDLK_AMPERSAND,          MY_SDLK_AMPERSAND},
        {SDLK_QUOTE,              MY_SDLK_QUOTE},
        {SDLK_LEFTPAREN,          MY_SDLK_LEFTPAREN},
        {SDLK_RIGHTPAREN,         MY_SDLK_RIGHTPAREN},
        {SDLK_ASTERISK,           MY_SDLK_ASTERISK},
        {SDLK_PLUS,               MY_SDLK_PLUS},
        {SDLK_COMMA,              MY_SDLK_COMMA},
        {SDLK_MINUS,              MY_SDLK_MINUS},
        {SDLK_PERIOD,             MY_SDLK_PERIOD},
        {SDLK_SLASH,              MY_SDLK_SLASH},
        {SDLK_0,                  MY_SDLK_0},
        {SDLK_1,                  MY_SDLK_1},
        {SDLK_2,                  MY_SDLK_2},
        {SDLK_3,                  MY_SDLK_3},
        {SDLK_4,                  MY_SDLK_4},
        {SDLK_5,                  MY_SDLK_5},
        {SDLK_6,                  MY_SDLK_6},
        {SDLK_7,                  MY_SDLK_7},
        {SDLK_8,                  MY_SDLK_8},
        {SDLK_9,                  MY_SDLK_9},
        {SDLK_COLON,              MY_SDLK_COLON},
        {SDLK_SEMICOLON,          MY_SDLK_SEMICOLON},
        {SDLK_LESS,               MY_SDLK_LESS},
        {SDLK_EQUALS,             MY_SDLK_EQUALS},
        {SDLK_GREATER,            MY_SDLK_GREATER},
        {SDLK_QUESTION,           MY_SDLK_QUESTION},
        {SDLK_AT,                 MY_SDLK_AT},
        /*
          Skip uppercase letters
        */
        {SDLK_LEFTBRACKET,        MY_SDLK_LEFTBRACKET},
        {SDLK_BACKSLASH,          MY_SDLK_BACKSLASH},
        {SDLK_RIGHTBRACKET,       MY_SDLK_RIGHTBRACKET},
        {SDLK_CARET,              MY_SDLK_CARET},
        {SDLK_UNDERSCORE,         MY_SDLK_UNDERSCORE},
        {SDLK_BACKQUOTE,          MY_SDLK_BACKQUOTE},
        {SDLK_a,                  MY_SDLK_a},
        {SDLK_b,                  MY_SDLK_b},
        {SDLK_c,                  MY_SDLK_c},
        {SDLK_d,                  MY_SDLK_d},
        {SDLK_e,                  MY_SDLK_e},
        {SDLK_f,                  MY_SDLK_f},
        {SDLK_g,                  MY_SDLK_g},
        {SDLK_h,                  MY_SDLK_h},
        {SDLK_i,                  MY_SDLK_i},
        {SDLK_j,                  MY_SDLK_j},
        {SDLK_k,                  MY_SDLK_k},
        {SDLK_l,                  MY_SDLK_l},
        {SDLK_m,                  MY_SDLK_m},
        {SDLK_n,                  MY_SDLK_n},
        {SDLK_o,                  MY_SDLK_o},
        {SDLK_p,                  MY_SDLK_p},
        {SDLK_q,                  MY_SDLK_q},
        {SDLK_r,                  MY_SDLK_r},
        {SDLK_s,                  MY_SDLK_s},
        {SDLK_t,                  MY_SDLK_t},
        {SDLK_u,                  MY_SDLK_u},
        {SDLK_v,                  MY_SDLK_v},
        {SDLK_w,                  MY_SDLK_w},
        {SDLK_x,                  MY_SDLK_x},
        {SDLK_y,                  MY_SDLK_y},
        {SDLK_z,                  MY_SDLK_z},

        {SDLK_CAPSLOCK,           MY_SDLK_CAPSLOCK},

        {SDLK_F1,                 MY_SDLK_F1},
        {SDLK_F2,                 MY_SDLK_F2},
        {SDLK_F3,                 MY_SDLK_F3},
        {SDLK_F4,                 MY_SDLK_F4},
        {SDLK_F5,                 MY_SDLK_F5},
        {SDLK_F6,                 MY_SDLK_F6},
        {SDLK_F7,                 MY_SDLK_F7},
        {SDLK_F8,                 MY_SDLK_F8},
        {SDLK_F9,                 MY_SDLK_F9},
        {SDLK_F10,                MY_SDLK_F10},
        {SDLK_F11,                MY_SDLK_F11},
        {SDLK_F12,                MY_SDLK_F12},

        {SDLK_PRINTSCREEN,        MY_SDLK_PRINTSCREEN},
        {SDLK_SCROLLLOCK,         MY_SDLK_SCROLLLOCK},
        {SDLK_PAUSE,              MY_SDLK_PAUSE},
        {SDLK_INSERT,             MY_SDLK_INSERT},
        {SDLK_HOME,               MY_SDLK_HOME},
        {SDLK_PAGEUP,             MY_SDLK_PAGEUP},
        {SDLK_DELETE,             MY_SDLK_DELETE},
        {SDLK_END,                MY_SDLK_END},
        {SDLK_PAGEDOWN,           MY_SDLK_PAGEDOWN},
        {SDLK_RIGHT,              MY_SDLK_RIGHT},
        {SDLK_LEFT,               MY_SDLK_LEFT},
        {SDLK_DOWN,               MY_SDLK_DOWN},
        {SDLK_UP,                 MY_SDLK_UP},

        {SDLK_NUMLOCKCLEAR,       MY_SDLK_NUMLOCKCLEAR},
        {SDLK_KP_DIVIDE,          MY_SDLK_KP_DIVIDE},
        {SDLK_KP_MULTIPLY,        MY_SDLK_KP_MULTIPLY},
        {SDLK_KP_MINUS,           MY_SDLK_KP_MINUS},
        {SDLK_KP_PLUS,            MY_SDLK_KP_PLUS},
        {SDLK_KP_ENTER,           MY_SDLK_KP_ENTER},
        {SDLK_KP_1,               MY_SDLK_KP_1},
        {SDLK_KP_2,               MY_SDLK_KP_2},
        {SDLK_KP_3,               MY_SDLK_KP_3},
        {SDLK_KP_4,               MY_SDLK_KP_4},
        {SDLK_KP_5,               MY_SDLK_KP_5},
        {SDLK_KP_6,               MY_SDLK_KP_6},
        {SDLK_KP_7,               MY_SDLK_KP_7},
        {SDLK_KP_8,               MY_SDLK_KP_8},
        {SDLK_KP_9,               MY_SDLK_KP_9},
        {SDLK_KP_0,               MY_SDLK_KP_0},
        {SDLK_KP_PERIOD,          MY_SDLK_KP_PERIOD},

        {SDLK_APPLICATION,        MY_SDLK_APPLICATION},
        {SDLK_POWER,              MY_SDLK_POWER},
        {SDLK_KP_EQUALS,          MY_SDLK_KP_EQUALS},
        {SDLK_F13,                MY_SDLK_F13},
        {SDLK_F14,                MY_SDLK_F14},
        {SDLK_F15,                MY_SDLK_F15},
        {SDLK_F16,                MY_SDLK_F16},
        {SDLK_F17,                MY_SDLK_F17},
        {SDLK_F18,                MY_SDLK_F18},
        {SDLK_F19,                MY_SDLK_F19},
        {SDLK_F20,                MY_SDLK_F20},
        {SDLK_F21,                MY_SDLK_F21},
        {SDLK_F22,                MY_SDLK_F22},
        {SDLK_F23,                MY_SDLK_F23},
        {SDLK_F24,                MY_SDLK_F24},
        {SDLK_EXECUTE,            MY_SDLK_EXECUTE},
        {SDLK_HELP,               MY_SDLK_HELP},
        {SDLK_MENU,               MY_SDLK_MENU},
        {SDLK_SELECT,             MY_SDLK_SELECT},
        {SDLK_STOP,               MY_SDLK_STOP},
        {SDLK_AGAIN,              MY_SDLK_AGAIN},
        {SDLK_UNDO,               MY_SDLK_UNDO},
        {SDLK_CUT,                MY_SDLK_CUT},
        {SDLK_COPY,               MY_SDLK_COPY},
        {SDLK_PASTE,              MY_SDLK_PASTE},
        {SDLK_FIND,               MY_SDLK_FIND},
        {SDLK_MUTE,               MY_SDLK_MUTE},
        {SDLK_VOLUMEUP,           MY_SDLK_VOLUMEUP},
        {SDLK_VOLUMEDOWN,         MY_SDLK_VOLUMEDOWN},
        {SDLK_KP_COMMA,           MY_SDLK_KP_COMMA},
        {SDLK_KP_EQUALSAS400,     MY_SDLK_KP_EQUALSAS400},

        {SDLK_ALTERASE,           MY_SDLK_ALTERASE},
        {SDLK_SYSREQ,             MY_SDLK_SYSREQ},
        {SDLK_CANCEL,             MY_SDLK_CANCEL},
        {SDLK_CLEAR,              MY_SDLK_CLEAR},
        {SDLK_PRIOR,              MY_SDLK_PRIOR},
        {SDLK_RETURN2,            MY_SDLK_RETURN2},
        {SDLK_SEPARATOR,          MY_SDLK_SEPARATOR},
        {SDLK_OUT,                MY_SDLK_OUT},
        {SDLK_OPER,               MY_SDLK_OPER},
        {SDLK_CLEARAGAIN,         MY_SDLK_CLEARAGAIN},
        {SDLK_CRSEL,              MY_SDLK_CRSEL},
        {SDLK_EXSEL,              MY_SDLK_EXSEL},

        {SDLK_KP_00,              MY_SDLK_KP_00},
        {SDLK_KP_000,             MY_SDLK_KP_000},
        {SDLK_THOUSANDSSEPARATOR, MY_SDLK_THOUSANDSSEPARATOR},
        {SDLK_DECIMALSEPARATOR,   MY_SDLK_DECIMALSEPARATOR},
        {SDLK_CURRENCYUNIT,       MY_SDLK_CURRENCYUNIT},
        {SDLK_CURRENCYSUBUNIT,    MY_SDLK_CURRENCYSUBUNIT},
        {SDLK_KP_LEFTPAREN,       MY_SDLK_KP_LEFTPAREN},
        {SDLK_KP_RIGHTPAREN,      MY_SDLK_KP_RIGHTPAREN},
        {SDLK_KP_LEFTBRACE,       MY_SDLK_KP_LEFTBRACE},
        {SDLK_KP_RIGHTBRACE,      MY_SDLK_KP_RIGHTBRACE},
        {SDLK_KP_TAB,             MY_SDLK_KP_TAB},
        {SDLK_KP_BACKSPACE,       MY_SDLK_KP_BACKSPACE},
        {SDLK_KP_A,               MY_SDLK_KP_A},
        {SDLK_KP_B,               MY_SDLK_KP_B},
        {SDLK_KP_C,               MY_SDLK_KP_C},
        {SDLK_KP_D,               MY_SDLK_KP_D},
        {SDLK_KP_E,               MY_SDLK_KP_E},
        {SDLK_KP_F,               MY_SDLK_KP_F},
        {SDLK_KP_XOR,             MY_SDLK_KP_XOR},
        {SDLK_KP_POWER,           MY_SDLK_KP_POWER},
        {SDLK_KP_PERCENT,         MY_SDLK_KP_PERCENT},
        {SDLK_KP_LESS,            MY_SDLK_KP_LESS},
        {SDLK_KP_GREATER,         MY_SDLK_KP_GREATER},
        {SDLK_KP_AMPERSAND,       MY_SDLK_KP_AMPERSAND},
        {SDLK_KP_DBLAMPERSAND,    MY_SDLK_KP_DBLAMPERSAND},
        {SDLK_KP_VERTICALBAR,     MY_SDLK_KP_VERTICALBAR},
        {SDLK_KP_DBLVERTICALBAR,  MY_SDLK_KP_DBLVERTICALBAR},
        {SDLK_KP_COLON,           MY_SDLK_KP_COLON},
        {SDLK_KP_HASH,            MY_SDLK_KP_HASH},
        {SDLK_KP_SPACE,           MY_SDLK_KP_SPACE},
        {SDLK_KP_AT,              MY_SDLK_KP_AT},
        {SDLK_KP_EXCLAM,          MY_SDLK_KP_EXCLAM},
        {SDLK_KP_MEMSTORE,        MY_SDLK_KP_MEMSTORE},
        {SDLK_KP_MEMRECALL,       MY_SDLK_KP_MEMRECALL},
        {SDLK_KP_MEMCLEAR,        MY_SDLK_KP_MEMCLEAR},
        {SDLK_KP_MEMADD,          MY_SDLK_KP_MEMADD},
        {SDLK_KP_MEMSUBTRACT,     MY_SDLK_KP_MEMSUBTRACT},
        {SDLK_KP_MEMMULTIPLY,     MY_SDLK_KP_MEMMULTIPLY},
        {SDLK_KP_MEMDIVIDE,       MY_SDLK_KP_MEMDIVIDE},
        {SDLK_KP_PLUSMINUS,       MY_SDLK_KP_PLUSMINUS},
        {SDLK_KP_CLEAR,           MY_SDLK_KP_CLEAR},
        {SDLK_KP_CLEARENTRY,      MY_SDLK_KP_CLEARENTRY},
        {SDLK_KP_BINARY,          MY_SDLK_KP_BINARY},
        {SDLK_KP_OCTAL,           MY_SDLK_KP_OCTAL},
        {SDLK_KP_DECIMAL,         MY_SDLK_KP_DECIMAL},
        {SDLK_KP_HEXADECIMAL,     MY_SDLK_KP_HEXADECIMAL},

        {SDLK_LCTRL,              MY_SDLK_LCTRL},
        {SDLK_LSHIFT,             MY_SDLK_LSHIFT},
        {SDLK_LALT,               MY_SDLK_LALT},
        {SDLK_LGUI,               MY_SDLK_LGUI},
        {SDLK_RCTRL,              MY_SDLK_RCTRL},
        {SDLK_RSHIFT,             MY_SDLK_RSHIFT},
        {SDLK_RALT,               MY_SDLK_RALT},
        {SDLK_RGUI,               MY_SDLK_RGUI},

        {SDLK_MODE,               MY_SDLK_MODE},

        {SDLK_AUDIONEXT,          MY_SDLK_AUDIONEXT},
        {SDLK_AUDIOPREV,          MY_SDLK_AUDIOPREV},
        {SDLK_AUDIOSTOP,          MY_SDLK_AUDIOSTOP},
        {SDLK_AUDIOPLAY,          MY_SDLK_AUDIOPLAY},
        {SDLK_AUDIOMUTE,          MY_SDLK_AUDIOMUTE},
        {SDLK_MEDIASELECT,        MY_SDLK_MEDIASELECT},
        {SDLK_WWW,                MY_SDLK_WWW},
        {SDLK_MAIL,               MY_SDLK_MAIL},
        {SDLK_CALCULATOR,         MY_SDLK_CALCULATOR},
        {SDLK_COMPUTER,           MY_SDLK_COMPUTER},
        {SDLK_AC_SEARCH,          MY_SDLK_AC_SEARCH},
        {SDLK_AC_HOME,            MY_SDLK_AC_HOME},
        {SDLK_AC_BACK,            MY_SDLK_AC_BACK},
        {SDLK_AC_FORWARD,         MY_SDLK_AC_FORWARD},
        {SDLK_AC_STOP,            MY_SDLK_AC_STOP},
        {SDLK_AC_REFRESH,         MY_SDLK_AC_REFRESH},
        {SDLK_AC_BOOKMARKS,       MY_SDLK_AC_BOOKMARKS},

        {SDLK_BRIGHTNESSDOWN,     MY_SDLK_BRIGHTNESSDOWN},
        {SDLK_BRIGHTNESSUP,       MY_SDLK_BRIGHTNESSUP},
        {SDLK_DISPLAYSWITCH,      MY_SDLK_DISPLAYSWITCH},
        {SDLK_KBDILLUMTOGGLE,     MY_SDLK_KBDILLUMTOGGLE},
        {SDLK_KBDILLUMDOWN,       MY_SDLK_KBDILLUMDOWN},
        {SDLK_KBDILLUMUP,         MY_SDLK_KBDILLUMUP},
        {SDLK_EJECT,              MY_SDLK_EJECT},
        {SDLK_SLEEP,              MY_SDLK_SLEEP},
        //{SDLK_APP1, MY_SDLK_APP1},
        //{SDLK_APP2, MY_SDLK_APP2},

        //{SDLK_AUDIOREWIND, MY_SDLK_AUDIOREWIND},
        //{SDLK_AUDIOFASTFORWARD, MY_SDLK_AUDIOFASTFORWARD},

        {SDLK_LAST,               MY_SDLK_UNKNOWN}
};

static int func_SDLK_TO_MY_SDLK(int argSDLK) {
    int i = 0;

    do {
        if (SDLK_to_My_SDLK_ARRAY[i].m_SDLK == argSDLK) {
            break;
        }
        i++;
    } while (SDLK_to_My_SDLK_ARRAY[i].m_SDLK != SDLK_LAST);
    return SDLK_to_My_SDLK_ARRAY[i].m_MySDLK;
}


static const char *const in_sdl_keys[SDLK_LAST] = {
        [MY_SDLK_BACKSPACE] = "backspace",
        [MY_SDLK_TAB] = "tab",
        [MY_SDLK_CLEAR] = "clear",
        [MY_SDLK_RETURN] = "return",
        [MY_SDLK_PAUSE] = "pause",
        [MY_SDLK_ESCAPE] = "escape",
        [MY_SDLK_SPACE] = "space",
        [MY_SDLK_EXCLAIM]  = "!",
        [MY_SDLK_QUOTEDBL]  = "\"",
        [MY_SDLK_HASH]  = "#",
        [MY_SDLK_DOLLAR]  = "$",
        [MY_SDLK_AMPERSAND]  = "&",
        [MY_SDLK_QUOTE] = "'",
        [MY_SDLK_LEFTPAREN] = "(",
        [MY_SDLK_RIGHTPAREN] = ")",
        [MY_SDLK_ASTERISK] = "*",
        [MY_SDLK_PLUS] = "+",
        [MY_SDLK_COMMA] = ",",
        [MY_SDLK_MINUS] = "-",
        [MY_SDLK_PERIOD] = ".",
        [MY_SDLK_SLASH] = "/",
        [MY_SDLK_0] = "0",
        [MY_SDLK_1] = "1",
        [MY_SDLK_2] = "2",
        [MY_SDLK_3] = "3",
        [MY_SDLK_4] = "4",
        [MY_SDLK_5] = "5",
        [MY_SDLK_6] = "6",
        [MY_SDLK_7] = "7",
        [MY_SDLK_8] = "8",
        [MY_SDLK_9] = "9",
        [MY_SDLK_COLON] = ":",
        [MY_SDLK_SEMICOLON] = ";",
        [MY_SDLK_LESS] = "<",
        [MY_SDLK_EQUALS] = "=",
        [MY_SDLK_GREATER] = ">",
        [MY_SDLK_QUESTION] = "?",
        [MY_SDLK_AT] = "@",
        [MY_SDLK_LEFTBRACKET] = "[",
        [MY_SDLK_BACKSLASH] = "\\",
        [MY_SDLK_RIGHTBRACKET] = "]",
        [MY_SDLK_CARET] = "^",
        [MY_SDLK_UNDERSCORE] = "_",
        [MY_SDLK_BACKQUOTE] = "`",
        [MY_SDLK_a] = "a",
        [MY_SDLK_b] = "b",
        [MY_SDLK_c] = "c",
        [MY_SDLK_d] = "d",
        [MY_SDLK_e] = "e",
        [MY_SDLK_f] = "f",
        [MY_SDLK_g] = "g",
        [MY_SDLK_h] = "h",
        [MY_SDLK_i] = "i",
        [MY_SDLK_j] = "j",
        [MY_SDLK_k] = "k",
        [MY_SDLK_l] = "l",
        [MY_SDLK_m] = "m",
        [MY_SDLK_n] = "n",
        [MY_SDLK_o] = "o",
        [MY_SDLK_p] = "p",
        [MY_SDLK_q] = "q",
        [MY_SDLK_r] = "r",
        [MY_SDLK_s] = "s",
        [MY_SDLK_t] = "t",
        [MY_SDLK_u] = "u",
        [MY_SDLK_v] = "v",
        [MY_SDLK_w] = "w",
        [MY_SDLK_x] = "x",
        [MY_SDLK_y] = "y",
        [MY_SDLK_z] = "z",
        [MY_SDLK_DELETE] = "delete",

        [MY_SDLK_KP_0] = "[0]",
        [MY_SDLK_KP_1] = "[1]",
        [MY_SDLK_KP_2] = "[2]",
        [MY_SDLK_KP_3] = "[3]",
        [MY_SDLK_KP_4] = "[4]",
        [MY_SDLK_KP_5] = "[5]",
        [MY_SDLK_KP_6] = "[6]",
        [MY_SDLK_KP_7] = "[7]",
        [MY_SDLK_KP_8] = "[8]",
        [MY_SDLK_KP_9] = "[9]",
        [MY_SDLK_KP_PERIOD] = "[.]",
        [MY_SDLK_KP_DIVIDE] = "[/]",
        [MY_SDLK_KP_MULTIPLY] = "[*]",
        [MY_SDLK_KP_MINUS] = "[-]",
        [MY_SDLK_KP_PLUS] = "[+]",
        [MY_SDLK_KP_ENTER] = "enter",
        [MY_SDLK_KP_EQUALS] = "equals",

        [MY_SDLK_UP] = "up",
        [MY_SDLK_DOWN] = "down",
        [MY_SDLK_RIGHT] = "right",
        [MY_SDLK_LEFT] = "left",
        [MY_SDLK_INSERT] = "insert",
        [MY_SDLK_HOME] = "home",
        [MY_SDLK_END] = "end",
        [MY_SDLK_PAGEUP] = "page up",
        [MY_SDLK_PAGEDOWN] = "page down",

        [MY_SDLK_F1] = "f1",
        [MY_SDLK_F2] = "f2",
        [MY_SDLK_F3] = "f3",
        [MY_SDLK_F4] = "f4",
        [MY_SDLK_F5] = "f5",
        [MY_SDLK_F6] = "f6",
        [MY_SDLK_F7] = "f7",
        [MY_SDLK_F8] = "f8",
        [MY_SDLK_F9] = "f9",
        [MY_SDLK_F10] = "f10",
        [MY_SDLK_F11] = "f11",
        [MY_SDLK_F12] = "f12",
        [MY_SDLK_F13] = "f13",
        [MY_SDLK_F14] = "f14",
        [MY_SDLK_F15] = "f15",

        [MY_SDLK_NUMLOCKCLEAR] = "numlock",
        [MY_SDLK_CAPSLOCK] = "caps lock",
        [MY_SDLK_SCROLLLOCK] = "scroll lock",
        [MY_SDLK_RSHIFT] = "right shift",
        [MY_SDLK_LSHIFT] = "left shift",
        [MY_SDLK_RCTRL] = "right ctrl",
        [MY_SDLK_LCTRL] = "left ctrl",
        [MY_SDLK_RALT] = "right alt",
        [MY_SDLK_LALT] = "left alt",
#ifdef NOTDEF
        [MY_SDLK_RMETA] = "right meta",
        [MY_SDLK_LMETA] = "left meta",
        [MY_SDLK_LSUPER] = "left super",	/* "Windows" keys */
        [MY_SDLK_RSUPER] = "right super",
#endif
        [MY_SDLK_MODE] = "alt gr",
#ifdef NOTDEF
        [MY_SDLK_COMPOSE] = "compose",
#endif
        [MY_SDLK_EJECT] = "eject",
        [MY_SDLK_SLEEP] = "sleep",
        [MY_SDLK_AUDIOPLAY] = "reset",
};


static void in_sdl_probe(const in_drv_t *drv) {
    // Do not register joysticks
    const struct in_pdata *pdata = drv->pdata;
    const char *const *key_names = in_sdl_keys;
    struct in_sdl_state *state;

    if (pdata->key_names)
        key_names = pdata->key_names;

    state = calloc(1, sizeof(*state));
    if (state == NULL) {
        fprintf(stderr, "in_sdl: OOM\n");
        return;
    }

    state->drv = drv;
    in_register(IN_SDL_PREFIX "keys", -1, state, SDLK_LAST,
                key_names, 0);
}

static void in_sdl_free(void *drv_data) {
    struct in_sdl_state *state = drv_data;

    if (state != NULL) {
        free(state);
    }
}

static const char *const *
in_sdl_get_key_names(const in_drv_t *drv, int *count) {
    const struct in_pdata *pdata = drv->pdata;
    *count = SDLK_LAST;

    if (pdata->key_names)
        return pdata->key_names;
    return in_sdl_keys;
}

/* could use SDL_GetKeyState, but this gives better packing */
static void update_keystate(keybits_t *keystate, int sym, int is_down) {
    keybits_t *ks_word, mask;

    mask = 1;
    mask <<= sym & (KEYBITS_WORD_BITS - 1);
    ks_word = keystate + sym / KEYBITS_WORD_BITS;
    if (is_down)
        *ks_word |= mask;
    else
        *ks_word &= ~mask;
}

static int handle_event(struct in_sdl_state *state, SDL_Event *event,
                        int *kc_out, int *down_out) {
    if (event->type != SDL_KEYDOWN && event->type != SDL_KEYUP)
        return -1;

    update_keystate(state->keystate, event->key.keysym.sym,
                    event->type == SDL_KEYDOWN);
    if (kc_out != NULL)
        *kc_out = event->key.keysym.sym;
    if (down_out != NULL)
        *down_out = event->type == SDL_KEYDOWN;

    return 1;
}

static int collect_events(struct in_sdl_state *state, int *one_kc, int *one_down) {
    SDL_Event events[4];

    int i = 0;
    Uint32 minType;
    Uint32 maxType;


    int count, maxcount;
    int ret, retval = 0;
    int num_events, num_peeped_events;
    SDL_Event *event;

    maxcount = (one_kc != NULL) ? 1 : sizeof(events) / sizeof(events[0]);

    SDL_PumpEvents();


    minType = SDL_KEYDOWN;
    maxType = SDL_KEYUP;
    num_events = SDL_PeepEvents(NULL,
                                0,
                                SDL_PEEKEVENT,
                                minType,
                                maxType);


    for (num_peeped_events = 0; num_peeped_events < num_events; num_peeped_events += count) {
        count = SDL_PeepEvents(events,
                               maxcount,
                               SDL_GETEVENT,
                               minType,
                               maxType);

        if (count <= 0)
            break;
        for (i = 0; i < count; i++) {
            event = &events[i];

            event->key.keysym.sym = func_SDLK_TO_MY_SDLK(event->key.keysym.sym);
            ret = handle_event(state,
                               event, one_kc, one_down);

            if (ret < 0) {
                switch (ret) {
                    case -2:
                        SDL_PushEvent(event);
                        break;
                    default:
                        if (ext_event_handler != NULL)
                            ext_event_handler(event);
                        break;
                }
                continue;
            }

            retval |= ret;
            if (one_kc != NULL && ret) {
                // don't lose events other devices might want to handle
                for (i++; i < count; i++)
                    SDL_PushEvent(&events[i]);
                goto out;
            }
        }
    }

    out:
    return retval;
}

static int in_sdl_update(void *drv_data, const int *binds, int *result) {
    struct in_sdl_state *state = drv_data;
    keybits_t mask;
    int i, sym, bit, b;

    if (collect_events(state, NULL, NULL) < 0)
        return 0;

    for (i = 0; i < SDLK_LAST / KEYBITS_WORD_BITS + 1; i++) {
        mask = state->keystate[i];
        if (mask == 0)
            continue;
        for (bit = 0; mask != 0; bit++, mask >>= 1) {
            if ((mask & 1) == 0)
                continue;
            sym = i * KEYBITS_WORD_BITS + bit;

            for (b = 0; b < IN_BINDTYPE_COUNT; b++)
                result[b] |= binds[IN_BIND_OFFS(sym, b)];
        }
    }

    return 0;
}

static int in_sdl_update_keycode(void *drv_data, int *is_down) {
    struct in_sdl_state *state = drv_data;
    int ret_kc = -1, ret_down = 0;

    collect_events(state, &ret_kc, &ret_down);

    if (is_down != NULL)
        *is_down = ret_down;

    return ret_kc;
}

static int in_sdl_menu_translate(void *drv_data, int keycode, char *charcode) {
    struct in_sdl_state *state = drv_data;
    const struct in_pdata *pdata = state->drv->pdata;
    const char *const *key_names = in_sdl_keys;
    const struct menu_keymap *map;
    int map_len;
    int ret = 0;
    int i;

    if (pdata->key_names)
        key_names = pdata->key_names;
    map = pdata->key_map;
    map_len = pdata->kmap_size;
    if (keycode < 0) {
        /* menu -> kc */
        keycode = -keycode;
        for (i = 0; i < map_len; i++)
            if (map[i].pbtn == keycode)
                return map[i].key;
    } else {
        for (i = 0; i < map_len; i++) {
            if (map[i].key == keycode) {
                ret = map[i].pbtn;
                break;
            }
        }

        if (charcode != NULL && (unsigned int) keycode < SDLK_LAST &&
            key_names[keycode] != NULL && key_names[keycode][1] == 0) {
            ret |= PBTN_CHAR;
            *charcode = key_names[keycode][0];
        }
    }
    return ret;
}

static const in_drv_t in_sdl_drv = {
        .prefix         = IN_SDL_PREFIX,
        .probe          = in_sdl_probe,
        .free           = in_sdl_free,
        .get_key_names  = in_sdl_get_key_names,
        .update         = in_sdl_update,
        .update_keycode = in_sdl_update_keycode,
        .menu_translate = in_sdl_menu_translate,
};

int in_sdl_init(const struct in_pdata *pdata, void (*handler)(void *event)) {
    if (!pdata) {
        fprintf(stderr, "in_sdl: Missing input platform data\n");
        return -1;
    }
    in_register_driver(&in_sdl_drv, pdata->defbinds, pdata);
    ext_event_handler = handler;
    return 0;
}
