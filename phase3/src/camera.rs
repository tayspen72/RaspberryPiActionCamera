use chrono;
use std::fs;
use std::path::{Path, PathBuf};
use std::process::{Child, Command, Stdio};

pub struct Camera {
    process: Option<Child>,
    events_path: PathBuf,
    videos_path: PathBuf,
}

impl Camera {
    pub fn new<P: AsRef<Path>>(output_path: P) -> std::io::Result<Self> {
        let output_path = output_path.as_ref().to_path_buf();
        let events_path = output_path.join("events");
        let videos_path = output_path.join("videos");

        fs::create_dir_all(&events_path).expect("Failed to create events directory");
        fs::create_dir_all(&videos_path).expect("Failed to create videos directory");

        Ok(Camera {
            process: None,
            events_path: events_path,
            videos_path: videos_path,
        })
    }

    fn get_timestamp(&self) -> String {
        let now = chrono::Local::now();
        now.format("%Y.%m.%d_%H.%M.%S").to_string()
    }

    pub fn start_circular_buffer(&mut self) -> std::io::Result<()> {
        if self.process.is_some() {
            return Err(std::io::Error::new(std::io::ErrorKind::AlreadyExists, "camera process is already running"));
        }

        let output_file = self.events_path.join("tmp_event.h264");

        let child = Command::new("rpicam-vid")
            .args(["-t", "0", "--inline", "--circular", "120", "-o", output_file.to_str().unwrap()])
            .stdout(Stdio::null())
            .stderr(Stdio::null())
            .spawn()?;

        self.process = Some(child);

        Ok(())
    }

    pub fn event_trigger(&mut self) -> std::io::Result<()> {
        let _ = self.stop();

        let tmp_filename = self.events_path.join("tmp_event.h264");
        if tmp_filename.exists() {
            let timestamp = self.get_timestamp();
            let new_filename = format!("event_{}.h264", timestamp);
            let new_path = self.events_path.join(new_filename);
            let _ = fs::rename(tmp_filename, new_path);
        }

        let _ = self.start_circular_buffer();

        Ok(())
    }

    pub fn start_recording(&mut self) -> std::io::Result<()> {
        if self.process.is_some() {
            return Err(std::io::Error::new(std::io::ErrorKind::AlreadyExists, "camera process is already running"));
        }

        let timestamp = self.get_timestamp();
        let filename = format!("video_{}.h264", timestamp);
        let output_file = self.videos_path.join(filename);

        let child = Command::new("rpicam-vid")
            .args(["-t", "0", "-o", output_file.to_str().unwrap()])
            .stdout(Stdio::null())
            .stderr(Stdio::null())
            .spawn()?;

        self.process = Some(child);

        Ok(())
    }

    pub fn stop(&mut self) -> std::io::Result<()> {
        if let Some(mut child) = self.process.take() {
            child.kill()?;
            child.wait()?;
        }

        Ok(())
    }
}
