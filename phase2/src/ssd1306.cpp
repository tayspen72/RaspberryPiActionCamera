/*
 * tayspen72
 *
 * ssd1306.cpp
 */

//==============================================================================
// Notes
//==============================================================================

//==============================================================================
// Includes
//==============================================================================
#include "ssd1306.hpp"

#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <iostream>

#include "ssd1306_font.hpp"

//==============================================================================
// Definitions
//==============================================================================
#define SSD1306_I2C_ADDRESS 0x3C

#define CONTROL_BYTE(continuation, data) ((uint8_t)(((continuation & 0b1) << 7) | ((data & 0b1) << 6)))

//==============================================================================
// Enumerations and Structures
//==============================================================================

//==============================================================================
// Private Function Prototypes
//==============================================================================

//==============================================================================
// Variables
//==============================================================================

//==============================================================================
// Public Functions
//==============================================================================
int Ssd1306::Open(const char* i2cBusName) {
	i2cFd = open(i2cBusName, O_RDWR);
	if (i2cFd < 0)
		return -1;

	int ret = ioctl(i2cFd, I2C_SLAVE, SSD1306_I2C_ADDRESS);
	if (ret < 0) {
		close(i2cFd);
		return -1;
	}
	return ret;
}

int Ssd1306::Close() {
	if (i2cFd >= 0) {
		close(i2cFd);
		i2cFd = -1;
	}
	return 0;
}

int Ssd1306::ClearBuffer() {
	return clearBuffer();
}

int Ssd1306::ClearDisplay() {
	clearBuffer();
	return drawBuffer();
}

int Ssd1306::DrawBuffer() {
	return drawBuffer();
}

int Ssd1306::DrawPart(int page_start, int page_end, int column_start, int column_end) {
	return drawPart(page_start, page_end, column_start, column_end);
}

int Ssd1306::Init() {
	return init();
}

int Ssd1306::WritePixel(int x, int y, bool isOn) {
	return writePixel(x, y, isOn);
}

int Ssd1306::WriteString(int page, int column, const char* string, uint8_t foreground, uint8_t background) {
	return writeString(page, column, string, foreground, background);
}

Ssd1306::Ssd1306(int num_columns, int num_rows) : NumColumns(num_columns), NumRows(num_rows) {
	displayBufferSize = num_columns * num_rows / 8;
	createBuffer();
}

Ssd1306::~Ssd1306() {
	Close();

	destroyBuffer();
}

//==============================================================================
// Private Functions
//==============================================================================
int Ssd1306::init() {
	setDisplayOn(false);
	setDisplayClockDivideRatio(0x0, 0x8);
	setMultiplexRatio(NumRows - 1);
	setDisplayOffset(0);
	setDisplayStartLine(0);
	setChargePumpSetting(ChargePumpSetting_t::ChargePumpEnabled);
	setMemoryAddressingMode(MemoryAddressMode_t::Horizontal);
	setSegmentRemap(true);
	setComOutputScanDirection(false);
	setComPinsHardwareConfig(ComPinsHardwareConfigSequence_t::ComPinSequential, ComPinsHardwareConfigRemap_t::ComPinRemapDisabled);
	setContrast(0x8F);
	setPrechargePeriod(0x2, 0x2);
	setVComDeselectLevel(VComDeselectLevel_t::Level_0_77);
	setDisplayInvert(false);
	ClearDisplay();
	setDisplayOn(true);
	return 0;
}

int Ssd1306::createBuffer() {
	displayBuffer = (uint8_t*)malloc(displayBufferSize);
	if (displayBuffer == NULL) {
		return -1;
	}
	memset(displayBuffer, 0, displayBufferSize);
	return 0;
}

void Ssd1306::destroyBuffer() {
	if (displayBuffer) {
		free(displayBuffer);
		displayBuffer = NULL;
	}
	displayBufferSize = 0;
}

/**
 * @brief Clear the buffer and initialize the control byte as data for long writes
 * @return NA
 */
int Ssd1306::clearBuffer() {
	memset(displayBuffer, 0x00, displayBufferSize);
	return 0;
}

/**
 * @brief Draw entire display buffer
 * @return 0 on success, -1 on failure.
 */
int Ssd1306::drawBuffer() {
	uint8_t* tmpBuffer = (uint8_t*)malloc(displayBufferSize + 1);

	tmpBuffer[0] = CONTROL_BYTE(0, 1);
	memcpy(tmpBuffer + 1, displayBuffer, displayBufferSize);

	setColumnAddress(0, NumColumns - 1);
	setPageAddress(0, (NumRows / 8) - 1);
	int ret = i2cWrite(tmpBuffer, displayBufferSize + 1);

	free(tmpBuffer);

	return ret;
}

/**
 * @brief Draw part of display buffer
 * @param page_start The starting page to be written. Not row.
 * @param page_end The ending page to be written. Not row.
 * @param column_start The starting column to be written.
 * @param column_end The ending column to be written.
 * @return 0 on success, -1 on failure.
 */
int Ssd1306::drawPart(int page_start, int page_end, int column_start, int column_end) {
	if ((page_start < 0) || (page_start >= NumRows / 8) || (page_end < page_start))
		return -1;
	else if ((column_start < 0) || (column_start >= NumColumns) || (column_end < column_start))
		return -1;

	int num_pages = page_end - page_start + 1;
	int num_columns = column_end - column_start + 1;
	int num_bytes = num_pages * num_columns;
	uint8_t* tmpBuffer = (uint8_t*)malloc(num_bytes + 1);

	tmpBuffer[0] = CONTROL_BYTE(0, 1);
	for (int p = page_start; p <= page_end; p++) {
		int tmp_buffer_offset = (p * num_columns) + 1;
		int display_buffer_offset = (p * NumColumns) + column_start;
		memcpy(tmpBuffer + tmp_buffer_offset, displayBuffer + display_buffer_offset, num_columns);
	}

	setColumnAddress(column_start, column_end);
	setPageAddress(page_start, page_end);
	int ret = i2cWrite(tmpBuffer, num_bytes + 1);

	free(tmpBuffer);

	return ret;
}

/**
 * @brief Write a single character to a position in the display buffer.
 * @note Does not update the display, only writes to the buffer
 * @param page The character page (num rows / 8).
 * @param column The column to start drawing the character.
 * @param c The character to be written.
 * @param foreground Foreground state 0 OFF 1 ON.
 * @param background Background state 0 OFF 1 ON.
 * @return 0 on success, -1 on failure.
 */
int Ssd1306::writeCharacter(int page, int column, char c, int foreground, int background) {
	int numPages = NumRows / 8;
	if ((page >= numPages) || (column + CHARACTER_WIDTH >= NumColumns))
		return -1;

	int characterMapOffset;
	if ((c >= 'A') && (c <= 'Z'))
		characterMapOffset = c - 'A';
	else if (c == ' ')
		characterMapOffset = 26;
	else
		return -1;

	foreground = foreground ? 1 : 0;
	background = background ? 1 : 0;

	int displayBufferOffset = column + (page * NumColumns);

	uint8_t tmp[CHARACTER_WIDTH];

	for (int i = 0; i < CHARACTER_WIDTH; i++) {
		tmp[i] = 0;
		uint8_t byte = _character_map[characterMapOffset][i];
		for (int bit = 0; bit < 8; bit++) {
			if (byte & (1 << bit))
				tmp[i] |= (foreground << bit);
			else
				tmp[i] |= (background << bit);
		}
	}

	memcpy(displayBuffer + displayBufferOffset, tmp, CHARACTER_WIDTH);

	return 0;
}

/**
 * @brief Write a pixel to the display buffer.
 * @note Does not update the display, only writes to the buffer
 * @param x The x coordinate of the pixel.
 * @param y The y coordinate of the pixel.
 * @param isOn The value of the pixel.
 * @return 0 on success, -1 on failure.
 */

int Ssd1306::writePixel(int x, int y, bool isOn) {
	(void)x;
	(void)y;
	(void)isOn;

	// int page = y / 8;
	// int offset = x + (page * NumColumns);
	// int mask = 1 << (y % 8);

	// TODO: This logic is all wrong
	// if ((x < 0) || (x >= NumColumns) || (y >= NumRows) || (y < 0))
	// return -1;

	// int row = y % 8;
	// displayBuffer[row] = isOn ? 1 : 0;

	return 0;
}

/**
 * @brief Write a character string to a position in the display buffer.
 * @note Does not update the display, only writes to the buffer
 * @param page The character page (num rows / 8).
 * @param column The column to start drawing the character.
 * @param string The character string to be written.
 * @param length The number of characters in the string to be written.
 * @param foreground Foreground state 0 OFF 1 ON.
 * @param background Background state 0 OFF 1 ON.
 * @return 0 on success, -1 on failure.
 */
int Ssd1306::writeString(int page, int column, const char* string, int foreground, int background) {
	int length = strlen(string);
	if (!length)
		return -1;

	std::cout << "writing string: " << string << "with length: " << length << std::endl;

	for (int i = 0; i < length; i++) {
		if (writeCharacter(page, column, string[i], foreground, background))
			return -1;

		column += CHARACTER_WIDTH;
	}

	return 0;
}

/**
 * @brief Sets the contrast of the display.
 * @param contrast A value from 0 to 255.
 * @return 0 on success, -1 on failure.
 */
int Ssd1306::setContrast(uint8_t contrast) {
	uint8_t buf[] = {
		CONTROL_BYTE(0, 0),
		ContrastControl,
		contrast
	};

	return i2cWrite(buf, sizeof(buf));
}

/**
 * @brief Forces the entire display to be ON regardless of RAM contents.
 * @param isForcedOn false (follows RAM) or true (all pixels ON).
 * @return 0 on success, -1 on failure.
 */
int Ssd1306::setDisplayForceOn(bool isForcedOn) {
	uint8_t buf[] = {
		CONTROL_BYTE(0, 0),
		isForcedOn ? DisplayForceOn_Enabled : DisplayForceOn_Disabled
	};

	return i2cWrite(buf, sizeof(buf));
}

/**
 * @brief Inverts the relationship between RAM data and pixel light.
 * @param isInverted false (normal) true (inverted).
 * @return 0 on success, -1 on failure.
 */
int Ssd1306::setDisplayInvert(bool isInverted) {
	uint8_t buf[] = {
		CONTROL_BYTE(0, 0),
		isInverted ? DisplayInvert_Enabled : DisplayInvert_Disabled
	};

	return i2cWrite(buf, sizeof(buf));
}

/**
 * @brief Turns the entire display panel ON or OFF (Sleep mode).
 * @param isOn true(On) false (Off).
 * @return 0 on success, -1 on failure.
 */
int Ssd1306::setDisplayOn(bool isOn) {
	uint8_t buf[] = {
		CONTROL_BYTE(0, 0),
		isOn ? DisplayState_On : DisplayState_Off
	};

	return i2cWrite(buf, sizeof(buf));
}

// TODO: this needs to be finished.. later..
/**
 * @brief: unimplemented
 * @param NA
 * @return 0 always
 */
int Ssd1306::setHorizontalScroll(bool isScrollRight, uint8_t startPageAddress, uint8_t timeInterval, uint8_t endPageAddress) {
	(void)isScrollRight;
	(void)startPageAddress;
	(void)timeInterval;
	(void)endPageAddress;

	return 0;
}

// TODO: this needs to be finished.. later..
/**
 * @brief: unimplemented
 * @param NA
 * @return 0 always
 */
int Ssd1306::setHorizontalAndVerticalScroll(bool isScrollRight, uint8_t startPageAddress, uint8_t timeInterval, uint8_t endPageAddress, uint8_t verticalOffset) {
	(void)isScrollRight;
	(void)startPageAddress;
	(void)timeInterval;
	(void)endPageAddress;
	(void)verticalOffset;

	return 0;
}

// TODO: this needs to be finished.. later..
/**
 * @brief: unimplemented
 * @param NA
 * @return 0 always
 */
int Ssd1306::setDeactivateScroll() {
	return 0;
}

// TODO: this needs to be finished.. later..
/**
 * @brief: unimplemented
 * @param NA
 * @return 0 always
 */
int Ssd1306::setActivateScroll() {
	return 0;
}

int Ssd1306::setVerticalScrollArea(uint8_t numFixedRows, uint8_t numScrollingRows) {
	(void)numFixedRows;
	(void)numScrollingRows;

	return 0;
}

// TODO: this needs to be finished.. later..
/**
 * @brief: unimplemented
 * @param NA
 * @return 0 always
 */
int setVerticalScrollArea(uint8_t numFixedRows, uint8_t numScrollingRows) {
	(void)numFixedRows;
	(void)numScrollingRows;

	return 0;
}

/**
 * @brief Sets the lower nibble of the column start address register
 * @param address
 * @return 0 on success, -1 on failure
 */
int Ssd1306::setLowerColumnAddress(uint8_t address) {
	uint8_t buf[] = {
		CONTROL_BYTE(0, 0),
		static_cast<uint8_t>(LowerColumnAddress | (address & 0xF))
	};

	return i2cWrite(buf, sizeof(buf));
}

/**
 * @brief Sets the upper nibble of the column start address register
 * @param address
 * @return 0 on success, -1 on failure
 */
int Ssd1306::setUpperColumnAddress(uint8_t address) {
	uint8_t buf[] = {
		CONTROL_BYTE(0, 0),
		static_cast<uint8_t>(UpperColumnAddress | ((address & 0xF0) >> 4))
	};

	return i2cWrite(buf, sizeof(buf));
}

/**
 * @brief Sets the memory addressing mode.
 * @param addressMode Horizontal, Vertical, or Page mode.
 * @return 0 on success, -1 on failure
 */
int Ssd1306::setMemoryAddressingMode(MemoryAddressMode_t memoryAddressMode) {
	uint8_t buf[] = {
		CONTROL_BYTE(0, 0),
		MemoryAddressMode,
		static_cast<uint8_t>(memoryAddressMode & 0x03)
	};

	return i2cWrite(buf, sizeof(buf));
}
/**
 * @brief Sets which RAM line is mapped to the top of the display (0-63).
 * @param columnAddressStart setup column start
 * @param columnAddressStart setup column end
 * @return 0 on success, -1 on failure
 */
int Ssd1306::setColumnAddress(uint8_t columnAddressStart, uint8_t columnAddressEnd) {
	uint8_t buf[] = {
		CONTROL_BYTE(0, 0),
		ColumnAddress,
		static_cast<uint8_t>(columnAddressStart & 0x7F),
		static_cast<uint8_t>(columnAddressEnd & 0x7F)
	};

	return i2cWrite(buf, sizeof(buf));
}
/**
 * @brief Sets which RAM line is mapped to the top of the display (0-63).
 * @param pageAddressStart setup page start
 * @param pageAddressStart setup page end
 * @return 0 on success, -1 on failure
 */
int Ssd1306::setPageAddress(uint8_t pageAddressStart, uint8_t pageAddressEnd) {
	uint8_t buf[] = {
		CONTROL_BYTE(0, 0),
		PageAddress,
		static_cast<uint8_t>(pageAddressStart & 0x7),
		static_cast<uint8_t>(pageAddressEnd & 0x7)
	};

	return i2cWrite(buf, sizeof(buf));
}
/**
 * @brief Sets which RAM line is mapped to the top of the display (0-63).
 * @param pageStartAddress
 * @return 0 on success, -1 on failure
 */
int Ssd1306::setPageStartAddress(uint8_t pageStartAddress) {
	uint8_t buf[] = {
		CONTROL_BYTE(0, 0),
		static_cast<uint8_t>(PageStartAddress | (pageStartAddress & 0x7))
	};

	return i2cWrite(buf, sizeof(buf));
}

/**
 * @brief Sets which RAM line is mapped to the top of the display (0-63).
 * @param startLine the first line
 * @return 0 on success, -1 on failure
 */
int Ssd1306::setDisplayStartLine(uint8_t startLine) {
	uint8_t buf[] = {
		CONTROL_BYTE(0, 0),
		static_cast<uint8_t>(DisplayStartLine | (startLine & 0x3F))
	};

	return i2cWrite(buf, sizeof(buf));
}

/**
 * @brief Remaps column addresses to SEG pins (Left/Right flip).
 * @param isSegmentRemapped false (0-0) true(0-127)
 * @return 0 on success, -1 on failure
 */
int Ssd1306::setSegmentRemap(bool isSegmentRemapped) {
	uint8_t buf[] = {
		CONTROL_BYTE(0, 0),
		isSegmentRemapped ? SegmentRemap_Reverse : SegmentRemap_Normal
	};

	return i2cWrite(buf, sizeof(buf));
}

/**
 * @brief Sets the multiplex ratio (duty cycle) of the display.
 * @param ratio the multiplex ratio
 * @return 0 on success, -1 on failure
 */
int Ssd1306::setMultiplexRatio(uint8_t ratio) {
	uint8_t buf[] = {
		CONTROL_BYTE(0, 0),
		MultiplexRatio,
		static_cast<uint8_t>(ratio & 0x3F)
	};

	return i2cWrite(buf, sizeof(buf));
}

/**
 * @brief Sets the COM output scan direction
 * @param isScanNormal true(incrementing) false(decrementing)
 * @return 0 on success, -1 on failure
 */
int Ssd1306::setComOutputScanDirection(bool isScanNormal) {
	uint8_t buf[] = {
		CONTROL_BYTE(0, 0),
		isScanNormal ? ComOutputScanDirection_Normal : ComOutputScanDirection_Remapped
	};

	return i2cWrite(buf, sizeof(buf));
}

/**
 * @brief Set vertical shift by COM.
 * @param displayOffset vertical shift [0,63]
 * @return 0 on success, -1 on failure
 */
int Ssd1306::setDisplayOffset(uint8_t displayOffset) {
	uint8_t buf[] = {
		CONTROL_BYTE(0, 0),
		DisplayOffset,
		static_cast<uint8_t>(displayOffset & 0x3F)
	};

	return i2cWrite(buf, sizeof(buf));
}

/**
 * @brief Sets the COM pins hardware configuration.
 * @param sequence sequential/alternative COM pin configuration
 * @param remap disable/enable COM pins left/right remap
 * @return 0 on success, -1 on failure
 */
int Ssd1306::setComPinsHardwareConfig(ComPinsHardwareConfigSequence_t sequence, ComPinsHardwareConfigRemap_t remap) {
	uint8_t buf[] = {
		CONTROL_BYTE(0, 0),
		ComPinsHardwareConfig,
		static_cast<uint8_t>(((remap & 0b1) << 5) | ((sequence & 0b1) << 4) | 0x02)
	};

	return i2cWrite(buf, sizeof(buf));
}

/**
 * @brief Sets the display clock divide ratio and oscillator frequency.
 * @param ratio divide ratio of the display clocks
 * @param frequency oscillator frequency
 * @return 0 on success, -1 on failure
 */
int Ssd1306::setDisplayClockDivideRatio(uint8_t ratio, uint8_t frequency) {
	uint8_t buf[] = {
		CONTROL_BYTE(0, 0),
		DisplayClock,
		static_cast<uint8_t>(((frequency & 0x0F) << 4) | (ratio & 0x0F))
	};

	return i2cWrite(buf, sizeof(buf));
}

/**
 * @brief Sets the phase 1/2 precharge period.
 * @param phase1 period in DCLK clock cycles
 * @param phase2 period in DCLK clock cycles
 * @return 0 on success, -1 on failure
 */
int Ssd1306::setPrechargePeriod(uint8_t phase1, uint8_t phase2) {
	uint8_t buf[] = {
		CONTROL_BYTE(0, 0),
		PreChargePeriod,
		static_cast<uint8_t>(((phase2 & 0x0F) << 4) | (phase1 & 0xF))
	};

	return i2cWrite(buf, sizeof(buf));
}

/**
 * @brief Sets the VCOMH deselect voltage level.
 * @param level COM deselect voltage level
 * @return 0 on success, -1 on failure
 */
int Ssd1306::setVComDeselectLevel(VComDeselectLevel_t level) {
	uint8_t buf[] = {
		CONTROL_BYTE(0, 0),
		VComDeselectLevel,
		static_cast<uint8_t>((level & 0b111) << 4)
	};

	return i2cWrite(buf, sizeof(buf));
}

/**
 * @brief Sets the command for no operation.
 * @return 0 on success, -1 on failure
 */
int Ssd1306::setNOP() {
	uint8_t buf[] = {
		CONTROL_BYTE(0, 0),
		NOP
	};

	return i2cWrite(buf, sizeof(buf));
}

/**
 * @brief Sets the charge pump enabled state.
 * @param setting Disabled or Enabled
 * @return 0 on success, -1 on failure
 */
int Ssd1306::setChargePumpSetting(ChargePumpSetting_t setting) {
	// From application node section 2. The pump must be enabled with the following sequence
	uint8_t buf[] = {
		CONTROL_BYTE(0, 0),
		ChargePumpSetting,
		static_cast<uint8_t>(((setting & 0b1) << 2) | 0x10)
	};

	return i2cWrite(buf, sizeof(buf));
}

int Ssd1306::i2cWrite(uint8_t* buf, uint16_t length) {
	if (write(i2cFd, buf, length) != length)
		return -1;
	return 0;
}

//==============================================================================
// Task Handler
//==============================================================================

//==============================================================================
// Interrupt
//==============================================================================
