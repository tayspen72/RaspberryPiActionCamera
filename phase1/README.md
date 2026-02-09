# Phase 1: Python Implementation

> Rapid prototyping implementation using Python's rich ecosystem for embedded Linux. Demonstrates quick iteration with `gpiozero`, `picamera2`, and a custom I²C display driver.

## Overview

This phase prioritizes development speed and code clarity, making it ideal for prototyping and educational purposes. It leverages Python's high-level abstractions while still maintaining direct hardware control where needed.

**Implementation highlights:**
- Hardware abstraction through established Python libraries
- Custom OLED driver for fine-grained display control
- Circular buffer implementation using picamera2's native output handler
- Event-driven architecture with callback-based button handling

## Architecture

The codebase is structured into discrete modules, each handling a specific hardware component or system function:

```
app.py              # Main controller and state machine
├── button.py       # GPIO button interface with debouncing
├── camera.py       # Video capture and circular buffering
├── display.py      # SSD1306 OLED driver (I²C communication)
└── font.py         # Bitmap font renderer (8×5 character set)
```

### Core Components

**State Management**
- `Mode` enumeration: EVENT, RECORD, STANDBY
- `State` enumeration: Recording status per mode
- Centralized state machine in `app.py` with atomic transitions

**Event System**
- Button callbacks: `on_press` (short press) and `on_hold` (long press)
- Mode-specific behavior routing
- Non-blocking event handlers

**Hardware Interfaces**
- **Button**: `gpiozero.Button` with 100ms debounce, 1.25s hold detection
- **Display**: Direct I²C control via `fcntl` (no external display library)
- **Camera**: `picamera2` with H.264 encoder and circular output buffer

## Display Layout

The 128×32 pixel OLED uses a two-tier layout:

**Top row** — Mode indicators:
```
EVENT          RECORD          STANDBY
  ↑              ↑               ↑
col 0          col 40         col 87
(active mode is inverted)
```

**Bottom rows** — Dynamic state information:
- RECORDING
- STOPPED
- WAITING FOR EVENT
- SAVING EVENT

## Quick Start

### Prerequisites

```bash
# Ensure user is in gpio group (if not already done)
sudo usermod -a -G gpio $USER
# Log out and back in
```

### Installation

```bash
# Navigate to phase1 directory
cd phase1

# Install Python dependencies
pip3 install -r requirements.txt
```

### Running the Application

```bash
python3 app.py
```

The camera will start in STANDBY mode. Long-press the button to cycle through modes.

## Hardware Configuration

| Component | Interface | Configuration | Library |
|-----------|-----------|---------------|---------|
| Push button | GPIO 23 | Active-low, internal pull-up | `gpiozero` |
| SSD1306 OLED | I²C Bus 1 | Address 0x3C, 128×32 pixels | Custom driver |
| Camera Module 2 | CSI | 1920×1080 @ 30fps, H.264 | `picamera2` |

## Module Reference

### `app.py` — Main Controller

Orchestrates the entire system, managing:
- Mode state machine and transitions
- Button event routing to mode-specific handlers
- Display updates synchronized with state changes
- Graceful shutdown on SIGINT

**Key functions:**
- `cycle_mode()` — Mode selection with long-press
- `handle_event_press()` / `handle_record_press()` — Mode-specific actions
- `update_display()` — Refresh OLED with current mode and state

### `button.py` — GPIO Interface

Wraps `gpiozero.Button` with project-specific configuration:
- 100ms bounce time (hardware debouncing supplement)
- 1.25s hold time threshold
- Callback registration for press and hold events

### `camera.py` — Video Capture

Manages camera operations with two capture strategies:

**Standard recording:**
- Direct file output with H.264 encoding
- Start/stop control for RECORD mode

**Circular buffer:**
- Continuously overwrites ring buffer
- Maintains last N seconds in memory (configurable)
- Flush-to-file on event trigger (EVENT mode)

### `display.py` — SSD1306 Driver

Full-featured OLED driver implemented from scratch:
- Direct I²C communication using `fcntl.ioctl()`
- Framebuffer management (1024-byte display buffer)
- Text rendering with custom font system
- Inversion support for mode highlighting

**Why custom driver?**  
Provides complete control over display behavior without external dependencies, demonstrating low-level I²C protocol handling in Python.

### `font.py` — Bitmap Font

8×5 pixel monospace font for uppercase ASCII (A-Z):
- Space-efficient bitmap encoding
- Column-major format for efficient rendering
- Designed for high contrast on monochrome OLED

## Implementation Comparison

This phase trades runtime performance for development velocity. See how it compares to other implementations:

| Aspect | Phase 1 (Python) | Phase 2 (C++) | Phase 3 (Rust) |
|--------|------------------|---------------|----------------|
| **Button handling** | `gpiozero` (high-level) | `libgpiod` (low-level) | `gpiod` crate |
| **Display driver** | `fcntl` I²C | POSIX I/O | `embedded-hal` traits |
| **Camera control** | `picamera2` (Python API) | `rpicam-vid` (subprocess) | `rpicam-vid` (subprocess) |
| **Boot time** | ~8-12s | ~3-5s | ~3-5s |
| **Memory footprint** | ~45MB (Python interpreter) | ~8MB | ~5MB |
| **Development time** | Fastest | Moderate | Moderate |
| **Type safety** | Runtime (duck typing) | Compile-time (manual) | Compile-time (enforced) |

## Development Notes

**Advantages of Python:**
- Rapid iteration with interpreted execution
- Rich library ecosystem (`picamera2`, `gpiozero`)
- Excellent for prototyping hardware interactions
- Clear, readable code for documentation and learning

**Trade-offs:**
- Higher memory overhead from Python runtime
- Slower boot time compared to compiled implementations
- GC pauses potentially affecting real-time responsiveness
- No compile-time type checking

**Best used for:**
- Initial hardware validation and testing
- Algorithm development and tuning (buffer sizes, timing)
- Educational demonstrations
- Prototypes where performance isn't critical

## Troubleshooting

**Camera not detected:**
```bash
# Check camera connection
vcgencmd get_camera

# Should show: supported=1 detected=1
```

**I²C device not found:**
```bash
# List I²C devices
i2cdetect -y 1

# Should show device at 0x3C
```

**Permission denied on GPIO:**
```bash
# Verify group membership
groups

# Should include 'gpio'
```

## Next Steps

Once this implementation is working, explore:
- **Phase 2 (C++)** for production-ready performance
- **Phase 3 (Rust)** for memory safety and modern embedded patterns
- Compare boot times and memory usage across implementations
- Profile power consumption differences

---

**Key Learning Outcomes:**  
Python GPIO libraries • I²C protocol basics • Video circular buffering • Event-driven embedded design • Rapid hardware prototyping
