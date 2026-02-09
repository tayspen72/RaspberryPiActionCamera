/*
 * tayspen72
 *
 * ssd1306_font.hpp
 */

#ifndef SSD1306_FONT_H_
#define SSD1306_FONT_H_


//==============================================================================
// Notes
//==============================================================================


//==============================================================================
// Definitions
//==============================================================================
#define CHARACTER_WIDTH 5
#define CHARACTER_HEIGHT 8

#define FONT_8x5_A		{ 0x00, 0x7C, 0x12, 0x12, 0x7C }
#define FONT_8x5_B		{ 0x00, 0x7E, 0x4A, 0x4A, 0x34 }
#define FONT_8x5_C		{ 0x00, 0x3C, 0x42, 0x42, 0x42 }
#define FONT_8x5_D		{ 0x00, 0x7E, 0x42, 0x42, 0x3C }
#define FONT_8x5_E		{ 0x00, 0x7E, 0x4A, 0x4A, 0x42 }
#define FONT_8x5_F		{ 0x00, 0x7E, 0x0A, 0x0A, 0x02 }
#define FONT_8x5_G		{ 0x00, 0x3C, 0x42, 0x4A, 0x3A }
#define FONT_8x5_H		{ 0x00, 0x7E, 0x08, 0x08, 0x7E }
#define FONT_8x5_I		{ 0x00, 0x42, 0x7E, 0x42, 0x00 }
#define FONT_8x5_J		{ 0x00, 0x22, 0x42, 0x42, 0x3E }
#define FONT_8x5_K		{ 0x00, 0x7E, 0x08, 0x14, 0x72 }
#define FONT_8x5_L		{ 0x00, 0x7E, 0x40, 0x40, 0x40 }
#define FONT_8x5_M		{ 0x7E, 0x04, 0x08, 0x04, 0x7E }
#define FONT_8x5_N		{ 0x00, 0x7E, 0x04, 0x08, 0x7E }
#define FONT_8x5_O		{ 0x00, 0x3C, 0x42, 0x42, 0x3C }
#define FONT_8x5_P		{ 0x00, 0x7E, 0x0A, 0x0A, 0x04 }
#define FONT_8x5_Q		{ 0x3E, 0x42, 0x62, 0x7E, 0x40 }
#define FONT_8x5_R		{ 0x00, 0x7E, 0x0A, 0x1A, 0x64 }
#define FONT_8x5_S		{ 0x00, 0x44, 0x4A, 0x4A, 0x30 }
#define FONT_8x5_T		{ 0x00, 0x02, 0x7E, 0x02, 0x00 }
#define FONT_8x5_U		{ 0x00, 0x3E, 0x40, 0x40, 0x3E }
#define FONT_8x5_V		{ 0x00, 0x3E, 0x40, 0x3E, 0x00 }
#define FONT_8x5_W		{ 0x3E, 0x40, 0x3C, 0x40, 0x3E }
#define FONT_8x5_X		{ 0x00, 0x76, 0x08, 0x76, 0x00 }
#define FONT_8x5_Y		{ 0x00, 0x0E, 0x70, 0x0E, 0x00 }
#define FONT_8x5_Z		{ 0x00, 0x62, 0x52, 0x4A, 0x46 }
#define FONT_8x5_SPACE	{ 0x00, 0x00, 0x00, 0x00, 0x00 }

//==============================================================================
// Includes
//==============================================================================
#include "stdint.h"

//==============================================================================
// Enumerations and Structures
//==============================================================================
typedef uint8_t Character_t[5];

//==============================================================================
// Function Prototypes
//==============================================================================


//==============================================================================
// Variables
//==============================================================================
static Character_t _character_map[] = {
	FONT_8x5_A,
	FONT_8x5_B,
	FONT_8x5_C,
	FONT_8x5_D,
	FONT_8x5_E,
	FONT_8x5_F,
	FONT_8x5_G,
	FONT_8x5_H,
	FONT_8x5_I,
	FONT_8x5_J,
	FONT_8x5_K,
	FONT_8x5_L,
	FONT_8x5_M,
	FONT_8x5_N,
	FONT_8x5_O,
	FONT_8x5_P,
	FONT_8x5_Q,
	FONT_8x5_R,
	FONT_8x5_S,
	FONT_8x5_T,
	FONT_8x5_U,
	FONT_8x5_V,
	FONT_8x5_W,
	FONT_8x5_X,
	FONT_8x5_Y,
	FONT_8x5_Z,
	FONT_8x5_SPACE
};

//==============================================================================
// Macro Functions
//==============================================================================


#endif /* SSD1306_FONT_H_ */
