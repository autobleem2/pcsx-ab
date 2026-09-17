/*
 * (C) Artur "screemer" Jakubowicz, 2020 - SDL AutoBleem GameControllerAPI
 *
 * This work is licensed under the terms of any of these licenses
 * (at your option):
 *  - GNU GPL, version 2 or later.
 *  - GNU LGPL, version 2.1 or later.
 *  - MAME license.
 * See the COPYING file in the top-level directory.
 */
#include <stdio.h>
#include <SDL2/SDL.h>
#include <sys/stat.h>

#include "../main.h"
#include "input.h"
#include "in_sdl2gc.h"
#include "vector.h"

#if SDL_MAJOR_VERSION == 2
// Game Controller API only on SDL2

typedef int bool;
#define true 1
#define false 0
//#define CONFIG_DEBUG

#define IN_SDL2GC_NUM_AXIS 4


#define IN_SDL2GC_PREFIX "sdl2gc:"
#define IN_SDL2GC_NBUTTONS 17

#define TRIGGER_LEFT 0
#define TRIGGER_RIGHT 1
#define NUM_TRIGGERS 2
#define DEADZONE 10000


#define PSE_PAD_TYPE_MOUSE            1
#define PSE_PAD_TYPE_NEGCON           2
#define PSE_PAD_TYPE_GUN              3
#define PSE_PAD_TYPE_STANDARD         4
#define PSE_PAD_TYPE_ANALOGJOY        5
#define PSE_PAD_TYPE_GUNCON           6
#define PSE_PAD_TYPE_ANALOGPAD        7

#define MAX_ABS_DEVS 4
#define MAX_GC_AXIS 32767
#define GC_DEADZONE  10

static void (*ext_event_handler)(void *event);

#define CHECK_BIT(var, pos) ((var) & (1UL<<(pos)))
#define SET_BIT(var, pos) ((var) |= (1UL << (pos)))
#define CLEAR_BIT(var, pos)  ((var) &= ~(1UL << (pos)));

int menu_opened_player = 0;


enum {
    DKEY_SELECT = 0,
    DKEY_L3,
    DKEY_R3,
    DKEY_START,
    DKEY_UP,
    DKEY_RIGHT,
    DKEY_DOWN,
    DKEY_LEFT,
    DKEY_L2,
    DKEY_R2,
    DKEY_L1,
    DKEY_R1,
    DKEY_TRIANGLE,
    DKEY_CIRCLE,
    DKEY_CROSS,
    DKEY_SQUARE,
};

static const struct in_default_bind in_sdl2gc_defbinds[] = {
        {SDL2GC_DPAD_UP,      IN_BINDTYPE_PLAYER12, DKEY_UP},
        {SDL2GC_DPAD_DOWN,    IN_BINDTYPE_PLAYER12, DKEY_DOWN},
        {SDL2GC_DPAD_LEFT,    IN_BINDTYPE_PLAYER12, DKEY_LEFT},
        {SDL2GC_DPAD_RIGHT,   IN_BINDTYPE_PLAYER12, DKEY_RIGHT},
        {SDL2GC_BTN_TRIANGLE, IN_BINDTYPE_PLAYER12, DKEY_TRIANGLE},
        {SDL2GC_BTN_CROSS,    IN_BINDTYPE_PLAYER12, DKEY_CROSS},
        {SDL2GC_BTN_CIRCLE,   IN_BINDTYPE_PLAYER12, DKEY_CIRCLE},
        {SDL2GC_BTN_SQUARE,   IN_BINDTYPE_PLAYER12, DKEY_SQUARE},
        {SDL2GC_BTN_START,    IN_BINDTYPE_PLAYER12, DKEY_START},
        {SDL2GC_BTN_SELECT,   IN_BINDTYPE_PLAYER12, DKEY_SELECT},
        {SDL2GC_BTN_L1,       IN_BINDTYPE_PLAYER12, DKEY_L1},
        {SDL2GC_BTN_R1,       IN_BINDTYPE_PLAYER12, DKEY_R1},
        {SDL2GC_BTN_L2,       IN_BINDTYPE_PLAYER12, DKEY_L2},
        {SDL2GC_BTN_R2,       IN_BINDTYPE_PLAYER12, DKEY_R2},
        {SDL2GC_BTN_L3,       IN_BINDTYPE_PLAYER12, DKEY_L3},
        {SDL2GC_BTN_R3,       IN_BINDTYPE_PLAYER12, DKEY_R3},
        {SDL2GC_BTN_PS,       IN_BINDTYPE_EMU,      SACTION_ENTER_MENU},

        {0,                   0,                    0}
};

static const char *in_sdl2gc_keys[IN_SDL2GC_NBUTTONS] = {
        [0 ... IN_SDL2GC_NBUTTONS - 1] = NULL,
        [SDL2GC_DPAD_UP]      = "Up",       //00
        [SDL2GC_DPAD_LEFT]   = "Left",      //01
        [SDL2GC_DPAD_DOWN]    = "Down",     //02
        [SDL2GC_DPAD_RIGHT]  = "Right",     //03
        [SDL2GC_BTN_START]    = "Start",    //04
        [SDL2GC_BTN_SELECT]  = "Select",    //05
        [SDL2GC_BTN_TRIANGLE] = "Triangle", //06
        [SDL2GC_BTN_CIRCLE]  = "Circle",    //07
        [SDL2GC_BTN_SQUARE]   = "Square",   //08
        [SDL2GC_BTN_CROSS]  = "Cross",      //09
        [SDL2GC_BTN_L1]       = "L1",       //0A
        [SDL2GC_BTN_L2]      = "L2",        //0B
        [SDL2GC_BTN_R1]       = "R1",       //0C
        [SDL2GC_BTN_R2]      = "R2",        //0D
        [SDL2GC_BTN_PS]       = "Home",      //0E
        [SDL2GC_BTN_L3]       = "L3",         //0E
        [SDL2GC_BTN_R3]       = "R3"         //0E

};

/* SDL controller button -> our button. Anything not listed must be -1 (in_sdl2gc_get_keybits skips -1):
 * SDL 2.0.x had 15 buttons and all were listed, but 2.0.14+ added MISC1/PADDLE1-4/TOUCHPAD, and with the
 * old default of 0 (= SDL2GC_DPAD_UP) every one of those, being unpressed, cleared d-pad UP. */
static const int in_sdl2gc_key_map[SDL_CONTROLLER_BUTTON_MAX] = {
        [0 ... SDL_CONTROLLER_BUTTON_MAX - 1] = -1,
        [SDL_CONTROLLER_BUTTON_A]             = SDL2GC_BTN_CROSS,       //00
        [SDL_CONTROLLER_BUTTON_B]             = SDL2GC_BTN_CIRCLE,      //01
        [SDL_CONTROLLER_BUTTON_X]             = SDL2GC_BTN_SQUARE,     //02
        [SDL_CONTROLLER_BUTTON_Y]             = SDL2GC_BTN_TRIANGLE,     //03
        [SDL_CONTROLLER_BUTTON_BACK]          = SDL2GC_BTN_SELECT,    //04
        [SDL_CONTROLLER_BUTTON_GUIDE]         = SDL2GC_BTN_PS,    //05
        [SDL_CONTROLLER_BUTTON_START]         = SDL2GC_BTN_START, //06
        [SDL_CONTROLLER_BUTTON_LEFTSTICK]     = SDL2GC_BTN_L3,    //07
        [SDL_CONTROLLER_BUTTON_RIGHTSTICK]    = SDL2GC_BTN_R3,   //08
        [SDL_CONTROLLER_BUTTON_LEFTSHOULDER]  = SDL2GC_BTN_L1,      //09
        [SDL_CONTROLLER_BUTTON_RIGHTSHOULDER] = SDL2GC_BTN_R1,       //0A
        [SDL_CONTROLLER_BUTTON_DPAD_UP]       = SDL2GC_DPAD_UP,        //0B
        [SDL_CONTROLLER_BUTTON_DPAD_DOWN]     = SDL2GC_DPAD_DOWN,       //0C
        [SDL_CONTROLLER_BUTTON_DPAD_LEFT]     = SDL2GC_DPAD_LEFT,        //0D
        [SDL_CONTROLLER_BUTTON_DPAD_RIGHT]    = SDL2GC_DPAD_RIGHT,      //0E

};


static const struct {
    short key;
    short pbtn;
} key_pbtn_map[] =
        {
                {SDL2GC_DPAD_UP,      PBTN_UP},
                {SDL2GC_DPAD_DOWN,    PBTN_DOWN},
                {SDL2GC_DPAD_LEFT,    PBTN_LEFT},
                {SDL2GC_DPAD_RIGHT,   PBTN_RIGHT},
                {SDL2GC_BTN_CROSS,    PBTN_MOK},
                {SDL2GC_BTN_CIRCLE,   PBTN_MBACK},
                {SDL2GC_BTN_TRIANGLE, PBTN_MA2},
                {SDL2GC_BTN_SQUARE,   PBTN_MA3},
                {SDL2GC_BTN_L1,       PBTN_L},
                {SDL2GC_BTN_R1,       PBTN_R},
                {SDL2GC_BTN_PS,       PBTN_MENU},

        };
#define KEY_PBTN_MAP_SIZE (sizeof(key_pbtn_map) / sizeof(key_pbtn_map[0]))

// configuration override
extern int in_adev[4], in_adev_axis[4][2];
extern int in_adev_is_nublike[4];


typedef unsigned long keybits_t; // 32 bits is enough

struct in_sdl2gc_state {
    const in_drv_t *drv;
    SDL_GameController *pad;
    SDL_Joystick *joy;
    keybits_t keystate[1];
    int player;
    int dev_id;
    bool has_ps;
    bool has_analogue;
    Uint32 trigger_debounce_start[NUM_TRIGGERS];
};

vector gamepads;
int probed_joysticks = 0;

static void in_sdl2gc_clean_state(struct in_sdl2gc_state *state) {
    state->keystate[0] = 0;
    state->trigger_debounce_start[TRIGGER_LEFT] = 0;
    state->trigger_debounce_start[TRIGGER_RIGHT] = 0;

}

static bool in_sdl2gc_file_exists(const char *path) {
    struct stat buffer;
    return (stat(path, &buffer) == 0);
}


static keybits_t in_sdl2gc_get_keybits(void *drv_data) {
    SDL_GameControllerUpdate();
    keybits_t new_bits = 0UL;
    struct in_sdl2gc_state *state = drv_data;
    SDL_GameController *pad = state->pad;

    int current_state;
    for (int i = SDL_CONTROLLER_BUTTON_A; i < SDL_CONTROLLER_BUTTON_MAX; i++) {
        current_state = SDL_GameControllerGetButton(pad, i);
        int new_button = in_sdl2gc_key_map[i];
        if (new_button == -1) continue;
        if (current_state) SET_BIT(new_bits, new_button);
        else
            CLEAR_BIT(new_bits, new_button);
    }

    // Home emulation
    if (!state->has_ps) {
        if (CHECK_BIT(new_bits, SDL2GC_BTN_SELECT) && CHECK_BIT(new_bits, SDL2GC_BTN_START)) {
            SET_BIT(new_bits, SDL2GC_BTN_PS);
            CLEAR_BIT(new_bits, SDL2GC_BTN_SELECT);
            CLEAR_BIT(new_bits, SDL2GC_BTN_START)
        } else if ((CHECK_BIT(new_bits, SDL2GC_BTN_START)) == 0 || (CHECK_BIT(new_bits, SDL2GC_BTN_SELECT) == 0)) {
            CLEAR_BIT(new_bits, SDL2GC_BTN_PS);
        }
    }


    // L2/R2 emula
    current_state = SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_TRIGGERLEFT);
    if (current_state > DEADZONE) {
        SET_BIT(new_bits, SDL2GC_BTN_L2);
    } else {
        CLEAR_BIT(new_bits, SDL2GC_BTN_L2);
    }
    current_state = SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
    if (current_state > DEADZONE) {
        SET_BIT(new_bits, SDL2GC_BTN_R2);
    } else {
        CLEAR_BIT(new_bits, SDL2GC_BTN_R2);
    }

    return new_bits;
}




static void check_and_reprobe() {

    for (int i = 0; i < VECTOR_TOTAL(gamepads); i++) {
        struct in_sdl2gc_state *state = VECTOR_GET(gamepads, struct in_sdl2gc_state *, i);
        if (i==0){

            in_adev[0]=state->dev_id;
            in_adev[1]=state->dev_id;
        } else
        {
            in_adev[2]=state->dev_id;
            in_adev[3]=state->dev_id;
        }
    }
    int current_joysticks = SDL_NumJoysticks();
    if (current_joysticks != probed_joysticks) {
        printf(IN_SDL2GC_PREFIX " Controller config changed\n");
        // cleanup
        SDL_PumpEvents();
        SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);
        // unattach all previous
        for (int i = 0; i < VECTOR_TOTAL(gamepads); i++) {
            struct in_sdl2gc_state *state = VECTOR_GET(gamepads, struct in_sdl2gc_state *, i);
            if (SDL_GameControllerGetAttached(state->pad)) {
                SDL_GameControllerClose(state->pad);
            }
            free(state);
        }
        VECTOR_FREE(gamepads);
        VECTOR_INIT(gamepads);
        in_probe();
    }
}


static int in_sdl2gc_update_keycode(void *drv_data, int *is_down) {
    static int old_val = 0;
    keybits_t val, diff;
    int i;
    check_and_reprobe();
   // update_settings(drv_data);
    struct in_sdl2gc_state *state = drv_data;
    if (menu_opened_player != state->player) {
        return -1;
    }
    val = in_sdl2gc_get_keybits(drv_data);
    if (CHECK_BIT(val, SDL2GC_BTN_PS)) {
        CLEAR_BIT(val, SDL2GC_BTN_PS); // no ps in the menu to prevent reopening
    }


    diff = val ^ old_val;
    if (diff == 0UL)
        return -1;

    /* take one bit only */
    for (i = 0; i < sizeof(diff) * 8; i++)
        if (diff & (1UL << i))
            break;

    old_val ^= 1UL << i;

    if (is_down)
        *is_down = !!(val & (1UL << i));
    return i;
}

static void in_sdl2gc_probe(const in_drv_t *drv) {
    VECTOR_INIT(gamepads);
    const char *const *key_names = in_sdl2gc_keys;
    struct in_sdl2gc_state *state;

    char system_db[] = "/etc/autobleem/gamecontrollerdb.txt";
    char ab_db[] = "/media/Autobleem/bin/autobleem/gamecontrollerdb.txt";
    char local_db[] = "gamecontrollerdb.txt";
    char name[1024];

    // Load mappings
    int loadedMappings = 0;
    // Check joystick API for all possible controllers

    printf(IN_SDL2GC_PREFIX "Loading gamecontrollerdb.\n");
    if (in_sdl2gc_file_exists(system_db)) {
        printf(IN_SDL2GC_PREFIX "Loading System gamecontrollerdb.\n");
        loadedMappings = SDL_GameControllerAddMappingsFromFile(system_db);
        printf(IN_SDL2GC_PREFIX "Loaded pad mappings %d\n", loadedMappings);
    } else  if (in_sdl2gc_file_exists(local_db) && (loadedMappings == 0)) {
        printf(IN_SDL2GC_PREFIX "Loading local gamecontrollerdb.\n");
        loadedMappings = SDL_GameControllerAddMappingsFromFile(local_db);
        printf(IN_SDL2GC_PREFIX "Loaded pad mappings %d\n", loadedMappings);
    } else  if (in_sdl2gc_file_exists(ab_db)) {
        printf(IN_SDL2GC_PREFIX "Loading AutobBleem gamecontrollerdb.\n");
        loadedMappings = SDL_GameControllerAddMappingsFromFile(ab_db);
        printf(IN_SDL2GC_PREFIX "Loaded pad mappings %d\n", loadedMappings);
    }

    printf(IN_SDL2GC_PREFIX "Loaded pad mappings %d\n", loadedMappings);


    probed_joysticks = SDL_NumJoysticks();
    printf(IN_SDL2GC_PREFIX "Scanning %d joysticks....\n", probed_joysticks);
    int numControllers = 0;

    // override settings
    in_adev[0] = -1;
    in_adev[1] = -1;
    in_adev[2] = -1;
    in_adev[3] = -1;
    in_adev_axis[0][0] = 0;
    in_adev_axis[0][1] = 1;
    in_adev_axis[1][0] = 2;
    in_adev_axis[1][1] = 3;
    in_adev_axis[2][0] = 0;
    in_adev_axis[2][1] = 1;
    in_adev_axis[3][0] = 2;
    in_adev_axis[3][1] = 3;
    in_adev_is_nublike[0] = 0;
    in_adev_is_nublike[1] = 0;
    in_adev_is_nublike[2] = 0;
    in_adev_is_nublike[3] = 0;
    // map both pads as digital - let user change it after all

    for (int idx = 0; idx < probed_joysticks; idx++) {
        if (!SDL_IsGameController(idx)) continue;
        // This is valid game controller - open it and show some information in logs
        SDL_GameController *pad = SDL_GameControllerOpen(idx);
        if (!pad) continue;
        printf(IN_SDL2GC_PREFIX "Testing joystick %d\n", idx);
        state = calloc(1, sizeof(*state));
        state->joy = SDL_GameControllerGetJoystick(pad);
        state->drv = drv;
        state->pad = pad;

        SDL_GameControllerButtonBind ps_bind = SDL_GameControllerGetBindForButton(pad, SDL_CONTROLLER_BUTTON_GUIDE);
        SDL_GameControllerButtonBind l1_axis_bind = SDL_GameControllerGetBindForAxis(pad, SDL_CONTROLLER_AXIS_LEFTX);
        if (ps_bind.bindType == SDL_CONTROLLER_BINDTYPE_NONE)
            state->has_ps = false;
        else
            state->has_ps = true;
        if (l1_axis_bind.bindType == SDL_CONTROLLER_BINDTYPE_NONE)
            state->has_analogue = false;
        else
            state->has_analogue = true;

        printf(IN_SDL2GC_PREFIX "State mem allocated\n");
        printf(IN_SDL2GC_PREFIX "HasPS:%d  Analogue:%d\n", state->has_ps, state->has_analogue);
        in_sdl2gc_clean_state(state);

        snprintf(name, sizeof(name), IN_SDL2GC_PREFIX "%s [%d]", "AutoBleem Game Controller", numControllers);

        // Show info
        const char *controller_name = SDL_GameControllerName(pad);
        SDL_JoystickGUID guid = SDL_JoystickGetGUID(state->joy);
        char buffer[200];
        SDL_JoystickGetGUIDString(guid, buffer, 200);


        printf(IN_SDL2GC_PREFIX "Probed controller: %s as # %d GUID:%s\n", controller_name, numControllers, buffer);
        char *mapping = SDL_GameControllerMapping(pad);
        printf(IN_SDL2GC_PREFIX "ButtonMap %s\n", mapping);
        in_register(name, -1, state, IN_SDL2GC_NBUTTONS, key_names, 0);
        int dev_id = in_name_to_id(name);
        state->dev_id = dev_id;

        // map analogue to first controller (add second analogue after input/pluginlib fix)
        if (numControllers == 0) {
            in_adev[0] = dev_id;
            in_adev[1] = dev_id;
        } else
        {
            in_adev[2] = dev_id;
            in_adev[3] = dev_id;
        }

        numControllers++;
        state->player = numControllers;

        VECTOR_ADD(gamepads, state);
        // do not probe more than 2 players (4 pads not supported in emu)
        if (numControllers > 1) p2_connected = 1; else p2_connected = 0;
        if (numControllers > 2) return;
    }
}


static int in_sdl2gc_menu_translate(void *drv_data, int keycode, char *charcode) {
    int i;
    if (keycode < 0) {
        /* menu -> kc */
        keycode = -keycode;
        for (i = 0; i < KEY_PBTN_MAP_SIZE; i++)
            if (key_pbtn_map[i].pbtn == keycode)
                return key_pbtn_map[i].key;
    } else {
        for (i = 0; i < KEY_PBTN_MAP_SIZE; i++)
            if (key_pbtn_map[i].key == keycode)
                return key_pbtn_map[i].pbtn;
    }

    return 0;
}

void print_int_bin(int val)
{
    for (int i=0;i<32;i++)
    {
        printf("%d",CHECK_BIT(val,i));
    }
    printf("\n");
}

static int in_sdl2gc_update(void *drv_data, const int *binds, int *result) {

    struct in_sdl2gc_state *state = drv_data;
    int sym, bit, b;

    check_and_reprobe();

    state->keystate[0] = in_sdl2gc_get_keybits(drv_data);

    if (CHECK_BIT(state->keystate[0], SDL2GC_BTN_PS)) {
        menu_opened_player = state->player;
    }

    keybits_t mask;
    mask = state->keystate[0];
    if (mask == 0)
        return 0;
    for (bit = 0; mask != 0; bit++, mask >>= 1) {
        if ((mask & 1) == 0)
            continue;
        sym = bit;
        for (b = 0; b < IN_BINDTYPE_COUNT; b++) {
            if (state->player==1) {
                result[b] |= binds[IN_BIND_OFFS(sym, b)];
            } else
            {
                if (b==IN_BINDTYPE_PLAYER12) {
                    result[b] |= binds[IN_BIND_OFFS(sym, b)] << 16;
                } else
                {
                    result[b] |= binds[IN_BIND_OFFS(sym, b)];
                };
            }
        }
    }
    return 0;
}

static const char *const *in_sdl2gc_get_key_names(const in_drv_t *drv, int *count) {
    const struct in_pdata *pdata = drv->pdata;
    *count = IN_SDL2GC_NBUTTONS;

    if (pdata->key_names)
        return pdata->key_names;
    return in_sdl2gc_keys;
}

static void in_sdl2gc_free(void *drv_data) {
    // called automatically by in_probe() to clean all devices and on exit !!!!
    for (int i = 0; i < VECTOR_TOTAL(gamepads); i++) {
        struct in_sdl2gc_state *state = VECTOR_GET(gamepads, struct in_sdl2gc_state *, i);
        if (SDL_GameControllerGetAttached(state->pad)) {
            SDL_GameControllerClose(state->pad);
        }
        free(state);
    }
    VECTOR_FREE(gamepads);
}


static int in_sdl2gc_update_analog(void *drv_data, int axis_id, int *result) {
    // update is called just before this
    SDL_GameControllerUpdate();
    struct in_sdl2gc_state *state = drv_data;
    if ((unsigned int) axis_id >= IN_SDL2GC_NUM_AXIS)
        return -1;

    if (!state->has_analogue)
        return -1;

    int targetAxis = SDL_CONTROLLER_AXIS_INVALID;
    switch (axis_id) {
        case 0:
            targetAxis = SDL_CONTROLLER_AXIS_LEFTX;
            break;
        case 1:
            targetAxis = SDL_CONTROLLER_AXIS_LEFTY;
            break;
        case 2:
            targetAxis = SDL_CONTROLLER_AXIS_RIGHTX;
            break;
        case 3:
            targetAxis = SDL_CONTROLLER_AXIS_RIGHTY;
            break;
    }

    Sint16 axis_val = SDL_GameControllerGetAxis(state->pad, targetAxis);
    int val = axis_val * IN_ABS_RANGE / MAX_GC_AXIS;
    if (abs(val) < GC_DEADZONE) val = 0;
    *result = val;
    return 0;
}

static int in_sdl2gc_get_config(void *drv_data, int what, int *val) {
    switch (what) {
        case IN_CFG_ABS_AXIS_COUNT:
            *val = IN_SDL2GC_NUM_AXIS;
            break;
        default:
            return -1;
    }
    return 0;
}



static const in_drv_t in_sdl2gc_drv = {
        .prefix         = IN_SDL2GC_PREFIX,
        .probe          = in_sdl2gc_probe,
        .free           = in_sdl2gc_free,
        .get_key_names  = in_sdl2gc_get_key_names,
        .get_config     = in_sdl2gc_get_config,
        .update_analog  = in_sdl2gc_update_analog,
        .update         = in_sdl2gc_update,
        .update_keycode = in_sdl2gc_update_keycode,
        .menu_translate = in_sdl2gc_menu_translate,
};

int in_sdl2gc_init(const struct in_pdata *pdata, void (*handler)(void *event)) {
    if (!pdata) {
        fprintf(stderr, "in_sdl2gc: Missing input platform data\n");
        return -1;
    }
    printf(IN_SDL2GC_PREFIX "Starting GameController API\n");

    SDL_version compiled;
    SDL_version linked;

    SDL_VERSION(&compiled);
    SDL_GetVersion(&linked);
    printf(IN_SDL2GC_PREFIX "We compiled against SDL version %d.%d.%d ...\n",
           compiled.major, compiled.minor, compiled.patch);
    printf(IN_SDL2GC_PREFIX "But we are linking against SDL version %d.%d.%d.\n",
           linked.major, linked.minor, linked.patch);

    SDL_JoystickEventState(SDL_IGNORE);
    SDL_GameControllerEventState(SDL_IGNORE);

    in_register_driver(&in_sdl2gc_drv, in_sdl2gc_defbinds, NULL);
    ext_event_handler = handler;
    return 0;
}

#endif
