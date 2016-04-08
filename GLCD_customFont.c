/*--------------------------------------------------
 * Name:    GLCD_customFonts.c
 * Purpose: Custom Graphic fonts 16x24 with 
 *          horizontal pixel packing
 * Rev.:    1.00
 *--------------------------------------------------
 *
 * Modification History
 * 09.12.15 Created (uVision5.17 + DFP2.6.0)
 *
 * Dr Mark Fisher, CMP, UEA, Norwich, UK
 *--------------------------------------------------*/
 
#include "Board_GLCD.h"
 
static const uint8_t customFont_16x24_h[] = {
/* PONG PADDLE */
  0x00, 0x3F, 0x00, 0x3F, 0x00, 0x3F, 0x00, 0x3F, 0x00, 0x3F, 0x00, 0x3F, 0x00, 0x3F, 0x00, 0x3F,
  0x00, 0x3F, 0x00, 0x3F, 0x00, 0x3F, 0x00, 0x3F, 0x00, 0x3F, 0x00, 0x3F, 0x00, 0x3F, 0x00, 0x3F,
  0x00, 0x3F, 0x00, 0x3F, 0x00, 0x3F, 0x00, 0x3F, 0x00, 0x3F, 0x00, 0x3F, 0x00, 0x3F, 0x00, 0x3F,  
};

GLCD_FONT GLCD_customFont_16x24 = {
  16,                                   ///< Character width
  24,                                   ///< Character height
  0,                                    ///< Character offset
  1,                                    ///< Character count
  customFont_16x24_h                    ///< Characters bitmaps
};
