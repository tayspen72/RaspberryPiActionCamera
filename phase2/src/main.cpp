/*
 * tayspen72
 *
 * main.cpp
 */

//==============================================================================
// Notes
//==============================================================================

//==============================================================================
// Includes
//==============================================================================
#include <unistd.h>

#include <csignal>
#include <cstring>
#include <iostream>
#include <thread>

#include "action_camera.hpp"
#include "button.hpp"
#include "ssd1306.hpp"

//==============================================================================
// Definitions
//==============================================================================
// Version History
#define FIRMWARE_VERSION "1.0.0"

// Button Behavior
#define BUTTON_CHIP "/dev/gpiochip0"
#define BUTTON_PIN 23
#define BUTTON_BOUNCE_TIME 100
#define BUTTON_HOLD_TIME 1250
// #define HOLD_REPEAT 3

// Display
#define I2C_BUS_NAME "/dev/i2c-1"
#define NUM_COLUMNS 128
#define NUM_ROWS 32
#define MODE_EVENT_TEXT "EVENT"
#define MODE_EVENT_POSITION 0
#define MODE_RECORD_TEXT "RECORD"
#define MODE_RECORD_POSITION 40
#define MODE_STANDBY_TEXT "STANDBY"
#define MODE_STANDBY_POSITION 87

// Status text positions (new - below mode indicators)
#define STATE_TEXT_PAGE 3
#define STATE_TEXT_POSITION 0

// Status messages (new)
#define STATE_WAITING_EVENT		"    WAITING FOR EVENT    "
#define STATE_SAVING_EVENT		"       SAVING EVENT      "
#define STATE_RECORDING			"        RECORDING        "
#define STATE_STOPPED			"         STOPPED         "

// Constants
// #define EVENT_PRE_LENGTH (2 * 60 * 30)
// #define EVENT_POST_LENGTH 30
// #define EVENT_FILENAME_FMT "%Y-%m-%d.%H%M%S.h264"
// #define EVENT_FILEPATH "./events/"
// #define VIDEO_FILENAME_FMT "%Y-%m-%d.%H%M%S.h264"
// #define VIDEO_FILEPATH "./videos/"
// #define MAX_PATH 256

//==============================================================================
// Enumerations and Structures
//==============================================================================

//==============================================================================
// Private Function Prototypes
//==============================================================================
static void handle_hold();
static void handle_press();
static void handle_release();
static void write_mode(Ssd1306* ssd1306, ActionCamera::Mode mode);
static void write_state(Ssd1306* ssd1306, ActionCamera::State state);
static void mode_change_callback(ActionCamera::Mode mode);
static void state_change_callback(ActionCamera::State state);

static void signal_handler(int signal);

//==============================================================================
// Variables
//==============================================================================
static std::atomic<bool> _button_hold_pending{ false };
static std::atomic<bool> _button_hold_end_pending{ false };
static std::atomic<bool> _button_press_pending{ false };
static std::atomic<bool> _running{ true };

static Ssd1306* _ssd1306 = nullptr;

//==============================================================================
// Main
//==============================================================================
int main() {
	std::signal(SIGINT, signal_handler);
	std::signal(SIGTERM, signal_handler);

	std::cout << "Starting Raspberry Pi Action Camera" << std::endl;
	std::cout << "FW: " << FIRMWARE_VERSION << std::endl;

	ActionCamera camera;

	Button button(BUTTON_HOLD_TIME);
	if (button.Open(BUTTON_CHIP, BUTTON_PIN) < 0) {
		std::cerr << "Failed to initialize button GPIO" << std::endl;
	}
	button.SetOnHold(handle_hold);
	button.SetOnPress(handle_press);
	button.SetOnRelease(handle_release);

	_ssd1306 = new Ssd1306(NUM_COLUMNS, NUM_ROWS);
	if (_ssd1306->Open(I2C_BUS_NAME) < 0) {
		std::cerr << "Failed to initialize I2C bus" << std::endl;
		_ssd1306->Close();
		return 1;
	}
	_ssd1306->Init();

	camera.SetTargetModeChangeCallback(mode_change_callback);
	camera.SetStateChangeCallback(state_change_callback);

	// Initial display update
	write_mode(_ssd1306, camera.GetTargetMode());
	write_state(_ssd1306, camera.GetState());

	std::cout << "Waiting for user input" << std::endl;

	while (_running) {
		if (_button_hold_pending) {
			_button_hold_pending = false;
			ActionCamera::Mode tmpMode = camera.GetNextMode(camera.GetTargetMode());
			camera.SetTargetMode(tmpMode);
		}

		if (_button_hold_end_pending) {
			_button_hold_end_pending = false;
			camera.StopCurrentMode();
			camera.StartNextMode();
		}

		if (_button_press_pending) {
			_button_press_pending = false;
			camera.InputHandler();
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	button.Close();
	_ssd1306->Close();
	delete (_ssd1306);
	_ssd1306 = nullptr;

	return 0;
}

//==============================================================================
// Private Functions
//==============================================================================
static void handle_hold() {
	_button_hold_pending = true;
}

static void handle_press() {
	_button_press_pending = true;
}

static void handle_release() {
	_button_hold_end_pending = true;
}

static void write_mode(Ssd1306* ssd1306, ActionCamera::Mode mode) {
	switch (mode) {
		case ActionCamera::Mode::Event:
			ssd1306->WriteString(0, MODE_EVENT_POSITION, MODE_EVENT_TEXT, 0, 1);
			ssd1306->WriteString(0, MODE_RECORD_POSITION, MODE_RECORD_TEXT, 1, 0);
			ssd1306->WriteString(0, MODE_STANDBY_POSITION, MODE_STANDBY_TEXT, 1, 0);
			break;

		case ActionCamera::Mode::Record:
			ssd1306->WriteString(0, MODE_EVENT_POSITION, MODE_EVENT_TEXT, 1, 0);
			ssd1306->WriteString(0, MODE_RECORD_POSITION, MODE_RECORD_TEXT, 0, 1);
			ssd1306->WriteString(0, MODE_STANDBY_POSITION, MODE_STANDBY_TEXT, 1, 0);
			break;

		case ActionCamera::Mode::Standby:
			ssd1306->WriteString(0, MODE_EVENT_POSITION, MODE_EVENT_TEXT, 1, 0);
			ssd1306->WriteString(0, MODE_RECORD_POSITION, MODE_RECORD_TEXT, 1, 0);
			ssd1306->WriteString(0, MODE_STANDBY_POSITION, MODE_STANDBY_TEXT, 0, 1);
			break;
	}

	ssd1306->DrawPart(0, 0, 0, 127);
}

static void write_state(Ssd1306* ssd1306, ActionCamera::State state) {
	switch (state) {
		case ActionCamera::State::WaitingForEvent:
			ssd1306->WriteString(STATE_TEXT_PAGE, 0, STATE_WAITING_EVENT, 1, 0);
			break;

		case ActionCamera::State::SavingEvent:
			ssd1306->WriteString(STATE_TEXT_PAGE, 0, STATE_SAVING_EVENT, 1, 0);
			break;

		case ActionCamera::State::Recording:
			ssd1306->WriteString(STATE_TEXT_PAGE, 0, STATE_RECORDING, 1, 0);
			break;

		case ActionCamera::State::Stopped:
			ssd1306->WriteString(STATE_TEXT_PAGE, 0, STATE_STOPPED, 1, 0);
			break;

		case ActionCamera::State::None:
		default:
			// No status text for None state
			break;
	}

	ssd1306->DrawPart(0, STATE_TEXT_PAGE, 0, 127);
}

static void mode_change_callback(ActionCamera::Mode mode) {
	static ActionCamera::Mode last_mode = ActionCamera::Mode::Standby;

	if (last_mode != mode) {
		last_mode = mode;
		write_mode(_ssd1306, last_mode);
	}
}

static void state_change_callback(ActionCamera::State state) {
	static ActionCamera::State last_state = ActionCamera::State::None;

	if (last_state != state) {
		last_state = state;
		write_state(_ssd1306, state);
	}
}

//==============================================================================
// Interrupt
//==============================================================================
static void signal_handler(int signal) {
	(void)signal;
	_running = false;
}
