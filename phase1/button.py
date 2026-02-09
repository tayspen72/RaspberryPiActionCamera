from gpiozero import Button

# Button Behavior
BUTTON_GPIO = 23
BOUNCE_TIME = 0.1
HOLD_TIME = 2
HOLD_REPEAT = 3


class ButtonHandler:
    def __init__(self):
        self.button = Button(
            BUTTON_GPIO,
            bounce_time=BOUNCE_TIME,
            hold_time=HOLD_TIME,
            hold_repeat=HOLD_REPEAT,
        )
        self.button.when_released = self._on_released
        self.button.when_held = self._on_held

        # App wires these in after construction
        self.on_press = None   # called on button release (short press)
        self.on_hold = None    # called on button hold

    def _on_released(self):
        if self.on_press:
            self.on_press()

    def _on_held(self):
        if self.on_hold:
            self.on_hold()
