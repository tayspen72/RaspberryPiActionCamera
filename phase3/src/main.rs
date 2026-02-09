mod button;
mod camera;
mod display;

fn main() {
    println!("Starting Raspberry Pi Action Camera");
    println!("FW: 1.0.0");
    println!("Press button for action, hold to cycle modes. Ctrl-C to exit.");

    let button = button::Button::new(1250).expect("Failed to create the button");
    let display = display::Display::new().expect("Failed to create the display");

    let mut camera = camera::Camera::new(button, display).expect("Failed to create the camera");

    camera.run().expect("Failed to run");
}
