/*
 * Nedflix for Original Xbox
 * Controller input handling using SDL2
 */

#include "nedflix.h"
#include <string.h>

#ifdef NXDK
#include <SDL.h>
#include <hal/debug.h>

/* SDL gamepad state */
static SDL_GameController *g_controller = NULL;
static bool g_sdl_initialized = false;

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
    /* Initialize SDL subsystems if not already done */
    if (!g_sdl_initialized) {
        if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) < 0) {
            LOG_ERROR("SDL_InitSubSystem failed: %s", SDL_GetError());
            return -1;
        }
        g_sdl_initialized = true;
    }

    /* Open the first available game controller */
    int num_joysticks = SDL_NumJoysticks();
    LOG("Found %d joystick(s)", num_joysticks);

    for (int i = 0; i < num_joysticks; i++) {
        if (SDL_IsGameController(i)) {
            g_controller = SDL_GameControllerOpen(i);
            if (g_controller) {
                LOG("Controller connected: %s", SDL_GameControllerName(g_controller));
                break;
            }
        }
    }

    if (!g_controller) {
        LOG("No controller found - keyboard input only");
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
    if (g_controller) {
        SDL_GameControllerClose(g_controller);
        g_controller = NULL;
    }
    if (g_sdl_initialized) {
        SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
        g_sdl_initialized = false;
    }
#endif
    LOG("Input shutdown");
}

/*
 * Update input state (call once per frame)
 */
void input_update(void)
{
#ifdef NXDK
    /* Pump SDL events */
    SDL_PumpEvents();

    uint16_t buttons = 0;

    if (g_controller) {
        /* Face buttons */
        if (SDL_GameControllerGetButton(g_controller, SDL_CONTROLLER_BUTTON_A))
            buttons |= BTN_A;
        if (SDL_GameControllerGetButton(g_controller, SDL_CONTROLLER_BUTTON_B))
            buttons |= BTN_B;
        if (SDL_GameControllerGetButton(g_controller, SDL_CONTROLLER_BUTTON_X))
            buttons |= BTN_X;
        if (SDL_GameControllerGetButton(g_controller, SDL_CONTROLLER_BUTTON_Y))
            buttons |= BTN_Y;

        /* Shoulder buttons (Xbox Black/White) */
        if (SDL_GameControllerGetButton(g_controller, SDL_CONTROLLER_BUTTON_LEFTSHOULDER))
            buttons |= BTN_WHITE;
        if (SDL_GameControllerGetButton(g_controller, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER))
            buttons |= BTN_BLACK;

        /* D-pad */
        if (SDL_GameControllerGetButton(g_controller, SDL_CONTROLLER_BUTTON_DPAD_UP))
            buttons |= BTN_DPAD_UP;
        if (SDL_GameControllerGetButton(g_controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN))
            buttons |= BTN_DPAD_DOWN;
        if (SDL_GameControllerGetButton(g_controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT))
            buttons |= BTN_DPAD_LEFT;
        if (SDL_GameControllerGetButton(g_controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT))
            buttons |= BTN_DPAD_RIGHT;

        /* Start/Back */
        if (SDL_GameControllerGetButton(g_controller, SDL_CONTROLLER_BUTTON_START))
            buttons |= BTN_START;
        if (SDL_GameControllerGetButton(g_controller, SDL_CONTROLLER_BUTTON_BACK))
            buttons |= BTN_BACK;

        /* Thumbstick clicks */
        if (SDL_GameControllerGetButton(g_controller, SDL_CONTROLLER_BUTTON_LEFTSTICK))
            buttons |= BTN_LEFT_THUMB;
        if (SDL_GameControllerGetButton(g_controller, SDL_CONTROLLER_BUTTON_RIGHTSTICK))
            buttons |= BTN_RIGHT_THUMB;

        /* Triggers (analog, but treat as digital) */
        int left_trigger = SDL_GameControllerGetAxis(g_controller, SDL_CONTROLLER_AXIS_TRIGGERLEFT);
        int right_trigger = SDL_GameControllerGetAxis(g_controller, SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
        if (left_trigger > 8000) buttons |= BTN_LEFT_TRIGGER;
        if (right_trigger > 8000) buttons |= BTN_RIGHT_TRIGGER;

        /* Map left stick to D-pad for navigation convenience */
        int stick_x = SDL_GameControllerGetAxis(g_controller, SDL_CONTROLLER_AXIS_LEFTX);
        int stick_y = SDL_GameControllerGetAxis(g_controller, SDL_CONTROLLER_AXIS_LEFTY);
        if (stick_y < -STICK_DEADZONE) buttons |= BTN_DPAD_UP;
        if (stick_y > STICK_DEADZONE) buttons |= BTN_DPAD_DOWN;
        if (stick_x < -STICK_DEADZONE) buttons |= BTN_DPAD_LEFT;
        if (stick_x > STICK_DEADZONE) buttons |= BTN_DPAD_RIGHT;
    }

    /* Calculate just-pressed buttons */
    uint16_t prev_buttons = g_app.buttons_pressed;
    g_app.buttons_pressed = buttons;
    g_app.buttons_just_pressed = buttons & ~prev_buttons;

    /* Update last input time */
    if (buttons != 0) {
        g_app.last_input_time = SDL_GetTicks();
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
    if (!g_controller) return 0;
    int value = SDL_GameControllerGetAxis(g_controller, SDL_CONTROLLER_AXIS_LEFTX);
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
    if (!g_controller) return 0;
    int value = SDL_GameControllerGetAxis(g_controller, SDL_CONTROLLER_AXIS_LEFTY);
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
    if (!g_controller) return 0;
    int value = SDL_GameControllerGetAxis(g_controller, SDL_CONTROLLER_AXIS_RIGHTX);
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
    if (!g_controller) return 0;
    int value = SDL_GameControllerGetAxis(g_controller, SDL_CONTROLLER_AXIS_RIGHTY);
    if (abs(value) < STICK_DEADZONE) return 0;
    return value;
#else
    return 0;
#endif
}

/*
 * Get left trigger value (0-32767)
 */
int input_get_left_trigger(void)
{
#ifdef NXDK
    if (!g_controller) return 0;
    return SDL_GameControllerGetAxis(g_controller, SDL_CONTROLLER_AXIS_TRIGGERLEFT);
#else
    return 0;
#endif
}

/*
 * Get right trigger value (0-32767)
 */
int input_get_right_trigger(void)
{
#ifdef NXDK
    if (!g_controller) return 0;
    return SDL_GameControllerGetAxis(g_controller, SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
#else
    return 0;
#endif
}
