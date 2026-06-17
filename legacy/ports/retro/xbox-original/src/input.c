/*
 * Nedflix for Original Xbox
 * Controller input handling using SDL2
 */

#include "nedflix.h"
#include <string.h>

#ifdef NXDK
#include <SDL.h>
#include <hal/debug.h>
#else
/* Non-Xbox builds also use SDL for input */
#include <SDL2/SDL.h>
#endif

/* SDL gamepad state */
static SDL_GameController *g_controller = NULL;
static bool g_sdl_initialized = false;

/* Dead zone for analog sticks */
#define STICK_DEADZONE 8000
#define TRIGGER_THRESHOLD 30

/*
 * Initialize input system
 */
int input_init(void)
{
    LOG("Initializing input...");

    /* Initialize SDL subsystems if not already done */
    if (!g_sdl_initialized) {
#ifdef NXDK
        if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) < 0) {
            LOG_ERROR("SDL_InitSubSystem failed: %s", SDL_GetError());
            return -1;
        }
#else
        /* Non-Xbox: Initialize both gamecontroller and keyboard */
        if (SDL_Init(SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS) < 0) {
            LOG_ERROR("SDL_Init failed: %s", SDL_GetError());
            return -1;
        }
#endif
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
        LOG("No controller found - keyboard input available");
    }

    return 0;
}

/*
 * Shutdown input system
 */
void input_shutdown(void)
{
    if (g_controller) {
        SDL_GameControllerClose(g_controller);
        g_controller = NULL;
    }
    if (g_sdl_initialized) {
#ifdef NXDK
        SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
#else
        SDL_Quit();
#endif
        g_sdl_initialized = false;
    }
    LOG("Input shutdown");
}

/*
 * Update input state (call once per frame)
 */
void input_update(void)
{
    /* Pump SDL events */
    SDL_PumpEvents();

    uint16_t buttons = 0;

    /* Process gamepad input if controller is connected */
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

    /* Always process keyboard input as fallback/alternative
     * Keyboard mapping:
     *   Arrow keys = D-pad
     *   Enter/Space = A button
     *   Backspace/Escape = B button
     *   X = X button
     *   Y = Y button
     *   Tab = Start
     *   Shift = Back
     *   Q/E = Left/Right trigger (LT/RT for library switching)
     *   1/2 = White/Black shoulder buttons
     */
    const Uint8 *keystate = SDL_GetKeyboardState(NULL);
    if (keystate) {
        /* D-pad / Arrow keys */
        if (keystate[SDL_SCANCODE_UP] || keystate[SDL_SCANCODE_W])
            buttons |= BTN_DPAD_UP;
        if (keystate[SDL_SCANCODE_DOWN] || keystate[SDL_SCANCODE_S])
            buttons |= BTN_DPAD_DOWN;
        if (keystate[SDL_SCANCODE_LEFT] || keystate[SDL_SCANCODE_A])
            buttons |= BTN_DPAD_LEFT;
        if (keystate[SDL_SCANCODE_RIGHT] || keystate[SDL_SCANCODE_D])
            buttons |= BTN_DPAD_RIGHT;

        /* Face buttons */
        if (keystate[SDL_SCANCODE_RETURN] || keystate[SDL_SCANCODE_SPACE] ||
            keystate[SDL_SCANCODE_Z])
            buttons |= BTN_A;
        if (keystate[SDL_SCANCODE_BACKSPACE] || keystate[SDL_SCANCODE_ESCAPE] ||
            keystate[SDL_SCANCODE_X])
            buttons |= BTN_B;
        if (keystate[SDL_SCANCODE_C])
            buttons |= BTN_X;
        if (keystate[SDL_SCANCODE_V])
            buttons |= BTN_Y;

        /* Start/Back */
        if (keystate[SDL_SCANCODE_TAB])
            buttons |= BTN_START;
        if (keystate[SDL_SCANCODE_LSHIFT] || keystate[SDL_SCANCODE_RSHIFT])
            buttons |= BTN_BACK;

        /* Triggers - Q/E for library switching */
        if (keystate[SDL_SCANCODE_Q])
            buttons |= BTN_LEFT_TRIGGER;
        if (keystate[SDL_SCANCODE_E])
            buttons |= BTN_RIGHT_TRIGGER;

        /* Shoulder buttons */
        if (keystate[SDL_SCANCODE_1])
            buttons |= BTN_WHITE;
        if (keystate[SDL_SCANCODE_2])
            buttons |= BTN_BLACK;

        /* Thumbstick clicks */
        if (keystate[SDL_SCANCODE_F])
            buttons |= BTN_LEFT_THUMB;
        if (keystate[SDL_SCANCODE_G])
            buttons |= BTN_RIGHT_THUMB;
    }

    /* Calculate just-pressed buttons */
    uint16_t prev_buttons = g_app.buttons_pressed;
    g_app.buttons_pressed = buttons;
    g_app.buttons_just_pressed = buttons & ~prev_buttons;

    /* Update last input time */
    if (buttons != 0) {
        g_app.last_input_time = SDL_GetTicks();
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
 * Get left stick X axis (-32768 to 32767)
 */
int input_get_left_stick_x(void)
{
    if (!g_controller) return 0;
    int value = SDL_GameControllerGetAxis(g_controller, SDL_CONTROLLER_AXIS_LEFTX);
    if (abs(value) < STICK_DEADZONE) return 0;
    return value;
}

/*
 * Get left stick Y axis (-32768 to 32767)
 */
int input_get_left_stick_y(void)
{
    if (!g_controller) return 0;
    int value = SDL_GameControllerGetAxis(g_controller, SDL_CONTROLLER_AXIS_LEFTY);
    if (abs(value) < STICK_DEADZONE) return 0;
    return value;
}

/*
 * Get right stick X axis (-32768 to 32767)
 */
int input_get_right_stick_x(void)
{
    if (!g_controller) return 0;
    int value = SDL_GameControllerGetAxis(g_controller, SDL_CONTROLLER_AXIS_RIGHTX);
    if (abs(value) < STICK_DEADZONE) return 0;
    return value;
}

/*
 * Get right stick Y axis (-32768 to 32767)
 */
int input_get_right_stick_y(void)
{
    if (!g_controller) return 0;
    int value = SDL_GameControllerGetAxis(g_controller, SDL_CONTROLLER_AXIS_RIGHTY);
    if (abs(value) < STICK_DEADZONE) return 0;
    return value;
}

/*
 * Get left trigger value (0-32767)
 */
int input_get_left_trigger(void)
{
    if (!g_controller) return 0;
    return SDL_GameControllerGetAxis(g_controller, SDL_CONTROLLER_AXIS_TRIGGERLEFT);
}

/*
 * Get right trigger value (0-32767)
 */
int input_get_right_trigger(void)
{
    if (!g_controller) return 0;
    return SDL_GameControllerGetAxis(g_controller, SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
}
