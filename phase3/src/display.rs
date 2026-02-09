use crate::camera::{Mode, State};
use embedded_graphics::{
    mono_font::{MonoTextStyle, ascii::FONT_5X8},
    pixelcolor::BinaryColor,
    prelude::*,
    primitives::{PrimitiveStyleBuilder, Rectangle},
    text::{Text, renderer::CharacterStyle},
};
use linux_embedded_hal::I2cdev;
use ssd1306::{I2CDisplayInterface, Ssd1306, mode::BufferedGraphicsMode, prelude::*};

// Type alias for the display driver
type DisplayDriver = Ssd1306<I2CInterface<I2cdev>, DisplaySize128x32, BufferedGraphicsMode<DisplaySize128x32>>;

pub struct Display {
    driver: DisplayDriver,
}

impl Display {
    pub fn new() -> Result<Self, i2cdev::linux::LinuxI2CError> {
        let i2c = I2cdev::new("/dev/i2c-1")?;
        let interface = I2CDisplayInterface::new(i2c);
        let mut driver = Ssd1306::new(interface, DisplaySize128x32, DisplayRotation::Rotate0).into_buffered_graphics_mode();
        driver.init().unwrap(); // Should handle error properly

        Ok(Display { driver })
    }

    fn clear_region(&mut self, x_start: i32, x_end: i32, y_start: i32, y_end: i32) {
        let width = (x_end - x_start + 1) as u32;
        let height = (y_end - y_start + 1) as u32;

        Rectangle::new(Point::new(x_start, y_start), Size::new(width, height))
            .into_styled(PrimitiveStyleBuilder::new().fill_color(BinaryColor::Off).build())
            .draw(&mut self.driver)
            .unwrap();
    }

    pub fn write_mode(&mut self, mode: Mode) {
        let text_y = 7; // Y position for all texts
        let column_y = 1; // Y position for all texts
        let font_height = FONT_5X8.character_size.height as i32;
        let column_width = 1;

        let event_x = 0;
        let event_text_width = "EVENT".len() as i32 * FONT_5X8.character_size.width as i32;
        let standby_text_width = "STANDBY".len() as i32 * FONT_5X8.character_size.width as i32;
        let standby_x = 128 - (standby_text_width + (2 * column_width)) - 1;
        let record_text_width = "RECORD".len() as i32 * FONT_5X8.character_size.width as i32;
        let empty_space = 128
            - (event_text_width + (2 * column_width))
            - (record_text_width + (2 * column_width))
            - (standby_text_width + (2 * column_width))
            - 1;
        let record_x = (event_text_width + (2 * column_width)) + (empty_space / 2);

        // Styles are created within the if/else to ensure they are owned MonoTextStyle values
        // and avoid lifetime/borrowing issues.
        let event_style = if mode == Mode::Event {
            let mut style = MonoTextStyle::new(&FONT_5X8, BinaryColor::Off);
            style.set_background_color(Some(BinaryColor::On));
            style
        } else {
            MonoTextStyle::new(&FONT_5X8, BinaryColor::On)
        };
        let record_style = if mode == Mode::Record {
            let mut style = MonoTextStyle::new(&FONT_5X8, BinaryColor::Off);
            style.set_background_color(Some(BinaryColor::On));
            style
        } else {
            MonoTextStyle::new(&FONT_5X8, BinaryColor::On)
        };
        let standby_style = if mode == Mode::Standby {
            let mut style = MonoTextStyle::new(&FONT_5X8, BinaryColor::Off);
            style.set_background_color(Some(BinaryColor::On));
            style
        } else {
            MonoTextStyle::new(&FONT_5X8, BinaryColor::On)
        };

        // Clear the row before writing over it again
        self.clear_region(0, 127, 0, 8);

        // EVENT
        if mode == Mode::Event {
            Rectangle::new(Point::new(event_x, column_y), Size::new(column_width as u32, font_height as u32))
                .into_styled(PrimitiveStyleBuilder::new().fill_color(BinaryColor::On).build())
                .draw(&mut self.driver)
                .unwrap();
            Rectangle::new(
                Point::new(event_x + column_width + event_text_width, column_y),
                Size::new(column_width as u32, font_height as u32),
            )
            .into_styled(PrimitiveStyleBuilder::new().fill_color(BinaryColor::On).build())
            .draw(&mut self.driver)
            .unwrap();
        }
        Text::new("EVENT", Point::new(event_x + column_width, text_y), event_style).draw(&mut self.driver).unwrap();

        // RECORD
        if mode == Mode::Record {
            Rectangle::new(Point::new(record_x, column_y), Size::new(column_width as u32, font_height as u32))
                .into_styled(PrimitiveStyleBuilder::new().fill_color(BinaryColor::On).build())
                .draw(&mut self.driver)
                .unwrap();
            Rectangle::new(
                Point::new(record_x + column_width + record_text_width, column_y),
                Size::new(column_width as u32, font_height as u32),
            )
            .into_styled(PrimitiveStyleBuilder::new().fill_color(BinaryColor::On).build())
            .draw(&mut self.driver)
            .unwrap();
        }
        Text::new("RECORD", Point::new(record_x + column_width, text_y), record_style).draw(&mut self.driver).unwrap();

        // STANDBY
        if mode == Mode::Standby {
            Rectangle::new(Point::new(standby_x, column_y), Size::new(column_width as u32, font_height as u32))
                .into_styled(PrimitiveStyleBuilder::new().fill_color(BinaryColor::On).build())
                .draw(&mut self.driver)
                .unwrap();
            Rectangle::new(
                Point::new(standby_x + column_width + standby_text_width, column_y),
                Size::new(column_width as u32, font_height as u32),
            )
            .into_styled(PrimitiveStyleBuilder::new().fill_color(BinaryColor::On).build())
            .draw(&mut self.driver)
            .unwrap();
        }
        Text::new("STANDBY", Point::new(standby_x + column_width, text_y), standby_style).draw(&mut self.driver).unwrap();

        self.driver.flush().unwrap();
    }

    pub fn write_state(&mut self, state: State) {
        let text_y = 31; // Y position for all texts
        let text_x = 1; // Y position for all texts

        let waiting_for_event_text = "    WAITING FOR EVENT    ";
        let saving_event_text = "          SAVING         ";
        let recording_text = "        RECORDING        ";
        let stopped_text = "         STOPPED         ";
        let none_text = "                         ";

        let state_text = match state {
            State::WaitingForEvent => waiting_for_event_text,
            State::SavingEvent => saving_event_text,
            State::Recording => recording_text,
            State::Stopped => stopped_text,
            State::None => none_text,
        };

        // Clear the row before writing over it again
        self.clear_region(0, 127, 23, 31);

        Text::new(state_text, Point::new(text_x, text_y), MonoTextStyle::new(&FONT_5X8, BinaryColor::On)).draw(&mut self.driver).unwrap();
        self.driver.flush().unwrap();
    }
}
