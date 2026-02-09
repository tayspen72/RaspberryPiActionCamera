# Phase 2

Phase 2 of the design is meant to be an equivalent implementation of the project written in C.

## Requirements

Some requirements for compiling and running the design:

### Libraries used in the design:

* gpiod
  * `sudo apt install libgpiod-dev`
* pthread

### Permissions

user must be added to the `gpio` group.

`sudo usermod -a -G gpio $USER`

## Design Concepts

Key concepts and technologies in the design are meant to be using modern and recommended practices.

### libgpiod

When accessing gpio, several technologies are common:

* [wiringpi](https://github.com/WiringPi/WiringPi)
  * deprecated and generally not recommended for future work, no expected updates to the codebase.
* [pigpio](https://abyz.me.uk/rpi/pigpio/)
  * generally more common and familiar to raspberry pi users
  * more simple callback handling
  * built-in debouncing
* [libgpiod](https://github.com/brgl/libgpiod)
  * modern standard for gpio interface
  * no daemon required - direct kernel character device interface
  * in active development

