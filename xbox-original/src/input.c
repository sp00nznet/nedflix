/*
 * Nedflix for Original Xbox
 * Controller input handling
 */

#include "nedflix.h"
#include <string.h>

#ifdef NXDK
#include <hal/input.h>
#include <xboxkrnl/xboxkrnl.h>

/* Xbox input state */
static XPAD_INPUT g_pad_state;
static XPAD_INPUT g_pad_prev;
static bool g_pad_connected = false;

#else
/* Stub for non-Xbox builds */
static uint16_t g_buttons = 0;
static uint16_t g_prev_buttons = 0;
#endif

/* Dead zone for analog sticks */
#define STICK_DEADZONE 8000
#define TRIGGER_THRESHOLD 30

/*
 * Initialize input system
 */
int input_init(void)
{
    LOG("Initializing input...");

#ifdef NXDK
    /* Initialize Xbox input system */
    XInputInit();

    /* Clear state */
    memset(&g_pad_state, 0, sizeof(g_pad_state));
    memset(&g_pad_prev, 0, sizeof(g_pad_prev));

    /* Check for controller */
    DWORD devices = XInputGetState(0, &g_pad_state);
    g_pad_connected = (devices != 0);

    if (g_pad_connected) {
        LOG("Controller connected");
    } else {
        LOG("No controller found");
    }
#endif

    return 0;
}

/*
 * Shutdown input system
 */
void input_shutdown(void)
{
#ifdef NXDK
    /* Nothing to cleanup */
#endif
    LOG("Input shutdown");
}

/*
 * Update input state (call once per frame)
 */
void input_update(void)
{
#ifdef NXDK
    /* Save previous state */
    memcpy(&g_pad_prev, &g_pad_state, sizeof(g_pad_state));

    /* Get new state */
    DWORD devices = XInputGetState(0, &g_pad_state);
    g_pad_connected = (devices != 0);

    /* Update global app state */
    uint16_t buttons = 0;

    if (g_pad_state.AnalogButtons[XPAD_A] > TRIGGER_THRESHOLD) buttons |= BTN_A;
    if (g_pad_state.AnalogButtons[XPAD_B] > TRIGGER_THRESHOLD) buttons |= BTN_B;
    if (g_pad_state.AnalogButtons[XPAD_X] > TRIGGER_THRESHOLD) buttons |= BTN_X;
    if (g_pad_state.AnalogButtons[XPAD_Y] > TRIGGER_THRESHOLD) buttons |= BTN_Y;
    if (g_pad_state.AnalogButtons[XPAD_BLACK] > TRIGGER_THRESHOLD) buttons |= BTN_BLACK;
    if (g_pad_state.AnalogButtons[XPAD_WHITE] > TRIGGER_THRESHOLD) buttons |= BTN_WHITE;
    if (g_pad_state.AnalogButtons[XPAD_LEFT_TRIGGER] > TRIGGER_THRESHOLD) buttons |= BTN_LEFT_TRIGGER;
    if (g_pad_state.AnalogButtons[XPAD_RIGHT_TRIGGER] > TRIGGER_THRESHOLD) buttons |= BTN_RIGHT_TRIGGER;

    if (g_pad_state.wButtons & XINPUT_GAMEPAD_DPAD_UP) buttons |= BTN_DPAD_UP;
    if (g_pad_state.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) buttons |= BTN_DPAD_DOWN;
    if (g_pad_state.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) buttons |= BTN_DPAD_LEFT;
    if (g_pad_state.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) buttons |= BTN_DPAD_RIGHT;
    if (g_pad_state.wButtons & XINPUT_GAMEPAD_START) buttons |= BTN_START;
    if (g_pad_state.wButtons & XINPUT_GAMEPAD_BACK) buttons |= BTN_BACK;
    if (g_pad_state.wButtons & XINPUT_GAMEPAD_LEFT_THUMB) buttons |= BTN_LEFT_THUMB;
    if (g_pad_state.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) buttons |= BTN_RIGHT_THUMB;

    /* Also map left stick to D-pad for navigation convenience */
    if (g_pad_state.sThumbLY > STICK_DEADZONE) buttons |= BTN_DPAD_UP;
    if (g_pad_state.sThumbLY < -STICK_DEADZONE) buttons |= BTN_DPAD_DOWN;
    if (g_pad_state.sThumbLX < -STICK_DEADZONE) buttons |= BTN_DPAD_LEFT;
    if (g_pad_state.sThumbLX > STICK_DEADZONE) buttons |= BTN_DPAD_RIGHT;

    /* Calculate just-pressed buttons */
    uint16_t prev_buttons = g_app.buttons_pressed;
    g_app.buttons_pressed = buttons;
    g_app.buttons_just_pressed = buttons & ~prev_buttons;

    /* Update last input time */
    if (buttons != 0) {
        g_app.last_input_time = GetTickCount();
    }

#else
    /* Non-Xbox: use keyboard simulation or just clear */
    g_prev_buttons = g_buttons;
    g_buttons = 0;

    g_app.buttons_just_pressed = g_buttons & ~g_prev_buttons;
    g_app.buttons_pressed = g_buttons;
#endif
}

/*
 * Check if button is currently held
 */
bool input_button_pressed(button_mask_t button)
{
    return (g_app.buttons_pressed & button) != 0;
}

/*
 * Check if button was just pressed this frame
 */
bool input_button_just_pressed(button_mask_t button)
{
    return (g_app.buttons_just_pressed & button) != 0;
}

/*
 * Get left stick X axis (-32768 to 32767)
 */
int input_get_left_stick_x(void)
{
#ifdef NXDK
    int value = g_pad_state.sThumbLX;
    if (abs(value) < STICK_DEADZONE) return 0;
    return value;
#else
    return 0;
#endif
}

/*
 * Get left stick Y axis (-32768 to 32767)
 */
int input_get_left_stick_y(void)
{
#ifdef NXDK
    int value = g_pad_state.sThumbLY;
    if (abs(value) < STICK_DEADZONE) return 0;
    return value;
#else
    return 0;
#endif
}

/*
 * Get right stick X axis (-32768 to 32767)
 */
int input_get_right_stick_x(void)
{
#ifdef NXDK
    int value = g_pad_state.sThumbRX;
    if (abs(value) < STICK_DEADZONE) return 0;
    return value;
#else
    return 0;
#endif
}

/*
 * Get right stick Y axis (-32768 to 32767)
 */
int input_get_right_stick_y(void)
{
#ifdef NXDK
    int value = g_pad_state.sThumbRY;
    if (abs(value) < STICK_DEADZONE) return 0;
    return value;
#else
    return 0;
#endif
}

/*
 * Get left trigger value (0-255)
 */
int input_get_left_trigger(void)
{
#ifdef NXDK
    return g_pad_state.AnalogButtons[XPAD_LEFT_TRIGGER];
#else
    return 0;
#endif
}

/*
 * Get right trigger value (0-255)
 */
int input_get_right_trigger(void)
{
#ifdef NXDK
    return g_pad_state.AnalogButtons[XPAD_RIGHT_TRIGGER];
#else
    return 0;
#endif
}
