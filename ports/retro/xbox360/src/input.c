/*
 * Nedflix for Xbox 360
 * Controller input handling using libxenon input
 *
 * TECHNICAL DEMO / NOVELTY PORT
 */

#include "nedflix.h"

/* Input state */
static struct controller_data_s g_controller;
static uint32_t g_buttons_current = 0;
static uint32_t g_buttons_previous = 0;

/* Deadzone for analog sticks */
#define STICK_DEADZONE 8000

/*
 * Initialize input
 */
int input_init(void)
{
    /* Input is initialized via USB */
    memset(&g_controller, 0, sizeof(g_controller));
    LOG("Input initialized");
    return 0;
}

/*
 * Shutdown input
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
    g_buttons_previous = g_buttons_current;
    g_buttons_current = 0;

    /* Get controller state (controller 0) */
    if (get_controller_data(&g_controller, 0)) {
        /* Map libxenon controller data to our button mask */
        if (g_controller.a) g_buttons_current |= BTN_A;
        if (g_controller.b) g_buttons_current |= BTN_B;
        if (g_controller.x) g_buttons_current |= BTN_X;
        if (g_controller.y) g_buttons_current |= BTN_Y;
        if (g_controller.start) g_buttons_current |= BTN_START;
        if (g_controller.back) g_buttons_current |= BTN_BACK;
        if (g_controller.lb) g_buttons_current |= BTN_LB;
        if (g_controller.rb) g_buttons_current |= BTN_RB;
        if (g_controller.up) g_buttons_current |= BTN_DPAD_UP;
        if (g_controller.down) g_buttons_current |= BTN_DPAD_DOWN;
        if (g_controller.left) g_buttons_current |= BTN_DPAD_LEFT;
        if (g_controller.right) g_buttons_current |= BTN_DPAD_RIGHT;
        if (g_controller.logo) g_buttons_current |= BTN_GUIDE;
        if (g_controller.s1_z) g_buttons_current |= BTN_LEFT_THUMB;
        if (g_controller.s2_z) g_buttons_current |= BTN_RIGHT_THUMB;

        /* Store analog values */
        g_app.left_stick_x = g_controller.s1_x;
        g_app.left_stick_y = g_controller.s1_y;
        g_app.right_stick_x = g_controller.s2_x;
        g_app.right_stick_y = g_controller.s2_y;
        g_app.left_trigger = g_controller.lt;
        g_app.right_trigger = g_controller.rt;

        /* Apply deadzone to sticks */
        if (abs(g_app.left_stick_x) < STICK_DEADZONE) g_app.left_stick_x = 0;
        if (abs(g_app.left_stick_y) < STICK_DEADZONE) g_app.left_stick_y = 0;
        if (abs(g_app.right_stick_x) < STICK_DEADZONE) g_app.right_stick_x = 0;
        if (abs(g_app.right_stick_y) < STICK_DEADZONE) g_app.right_stick_y = 0;
    }

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
    return (g_app.buttons_just_pressed & button) != 0;
}

/*
 * Get left analog stick X (-32768 to 32767)
 */
int16_t input_get_left_stick_x(void)
{
    return g_app.left_stick_x;
}

/*
 * Get left analog stick Y
 */
int16_t input_get_left_stick_y(void)
{
    return g_app.left_stick_y;
}

/*
 * Get right analog stick X
 */
int16_t input_get_right_stick_x(void)
{
    return g_app.right_stick_x;
}

/*
 * Get right analog stick Y
 */
int16_t input_get_right_stick_y(void)
{
    return g_app.right_stick_y;
}

/*
 * Get left trigger (0-255)
 */
uint8_t input_get_left_trigger(void)
{
    return g_app.left_trigger;
}

/*
 * Get right trigger (0-255)
 */
uint8_t input_get_right_trigger(void)
{
    return g_app.right_trigger;
}
