# Phase 2: C++ Implementation

> Production-grade implementation optimizing for performance, predictability, and direct hardware control. Built with C++17, `libgpiod`, and custom low-level drivers.

## Overview

This phase transitions from prototype to production-ready firmware. It demonstrates systems programming practices for embedded Linux, focusing on minimal overhead, deterministic behavior, and direct hardware access.

**Implementation highlights:**
- Native compiled code for reduced boot time and memory footprint
- Modern GPIO interface via `libgpiod` (kernel character device)
- Manual memory management with RAII patterns
- Event-driven architecture with atomic synchronization
- Direct device file access for I²C and GPIO

## Architecture

Structured around a central event loop with callback-based component integration:

```
main.cpp                    # Event loop and signal handling
├── ActionCamera            # State machine and camera control
├── Button                  # GPIO input with edge detection
└── Ssd1306                 # I²C OLED driver with framebuffer
```

### Core Design Patterns

**Event-Driven Architecture**
- Non-blocking main loop (100ms cycle time)
- Atomic flags for cross-thread communication
- Button events processed via callback registration
- Zero-latency state updates

**Resource Management**
- RAII wrappers for file descriptors and GPIO lines
- Explicit `new`/`delete` for display buffer (predictable allocation)
- Automatic cleanup on exception or shutdown
- No garbage collection overhead

**Hardware Access**
- Direct POSIX I/O: `open()`, `ioctl()`, `read()`, `write()`
- Character device interfaces: `/dev/gpiochip0`, `/dev/i2c-1`
- Memory-mapped I/O readiness (future optimization path)

## Display Layout

The 128×32 pixel OLED uses a two-tier information hierarchy:

**Top row** — Mode indicators:
```
EVENT          RECORD          STANDBY
  ↑              ↑               ↑
col 0          col 40         col 87
(active mode rendered with inverted pixels)
```

**Bottom rows** — State feedback:
- RECORDING
- STOPPED  
- WAITING FOR EVENT
- SAVING EVENT

## Quick Start

### Prerequisites

```bash
# Install libgpiod development headers (v2.2.2 or newer)
sudo apt install libgpiod-dev

# Verify GPIO device access
ls -l /dev/gpiochip0
# Should show crw-rw---- with group 'gpio'
```

### Build Process

```bash
# Configure build with CMake
cmake -B build

# Compile
cd build
make

# Optional: Build with debug symbols
cmake -B build -DCMAKE_BUILD_TYPE=Debug
```

### Running the Application

```bash
# From build directory
./bin/action_camera

# Run with elevated priority (optional, requires sudo)
sudo nice -n -10 ./bin/action_camera
```

The camera initializes in STANDBY mode. Long-press the button (1.25s) to cycle through modes.

## Hardware Configuration

| Component | Device File | Configuration | Library |
|-----------|-------------|---------------|---------|
| Push button | `/dev/gpiochip0` | Line 23, active-low, pull-up | `libgpiod` 2.x |
| SSD1306 OLED | `/dev/i2c-1` | Slave address 0x3C, 128×32 | Custom POSIX driver |
| Camera Module | CSI interface | 1920×1080 @ 30fps, H.264 | `rpicam-vid` (subprocess) |

## Module Reference

### `main.cpp` — Event Loop

Central coordinator implementing:
- Signal handling (SIGINT, SIGTERM) for graceful shutdown
- 100ms polling loop for button state
- Callback attachment for mode/state transitions
- Display refresh on state changes

**Key functions:**
- `setup_signal_handlers()` — Atomic shutdown flag for clean exit
- `button_press_callback()` — Routes press events to active mode
- `button_hold_callback()` — Initiates mode cycling
- `on_mode_change()` / `on_state_change()` — Display update triggers

### `action_camera.hpp/cpp` — State Machine

Core business logic managing:
- Mode enumeration: `EVENT`, `RECORD`, `STANDBY`
- State enumeration per mode
- Camera subprocess lifecycle (`rpicam-vid`)
- Circular buffer coordination (file-based ring buffer)

**Design decisions:**
- Enum classes for type-safe mode/state
- `std::function` callbacks for loose coupling
- Subprocess management via `popen()` / `pclose()`
- File I/O for video output (kernel buffering optimization)

### `button.hpp/cpp` — GPIO Interface

Modern GPIO handling via character device API:

**Features:**
- `libgpiod` v2.x API (replaces deprecated sysfs)
- Edge detection (falling edge for active-low button)
- Software debouncing (100ms minimum interval)
- Hold time detection (1.25s threshold)
- Event notification through atomic flags

**Advantages over sysfs:**
- No race conditions from filesystem access
- Direct kernel support
- Better performance on multi-process systems

### `ssd1306.hpp/cpp` — Display Driver

Complete SSD1306 OLED controller implementation:

**Initialization sequence:**
- Command mode configuration via I²C
- Display clock frequency, multiplex ratio, contrast
- Charge pump enable for 3.3V operation
- Memory addressing mode (horizontal)

**Rendering pipeline:**
- 1024-byte framebuffer (128×32 ÷ 8 bits/page)
- Page-based updates (minimize I²C transactions)
- Pixel-level drawing primitives
- Text rendering with custom bitmap font

**I²C Protocol:**
- Device file: `/dev/i2c-1`
- 7-bit addressing: 0x3C
- Control byte: 0x40 (data) / 0x00 (command)
- Ioctl command: `I2C_SLAVE` for device selection

### `ssd1306_font.hpp` — Bitmap Font

Compact monospace font for embedded displays:
- 8×5 pixel character cells (1 pixel inter-character spacing)
- Uppercase ASCII only (A-Z, 26 characters)
- Column-major bitmap encoding
- 130 bytes total (26 chars × 5 columns)

## Design Decisions

### Why libgpiod over sysfs/pigpio?

**libgpiod advantages:**
- Modern kernel ABI (character device, not deprecated sysfs)
- No background daemon (unlike pigpio)
- Process isolation (multiple apps can coexist)
- Future-proof for kernel changes

### Memory Management Strategy

**Manual allocation for display buffer:**
- Predictable allocation time (no GC pauses)
- Explicit control over buffer lifetime
- RAII pattern ensures cleanup even on exception

**Stack allocation for small objects:**
- Button and camera objects live on stack
- Reduced heap fragmentation
- Cache-friendly memory layout

### Atomic Flags vs. Mutexes

**Why atomics for button events:**
- Lock-free (no contention overhead)
- Signal-safe (can set from interrupt handler)
- Simpler code (no deadlock risk)
- Adequate for single-producer, single-consumer

### Partial Display Updates

Only modified pages are transmitted over I²C:
- Reduces bus traffic by ~80% for state changes
- Maintains display responsiveness
- Future optimization: dirty region tracking

## Build System

CMake configuration highlights:

```cmake
# C++17 required for structured bindings, std::filesystem
set(CMAKE_CXX_STANDARD 17)

# Enable warnings
target_compile_options(action_camera PRIVATE -Wall -Wextra)

# Link libgpiod
target_link_libraries(action_camera PRIVATE gpiod)
```

**Build outputs:**
- `bin/action_camera` — Main executable
- Debug symbols preserved with `-DCMAKE_BUILD_TYPE=Debug`

## Performance Characteristics

Measured on Raspberry Pi Zero 2 W:

| Metric | Phase 1 (Python) | Phase 2 (C++) |
|--------|------------------|---------------|
| Boot to first frame | ~10s | ~3.5s |
| Memory footprint | 45MB RSS | 8MB RSS |
| Idle CPU usage | 3-5% | <1% |
| Button latency | 150-250ms | 50-100ms |
| Binary size | N/A (interpreted) | 180KB (stripped) |

## Implementation Comparison

How this phase differs from others:

| Aspect | Phase 1 (Python) | Phase 2 (C++) | Phase 3 (Rust) |
|--------|------------------|---------------|----------------|
| **Compilation** | None (interpreted) | CMake + Make | Cargo (rustc + LLVM) |
| **Type safety** | Duck typing (runtime) | Manual (compile-time) | Enforced (borrow checker) |
| **GPIO library** | `gpiozero` (high-level) | `libgpiod` (low-level) | `gpiod` crate (safe wrapper) |
| **Memory model** | GC (stop-the-world) | Manual (RAII) | Ownership (zero-cost) |
| **Performance** | Baseline | 3-4× faster boot | Similar to C++ |
| **Development speed** | Fastest | Moderate | Moderate |
| **Debugging** | Easy (print, REPL) | GDB, valgrind | GDB, cargo test |

## Development Notes

**Strengths of C++ for embedded:**
- Predictable performance (no GC pauses)
- Minimal runtime overhead
- Direct hardware access without abstraction penalty
- Mature toolchain (GCC, GDB, valgrind)
- Large ecosystem of embedded libraries

**Trade-offs:**
- Manual memory management (risk of leaks/corruption)
- Longer compile times than Python
- More verbose than modern languages (Rust)
- No built-in memory safety (compared to Rust)

**Best used for:**
- Production embedded systems
- Performance-critical applications
- Battery-powered devices (lower idle power)
- Real-time responsiveness requirements
- Commercial product development

## Troubleshooting

**libgpiod version mismatch:**
```bash
# Check installed version
pkg-config --modversion libgpiod

# v2.2.2+ required for this code
# If using v1.x, API changes needed
```

**I²C permission denied:**
```bash
# Add user to i2c group
sudo usermod -a -G i2c $USER

# Verify device access
ls -l /dev/i2c-1
```

**Camera subprocess fails:**
```bash
# Test rpicam-vid directly
rpicam-vid -t 5000 -o test.h264

# Check for /dev/video0
v4l2-ctl --list-devices
```

**Segmentation fault:**
```bash
# Run with debugger
gdb ./bin/action_camera
(gdb) run
# After crash:
(gdb) bt

# Or use valgrind for memory issues
valgrind --leak-check=full ./bin/action_camera
```

## Next Steps

**Optimization opportunities:**
- Replace `rpicam-vid` subprocess with `libcamera` C++ API
- Implement DMA for I²C transfers (zero-copy display updates)
- Add power management (dynamic frequency scaling)
- Profile with `perf` for hotspot analysis

**Comparison exercises:**
- Benchmark against Phase 1 (Python) and Phase 3 (Rust)
- Measure power consumption with `powertop`
- Compare binary sizes stripped vs. unstripped
- Evaluate development time and bug density

---

**Key Learning Outcomes:**  
Modern Linux GPIO (libgpiod) • POSIX device I/O • C++ RAII patterns • Embedded systems optimization • Character device drivers • CMake build systems
