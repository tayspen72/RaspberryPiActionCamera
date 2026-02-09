use crate::button;
use crate::display::Display;

use button::{Button, ButtonEvent};

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

pub struct Camera {
    button: Button,
    display: Display,
    current_mode: Mode,
    target_mode: Mode,
    state: State,
}

impl Camera {
    pub fn new(button: Button, display: Display) -> std::io::Result<Self> {
        let mut camera = Camera {
            button,
            display,
            current_mode: Mode::Standby,
            target_mode: Mode::Standby,
            state: State::None,
        };

        camera.display.write_mode(camera.current_mode);
        camera.display.write_state(camera.state);

        Ok(camera)
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
                println!("camera: stopping standby mode");
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
                println!("camera: starting standby mode");
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
                println!("camera: input has no effect in standby mode");
            }
        };
    }

    pub fn event_mode_start(&mut self) {
        println!("camera: waiting for event");
        self.state = State::WaitingForEvent;
        self.display.write_state(self.state);
        // TODO: start event recording
    }

    pub fn event_mode_stop(&mut self) {
        println!("camera: cleaning up event mode before next mode");
        // TODO: stop event recording
        self.state = State::None;
        self.display.write_state(self.state);
    }

    pub fn event_mode_trigger(&mut self) {
        println!("camera: event trigger received");
        self.state = State::SavingEvent;
        self.display.write_state(self.state);
        // TODO: stop recording and save event recording

        // event stored, start recording again
        self.event_mode_start();
    }

    pub fn record_mode_start(&mut self) {
        println!("camera: starting recording");
        self.state = State::Recording;
        self.display.write_state(self.state);
        // TODO: start recording
    }

    pub fn record_mode_stop(&mut self) {
        println!("camera: stopping recording");
        self.state = State::Stopped;
        self.display.write_state(self.state);
        // TODO: stop recording
    }

    pub fn run(&mut self) -> std::io::Result<()> {
        loop {
            match self.button.receiver.recv() {
                Ok(ButtonEvent::ShortPress) => self.input_handler(),
                Ok(ButtonEvent::Hold) => self.set_target_mode(Camera::get_next_mode(self.get_target_mode())),
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
