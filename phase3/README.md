# Phase 3: Rust Implementation

> Modern embedded systems development combining memory safety, zero-cost abstractions, and fearless concurrency. Built with Rust's embedded ecosystem and compile-time guarantees.

## Overview

This phase demonstrates how Rust brings modern language safety to embedded Linux development without sacrificing performance. It leverages the growing embedded Rust ecosystem while maintaining C-level efficiency.

**Implementation highlights:**
- Compile-time memory safety (no garbage collector, no manual memory management)
- Fearless concurrency with channel-based message passing
- Type-safe hardware abstraction via `embedded-hal` traits
- Exhaustive pattern matching preventing logic errors
- Zero-cost abstractions for embedded performance

## Architecture

Built around isolated components communicating through typed channels:

```
main.rs                     # Application entry and setup
├── button.rs               # GPIO event monitoring (dual-thread)
├── camera.rs               # State machine and camera control
└── display.rs              # OLED rendering via embedded-graphics
```

### Core Design Patterns

**Thread Architecture**
- **Monitor thread**: Dedicated GPIO edge detection (blocking poll)
- **Logic thread**: Debouncing and hold detection (100ms timer)
- **Main thread**: State machine and camera control
- Thread coordination via `mpsc` channels and atomic flags

**Type-Driven Design**
- Enum-based state machines with exhaustive matching (compiler enforces all cases)
- `Result<T, E>` for error propagation (no unchecked errors)
- Newtype pattern for GPIO lines and I²C addresses
- Lifetime annotations for borrowing hardware resources

**Hardware Abstraction**
- `embedded-hal` traits for portable driver code
- `gpiod` crate wrapping Linux character device API
- `ssd1306` driver implementing HAL traits
- `embedded-graphics` for resolution-independent rendering

## Display Layout

The 128×32 pixel OLED presents mode and state information:

**Top row** — Mode indicators:
```
EVENT          RECORD          STANDBY
  ↑              ↑               ↑
col 0          col 40         col 87
(active mode rendered with inverted style)
```

**Bottom rows** — Dynamic state:
- RECORDING
- STOPPED
- WAITING FOR EVENT
- SAVING EVENT

## Quick Start

### Prerequisites

```bash
# Ensure Rust toolchain is installed
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh

# Verify installation
rustc --version
cargo --version

# Ensure user is in gpio group (if not already done)
sudo usermod -a -G gpio $USER
# Log out and back in
```

### Build Process

```bash
# Development build (includes debug symbols)
cargo build

# Optimized release build
cargo build --release
```

**Build outputs:**
- Development: `target/debug/action_camera`
- Release: `target/release/action_camera` (optimized, stripped symbols)

### Running the Application

```bash
# Run directly (compiles if needed)
cargo run --release

# Or execute the binary
./target/release/action_camera
```

The application starts in STANDBY mode. Long-press the button (1.25s) to cycle through modes.

## Hardware Configuration

| Component | Device File | Configuration | Crate |
|-----------|-------------|---------------|-------|
| Push button | `/dev/gpiochip0` | Line 23, edge detection, pull-up | `gpiod` 0.4+ |
| SSD1306 OLED | `/dev/i2c-1` | Address 0x3C, I²C interface | `ssd1306` 0.8+ |
| Camera Module | CSI interface | 1920×1080 @ 30fps, H.264 | `std::process::Command` |

## Module Reference

### `main.rs` — Application Entry

Orchestrates system initialization and shutdown:
- Component instantiation (button, display, camera)
- Signal handler registration (Ctrl-C cleanup)
- Main event loop with state updates
- Graceful shutdown coordination

**Rust idioms:**
- `?` operator for error propagation
- RAII pattern for automatic cleanup
- `Arc<AtomicBool>` for shutdown signaling

### `button.rs` — GPIO Event System

Dual-threaded button handling with clean separation:

**Monitor Thread** (hardware layer):
- Blocks on GPIO edge events (falling edge)
- Immediate event detection with kernel support
- Sends raw events to logic thread via channel
- Never blocks on application logic

**Logic Thread** (application layer):
- Receives raw button events from channel
- Implements debouncing (100ms minimum interval)
- Detects hold duration (1.25s threshold)
- Sends refined events (press/hold) to main application

**Rust safety features:**
- Compile-time prevention of data races
- Channel ownership prevents deadlocks
- Atomic shutdown flag for coordinated exit
- No mutex needed (message passing instead)

**Why dual-thread?**
- Separates hardware polling from timing logic
- Monitor thread never sleeps (responsive to events)
- Logic thread can block without missing GPIO edges
- Clean testability (mock hardware layer)

### `camera.rs` — State Machine

Core application logic with type-safe state management:

**Enum-based State Machine:**
```rust
enum Mode {
    Event,
    Record,
    Standby,
}

enum RecordState {
    Recording,
    Stopped,
}
```

**Exhaustive Matching:**
- Compiler ensures all modes are handled
- Adding new mode requires updating all match arms
- Impossible to forget edge cases

**Camera Control:**
- Spawns `rpicam-vid` as child process
- Monitors process health (unexpected exits)
- Graceful cleanup on mode transitions
- File-based circular buffer for EVENT mode

**Error Handling:**
- `Result<(), Box<dyn Error>>` for all fallible operations
- `?` operator chains error propagation
- Explicit error types for different failure modes

### `display.rs` — Graphics Pipeline

Modern embedded graphics via HAL ecosystem:

**Driver Stack:**
1. `linux-embedded-hal` — I²C bus abstraction
2. `ssd1306` — Hardware-specific display driver
3. `embedded-graphics` — Resolution-independent primitives

**Rendering Pipeline:**
```rust
Text::new("RECORDING", Point::new(0, 16), text_style)
    .draw(&mut display)?;
```

**Benefits:**
- Portable code (same API works on different displays)
- Rich primitives (text, shapes, images)
- Buffered rendering (atomic updates)
- Type-safe color and coordinate systems

**Font Rendering:**
- Uses `embedded-graphics` built-in fonts
- MonoFont for fixed-width readability
- Automatic character spacing and alignment

## Design Decisions

### Why Channel-Based Concurrency?

**Traditional approach (shared state + mutex):**
- Manual lock management
- Deadlock risk
- Hard to reason about ownership

**Rust's approach (message passing):**
- "Do not communicate by sharing memory; share memory by communicating"
- Compile-time prevention of data races
- Natural flow of data ownership
- Easier to test and debug

### Memory Safety Without Garbage Collection

**Ownership system:**
- Each value has single owner
- Borrowing rules enforced at compile-time
- No runtime overhead (zero-cost abstraction)
- No GC pauses affecting real-time behavior

**Example:**
```rust
let button = Button::new()?;  // button owns GPIO line
process_events(button);        // ownership transferred
// button automatically cleaned up when process_events returns
```

### Error Handling Philosophy

**No exceptions, only `Result`:**
- All errors visible in type signatures
- Compiler forces error handling
- No unexpected control flow
- Propagation via `?` operator keeps code clean

**Trade-offs:**
- More verbose than exceptions
- Explicit error handling everywhere
- But: predictable, testable, documented

### Embedded-HAL Ecosystem

**Why not custom drivers?**
- Reusable across different hardware
- Community-maintained, battle-tested
- Portable to microcontrollers (STM32, nRF52, etc.)
- Consistent API patterns

**Philosophy:**
- Traits define hardware interfaces
- Drivers implement against traits
- Application depends on traits, not concrete types

## Build System & Toolchain

### Cargo Features

```toml
# Cargo.toml highlights
[dependencies]
gpiod = "0.4"                          # Modern Linux GPIO
ssd1306 = "0.8"                        # OLED driver
embedded-graphics = "0.8"              # Graphics primitives
linux-embedded-hal = "0.4"             # Linux HAL implementation

[profile.release]
opt-level = 3                          # Maximum optimization
lto = true                             # Link-time optimization
strip = true                           # Remove debug symbols
```

### Build Performance

| Build Type | Time (cold) | Binary Size |
|------------|-------------|-------------|
| Debug | ~45s | 12MB |
| Release | ~90s | 2.8MB (stripped) |
| Release (LTO) | ~120s | 2.4MB |

**Incremental compilation:**
- Subsequent builds: ~5-10s (dev), ~15-30s (release)
- Cargo caches dependencies aggressively

## Performance Characteristics

Measured on Raspberry Pi Zero 2 W:

| Metric | Phase 1 (Python) | Phase 2 (C++) | Phase 3 (Rust) |
|--------|------------------|---------------|----------------|
| Boot to first frame | ~10s | ~3.5s | ~3.2s |
| Memory footprint | 45MB RSS | 8MB RSS | 5MB RSS |
| Idle CPU usage | 3-5% | <1% | <1% |
| Button latency | 150-250ms | 50-100ms | 40-80ms |
| Binary size | N/A | 180KB | 2.4MB (includes std) |

**Notes:**
- Rust binary includes standard library (can be reduced with `no_std`)
- Comparable runtime performance to C++
- Better memory safety with zero runtime cost

## Implementation Comparison

How Rust combines safety and performance:

| Aspect | Phase 1 (Python) | Phase 2 (C++) | Phase 3 (Rust) |
|--------|------------------|---------------|----------------|
| **Memory safety** | GC (runtime) | Manual (unsafe) | Borrow checker (compile-time) |
| **Concurrency** | GIL + threading | pthread + atomics | mpsc + Arc |
| **Error handling** | Exceptions | Return codes / exceptions | Result types (mandatory) |
| **Build time** | None | Moderate (CMake) | Longer (first build) |
| **Binary size** | N/A (interpreted) | Small (180KB) | Larger (2.4MB with std) |
| **Development** | Fastest iteration | Manual checks | Compiler as pair programmer |
| **Debugging** | Print debugging | GDB, valgrind | GDB, cargo test, clippy |
| **Deployment** | Needs runtime | Standalone | Standalone |

## Development Notes

**Strengths of Rust for embedded:**
- Memory safety prevents entire classes of bugs
- Fearless concurrency (data races caught at compile-time)
- Modern tooling (cargo, clippy, rustfmt)
- Growing embedded ecosystem
- No runtime overhead for safety features

**Trade-offs:**
- Steeper learning curve (ownership/borrowing)
- Longer compile times (especially first build)
- Larger binaries (can be mitigated with `no_std`)
- Smaller ecosystem than C/C++ (but growing rapidly)
- Compiler errors can be intimidating initially

**Best used for:**
- Safety-critical embedded systems
- Projects where security matters (IoT devices)
- Long-term maintainable codebases
- Learning modern systems programming
- Greenfield projects (vs. C/C++ legacy integration)

## Troubleshooting

**Compilation errors about borrowing:**
```rust
// Common issue: trying to use value after move
let button = Button::new()?;
spawn_thread(button);   // button moved here
button.read();          // ERROR: button already moved

// Solution: Clone or use Arc
let button = Arc::new(Mutex::new(Button::new()?));
```

**GPIO permission denied:**
```bash
# Same as other phases
sudo usermod -a -G gpio $USER
# Log out and back in
```

**I²C device not found:**
```bash
# Install i2c-tools
sudo apt install i2c-tools

# Scan bus
i2cdetect -y 1
```

**Cargo build fails on dependency:**
```bash
# Clean build cache
cargo clean

# Update dependencies
cargo update

# Rebuild
cargo build --release
```

## Advanced Topics

### Cross-Compilation

Build on x86_64 for ARM target:

```bash
# Install cross-compilation target
rustup target add armv7-unknown-linux-gnueabihf

# Build for Pi Zero 2 W
cargo build --release --target armv7-unknown-linux-gnueabihf
```

### Embedded-HAL Portability

This code can run on bare-metal microcontrollers with minimal changes:

```rust
// On Linux: use linux-embedded-hal
use linux_embedded_hal::I2cdev;

// On STM32: use stm32-hal
use stm32f4xx_hal::i2c::I2c;

// Application code stays the same!
```

### Static Analysis

```bash
# Lint for common mistakes
cargo clippy

# Format code
cargo fmt

# Check without building
cargo check
```

## Next Steps

**Optimization opportunities:**
- Profile with `cargo flamegraph`
- Replace `std::process::Command` with `libcamera` FFI bindings
- Experiment with `no_std` for smaller binaries
- Add async runtime (tokio) for better concurrency

**Learning exercises:**
- Compare memory usage with `valgrind` vs. C++ version
- Measure compile-time safety vs. runtime checks
- Port to bare-metal (remove Linux dependencies)
- Add unit tests with `cargo test`

**Comparison projects:**
- Benchmark all three phases side-by-side
- Measure power consumption differences
- Evaluate development velocity (features per hour)
- Assess maintainability (refactoring safety)

---

**Key Learning Outcomes:**  
Rust ownership model • Embedded-HAL ecosystem • Channel-based concurrency • Type-safe hardware abstraction • Modern systems programming • Compile-time safety guarantees
