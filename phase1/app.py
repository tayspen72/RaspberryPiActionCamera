from enum import Enum
import signal
import sys

from button import ButtonHandler
from camera import Camera
from display import Display

# Version History
FIRMWARE_VERSION = "1.0.0"

class Mode(Enum):
    EVENT    = 0
    RECORD   = 1
    STANDBY  = 2

class State(Enum):
    WAITING_FOR_EVENT  = 0
    SAVING_EVENT       = 1
    RECORDING          = 2
    STOPPED            = 3
    NONE               = 4

class ActionCam:
    def __init__(self):
        print("Starting Raspberry Pi Action Camera")
        print("FW: " + FIRMWARE_VERSION)

        self.CurrentMode = Mode.STANDBY
        self.TargetMode = Mode.STANDBY
        self.IsModePending = False
        self.IsActive = False

        self.CurrentState = State.NONE

        self.button = ButtonHandler()
        self.button.on_press = self.button_handler
        self.button.on_hold = self.button_held_handler

        self.display = Display()
        self.display.show_mode(self.CurrentMode.value)
        self.display.show_state(self.CurrentState.value)
        self.camera = Camera()

    # Button handler is called when button is released.
    # Could have been a press or a hold - respond according to IsModePending flag
    def button_handler(self):
        if self.IsModePending:
            self.IsModePending = False
            self.check_safe_stop()

            print(f"Setting current mode to {self.TargetMode}")
            self.CurrentMode = self.TargetMode
            self.display.show_mode(self.CurrentMode.value)

            match self.CurrentMode:
                case Mode.EVENT:
                    self.IsActive = True
                    self.camera.start_passive()
                    self._set_state(State.WAITING_FOR_EVENT)
                case Mode.RECORD:
                    self.IsActive = True
                    self.camera.start_active()
                    self._set_state(State.RECORDING)
                case Mode.STANDBY:
                    self._set_state(State.NONE)

        else:
            match self.CurrentMode:
                case Mode.EVENT:
                    print("Received event trigger!")
                    self._set_state(State.SAVING_EVENT)
                    self.camera.trigger_event()
                    self._set_state(State.WAITING_FOR_EVENT)
                case Mode.RECORD:
                    if self.IsActive:
                        print("Pausing active recording")
                        self.IsActive = False
                        self.camera.stop()
                        self._set_state(State.STOPPED)
                    else:
                        print("Resuming active recording")
                        self.IsActive = True
                        self.camera.start_active()
                        self._set_state(State.RECORDING)
                case Mode.STANDBY:
                    print("Press has no effect in standby mode")
                    pass

    def button_held_handler(self):
        match self.TargetMode:
            case Mode.EVENT:
                self.TargetMode = Mode.RECORD
            case Mode.RECORD:
                self.TargetMode = Mode.STANDBY
            case Mode.STANDBY:
                self.TargetMode = Mode.EVENT

        self.display.show_mode(self.TargetMode.value)
        self.IsModePending = True

    def _set_state(self, new_state: State):
        self.CurrentState = new_state
        self.display.show_state(new_state.value)

    def check_safe_stop(self):
        if self.IsActive:
            print("Stopping current recording to change modes")
            self.camera.stop()
            self._set_state(State.STOPPED)
        self.IsActive = False

def signal_handler(sig, frame):
    sys.exit(0)

if __name__ == "__main__":
    cam = ActionCam()

    signal.signal(signal.SIGINT, signal_handler)
    signal.pause()
