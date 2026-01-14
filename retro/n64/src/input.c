/*
 * Nedflix N64 - Input handling
 */

#include "nedflix.h"

static uint32_t prev_buttons = 0;

int input_init(void)
{
    LOG("Input init");
    return 0;
}

void input_shutdown(void)
{
}

void input_update(void)
{
    controller_scan();
    struct controller_data keys = get_keys_down();
    struct controller_data held = get_keys_held();
    struct SI_condat *c = &held.c[0];

    prev_buttons = g_app.buttons_pressed;
    g_app.buttons_pressed = 0;
    g_app.buttons_just_pressed = 0;

    if (c->A) g_app.buttons_pressed |= BTN_A;
    if (c->B) g_app.buttons_pressed |= BTN_B;
    if (c->Z) g_app.buttons_pressed |= BTN_Z;
    if (c->start) g_app.buttons_pressed |= BTN_START;
    if (c->up) g_app.buttons_pressed |= BTN_DPAD_UP;
    if (c->down) g_app.buttons_pressed |= BTN_DPAD_DOWN;
    if (c->left) g_app.buttons_pressed |= BTN_DPAD_LEFT;
    if (c->right) g_app.buttons_pressed |= BTN_DPAD_RIGHT;
    if (c->L) g_app.buttons_pressed |= BTN_L;
    if (c->R) g_app.buttons_pressed |= BTN_R;
    if (c->C_up) g_app.buttons_pressed |= BTN_C_UP;
    if (c->C_down) g_app.buttons_pressed |= BTN_C_DOWN;
    if (c->C_left) g_app.buttons_pressed |= BTN_C_LEFT;
    if (c->C_right) g_app.buttons_pressed |= BTN_C_RIGHT;

    g_app.buttons_just_pressed = g_app.buttons_pressed & ~prev_buttons;

    g_app.stick_x = c->x;
    g_app.stick_y = c->y;
}

bool input_pressed(button_mask_t button)
{
    return (g_app.buttons_just_pressed & button) != 0;
}

bool input_held(button_mask_t button)
{
    return (g_app.buttons_pressed & button) != 0;
}

int input_get_stick_x(void)
{
    return g_app.stick_x;
}

int input_get_stick_y(void)
{
    return g_app.stick_y;
}
