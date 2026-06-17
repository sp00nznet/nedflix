/*
 * Nedflix for Nintendo GameCube
 * Controller input handling using libogc PAD
 *
 * TECHNICAL DEMO / NOVELTY PORT
 */

#include "nedflix.h"

/* Input state */
static uint32_t g_buttons_current;
static uint32_t g_buttons_previous;
static int g_stick_x;
static int g_stick_y;
static int g_cstick_x;
static int g_cstick_y;

/* Deadzone for analog sticks */
#define STICK_DEADZONE 20

/*
 * Initialize input subsystem
 */
int input_init(void)
{
    /* PAD already initialized in main */
    g_buttons_current = 0;
    g_buttons_previous = 0;
    g_stick_x = 0;
    g_stick_y = 0;
    g_cstick_x = 0;
    g_cstick_y = 0;

    return 0;
}

/*
 * Shutdown input subsystem
 */
void input_shutdown(void)
{
    /* Nothing to clean up */
}

/*
 * Update input state
 */
void input_update(void)
{
    /* Scan controller ports */
    PAD_ScanPads();

    /* Store previous state */
    g_buttons_previous = g_buttons_current;

    /* Read controller 0 */
    g_buttons_current = PAD_ButtonsHeld(0);

    /* Read analog sticks */
    g_stick_x = PAD_StickX(0);
    g_stick_y = PAD_StickY(0);
    g_cstick_x = PAD_SubStickX(0);
    g_cstick_y = PAD_SubStickY(0);

    /* Apply deadzone */
    if (abs(g_stick_x) < STICK_DEADZONE) g_stick_x = 0;
    if (abs(g_stick_y) < STICK_DEADZONE) g_stick_y = 0;
    if (abs(g_cstick_x) < STICK_DEADZONE) g_cstick_x = 0;
    if (abs(g_cstick_y) < STICK_DEADZONE) g_cstick_y = 0;

    /* Update global state */
    g_app.buttons_prev = g_app.buttons_pressed;
    g_app.buttons_pressed = g_buttons_current;
    g_app.buttons_just_pressed = (g_buttons_current ^ g_buttons_previous) & g_buttons_current;
}

/*
 * Check if button is currently pressed
 */
bool input_button_pressed(uint32_t button)
{
    return (g_buttons_current & button) != 0;
}

/*
 * Check if button was just pressed this frame
 */
bool input_button_just_pressed(uint32_t button)
{
    uint32_t just_pressed = (g_buttons_current ^ g_buttons_previous) & g_buttons_current;
    return (just_pressed & button) != 0;
}

/*
 * Get main analog stick X (-128 to 127)
 */
int input_get_stick_x(void)
{
    return g_stick_x;
}

/*
 * Get main analog stick Y (-128 to 127)
 */
int input_get_stick_y(void)
{
    return g_stick_y;
}

/*
 * Get C-stick X (-128 to 127)
 */
int input_get_cstick_x(void)
{
    return g_cstick_x;
}

/*
 * Get C-stick Y (-128 to 127)
 */
int input_get_cstick_y(void)
{
    return g_cstick_y;
}
