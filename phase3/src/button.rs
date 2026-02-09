use gpiod::{Bias, Chip, EdgeDetect, Options};
use std::sync::mpsc::{self, Receiver, RecvTimeoutError};
use std::sync::{
    Arc,
    atomic::{AtomicBool, Ordering},
};
use std::thread::{self, JoinHandle};
use std::time::Duration;

// NOTE: GPIO pin is now taken from the phase2 main.cpp
const BUTTON_PIN: u32 = 23;

#[derive(Debug, PartialEq, Eq)]
enum RawButtonEvent {
    Pressed,
    Released,
}

#[derive(Debug, PartialEq, Eq)]
pub enum ButtonEvent {
    ShortPress,
    Hold,
    HoldRelease,
}

pub struct Button {
    pub receiver: Receiver<ButtonEvent>,
    stop_flag: Arc<AtomicBool>,
    monitor_thread: Option<JoinHandle<()>>,
    logic_thread: Option<JoinHandle<()>>,
}

impl Button {
    pub fn new(hold_duration_ms: u64) -> std::io::Result<Self> {
        let chip = Chip::new("gpiochip0")?;
        let opts = Options::input(&[BUTTON_PIN]).edge(EdgeDetect::Both).bias(Bias::PullUp).consumer("action-camera-button");
        let mut lines = chip.request_lines(opts)?;

        let (raw_sender, raw_receiver) = mpsc::channel();
        let (user_sender, user_receiver) = mpsc::channel();

        let stop_flag = Arc::new(AtomicBool::new(false));
        let monitor_stop_flag = stop_flag.clone();
        let logic_stop_flag = stop_flag.clone();

        let monitor_thread = thread::spawn(move || {
            while !monitor_stop_flag.load(Ordering::Relaxed) {
                match lines.read_event() {
                    Ok(event) => {
                        let raw_event = match event.edge {
                            gpiod::Edge::Falling => RawButtonEvent::Pressed,
                            gpiod::Edge::Rising => RawButtonEvent::Released,
                        };
                        if raw_sender.send(raw_event).is_err() {
                            break; // Logic thread disconnected
                        }
                    }
                    Err(_) => break, // GPIO error
                }
            }
        });

        let logic_thread = thread::spawn(move || {
            loop {
                if logic_stop_flag.load(Ordering::Relaxed) {
                    break;
                }
                // Wait for a press
                match raw_receiver.recv() {
                    Ok(RawButtonEvent::Pressed) => {
                        let hold_duration = Duration::from_millis(hold_duration_ms);
                        let cycle_duration = Duration::from_millis(1250); // How often to send Hold events

                        // Now wait for release or hold
                        match raw_receiver.recv_timeout(hold_duration) {
                            Ok(RawButtonEvent::Released) => {
                                // It was a short press
                                if user_sender.send(ButtonEvent::ShortPress).is_err() {
                                    break;
                                }
                            }
                            Err(RecvTimeoutError::Timeout) => {
                                // Hold detected, send the first hold event
                                if user_sender.send(ButtonEvent::Hold).is_err() {
                                    break;
                                }

                                // Now loop to send cycling hold events
                                loop {
                                    match raw_receiver.recv_timeout(cycle_duration) {
                                        Ok(RawButtonEvent::Released) => {
                                            if user_sender.send(ButtonEvent::HoldRelease).is_err() {
                                                break;
                                            }
                                            break; // Exit cycle loop
                                        }
                                        Err(RecvTimeoutError::Timeout) => {
                                            if user_sender.send(ButtonEvent::Hold).is_err() {
                                                break;
                                            }
                                        }
                                        _ => {
                                            break;
                                        } // Disconnected or other error
                                    }
                                }
                            }
                            _ => {
                                break;
                            } // Disconnected or other error
                        }
                    }
                    Ok(RawButtonEvent::Released) => {
                        // We got a release when we weren't expecting one, ignore.
                    }
                    Err(_) => {
                        break; // Monitor thread disconnected
                    }
                }
            }
        });

        Ok(Button {
            receiver: user_receiver,
            stop_flag,
            monitor_thread: Some(monitor_thread),
            logic_thread: Some(logic_thread),
        })
    }
}

impl Drop for Button {
    fn drop(&mut self) {
        self.stop_flag.store(true, Ordering::Relaxed);

        if let Some(handle) = self.logic_thread.take() {
            handle.join().unwrap();
        }
        // Dropping the logic thread sender will cause the monitor thread to exit its loop
        if let Some(handle) = self.monitor_thread.take() {
            handle.join().unwrap();
        }
    }
}
