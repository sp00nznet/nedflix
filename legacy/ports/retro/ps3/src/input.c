/*
 * Nedflix PS3 - DualShock 3 controller input
 */

#include "nedflix.h"
#include <stdio.h>
#include <string.h>
#include <io/pad.h>

static padInfo pad_info;
static padData pad_data;
static u32 prev_buttons = 0;

/* Initialize input */
int input_init(void)
{
    printf("Initializing pad input...\n");

    ioPadInit(7);

    /* Wait for controller */
    int timeout = 100;
    while (timeout > 0) {
        ioPadGetInfo(&pad_info);
        if (pad_info.status[0]) {
            printf("Controller connected\n");
            return 0;
        }
        usleep(50000);
        timeout--;
    }

    printf("Warning: No controller detected\n");
    return 0;  /* Non-fatal */
}

/* Shutdown input */
void input_shutdown(void)
{
    ioPadEnd();
}

/* Update input state */
void input_update(void)
{
    prev_buttons = g_app.buttons_pressed;

    ioPadGetInfo(&pad_info);

    if (pad_info.status[0]) {
        ioPadGetData(0, &pad_data);

        u32 buttons = 0;

        /* Map pad buttons to our button mask */
        if (pad_data.BTN_CROSS)    buttons |= BTN_CROSS;
        if (pad_data.BTN_CIRCLE)   buttons |= BTN_CIRCLE;
        if (pad_data.BTN_SQUARE)   buttons |= BTN_SQUARE;
        if (pad_data.BTN_TRIANGLE) buttons |= BTN_TRIANGLE;
        if (pad_data.BTN_START)    buttons |= BTN_START;
        if (pad_data.BTN_SELECT)   buttons |= BTN_SELECT;
        if (pad_data.BTN_UP)       buttons |= BTN_UP;
        if (pad_data.BTN_DOWN)     buttons |= BTN_DOWN;
        if (pad_data.BTN_LEFT)     buttons |= BTN_LEFT;
        if (pad_data.BTN_RIGHT)    buttons |= BTN_RIGHT;
        if (pad_data.BTN_L1)       buttons |= BTN_L1;
        if (pad_data.BTN_R1)       buttons |= BTN_R1;
        if (pad_data.BTN_L2)       buttons |= BTN_L2;
        if (pad_data.BTN_R2)       buttons |= BTN_R2;
        if (pad_data.BTN_L3)       buttons |= BTN_L3;
        if (pad_data.BTN_R3)       buttons |= BTN_R3;

        g_app.buttons_pressed = buttons;
        g_app.buttons_just_pressed = buttons & ~prev_buttons;

        /* Analog sticks */
        g_app.lstick_x = (int16_t)(pad_data.ANA_L_H - 128) * 256;
        g_app.lstick_y = (int16_t)(pad_data.ANA_L_V - 128) * 256;
        g_app.rstick_x = (int16_t)(pad_data.ANA_R_H - 128) * 256;
        g_app.rstick_y = (int16_t)(pad_data.ANA_R_V - 128) * 256;

        /* Pressure-sensitive triggers */
        g_app.l2_pressure = pad_data.PRE_L2;
        g_app.r2_pressure = pad_data.PRE_R2;
    }
}

/* Check if button was just pressed this frame */
bool input_pressed(button_mask_t button)
{
    return (g_app.buttons_just_pressed & button) != 0;
}

/* Check if button is currently held */
bool input_held(button_mask_t button)
{
    return (g_app.buttons_pressed & button) != 0;
}
