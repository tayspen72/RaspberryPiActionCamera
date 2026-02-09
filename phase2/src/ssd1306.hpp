/*
 * tayspen72
 *
 * ssd1306.h
 */

#ifndef SSD1306_H_
#define SSD1306_H_

//==============================================================================
// Notes
//==============================================================================

//==============================================================================
// Definitions
//==============================================================================

//==============================================================================
// Includes
//==============================================================================
#include <stdint.h>

//==============================================================================
// Enumerations and Structures
//==============================================================================

//==============================================================================
// Classes
//==============================================================================
class Ssd1306
{
	public:
		int NumColumns = 0;
		int NumRows = 0;

		int Open(const char* i2cBusName);
		int Close();
		int ClearBuffer();
		int ClearDisplay();
		int DrawBuffer();
		int DrawPart(int page_start, int page_end, int column_start, int column_end);
		int Init();
		int WritePixel(int x, int y, bool isOn);
		int WriteString(int page, int column, const char* string, uint8_t foreground, uint8_t background);

		Ssd1306(int numColumns, int numRows);
		~Ssd1306();

	private:
		enum Command_t : uint8_t
		{
			// Fundamental Commands
			ContrastControl = 0x81,
			DisplayForceOn = 0xA4,
			DisplayForceOn_Disabled = 0xA4,
			DisplayForceOn_Enabled = 0xA5,
			DisplayInvert = 0xA6,
			DisplayInvert_Disabled = 0xA6,
			DisplayInvert_Enabled = 0xA7,
			DisplayState = 0xAE,
			DisplayState_Off = 0xAE,
			DisplayState_On = 0xAF,
			// Horizontal Scrolling
			HScrollDirection = 0x26,
			HScrollDirection_Right = 0x26,
			HScrollDirection_Left = 0x27,
			HScrollDummy1 = 0xA0,
			HScrollStartAddress = 0xB0,
			HScrollInterval = 0xC0,
			HScrollEndAddress = 0xD0,
			HScrollDummy2 = 0xE0,
			HScrollDummy3 = 0xFF,
			// Vertical Scrolling
			VScrollDirection = 0x29,
			VScrollDirection_Right = 0x29,
			VScrollDirection_Leftt = 0x2A,
			VScrollDummy1 = 0xA0,
			VScrollStartAddress = 0xB0,
			VScrollInterval = 0xC0,
			VScrollEndAddress = 0xD0,
			VScrollOffset = 0xE0,
			// Scrolling Commands
			DeactivateScroll = 0x2E,
			ActivateScroll = 0x2F,
			VScrollArea = 0xA3,
			VScrollAreaFixedRows = 0xA0,
			VScrollAreaScrollRows = 0xB0,
			// Addressing Settings
			LowerColumnAddress = 0x00,
			UpperColumnAddress = 0x10,
			MemoryAddressMode = 0x20,
			ColumnAddress = 0x21,
			PageAddress = 0x22,
			PageStartAddress = 0xB0,
			// Hardware Configuration
			DisplayStartLine = 0x40,
			SegmentRemap = 0xA0,
			SegmentRemap_Normal = 0xA0,
			SegmentRemap_Reverse = 0xA1,
			MultiplexRatio = 0xA8,
			ComOutputScanDirection = 0xC0,
			ComOutputScanDirection_Normal = 0xC0,
			ComOutputScanDirection_Remapped = 0xC8,
			DisplayOffset = 0xD3,
			ComPinsHardwareConfig = 0xDA,
			// Timing and Driving Scheme
			DisplayClock = 0xD5,
			PreChargePeriod = 0xD9,
			VComDeselectLevel = 0xDB,
			NOP = 0xE3,
			// Charge Pump
			ChargePumpSetting = 0x8D
		};

		enum MemoryAddressMode_t : uint8_t
		{
			Horizontal = 0b00,
			Vertical = 0b01,
			Page = 0b10
		};

		enum ComPinsHardwareConfigSequence_t : uint8_t
		{
			ComPinSequential = 0,
			ComPinAlternative = 1
		};

		enum ComPinsHardwareConfigRemap_t : uint8_t
		{
			ComPinRemapDisabled = 0,
			ComPinRemapEnabled = 1
		};

		enum VComDeselectLevel_t
		{
			Level_0_65 = 0b000,
			Level_0_77 = 0b010,
			Level_0_83 = 0b011
		};

		enum ChargePumpSetting_t
		{
			ChargePumpDisabled = 0,
			ChargePumpEnabled = 1
		};

		int i2cFd = 0;
		uint8_t* displayBuffer = nullptr;
		int displayBufferSize = 0;

		int init();
		// Graphics buffer functions
		int createBuffer();
		int clearBuffer();
		void destroyBuffer();
		int drawBuffer();
		int drawPart(int page_start, int page_end, int column_start, int column_end);
		// Write functions (write to buffer, not to display)
		int writeCharacter(int row, int column, char c, int foreground, int background);
		int writePixel(int x, int y, bool isOn);
		int writeString(int page, int column, const char*, int foreground, int background);

		// Fundamental Command Table
		int setContrast(uint8_t contrast);
		int setDisplayForceOn(bool isForcedOn);
		int setDisplayInvert(bool isInverted);
		int setDisplayOn(bool isOn);
		// Scrolling
		int setHorizontalScroll(bool isScrollRight, uint8_t startPageAddress, uint8_t timeInterval, uint8_t endPageAddress);
		int setHorizontalAndVerticalScroll(bool isScrollRight, uint8_t startPageAddress, uint8_t timeInterval, uint8_t endPageAddress, uint8_t verticalOffset);
		int setDeactivateScroll();
		int setActivateScroll();
		int setVerticalScrollArea(uint8_t numFixedRows, uint8_t numScrollingRows);
		// Address Setting Command Table
		int setLowerColumnAddress(uint8_t address);
		int setUpperColumnAddress(uint8_t address);
		int setMemoryAddressingMode(MemoryAddressMode_t memoryAddressMode);
		int setColumnAddress(uint8_t columnAddressStart, uint8_t columnAddressEnd);
		int setPageAddress(uint8_t pageAddressStart, uint8_t pageAddressEnd);
		int setPageStartAddress(uint8_t pageStartAddress);
		// Hardware Configuration
		int setDisplayStartLine(uint8_t startLine);
		int setSegmentRemap(bool isSegmentRemapped);
		int setMultiplexRatio(uint8_t multiplexRatio);
		int setComOutputScanDirection(bool isScanNormal);
		int setDisplayOffset(uint8_t displayOffset);
		int setComPinsHardwareConfig(ComPinsHardwareConfigSequence_t sequence, ComPinsHardwareConfigRemap_t remap);
		// Timing and Driving Scheme
		int setDisplayClockDivideRatio(uint8_t divideRatio, uint8_t oscillatorFrequency);
		int setPrechargePeriod(uint8_t phase1, uint8_t phase2);
		int setVComDeselectLevel(VComDeselectLevel_t deselectLevel);
		int setNOP();
		// Charge Pump Setting
		int setChargePumpSetting(ChargePumpSetting_t setting);
		// I2C Interface
		int i2cWrite(uint8_t* data, uint16_t length);
};

//==============================================================================
// Variables
//==============================================================================

//==============================================================================
// Macro Functions
//==============================================================================

#endif /* SSD1306_H_ */
