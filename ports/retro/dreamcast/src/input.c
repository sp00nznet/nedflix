/*
 * Nedflix for Sega Dreamcast
 * Controller input handling
 *
 * Dreamcast controller has:
 * - D-pad
 * - A, B, X, Y buttons
 * - Start button
 * - Analog stick
 * - L and R analog triggers
 */

#include "nedflix.h"
#include <string.h>

#include <dc/maple.h>
#include <dc/maple/controller.h>

/* Input state */
static struct {
    bool initialized;
    maple_device_t *controller;
    uint32 buttons;
    uint32 prev_buttons;
    int stick_x;
    int stick_y;
    int ltrig;
    int rtrig;
} g_input;

/* Dead zone for analog stick */
#define STICK_DEADZONE 20
#define TRIGGER_THRESHOLD 30

/* Button repeat timing */
#define REPEAT_DELAY_MS   400
#define REPEAT_RATE_MS    100

static uint64 repeat_start_time = 0;
static uint64 last_repeat_time = 0;
static uint32 repeat_buttons = 0;

/*
 * Initialize input system
 */
int input_init(void)
{
    LOG("Initializing input...");

    memset(&g_input, 0, sizeof(g_input));

    /* Find controller on first port */
    g_input.controller = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);

    if (g_input.controller) {
        LOG("Controller found on port %c", 'A' + g_input.controller->port);
    } else {
        LOG("No controller found, will retry");
    }

    g_input.initialized = true;
    return 0;
}

/*
 * Shutdown input system
 */
void input_shutdown(void)
{
    g_input.initialized = false;
    LOG("Input shutdown");
}

/*
 * Map Dreamcast buttons to our button flags
 */
static uint32 map_dc_buttons(uint32 dc_buttons, int stick_x, int stick_y,
                             int ltrig, int rtrig)
{
    uint32 buttons = 0;

    /* Face buttons (active low in KOS) */
    if (dc_buttons & CONT_A)     buttons |= BTN_A;
    if (dc_buttons & CONT_B)     buttons |= BTN_B;
    if (dc_buttons & CONT_X)     buttons |= BTN_X;
    if (dc_buttons & CONT_Y)     buttons |= BTN_Y;
    if (dc_buttons & CONT_START) buttons |= BTN_START;

    /* D-pad */
    if (dc_buttons & CONT_DPAD_UP)    buttons |= BTN_DPAD_UP;
    if (dc_buttons & CONT_DPAD_DOWN)  buttons |= BTN_DPAD_DOWN;
    if (dc_buttons & CONT_DPAD_LEFT)  buttons |= BTN_DPAD_LEFT;
    if (dc_buttons & CONT_DPAD_RIGHT) buttons |= BTN_DPAD_RIGHT;

    /* Map analog stick to d-pad for menu navigation */
    if (stick_y < -STICK_DEADZONE) buttons |= BTN_DPAD_UP;
    if (stick_y > STICK_DEADZONE)  buttons |= BTN_DPAD_DOWN;
    if (stick_x < -STICK_DEADZONE) buttons |= BTN_DPAD_LEFT;
    if (stick_x > STICK_DEADZONE)  buttons |= BTN_DPAD_RIGHT;

    /* Triggers */
    if (ltrig > TRIGGER_THRESHOLD) buttons |= BTN_LEFT_TRIGGER;
    if (rtrig > TRIGGER_THRESHOLD) buttons |= BTN_RIGHT_TRIGGER;

    return buttons;
}

/*
 * Handle button repeat for held buttons
 */
static uint32 handle_button_repeat(uint32 current, uint32 prev)
{
    uint32 just_pressed = current & ~prev;

    /* Check for d-pad/stick repeat (for menu navigation) */
    uint32 nav_buttons = BTN_DPAD_UP | BTN_DPAD_DOWN | BTN_DPAD_LEFT | BTN_DPAD_RIGHT;
    uint32 nav_held = current & nav_buttons;

    if (nav_held) {
        uint64 now = timer_ms_gettime64();

        if (nav_held != repeat_buttons) {
            /* New direction pressed */
            repeat_buttons = nav_held;
            repeat_start_time = now;
            last_repeat_time = now;
        } else {
            /* Same direction held */
            uint64 held_time = now - repeat_start_time;

            if (held_time > REPEAT_DELAY_MS) {
                /* In repeat mode */
                if (now - last_repeat_time > REPEAT_RATE_MS) {
                    just_pressed |= nav_held;
                    last_repeat_time = now;
                }
            }
        }
    } else {
        repeat_buttons = 0;
    }

    return just_pressed;
}

/*
 * Update input state (call once per frame)
 */
void input_update(void)
{
    if (!g_input.initialized) return;

    /* Try to find controller if not already found */
    if (!g_input.controller) {
        g_input.controller = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);
    }

    /* Save previous state */
    g_input.prev_buttons = g_input.buttons;

    if (g_input.controller) {
        cont_state_t *state = (cont_state_t *)maple_dev_status(g_input.controller);

        if (state) {
            /* Read analog values */
            /* KOS returns joystick as signed bytes, center at 0 */
            g_input.stick_x = state->joyx;
            g_input.stick_y = state->joyy;
            g_input.ltrig = state->ltrig;
            g_input.rtrig = state->rtrig;

            /* Map buttons */
            g_input.buttons = map_dc_buttons(state->buttons,
                                              g_input.stick_x,
                                              g_input.stick_y,
                                              g_input.ltrig,
                                              g_input.rtrig);
        }
    } else {
        g_input.buttons = 0;
        g_input.stick_x = 0;
        g_input.stick_y = 0;
        g_input.ltrig = 0;
        g_input.rtrig = 0;
    }

    /* Calculate just-pressed with repeat handling */
    g_app.buttons_just_pressed = handle_button_repeat(g_input.buttons,
                                                       g_input.prev_buttons);
    g_app.buttons_pressed = g_input.buttons;

    /* Update last input time */
    if (g_input.buttons != 0) {
        g_app.last_input_time = timer_ms_gettime64();
    }
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
 * Get analog stick X axis (-128 to 127)
 */
int input_get_stick_x(void)
{
    if (abs(g_input.stick_x) < STICK_DEADZONE) return 0;
    return g_input.stick_x;
}

/*
 * Get analog stick Y axis (-128 to 127)
 */
int input_get_stick_y(void)
{
    if (abs(g_input.stick_y) < STICK_DEADZONE) return 0;
    return g_input.stick_y;
}

/*
 * Get left trigger value (0-255)
 */
int input_get_left_trigger(void)
{
    return g_input.ltrig;
}

/*
 * Get right trigger value (0-255)
 */
int input_get_right_trigger(void)
{
    return g_input.rtrig;
}

/*
 * Check if controller is connected
 */
bool input_controller_connected(void)
{
    return g_input.controller != NULL;
}

/*
 * Rumble support (requires Jump Pack/Puru Puru Pack)
 */
void input_rumble(int duration_ms)
{
    maple_device_t *purupuru = maple_enum_type(0, MAPLE_FUNC_PURUPURU);

    if (purupuru) {
        /* Start rumble */
        purupuru_rumble_raw(purupuru, 0x10007);  /* Standard rumble effect */

        /* Note: Would need to stop rumble after duration_ms */
        /* For now, just start it and let it time out */
        (void)duration_ms;
    }
}
