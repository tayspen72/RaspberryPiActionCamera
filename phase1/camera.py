from pathlib import Path
from picamera2 import Picamera2
from picamera2.encoders import H264Encoder
from picamera2.outputs import CircularOutput
import time

# Recording Constants
EVENT_PRE_LENGTH = 2 * 60 * 30
EVENT_POST_LENGTH = 30
EVENT_FILENAME = "%Y-%m-%d.%H%M%S.h264"
EVENT_FILEPATH = "./events/"
VIDEO_FILENAME = "%Y-%m-%d.%H%M%S.h264"
VIDEO_FILEPATH = "./videos/"


class Camera:
    def __init__(self):
        self.picam2 = Picamera2()
        config = self.picam2.create_video_configuration()
        self.picam2.configure(config)

        self.encoder = H264Encoder()
        self.circular_output = CircularOutput(buffersize=EVENT_PRE_LENGTH)

        event_dir = Path(EVENT_FILEPATH)
        event_dir.mkdir(parents=True, exist_ok=True)
        video_dir = Path(VIDEO_FILEPATH)
        video_dir.mkdir(parents=True, exist_ok=True)

    def start_active(self):
        filename = time.strftime(VIDEO_FILENAME)
        self.picam2.start_recording(self.encoder, VIDEO_FILEPATH + filename)

    def start_passive(self):
        self.picam2.start_recording(self.encoder, self.circular_output)

    def trigger_event(self):
        timestamp = time.strftime(EVENT_FILENAME)
        self.circular_output.fileoutput = EVENT_FILEPATH + timestamp
        self.circular_output.start()

        # Hold long enough to capture post-event footage
        time.sleep(EVENT_POST_LENGTH)

        self.circular_output.stop()

    def stop(self):
        self.picam2.stop_recording()
