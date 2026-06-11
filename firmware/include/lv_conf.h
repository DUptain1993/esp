#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/*====================
   COLOR SETTINGS
 *====================*/
#define LV_COLOR_DEPTH 16
#define LV_COLOR_16_SWAP 1 // Diagnostic said this was likely needed

/*=========================
   MEMORY SETTINGS
 *=========================*/
#define LV_MEM_CUSTOM 0
#define LV_MEM_SIZE (64U * 1024U)

/*====================
   HAL SETTINGS
 *====================*/
#define LV_TICK_CUSTOM 1
#define LV_TICK_CUSTOM_INCLUDE "Arduino.h"
#define LV_TICK_CUSTOM_SYS_TIME_EXPR (millis())

#define LV_DPI_DEF 130

/*=======================
 * FEATURE CONFIGURATION
 *=======================*/
#define LV_USE_ASSERT_NULL          1
#define LV_USE_ASSERT_MEM           1
#define LV_USE_USER_DATA            1

/*===================
 *  GRAPHICS OBJECTS
 *===================*/
#define LV_USE_LABEL        1
#define LV_USE_BTN          1
#define LV_USE_BAR          1
#define LV_USE_LIST         1
#define LV_USE_TILEVIEW     1
#define LV_USE_TEXTAREA     1
#define LV_USE_CANVAS       1
#define LV_USE_LINE         1
#define LV_USE_ROLLER       1
#define LV_USE_SLIDER       1
#define LV_USE_SWITCH       1
#define LV_USE_TABLE        1

/*==================
 *  EXTRA COMPONENTS
 *==================*/
#define LV_USE_FLEX         1
#define LV_USE_GRID         1

/*==================
 *  FONT USAGE
 *==================*/
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_DEFAULT &lv_font_montserrat_14

#endif
