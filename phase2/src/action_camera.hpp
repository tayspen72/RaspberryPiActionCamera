/*
 * tayspen72
 *
 * action_camera.hpp
 */

#ifndef ACTION_CAMERA_HPP_
#define ACTION_CAMERA_HPP_

//==============================================================================
// Notes
//==============================================================================

//==============================================================================
// Definitions
//==============================================================================

//==============================================================================
// Includes
//==============================================================================
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <string>

//==============================================================================
// Enumerations and Structures
//==============================================================================

//==============================================================================
// Classes
//==============================================================================
class ActionCamera
{
	public:
		typedef enum
		{
			Event,
			Record,
			Standby
		} Mode;

		typedef enum
		{
			// Event
			WaitingForEvent,
			SavingEvent,
			// Record
			Recording,
			Stopped,
			// Standby
			None
		} State;

		typedef void (*ModeChangeCallback)(Mode mode);
		typedef void (*StateChangeCallback)(State state);

		Mode GetCurrentMode() { return current_mode_; };
		Mode GetTargetMode() { return target_mode_; };
		Mode GetNextMode(Mode mode);
		State GetState() { return state_; };
		void InputHandler();
		void SetTargetMode(Mode mode);
		void StopCurrentMode();
		void StartNextMode();
		void SetCurrentModeChangeCallback(ModeChangeCallback callback) { current_mode_change_callback_ = callback; }
		void SetTargetModeChangeCallback(ModeChangeCallback callback) { target_mode_change_callback_ = callback; }
		void SetStateChangeCallback(StateChangeCallback callback) { state_change_callback_ = callback; }

		ActionCamera();
		~ActionCamera();

	private:
		const std::string EVENT_DEFAULT_NAME = "tmp_event.h264";
		const std::string EVENTS_FILEPATH = "/home/pi/Projects/RaspberryPiActionCamera/phase2/events/";
		const std::string VIDEOS_FILEPATH = "/home/pi/Projects/RaspberryPiActionCamera/phase2/videos/";
		Mode current_mode_ = Mode::Standby;
		Mode target_mode_ = Mode::Standby;
		State state_ = State::None;
		ModeChangeCallback current_mode_change_callback_ = nullptr;
		ModeChangeCallback target_mode_change_callback_ = nullptr;
		StateChangeCallback state_change_callback_ = nullptr;
		pid_t recording_pid_ = -1;
		int file_counter_ = 0;

		void eventModeStart();
		void eventModeStop();
		void eventModeTrigger();
		void recordModeStart();
		void recordModeStop();
		void currentModeChangeNotify(Mode mode);
		void targetModeChangeNotify(Mode mode);
		void stateChangeNotify(State state);

		std::string getCurrentTimestamp() {
			time_t now = time(nullptr);
			char buf[80];
			strftime(buf, sizeof(buf), "%Y-%m-%d.%H%M%S", localtime(&now));
			return std::string(buf);
		}

		pid_t getPidOfRpicamVid() {
			pid_t pid = -1;
			FILE* fp = popen("pgrep rpicam-vid", "r");
			if (fp) {
				fscanf(fp, "%d", &pid);
				pclose(fp);
			}
			return pid;
		}
};

//==============================================================================
// Variables
//==============================================================================

//==============================================================================
// Macro Functions
//==============================================================================

#endif /* ACTION_CAMERA_HPP_ */
