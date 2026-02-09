mod app;
mod button;
mod camera;
mod display;

fn main() {
    println!("Starting Raspberry Pi Action Camera");
    println!("FW: 1.0.0");
    println!("Press button for action, hold to cycle modes. Ctrl-C to exit.");

    let button = button::Button::new(1250).expect("Failed to create the button");
    let camera = camera::Camera::new(".").expect("Failed to create the camera API");
    let display = display::Display::new().expect("Failed to create the display");

    let mut app = app::ActionCamera::new(button, camera, display).expect("Failed to create the camera");

    app.run().expect("Failed to run");
}
