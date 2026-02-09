/*
 * tayspen72
 *
 * button.hpp
 */

#ifndef BUTTON_HPP_
#define BUTTON_HPP_

//==============================================================================
// Notes
//==============================================================================

//==============================================================================
// Definitions
//==============================================================================

//==============================================================================
// Includes
//==============================================================================
#include <gpiod.h>

#include <atomic>
#include <chrono>
#include <functional>
#include <string>
#include <thread>

//==============================================================================
// Enumerations and Structures
//==============================================================================

//==============================================================================
// Classes
//==============================================================================
class Button
{
	public:
		int Open(const char* chip_name, unsigned int gpio_pin);
		int Close();
		void SetOnHold(std::function<void()> on_hold) {
			on_hold_ = on_hold;
		}
		void SetOnPress(std::function<void()> on_press) {
			on_press_ = on_press;
		}
		void SetOnRelease(std::function<void()> on_release) {
			on_release_ = on_release;
		}
		Button(int hold_duration_ms);
		~Button();

	private:
		enum ButtonEvent
		{
			Presse,
			Release,
			Hold
		};
		enum ButtonState
		{
			Pressed = 0,
			Released = 1
		};
		std::string chip_name_;
		unsigned int gpio_pin_;
		struct gpiod_chip* chip_;
		struct gpiod_line_request* line_request_;
		std::chrono::milliseconds hold_duration_;
		std::atomic<ButtonState> button_state_{ ButtonState::Released };
		std::thread button_hold_thread_;
		std::thread button_monitor_thread_;
		std::atomic<bool> button_monitor_active_{ false };
		std::function<void()> on_hold_;
		std::function<void()> on_press_;
		std::function<void()> on_release_;

		void buttonMonitor();
		void buttonEventHandler();
		void buttonHoldHandler() {
			if (on_hold_)
				on_hold_();
		}
		void buttonPressHandler() {
			if (on_press_)
				on_press_();
		}
		void buttonReleaseHandler() {
			if (on_release_)
				on_release_();
		}
};

//==============================================================================
// Variables
//==============================================================================

//==============================================================================
// Macro Functions
//==============================================================================

#endif /* BUTTON_HPP_ */
