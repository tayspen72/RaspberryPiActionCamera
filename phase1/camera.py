from pathlib import Path
import subprocess
from subprocess import DEVNULL
import time

# Recording Constants
EVENT_PRE_LENGTH = 2 * 60 * 30
EVENT_POST_LENGTH = 30
EVENT_FILENAME = "%Y-%m-%d.%H%M%S.h264"
EVENT_FILEPATH = "./events/"
EVENT_TEMP_FILENAME = "event_tmp.h264"
VIDEO_FILENAME = "%Y-%m-%d.%H%M%S.h264"
VIDEO_FILEPATH = "./videos/"


class Camera:
    def __init__(self):
        self.process = None

        event_dir = Path(EVENT_FILEPATH)
        event_dir.mkdir(parents=True, exist_ok=True)
        video_dir = Path(VIDEO_FILEPATH)
        video_dir.mkdir(parents=True, exist_ok=True)

    def start_recording(self):
        timestamp = time.strftime(VIDEO_FILENAME)
        output_file = Path(VIDEO_FILEPATH) / timestamp

        self.process = subprocess.Popen(["rpicam-vid", "-t", "0", "-o", str(output_file)],
            stdout=DEVNULL,
            stderr=DEVNULL
        )

    def start_event_capture(self):
        output_file = Path(EVENT_FILEPATH) / EVENT_TEMP_FILENAME

        self.process = subprocess.Popen(["rpicam-vid", "-t", "0", "--inline", "--circular", "120", "-o", str(output_file)],
            stdout=DEVNULL,
            stderr=DEVNULL
        )

    def trigger_event(self):
        if self.process:
            self.process.terminate()
            self.process.wait()
            self.process = None

        tmp_filename = Path(EVENT_FILEPATH) / EVENT_TEMP_FILENAME
        if tmp_filename.exists():
            timestamp = time.strftime(EVENT_FILENAME)
            output_file = Path(EVENT_FILEPATH) / timestamp
            tmp_filename.rename(output_file)

        self.start_event_capture()

    def stop(self):
        if self.process:
            self.process.terminate()
            self.process.wait()
            self.process = None
