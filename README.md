# Raspberry Pi Action Camera

> A battery-powered action camera built on Raspberry Pi Zero 2 W with three capture modes, single-button control, and three complete language implementations for comparative embedded systems development.

![Project Status](https://img.shields.io/badge/status-active-success.svg)
![License](https://img.shields.io/badge/license-MIT-blue.svg)

## Overview

This project implements a fully functional action camera using a Raspberry Pi Zero 2 W, exploring embedded Linux development through three progressive implementations in Python, C++, and Rust. It's designed as both a practical device and a learning platform for comparing language ecosystems in embedded contexts.

**Key Features:**
- Three distinct capture modes optimized for different use cases
- Pre-event buffering captures action *before* button press
- Single-button interface with intelligent state management
- Real-time OLED feedback
- Complete implementations in Python, C++, and Rust for performance comparison

## How It Works

### Operating Modes

| Mode | Description | Use Case |
|------|-------------|----------|
| **STANDBY** | Low-power idle state | Battery conservation between sessions |
| **RECORD** | Toggle recording on/off | Traditional action camera behavior |
| **EVENT** | Continuous circular buffer with event capture | Capture unexpected moments retroactively |

**Control Interface:**
- **Short press** — Trigger action in current mode (start/stop recording or save event)
- **Long press (1.25s)** — Cycle through modes

**Display Feedback:**
- Top row: Mode indicators (active mode highlighted)
- Bottom row: Current state (RECORDING/STOPPED/WAITING/SAVING)

### Event Capture Mode

The EVENT mode implements a circular buffer that continuously records, keeping the last N seconds in memory. When you press the button, it saves what *just happened*—essential for capturing unpredictable action without leaving the camera running continuously.

## Hardware

### Bill of Materials

| Component | Part Number | Purpose |
|-----------|-------------|---------|
| Single-board computer | Raspberry Pi Zero 2 W | Quad-core ARM Cortex-A53, WiFi/BT |
| Camera | Raspberry Pi Camera Module 2 | 8MP, CSI interface, good low-light performance |
| Display | SSD1306 OLED (128×32) | I²C interface, low power consumption |
| Enclosure | Official Pi Zero Case | Camera module integration, thermal management |
| Control panel | Custom PCB | Hardware debouncing, clean GPIO interface |
| Power | USB power bank (5V/2A+) | Any standard power bank works |

### Pin Configuration

```
GPIO 23 → Push button (active-low, internal pull-up enabled)
I²C Bus 1, Address 0x3C → SSD1306 OLED display
CSI Interface → Camera Module 2
```

## Software Architecture

### Three-Phase Implementation Study

This project implements identical functionality in three languages to explore the trade-offs between development speed, runtime performance, and code safety in embedded systems.

| Phase | Language | Strengths | Best For |
|-------|----------|-----------|----------|
| **Phase 1** | Python | Rapid prototyping, rich libraries | Proof-of-concept, education |
| **Phase 2** | C++ | Direct hardware control, mature ecosystem | Production systems, performance-critical |
| **Phase 3** | Rust | Memory safety, modern tooling | Safety-critical, maintainable embedded code |

**Comparative Metrics:**
- Boot-to-capture latency
- Idle and active power consumption
- Memory footprint and binary size
- Development and debug experience
- Code maintainability over time

### Design Highlights

**State Machine Architecture**
- Debounced button handling (100ms window)
- Mode preview system with visual confirmation
- Context-aware button behavior per mode
- Thread-safe state transitions

**Hardware Abstraction**
- Centralized GPIO and I²C configuration
- Pin assignments defined as constants
- Easy hardware swapping through configuration

**Circular Buffer Implementation**
- Python: `collections.deque` with max length
- C++: Manual memory management with ring buffer
- Rust: `VecDeque` with pre-allocated capacity

## Getting Started

### Prerequisites

```bash
# Add user to GPIO group (required for hardware access)
sudo usermod -a -G gpio $USER
# Log out and back in for group changes to take effect
```

### Installation & Usage

#### Phase 1: Python Implementation

```bash
cd phase1
pip3 install -r requirements.txt
python3 app.py
```

#### Phase 2: C++ Implementation

```bash
cd phase2
cmake -B build
cd build
make
./bin/action_camera
```

## Development Roadmap

### Planned Enhancements

- **AI Integration:** CameraTrapAI for automated wildlife species detection
- **Smart Capture Modes:** Context-aware recording (e.g., fishing mode: detect strikes vs. catches)
- **Yocto Build System:** Custom minimal Linux image (Phase 4)
- **Web Configuration:** WiFi hotspot + responsive web UI for settings
- **GPS Logging:** Serial GPS module integration with video metadata
- **Power Optimization:** Deep sleep modes, wake-on-interrupt, power profiling

## Technical References

**Libraries & Frameworks:**
- [libgpiod](https://github.com/brgl/libgpiod) — Modern Linux GPIO interface
- [picamera2](https://github.com/raspberrypi/picamera2) — Official Raspberry Pi camera library (Python)
- [rpicam-apps](https://github.com/raspberrypi/rpicam-apps) — Raspberry Pi camera reference implementations (C++)
- [embedded-hal](https://github.com/rust-embedded/embedded-hal) — Rust hardware abstraction layer

**Learning Resources:**
- Raspberry Pi documentation for camera and GPIO subsystems
- Linux kernel documentation for V4L2 (Video4Linux2) API
- Embedded Rust book for microcontroller programming patterns

## License

This project is licensed under the MIT License - see LICENSE file for details.

## Acknowledgments

Built with inspiration from the maker community and professional embedded systems practices. Special thanks to the Raspberry Pi Foundation for excellent hardware documentation and the open-source embedded communities for Python, C++, and Rust.

---

**Project Type:** Embedded Systems | Computer Vision | Comparative Language Study  
**Skills Demonstrated:** Linux system programming, hardware interfacing, state machine design, cross-language development, embedded optimization
