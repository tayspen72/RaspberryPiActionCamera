
/*
 * tayspen72
 *
 * action_camera.cpp
 */

//==============================================================================
// Notes
//==============================================================================

//==============================================================================
// Includes
//==============================================================================
#include "action_camera.hpp"

#include <signal.h>
#include <sys/stat.h>

#include <iostream>
#include <string>

//==============================================================================
// Definitions
//==============================================================================

//==============================================================================
// Definitions
//==============================================================================

//==============================================================================
// Enumerations and Structures
//==============================================================================

//==============================================================================
// Private Function Prototypes
//==============================================================================

//==============================================================================
// Variables
//==============================================================================

//==============================================================================
// Public Functions
//==============================================================================
ActionCamera::ActionCamera() {
	struct stat info;
	if (stat(EVENTS_FILEPATH.c_str(), &info) == -1)
		mkdir(EVENTS_FILEPATH.c_str(), 0755);

	if (stat(VIDEOS_FILEPATH.c_str(), &info) == -1)
		mkdir(VIDEOS_FILEPATH.c_str(), 0755);
}

ActionCamera::~ActionCamera() {}

ActionCamera::Mode ActionCamera::GetNextMode(Mode mode) {
	switch (mode) {
		case Mode::Event:
			return Mode::Record;
		case Mode::Standby:
			return Mode::Event;
		case Mode::Record:
		default:
			return Mode::Standby;
	}
}

void ActionCamera::InputHandler() {
	switch (current_mode_) {
		case Mode::Event:
			eventModeTrigger();
			break;

		case Mode::Record:
			if (recording_pid_ != -1)
				recordModeStop();
			else
				recordModeStart();
			break;

		case Mode::Standby:
		default:
			// Nothing to see here.
			break;
	}
}

void ActionCamera::SetTargetMode(Mode mode) {
	target_mode_ = mode;

	// Notify display of mode change
	targetModeChangeNotify(target_mode_);
}

void ActionCamera::StopCurrentMode() {
	switch (current_mode_) {
		case Mode::Event:
			std::cout << "ActionCamera: Stopping event mode" << std::endl;
			eventModeStop();
			break;

		case Mode::Record:
			std::cout << "ActionCamera: Stopping record mode" << std::endl;
			if (recording_pid_) {
				std::cout << "ActionCamera: Stopping current recording before changing mode" << std::endl;
				recordModeStop();
			}
			break;

		default:
		case Mode::Standby:
			std::cout << "ActionCamera: Stopping standby mode" << std::endl;
			break;
	}
}

void ActionCamera::StartNextMode() {
	switch (target_mode_) {
		case Mode::Event:
			std::cout << "ActionCamera: Starting Event mode" << std::endl;
			eventModeStart();
			break;

		case Mode::Record:
			std::cout << "ActionCamera: Starting Record mode" << std::endl;
			recordModeStart();
			break;

		case Mode::Standby:
			std::cout << "ActionCamera: Starting Standby mode" << std::endl;
			// Nothing to see here.
			break;
	}

	current_mode_ = target_mode_;
	currentModeChangeNotify(current_mode_);
}

//==============================================================================
// Private Functions
//==============================================================================
void ActionCamera::eventModeStart() {
	// Update display state before starting recording
	state_ = State::WaitingForEvent;
	stateChangeNotify(state_);

	std::string filename = EVENTS_FILEPATH + EVENT_DEFAULT_NAME;
	std::string cmd = "rpicam-vid -t 0 --inline --circular 120 -o " + filename + " &";
	system(cmd.c_str());

	sleep(1);
	recording_pid_ = getPidOfRpicamVid();
	std::cout << "ActionCamera: Passive mode started (2min circular buffer)" << std::endl;
}

void ActionCamera::eventModeStop() {
	if (recording_pid_ != -1) {
		kill(recording_pid_, SIGINT);

		sleep(1);  // Wait for file to be written
		std::string old_name = EVENTS_FILEPATH + EVENT_DEFAULT_NAME;
		std::string new_name = EVENTS_FILEPATH + "event_" + getCurrentTimestamp() + ".h264";
		rename(old_name.c_str(), new_name.c_str());
	}
	recording_pid_ = -1;

	// Clear display state when leaving event mode
	state_ = State::None;
	stateChangeNotify(state_);

	std::cout << "ActionCamera: Cleaning up current event capture before changing mode" << std::endl;
}

void ActionCamera::eventModeTrigger() {
	// Update display state to show saving
	state_ = State::SavingEvent;
	stateChangeNotify(state_);

	if (recording_pid_ != -1) {
		// Send SIGINT to save buffer and stop
		kill(recording_pid_, SIGINT);

		// Rename file with timestamp
		sleep(1);  // Wait for file to be written
		std::string old_name = EVENTS_FILEPATH + EVENT_DEFAULT_NAME;
		std::string new_name = EVENTS_FILEPATH + "event_" + getCurrentTimestamp() + ".h264";
		rename(old_name.c_str(), new_name.c_str());
	}
	recording_pid_ = -1;
	std::cout << "ActionCamera: Event saved" << std::endl;

	eventModeStart();
}

void ActionCamera::recordModeStart() {
	// Update display state before starting recording
	state_ = State::Recording;
	stateChangeNotify(state_);
	std::string filename = VIDEOS_FILEPATH + "video_" + getCurrentTimestamp() + ".h264";
	std::string cmd = "rpicam-vid -t 0 -o " + filename + " &";
	system(cmd.c_str());

	sleep(1);
	recording_pid_ = getPidOfRpicamVid();
	std::cout << "ActionCamera: Active recording started" << std::endl;
}

void ActionCamera::recordModeStop() {
	if (recording_pid_ != -1) {
		kill(recording_pid_, SIGINT);
	}
	recording_pid_ = -1;

	// Update display state to show stopped
	state_ = State::Stopped;
	stateChangeNotify(state_);

	std::cout << "ActionCamera: Active recording stopped" << std::endl;
}

void ActionCamera::currentModeChangeNotify(Mode mode) {
	if (current_mode_change_callback_) {
		current_mode_change_callback_(mode);
	}
}

void ActionCamera::targetModeChangeNotify(Mode mode) {
	if (target_mode_change_callback_) {
		target_mode_change_callback_(mode);
	}
}

void ActionCamera::stateChangeNotify(State state) {
	if (state_change_callback_) {
		state_change_callback_(state);
	}
}

//==============================================================================
// Task Handler
//==============================================================================

//==============================================================================
// Interrupt
//==============================================================================
// Task Handler
//==============================================================================

//==============================================================================
// Interrupt
//==============================================================================
