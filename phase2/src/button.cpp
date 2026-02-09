/*
 * tayspen72
 *
 * button.cpp
 */

//==============================================================================
// Notes
//==============================================================================

//==============================================================================
// Includes
//==============================================================================
#include "button.hpp"

#include <gpiod.h>

#include <cerrno>
#include <chrono>
#include <cstring>
#include <iostream>

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
int Button::Open(const char* chip_name, unsigned int pin) {
	chip_name_ = chip_name;
	gpio_pin_ = pin;
	chip_ = gpiod_chip_open(this->chip_name_.c_str());
	if (!chip_) {
		std::cerr << "Failed to open GPIO chip: " << strerror(errno) << std::endl;
		return -1;
	}

	// Create request config
	struct gpiod_request_config* req_cfg = gpiod_request_config_new();
	if (!req_cfg) {
		std::cerr << "Failed to create request config" << std::endl;
		gpiod_chip_close(chip_);
		chip_ = nullptr;
		return -1;
	}
	gpiod_request_config_set_consumer(req_cfg, "action_camera_button");

	// Create line config
	struct gpiod_line_config* line_cfg = gpiod_line_config_new();
	if (!line_cfg) {
		std::cerr << "Failed to create line config" << std::endl;
		gpiod_request_config_free(req_cfg);
		gpiod_chip_close(chip_);
		chip_ = nullptr;
		return -1;
	}

	// Create line settings
	struct gpiod_line_settings* settings = gpiod_line_settings_new();
	if (!settings) {
		std::cerr << "Failed to create line settings" << std::endl;
		gpiod_line_config_free(line_cfg);
		gpiod_request_config_free(req_cfg);
		gpiod_chip_close(chip_);
		chip_ = nullptr;
		return -1;
	}
	gpiod_line_settings_set_edge_detection(settings, GPIOD_LINE_EDGE_BOTH);
	gpiod_line_settings_set_bias(settings, GPIOD_LINE_BIAS_PULL_UP);

	// Add settings for our GPIO pin
	int ret = gpiod_line_config_add_line_settings(line_cfg, &this->gpio_pin_, 1, settings);
	if (ret < 0) {
		std::cerr << "Failed to add line settings: " << strerror(errno) << std::endl;
		gpiod_line_settings_free(settings);
		gpiod_line_config_free(line_cfg);
		gpiod_request_config_free(req_cfg);
		gpiod_chip_close(chip_);
		chip_ = nullptr;
		return -1;
	}

	// Request the lines
	line_request_ = gpiod_chip_request_lines(chip_, req_cfg, line_cfg);
	if (!line_request_) {
		std::cerr << "Failed to request GPIO lines: " << strerror(errno) << std::endl;
		gpiod_line_settings_free(settings);
		gpiod_line_config_free(line_cfg);
		gpiod_request_config_free(req_cfg);
		gpiod_chip_close(chip_);
		chip_ = nullptr;
		return -1;
	}

	// Clean up configs (not needed after request)
	gpiod_line_settings_free(settings);
	gpiod_line_config_free(line_cfg);
	gpiod_request_config_free(req_cfg);

	button_monitor_thread_ = std::thread(&Button::buttonMonitor, this);
	button_monitor_active_ = true;

	return 0;
}

int Button::Close() {
	button_state_ = ButtonState::Released;
	if (button_hold_thread_.joinable())
		button_hold_thread_.join();

	button_monitor_active_ = false;
	if (button_monitor_thread_.joinable())
		button_monitor_thread_.join();

	if (line_request_) {
		gpiod_line_request_release(line_request_);
		line_request_ = nullptr;
	}

	if (chip_) {
		gpiod_chip_close(chip_);
		chip_ = nullptr;
	}

	return 0;
}

Button::Button(int hold_duration_ms) {
	hold_duration_ = std::chrono::milliseconds(hold_duration_ms);
}

Button::~Button() {
	Close();
}

//==============================================================================
// Private Functions
//==============================================================================
void Button::buttonMonitor() {
	const int DELAY_IN_NS = 100000000;
	const size_t MAX_EVENTS = 5;
	struct gpiod_edge_event_buffer* event_buffer = gpiod_edge_event_buffer_new(MAX_EVENTS);
	if (!event_buffer) {
		std::cerr << "Failed to create edge event buffer" << std::endl;
		return;
	}

	while (button_monitor_active_) {
		int ret = gpiod_line_request_wait_edge_events(line_request_, DELAY_IN_NS);
		if (!ret)
			continue;
		else if (ret > 0) {
			size_t num_events = gpiod_line_request_read_edge_events(line_request_, event_buffer, MAX_EVENTS);
			for (std::size_t i = 0; i < num_events; i++) {
				struct gpiod_edge_event* event = gpiod_edge_event_buffer_get_event(event_buffer, i);

				if (gpiod_edge_event_get_event_type(event) == GPIOD_EDGE_EVENT_FALLING_EDGE) {
					button_state_ = ButtonState::Pressed;
					button_hold_thread_ = std::thread(&Button::buttonEventHandler, this);
				}
				else if (gpiod_edge_event_get_event_type(event) == GPIOD_EDGE_EVENT_RISING_EDGE) {
					button_state_ = ButtonState::Released;

					if (button_hold_thread_.joinable())
						button_hold_thread_.join();
				}
			}
		}
		else {
			std::cerr << "Error waiting for GPIO events: " << strerror(errno) << std::endl;
		}
	}

	gpiod_edge_event_buffer_free(event_buffer);
}

void Button::buttonEventHandler() {
	const int BUTTON_MONITOR_RESOLUTION = 100;
	int durationMs = 0;
	bool isHeld = false;

	while (button_state_ == ButtonState::Pressed) {
		std::this_thread::sleep_for(std::chrono::milliseconds(BUTTON_MONITOR_RESOLUTION));
		durationMs += BUTTON_MONITOR_RESOLUTION;
		if (std::chrono::milliseconds(durationMs) >= hold_duration_) {
			durationMs = 0;
			if (on_hold_) {
				isHeld = true;
				on_hold_();
			}
		}
	}

	if (!isHeld && on_press_)
		on_press_();
	else if (on_release_)
		on_release_();
}

//==============================================================================
// Task Handler
//==============================================================================

//==============================================================================
// Interrupt
//==============================================================================
