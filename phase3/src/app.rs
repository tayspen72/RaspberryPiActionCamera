use crate::button::{Button, ButtonEvent};
use crate::camera::Camera;
use crate::display::Display;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Mode {
    Event,
    Record,
    Standby,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum State {
    WaitingForEvent,
    SavingEvent,
    Recording,
    Stopped,
    None,
}

pub struct ActionCamera {
    button: Button,
    camera: Camera,
    display: Display,
    current_mode: Mode,
    target_mode: Mode,
    state: State,
}

impl ActionCamera {
    pub fn new(button: Button, camera: Camera, display: Display) -> std::io::Result<Self> {
        let mut app = ActionCamera {
            button,
            camera,
            display,
            current_mode: Mode::Standby,
            target_mode: Mode::Standby,
            state: State::None,
        };

        app.display.write_mode(app.current_mode);
        app.display.write_state(app.state);

        Ok(app)
    }

    pub fn get_next_mode(mode: Mode) -> Mode {
        match mode {
            Mode::Event => Mode::Record,
            Mode::Record => Mode::Standby,
            Mode::Standby => Mode::Event,
        }
    }

    pub fn get_target_mode(&self) -> Mode {
        self.target_mode
    }

    pub fn set_target_mode(&mut self, mode: Mode) {
        self.target_mode = mode;
        self.display.write_mode(mode);
    }

    pub fn stop_current_mode(&mut self) {
        if self.target_mode == self.current_mode {
            return;
        }

        match self.current_mode {
            Mode::Event => {
                self.event_mode_stop();
            }
            Mode::Record => {
                self.record_mode_stop();
            }
            Mode::Standby => {
                println!("app: stopping standby mode");
                self.state = State::None;
                self.display.write_state(self.state);
            }
        };
    }

    pub fn start_next_mode(&mut self) {
        if self.target_mode == self.current_mode {
            return;
        }

        match self.target_mode {
            Mode::Event => {
                self.event_mode_start();
            }
            Mode::Record => {
                self.record_mode_start();
            }
            Mode::Standby => {
                println!("app: starting standby mode");
                self.state = State::None;
                self.display.write_state(self.state);
            }
        };

        self.current_mode = self.target_mode;
    }

    pub fn input_handler(&mut self) {
        match self.current_mode {
            Mode::Event => {
                self.event_mode_trigger();
            }
            Mode::Record => {
                if self.state == State::Recording {
                    self.record_mode_stop();
                } else {
                    self.record_mode_start();
                }
            }
            Mode::Standby => {
                println!("app: input has no effect in standby mode");
            }
        };
    }

    pub fn event_mode_start(&mut self) {
        println!("app: waiting for event");
        self.state = State::WaitingForEvent;
        self.display.write_state(self.state);
        let _ = self.camera.start_circular_buffer();
    }

    pub fn event_mode_stop(&mut self) {
        println!("app: cleaning up event mode before next mode");
        let _ = self.camera.stop();
        self.state = State::None;
        self.display.write_state(self.state);
    }

    pub fn event_mode_trigger(&mut self) {
        println!("app: event trigger received");
        self.state = State::SavingEvent;
        self.display.write_state(self.state);

        // event_trigger will stop recording, save the file, and restart the recording
        let _ = self.camera.event_trigger();

        self.state = State::WaitingForEvent;
        self.display.write_state(self.state);
    }

    pub fn record_mode_start(&mut self) {
        println!("app: starting recording");
        self.state = State::Recording;
        self.display.write_state(self.state);
        let _ = self.camera.start_recording();
    }

    pub fn record_mode_stop(&mut self) {
        println!("app: stopping recording");
        self.state = State::Stopped;
        self.display.write_state(self.state);
        let _ = self.camera.stop();
    }

    pub fn run(&mut self) -> std::io::Result<()> {
        loop {
            match self.button.receiver.recv() {
                Ok(ButtonEvent::ShortPress) => self.input_handler(),
                Ok(ButtonEvent::Hold) => self.set_target_mode(ActionCamera::get_next_mode(self.get_target_mode())),
                Ok(ButtonEvent::HoldRelease) => {
                    self.stop_current_mode();
                    self.start_next_mode();
                }
                Err(_) => break,
            }
        }

        Ok(())
    }
}
