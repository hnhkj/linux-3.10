/*
 *
 * Copyright (c) 2016 Allwinnertech Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */
/*
 * A V4L2 driver for hm5065 cameras.
 *
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/videodev2.h>
#include <linux/clk.h>
#include <media/v4l2-device.h>
#include <media/v4l2-chip-ident.h>
#include <media/v4l2-mediabus.h>
#include <linux/io.h>


#include "camera.h"


MODULE_AUTHOR("raymonxiu");
MODULE_DESCRIPTION("A low-level driver for hm5065 sensors");
MODULE_LICENSE("GPL");

#define AF_WIN_NEW_COORD
//for internel driver debug
#define DEV_DBG_EN      0
#if(DEV_DBG_EN == 1)
#define vfe_dev_dbg(x,arg...) printk("[HM5065]"x,##arg)
#else
#define vfe_dev_dbg(x,arg...)
#endif
#define vfe_dev_err(x,arg...) printk("[HM5065]"x,##arg)
#define vfe_dev_print(x,arg...) printk("[HM5065]"x,##arg)

#define LOG_ERR_RET(x)  { \
                          int ret;  \
                          ret = x; \
                          if(ret < 0) {\
                            vfe_dev_err("error at %s\n",__func__);  \
                            return ret; \
                          } \
                        }

//define module timing
#define MCLK              (24*1000*1000)
#define VREF_POL          V4L2_MBUS_VSYNC_ACTIVE_LOW
#define HREF_POL          V4L2_MBUS_HSYNC_ACTIVE_HIGH
#define CLK_POL           V4L2_MBUS_PCLK_SAMPLE_RISING
#define V4L2_IDENT_SENSOR 0x039E

//define the voltage level of control signal
#define CSI_STBY_ON     0
#define CSI_STBY_OFF    1
#define CSI_RST_ON      0
#define CSI_RST_OFF     1
#define CSI_PWR_ON      1
#define CSI_PWR_OFF     0
#define CSI_AF_PWR_ON   1
#define CSI_AF_PWR_OFF  0

#define SENSOR_NAME "hm5065"
#define regval_list reg_list_a16_d8

#define REG_TERM 0xfffe
#define VAL_TERM 0xfe
#define REG_DLY  0xfffe

#define CONTINUEOUS_AF
//#define AUTO_FPS
#define DENOISE_LV_AUTO
#define SHARPNESS 0x10

#ifdef AUTO_FPS
//#define AF_FAST
#endif

#ifndef DENOISE_LV_AUTO
#define DENOISE_LV 0x8
#endif

static bool single_af = false;
static unsigned char af_pos_h,af_pos_l;

/*
 * Our nominal (default) frame rate.
 */
#define SENSOR_FRAME_RATE 30

/*
 * The hm5065 sits on i2c with ID 0x3e
 */
#define I2C_ADDR 0x3e

//static struct delayed_work sensor_s_ae_ratio_work;
static struct v4l2_subdev *glb_sd;

/*
 * Information we maintain about a known sensor.
 */
struct sensor_format_struct;  /* coming later */

struct cfg_array { /* coming later */
	struct regval_list * regs;
	int size;
};

static inline struct sensor_info *to_state(struct v4l2_subdev *sd)
{
  return container_of(sd, struct sensor_info, sd);
}


/*
 * The default register settings
 *
 */

static struct regval_list sensor_default_regs[] = {
	{0xFFFF,0x01},
	{0x9000,0x03},
	{0xA000,0x90},
	{0xA001,0x0C},
	{0xA002,0x56},
	{0xA003,0xE0},
	{0xA004,0xFE},
	{0xA005,0xA3},
	{0xA006,0xE0},
	{0xA007,0xFF},
	{0xA008,0x12},
	{0xA009,0x42},
	{0xA00A,0x85},
	{0xA00B,0x90},
	{0xA00C,0x01},
	{0xA00D,0xB7},
	{0xA00E,0xEE},
	{0xA00F,0xF0},
	{0xA010,0xFC},
	{0xA011,0xA3},
	{0xA012,0xEF},
	{0xA013,0xF0},
	{0xA014,0xFD},
	{0xA015,0x90},
	{0xA016,0x06},
	{0xA017,0x05},
	{0xA018,0xE0},
	{0xA019,0x75},
	{0xA01A,0xF0},
	{0xA01B,0x02},
	{0xA01C,0xA4},
	{0xA01D,0x2D},
	{0xA01E,0xFF},
	{0xA01F,0xE5},
	{0xA020,0xF0},
	{0xA021,0x3C},
	{0xA022,0xFE},
	{0xA023,0xAB},
	{0xA024,0x07},
	{0xA025,0xFA},
	{0xA026,0x33},
	{0xA027,0x95},
	{0xA028,0xE0},
	{0xA029,0xF9},
	{0xA02A,0xF8},
	{0xA02B,0x90},
	{0xA02C,0x0B},
	{0xA02D,0x4B},
	{0xA02E,0xE0},
	{0xA02F,0xFE},
	{0xA030,0xA3},
	{0xA031,0xE0},
	{0xA032,0xFF},
	{0xA033,0xEE},
	{0xA034,0x33},
	{0xA035,0x95},
	{0xA036,0xE0},
	{0xA037,0xFD},
	{0xA038,0xFC},
	{0xA039,0x12},
	{0xA03A,0x0C},
	{0xA03B,0x7B},
	{0xA03C,0x90},
	{0xA03D,0x01},
	{0xA03E,0xB9},
	{0xA03F,0x12},
	{0xA040,0x0E},
	{0xA041,0x05},
	{0xA042,0x90},
	{0xA043,0x01},
	{0xA044,0xB9},
	{0xA045,0xE0},
	{0xA046,0xFC},
	{0xA047,0xA3},
	{0xA048,0xE0},
	{0xA049,0xFD},
	{0xA04A,0xA3},
	{0xA04B,0xE0},
	{0xA04C,0xFE},
	{0xA04D,0xA3},
	{0xA04E,0xE0},
	{0xA04F,0xFF},
	{0xA050,0x78},
	{0xA051,0x08},
	{0xA052,0x12},
	{0xA053,0x0D},
	{0xA054,0xBF},
	{0xA055,0xA8},
	{0xA056,0x04},
	{0xA057,0xA9},
	{0xA058,0x05},
	{0xA059,0xAA},
	{0xA05A,0x06},
	{0xA05B,0xAB},
	{0xA05C,0x07},
	{0xA05D,0x90},
	{0xA05E,0x0B},
	{0xA05F,0x49},
	{0xA060,0xE0},
	{0xA061,0xFE},
	{0xA062,0xA3},
	{0xA063,0xE0},
	{0xA064,0xFF},
	{0xA065,0xEE},
	{0xA066,0x33},
	{0xA067,0x95},
	{0xA068,0xE0},
	{0xA069,0xFD},
	{0xA06A,0xFC},
	{0xA06B,0xC3},
	{0xA06C,0xEF},
	{0xA06D,0x9B},
	{0xA06E,0xFF},
	{0xA06F,0xEE},
	{0xA070,0x9A},
	{0xA071,0xFE},
	{0xA072,0xED},
	{0xA073,0x99},
	{0xA074,0xFD},
	{0xA075,0xEC},
	{0xA076,0x98},
	{0xA077,0xFC},
	{0xA078,0x78},
	{0xA079,0x01},
	{0xA07A,0x12},
	{0xA07B,0x0D},
	{0xA07C,0xBF},
	{0xA07D,0x90},
	{0xA07E,0x0C},
	{0xA07F,0x4A},
	{0xA080,0xE0},
	{0xA081,0xFC},
	{0xA082,0xA3},
	{0xA083,0xE0},
	{0xA084,0xF5},
	{0xA085,0x82},
	{0xA086,0x8C},
	{0xA087,0x83},
	{0xA088,0xC0},
	{0xA089,0x83},
	{0xA08A,0xC0},
	{0xA08B,0x82},
	{0xA08C,0x90},
	{0xA08D,0x0B},
	{0xA08E,0x48},
	{0xA08F,0xE0},
	{0xA090,0xD0},
	{0xA091,0x82},
	{0xA092,0xD0},
	{0xA093,0x83},
	{0xA094,0x75},
	{0xA095,0xF0},
	{0xA096,0x02},
	{0xA097,0x12},
	{0xA098,0x0E},
	{0xA099,0x45},
	{0xA09A,0xEE},
	{0xA09B,0xF0},
	{0xA09C,0xA3},
	{0xA09D,0xEF},
	{0xA09E,0xF0},
	{0xA09F,0x02},
	{0xA0A0,0xBA},
	{0xA0A1,0xD8},
	{0xA0A2,0x90},
	{0xA0A3,0x30},
	{0xA0A4,0x18},
	{0xA0A5,0xe4},
	{0xA0A6,0xf0},
	{0xA0A7,0x74},
	{0xA0A8,0x3f},
	{0xA0A9,0xf0},
	{0xA0AA,0x22},
	{0xA0BF,0x90},
	{0xA0C0,0x00},
	{0xA0C1,0x5E},
	{0xA0C2,0xE0},
	{0xA0C3,0xFF},
	{0xA0C4,0x70},
	{0xA0C5,0x20},
	{0xA0C6,0x90},
	{0xA0C7,0x47},
	{0xA0C8,0x04},
	{0xA0C9,0x74},
	{0xA0CA,0x0A},
	{0xA0CB,0xF0},
	{0xA0CC,0xA3},
	{0xA0CD,0x74},
	{0xA0CE,0x30},
	{0xA0CF,0xF0},
	{0xA0D0,0x90},
	{0xA0D1,0x47},
	{0xA0D2,0x0C},
	{0xA0D3,0x74},
	{0xA0D4,0x07},
	{0xA0D5,0xF0},
	{0xA0D6,0xA3},
	{0xA0D7,0x74},
	{0xA0D8,0xA8},
	{0xA0D9,0xF0},
	{0xA0DA,0x90},
	{0xA0DB,0x47},
	{0xA0DC,0xA4},
	{0xA0DD,0x74},
	{0xA0DE,0x01},
	{0xA0DF,0xF0},
	{0xA0E0,0x90},
	{0xA0E1,0x47},
	{0xA0E2,0xA8},
	{0xA0E3,0xF0},
	{0xA0E4,0x80},
	{0xA0E5,0x50},
	{0xA0E6,0xEF},
	{0xA0E7,0x64},
	{0xA0E8,0x01},
	{0xA0E9,0x60},
	{0xA0EA,0x04},
	{0xA0EB,0xEF},
	{0xA0EC,0xB4},
	{0xA0ED,0x03},
	{0xA0EE,0x20},
	{0xA0EF,0x90},
	{0xA0F0,0x47},
	{0xA0F1,0x04},
	{0xA0F2,0x74},
	{0xA0F3,0x05},
	{0xA0F4,0xF0},
	{0xA0F5,0xA3},
	{0xA0F6,0x74},
	{0xA0F7,0x18},
	{0xA0F8,0xF0},
	{0xA0F9,0x90},
	{0xA0FA,0x47},
	{0xA0FB,0x0C},
	{0xA0FC,0x74},
	{0xA0FD,0x03},
	{0xA0FE,0xF0},
	{0xA0FF,0xA3},
	{0xA100,0x74},
	{0xA101,0xD4},
	{0xA102,0xF0},
	{0xA103,0x90},
	{0xA104,0x47},
	{0xA105,0xA4},
	{0xA106,0x74},
	{0xA107,0x02},
	{0xA108,0xF0},
	{0xA109,0x90},
	{0xA10A,0x47},
	{0xA10B,0xA8},
	{0xA10C,0xF0},
	{0xA10D,0x80},
	{0xA10E,0x27},
	{0xA10F,0xEF},
	{0xA110,0x64},
	{0xA111,0x02},
	{0xA112,0x60},
	{0xA113,0x04},
	{0xA114,0xEF},
	{0xA115,0xB4},
	{0xA116,0x04},
	{0xA117,0x1E},
	{0xA118,0x90},
	{0xA119,0x47},
	{0xA11A,0x04},
	{0xA11B,0x74},
	{0xA11C,0x02},
	{0xA11D,0xF0},
	{0xA11E,0xA3},
	{0xA11F,0x74},
	{0xA120,0x8C},
	{0xA121,0xF0},
	{0xA122,0x90},
	{0xA123,0x47},
	{0xA124,0x0C},
	{0xA125,0x74},
	{0xA126,0x01},
	{0xA127,0xF0},
	{0xA128,0xA3},
	{0xA129,0x74},
	{0xA12A,0xEA},
	{0xA12B,0xF0},
	{0xA12C,0x90},
	{0xA12D,0x47},
	{0xA12E,0xA4},
	{0xA12F,0x74},
	{0xA130,0x04},
	{0xA131,0xF0},
	{0xA132,0x90},
	{0xA133,0x47},
	{0xA134,0xA8},
	{0xA135,0xF0},
	{0xA136,0x22},
	{0xA137,0x74},
	{0xA138,0x04},
	{0xA139,0xF0},
	{0xA13A,0xA3},
	{0xA13B,0x74},
	{0xA13C,0x20},
	{0xA13D,0xF0},
	{0xA13E,0xE4},
	{0xA13F,0xF5},
	{0xA140,0x22},
	{0xA141,0xE5},
	{0xA142,0x22},
	{0xA143,0xC3},
	{0xA144,0x94},
	{0xA145,0x40},
	{0xA146,0x40},
	{0xA147,0x03},
	{0xA148,0x02},
	{0xA149,0xF1},
	{0xA14A,0xFD},
	{0xA14B,0x90},
	{0xA14C,0x0A},
	{0xA14D,0xBA},
	{0xA14E,0xE0},
	{0xA14F,0xFE},
	{0xA150,0xA3},
	{0xA151,0xE0},
	{0xA152,0xFF},
	{0xA153,0xF5},
	{0xA154,0x82},
	{0xA155,0x8E},
	{0xA156,0x83},
	{0xA157,0xE0},
	{0xA158,0x54},
	{0xA159,0x70},
	{0xA15A,0xFD},
	{0xA15B,0xC4},
	{0xA15C,0x54},
	{0xA15D,0x0F},
	{0xA15E,0xFD},
	{0xA15F,0x90},
	{0xA160,0x0A},
	{0xA161,0xBC},
	{0xA162,0xE0},
	{0xA163,0xFA},
	{0xA164,0xA3},
	{0xA165,0xE0},
	{0xA166,0xF5},
	{0xA167,0x82},
	{0xA168,0x8A},
	{0xA169,0x83},
	{0xA16A,0xED},
	{0xA16B,0xF0},
	{0xA16C,0x90},
	{0xA16D,0x0A},
	{0xA16E,0xBD},
	{0xA16F,0xE0},
	{0xA170,0x04},
	{0xA171,0xF0},
	{0xA172,0x70},
	{0xA173,0x06},
	{0xA174,0x90},
	{0xA175,0x0A},
	{0xA176,0xBC},
	{0xA177,0xE0},
	{0xA178,0x04},
	{0xA179,0xF0},
	{0xA17A,0x8F},
	{0xA17B,0x82},
	{0xA17C,0x8E},
	{0xA17D,0x83},
	{0xA17E,0xA3},
	{0xA17F,0xE0},
	{0xA180,0xFF},
	{0xA181,0x90},
	{0xA182,0x0A},
	{0xA183,0xBC},
	{0xA184,0xE0},
	{0xA185,0xFC},
	{0xA186,0xA3},
	{0xA187,0xE0},
	{0xA188,0xF5},
	{0xA189,0x82},
	{0xA18A,0x8C},
	{0xA18B,0x83},
	{0xA18C,0xEF},
	{0xA18D,0xF0},
	{0xA18E,0x90},
	{0xA18F,0x0A},
	{0xA190,0xBD},
	{0xA191,0xE0},
	{0xA192,0x04},
	{0xA193,0xF0},
	{0xA194,0x70},
	{0xA195,0x06},
	{0xA196,0x90},
	{0xA197,0x0A},
	{0xA198,0xBC},
	{0xA199,0xE0},
	{0xA19A,0x04},
	{0xA19B,0xF0},
	{0xA19C,0x90},
	{0xA19D,0x0A},
	{0xA19E,0xBA},
	{0xA19F,0xE0},
	{0xA1A0,0xFE},
	{0xA1A1,0xA3},
	{0xA1A2,0xE0},
	{0xA1A3,0xFF},
	{0xA1A4,0xF5},
	{0xA1A5,0x82},
	{0xA1A6,0x8E},
	{0xA1A7,0x83},
	{0xA1A8,0xE0},
	{0xA1A9,0x54},
	{0xA1AA,0x07},
	{0xA1AB,0xFD},
	{0xA1AC,0x90},
	{0xA1AD,0x0A},
	{0xA1AE,0xBC},
	{0xA1AF,0xE0},
	{0xA1B0,0xFA},
	{0xA1B1,0xA3},
	{0xA1B2,0xE0},
	{0xA1B3,0xF5},
	{0xA1B4,0x82},
	{0xA1B5,0x8A},
	{0xA1B6,0x83},
	{0xA1B7,0xED},
	{0xA1B8,0xF0},
	{0xA1B9,0x90},
	{0xA1BA,0x0A},
	{0xA1BB,0xBD},
	{0xA1BC,0xE0},
	{0xA1BD,0x04},
	{0xA1BE,0xF0},
	{0xA1BF,0x70},
	{0xA1C0,0x06},
	{0xA1C1,0x90},
	{0xA1C2,0x0A},
	{0xA1C3,0xBC},
	{0xA1C4,0xE0},
	{0xA1C5,0x04},
	{0xA1C6,0xF0},
	{0xA1C7,0x8F},
	{0xA1C8,0x82},
	{0xA1C9,0x8E},
	{0xA1CA,0x83},
	{0xA1CB,0xA3},
	{0xA1CC,0xA3},
	{0xA1CD,0xE0},
	{0xA1CE,0xFF},
	{0xA1CF,0x90},
	{0xA1D0,0x0A},
	{0xA1D1,0xBC},
	{0xA1D2,0xE0},
	{0xA1D3,0xFC},
	{0xA1D4,0xA3},
	{0xA1D5,0xE0},
	{0xA1D6,0xF5},
	{0xA1D7,0x82},
	{0xA1D8,0x8C},
	{0xA1D9,0x83},
	{0xA1DA,0xEF},
	{0xA1DB,0xF0},
	{0xA1DC,0x90},
	{0xA1DD,0x0A},
	{0xA1DE,0xBD},
	{0xA1DF,0xE0},
	{0xA1E0,0x04},
	{0xA1E1,0xF0},
	{0xA1E2,0x70},
	{0xA1E3,0x06},
	{0xA1E4,0x90},
	{0xA1E5,0x0A},
	{0xA1E6,0xBC},
	{0xA1E7,0xE0},
	{0xA1E8,0x04},
	{0xA1E9,0xF0},
	{0xA1EA,0x90},
	{0xA1EB,0x0A},
	{0xA1EC,0xBB},
	{0xA1ED,0xE0},
	{0xA1EE,0x24},
	{0xA1EF,0x03},
	{0xA1F0,0xF0},
	{0xA1F1,0x90},
	{0xA1F2,0x0A},
	{0xA1F3,0xBA},
	{0xA1F4,0xE0},
	{0xA1F5,0x34},
	{0xA1F6,0x00},
	{0xA1F7,0xF0},
	{0xA1F8,0x05},
	{0xA1F9,0x22},
	{0xA1FA,0x02},
	{0xA1FB,0xF1},
	{0xA1FC,0x41},
	{0xA1FD,0x90},
	{0xA1FE,0x0A},
	{0xA1FF,0xBA},
	{0xA200,0x74},
	{0xA201,0x0E},
	{0xA202,0xF0},
	{0xA203,0xA3},
	{0xA204,0x74},
	{0xA205,0xDC},
	{0xA206,0xF0},
	{0xA207,0xA3},
	{0xA208,0x74},
	{0xA209,0x05},
	{0xA20A,0xF0},
	{0xA20B,0xA3},
	{0xA20C,0x74},
	{0xA20D,0x61},
	{0xA20E,0xF0},
	{0xA20F,0x90},
	{0xA210,0x0A},
	{0xA211,0xBA},
	{0xA212,0xE0},
	{0xA213,0xFE},
	{0xA214,0xA3},
	{0xA215,0xE0},
	{0xA216,0xAA},
	{0xA217,0x06},
	{0xA218,0xF9},
	{0xA219,0x7B},
	{0xA21A,0x01},
	{0xA21B,0xC0},
	{0xA21C,0x02},
	{0xA21D,0xA3},
	{0xA21E,0xE0},
	{0xA21F,0xFE},
	{0xA220,0xA3},
	{0xA221,0xE0},
	{0xA222,0xAA},
	{0xA223,0x06},
	{0xA224,0xF8},
	{0xA225,0xAC},
	{0xA226,0x02},
	{0xA227,0x7D},
	{0xA228,0x01},
	{0xA229,0xD0},
	{0xA22A,0x02},
	{0xA22B,0x7E},
	{0xA22C,0x00},
	{0xA22D,0x7F},
	{0xA22E,0x04},
	{0xA22F,0x12},
	{0xA230,0x0F},
	{0xA231,0x6F},
	{0xA232,0x02},
	{0xA233,0x66},
	{0xA234,0xD9},
	{0xA235,0x90},
	{0xA236,0x07},
	{0xA237,0xD0},
	{0xA238,0x02},
	{0xA239,0xA2},
	{0xA23A,0x69},
	{0xA240,0x02},
	{0xA241,0x21},
	{0xA242,0x7F},
	{0xA243,0x02},
	{0xA244,0x21},
	{0xA245,0xF4},
	{0xA246,0x02},
	{0xA247,0xA6},
	{0xA248,0x15},
	{0xA249,0x60},
	{0xA24A,0x0A},
	{0xA24B,0xEF},
	{0xA24C,0xB4},
	{0xA24D,0x01},
	{0xA24E,0x16},
	{0xA24F,0x90},
	{0xA250,0x00},
	{0xA251,0x5D},
	{0xA252,0xE0},
	{0xA253,0x70},
	{0xA254,0x10},
	{0xA255,0x12},
	{0xA256,0x26},
	{0xA257,0xC8},
	{0xA258,0x90},
	{0xA259,0x00},
	{0xA25A,0x11},
	{0xA25B,0x74},
	{0xA25C,0x30},
	{0xA25D,0xF0},
	{0xA25E,0x90},
	{0xA25F,0x00},
	{0xA260,0x10},
	{0xA261,0x74},
	{0xA262,0x01},
	{0xA263,0xF0},
	{0xA264,0x22},
	{0xA265,0x12},
	{0xA266,0x25},
	{0xA267,0xA8},
	{0xA268,0x02},
	{0xA269,0x29},
	{0xA26A,0xFC},
	{0xA26B,0x44},
	{0xA26C,0x18},
	{0xA26D,0xF0},
	{0xA26E,0x90},
	{0xA26F,0x72},
	{0xA270,0x18},
	{0xA271,0xE0},
	{0xA272,0x44},
	{0xA273,0x18},
	{0xA274,0xF0},
	{0xA275,0x00},
	{0xA276,0x00},
	{0xA277,0x00},
	{0xA278,0x00},
	{0xA279,0x00},
	{0xA27A,0x00},
	{0xA27B,0x90},
	{0xA27C,0x72},
	{0xA27D,0x08},
	{0xA27E,0xE0},
	{0xA27F,0x44},
	{0xA280,0x10},
	{0xA281,0xF0},
	{0xA282,0x90},
	{0xA283,0x72},
	{0xA284,0x14},
	{0xA285,0xE0},
	{0xA286,0x54},
	{0xA287,0xFD},
	{0xA288,0xF0},
	{0xA289,0x22},
	{0xA29B,0xF0},
	{0xA29C,0xD3},
	{0xA29D,0x90},
	{0xA29E,0x07},
	{0xA29F,0x91},
	{0xA2A0,0xE0},
	{0xA2A1,0x94},
	{0xA2A2,0x21},
	{0xA2A3,0x90},
	{0xA2A4,0x07},
	{0xA2A5,0x90},
	{0xA2A6,0xE0},
	{0xA2A7,0x64},
	{0xA2A8,0x80},
	{0xA2A9,0x94},
	{0xA2AA,0x81},
	{0xA2AB,0x40},
	{0xA2AC,0x08},
	{0xA2AD,0x90},
	{0xA2AE,0x07},
	{0xA2AF,0xCB},
	{0xA2B0,0x74},
	{0xA2B1,0xFF},
	{0xA2B2,0xF0},
	{0xA2B3,0x80},
	{0xA2B4,0x06},
	{0xA2B5,0x90},
	{0xA2B6,0x07},
	{0xA2B7,0xCB},
	{0xA2B8,0x74},
	{0xA2B9,0x01},
	{0xA2BA,0xF0},
	{0xA2BB,0x02},
	{0xA2BC,0xB5},
	{0xA2BD,0xC3},
	{0xA2BE,0x90},
	{0xA2BF,0x08},
	{0xA2C0,0x34},
	{0xA2C1,0xE0},
	{0xA2C2,0xFC},
	{0xA2C3,0xA3},
	{0xA2C4,0xE0},
	{0xA2C5,0xFD},
	{0xA2C6,0xA3},
	{0xA2C7,0xE0},
	{0xA2C8,0xFE},
	{0xA2C9,0xA3},
	{0xA2CA,0xE0},
	{0xA2CB,0xFF},
	{0xA2CC,0x90},
	{0xA2CD,0x07},
	{0xA2CE,0xD0},
	{0xA2CF,0xE0},
	{0xA2D0,0xF8},
	{0xA2D1,0xA3},
	{0xA2D2,0xE0},
	{0xA2D3,0xF9},
	{0xA2D4,0xA3},
	{0xA2D5,0xE0},
	{0xA2D6,0xFA},
	{0xA2D7,0xA3},
	{0xA2D8,0xE0},
	{0xA2D9,0xFB},
	{0xA2DA,0xD3},
	{0xA2DB,0x12},
	{0xA2DC,0x0D},
	{0xA2DD,0xAE},
	{0xA2DE,0x40},
	{0xA2DF,0x0B},
	{0xA2E0,0x12},
	{0xA2E1,0xB5},
	{0xA2E2,0x49},
	{0xA2E3,0x90},
	{0xA2E4,0x07},
	{0xA2E5,0xA4},
	{0xA2E6,0x74},
	{0xA2E7,0x02},
	{0xA2E8,0xF0},
	{0xA2E9,0x80},
	{0xA2EA,0x09},
	{0xA2EB,0x12},
	{0xA2EC,0xB7},
	{0xA2ED,0x51},
	{0xA2EE,0x90},
	{0xA2EF,0x07},
	{0xA2F0,0xA4},
	{0xA2F1,0x74},
	{0xA2F2,0x05},
	{0xA2F3,0xF0},
	{0xA2F4,0x02},
	{0xA2F5,0xA2},
	{0xA2F6,0xDA},
	{0xA2F7,0x90},
	{0xA2F8,0x0E},
	{0xA2F9,0xE0},
	{0xA2FA,0xE0},
	{0xA2FB,0xFD},
	{0xA2FC,0xA3},
	{0xA2FD,0xE0},
	{0xA2FE,0x90},
	{0xA2FF,0x02},
	{0xA300,0xA2},
	{0xA301,0xCD},
	{0xA302,0xF0},
	{0xA303,0xA3},
	{0xA304,0xED},
	{0xA305,0xF0},
	{0xA306,0x90},
	{0xA307,0x0E},
	{0xA308,0xE2},
	{0xA309,0xE0},
	{0xA30A,0xFD},
	{0xA30B,0xA3},
	{0xA30C,0xE0},
	{0xA30D,0x90},
	{0xA30E,0x02},
	{0xA30F,0xA8},
	{0xA310,0xCD},
	{0xA311,0xF0},
	{0xA312,0xA3},
	{0xA313,0xED},
	{0xA314,0xF0},
	{0xA315,0xE4},
	{0xA316,0x90},
	{0xA317,0x06},
	{0xA318,0x38},
	{0xA319,0xF0},
	{0xA31A,0x02},
	{0xA31B,0x67},
	{0xA31C,0x63},
	{0xA31D,0x90},
	{0xA31E,0x0E},
	{0xA31F,0xE8},
	{0xA320,0xE0},
	{0xA321,0x90},
	{0xA322,0x02},
	{0xA323,0x62},
	{0xA324,0xF0},
	{0xA325,0x90},
	{0xA326,0x0E},
	{0xA327,0xE9},
	{0xA328,0xE0},
	{0xA329,0x90},
	{0xA32A,0x02},
	{0xA32B,0x63},
	{0xA32C,0xF0},
	{0xA32D,0x02},
	{0xA32E,0x67},
	{0xA32F,0x1F},
	{0xA33B,0x90},
	{0xA33C,0x0E},
	{0xA33D,0x14},
	{0xA33E,0xE0},
	{0xA33F,0xFE},
	{0xA340,0xA3},
	{0xA341,0xE0},
	{0xA342,0xFF},
	{0xA343,0x90},
	{0xA344,0x06},
	{0xA345,0xD9},
	{0xA346,0xEE},
	{0xA347,0xF0},
	{0xA348,0xA3},
	{0xA349,0xEF},
	{0xA34A,0xF0},
	{0xA34B,0x90},
	{0xA34C,0x0E},
	{0xA34D,0x18},
	{0xA34E,0xE0},
	{0xA34F,0xFD},
	{0xA350,0x7C},
	{0xA351,0x00},
	{0xA352,0xC3},
	{0xA353,0xEF},
	{0xA354,0x9D},
	{0xA355,0xEE},
	{0xA356,0x9C},
	{0xA357,0x50},
	{0xA358,0x09},
	{0xA359,0xE4},
	{0xA35A,0x90},
	{0xA35B,0x06},
	{0xA35C,0xD7},
	{0xA35D,0xF0},
	{0xA35E,0xA3},
	{0xA35F,0xF0},
	{0xA360,0x80},
	{0xA361,0x13},
	{0xA362,0xC3},
	{0xA363,0x90},
	{0xA364,0x06},
	{0xA365,0xDA},
	{0xA366,0xE0},
	{0xA367,0x9D},
	{0xA368,0xFE},
	{0xA369,0x90},
	{0xA36A,0x06},
	{0xA36B,0xD9},
	{0xA36C,0xE0},
	{0xA36D,0x9C},
	{0xA36E,0x90},
	{0xA36F,0x06},
	{0xA370,0xD7},
	{0xA371,0xF0},
	{0xA372,0xA3},
	{0xA373,0xCE},
	{0xA374,0xF0},
	{0xA375,0x90},
	{0xA376,0x0E},
	{0xA377,0x18},
	{0xA378,0xE0},
	{0xA379,0xF9},
	{0xA37A,0xFF},
	{0xA37B,0x90},
	{0xA37C,0x06},
	{0xA37D,0xC2},
	{0xA37E,0xE0},
	{0xA37F,0xFC},
	{0xA380,0xA3},
	{0xA381,0xE0},
	{0xA382,0xFD},
	{0xA383,0xC3},
	{0xA384,0x9F},
	{0xA385,0xFF},
	{0xA386,0xEC},
	{0xA387,0x94},
	{0xA388,0x00},
	{0xA389,0xFE},
	{0xA38A,0x90},
	{0xA38B,0x0E},
	{0xA38C,0x16},
	{0xA38D,0xE0},
	{0xA38E,0xFA},
	{0xA38F,0xA3},
	{0xA390,0xE0},
	{0xA391,0xFB},
	{0xA392,0xD3},
	{0xA393,0x9F},
	{0xA394,0xEA},
	{0xA395,0x9E},
	{0xA396,0x40},
	{0xA397,0x0A},
	{0xA398,0x90},
	{0xA399,0x06},
	{0xA39A,0xD5},
	{0xA39B,0xEC},
	{0xA39C,0xF0},
	{0xA39D,0xA3},
	{0xA39E,0xED},
	{0xA39F,0xF0},
	{0xA3A0,0x80},
	{0xA3A1,0x0E},
	{0xA3A2,0xE9},
	{0xA3A3,0x7E},
	{0xA3A4,0x00},
	{0xA3A5,0x2B},
	{0xA3A6,0xFF},
	{0xA3A7,0xEE},
	{0xA3A8,0x3A},
	{0xA3A9,0x90},
	{0xA3AA,0x06},
	{0xA3AB,0xD5},
	{0xA3AC,0xF0},
	{0xA3AD,0xA3},
	{0xA3AE,0xEF},
	{0xA3AF,0xF0},
	{0xA3B0,0xE9},
	{0xA3B1,0xFB},
	{0xA3B2,0x7A},
	{0xA3B3,0x00},
	{0xA3B4,0x90},
	{0xA3B5,0x0E},
	{0xA3B6,0x15},
	{0xA3B7,0xE0},
	{0xA3B8,0x2B},
	{0xA3B9,0xFE},
	{0xA3BA,0x90},
	{0xA3BB,0x0E},
	{0xA3BC,0x14},
	{0xA3BD,0xE0},
	{0xA3BE,0x3A},
	{0xA3BF,0x90},
	{0xA3C0,0x06},
	{0xA3C1,0xE1},
	{0xA3C2,0xF0},
	{0xA3C3,0xA3},
	{0xA3C4,0xCE},
	{0xA3C5,0xF0},
	{0xA3C6,0xC3},
	{0xA3C7,0x90},
	{0xA3C8,0x0E},
	{0xA3C9,0x17},
	{0xA3CA,0xE0},
	{0xA3CB,0x9B},
	{0xA3CC,0xFE},
	{0xA3CD,0x90},
	{0xA3CE,0x0E},
	{0xA3CF,0x16},
	{0xA3D0,0x02},
	{0xA3D1,0x20},
	{0xA3D2,0xD5},
	{0xA3D3,0x90},
	{0xA3d4,0x0E},
	{0xA3d5,0xE4},
	{0xA3d6,0xE0},
	{0xA3d7,0x90},
	{0xA3d8,0x02},
	{0xA3d9,0x66},
	{0xA3da,0xF0},
	{0xA3DB,0x90},
	{0xA3dc,0x0E},
	{0xA3dd,0xE5},
	{0xA3de,0xE0},
	{0xA3df,0x90},
	{0xA3e0,0x02},
	{0xA3e1,0x64},
	{0xA3e2,0xF0},
	{0xA3e3,0x90},
	{0xA3e4,0x0E},
	{0xA3e5,0xE6},
	{0xA3e6,0xE0},
	{0xA3e7,0x90},
	{0xA3e8,0x02},
	{0xA3e9,0x65},
	{0xA3ea,0xF0},
	{0xA3eb,0x02},
	{0xA3ec,0x67},
	{0xA3ed,0xA5},
	{0xA3f0,0x12},
	{0xA3f1,0x47},
	{0xA3f2,0x59},
	{0xA3f3,0x90},
	{0xA3f4,0x00},
	{0xA3f5,0xB5},
	{0xA3f6,0xE0},
	{0xA3f7,0xB4},
	{0xA3f8,0x02},
	{0xA3f9,0x03},
	{0xA3fa,0x12},
	{0xA3fb,0x47},
	{0xA3fc,0x59},
	{0xA3fd,0x02},
	{0xA3fe,0xC5},
	{0xA3ff,0xC3},
	{0xA400,0x90},
	{0xA401,0x00},
	{0xA402,0x3D},
	{0xA403,0xF0},
	{0xA404,0x90},
	{0xA405,0x00},
	{0xA406,0x84},
	{0xA407,0xE0},
	{0xA408,0xFE},
	{0xA409,0x90},
	{0xA40A,0x00},
	{0xA40B,0x3E},
	{0xA40C,0xF0},
	{0xA40D,0xEF},
	{0xA40E,0x70},
	{0xA40F,0x03},
	{0xA410,0xEE},
	{0xA411,0x60},
	{0xA412,0x04},
	{0xA413,0x7F},
	{0xA414,0x01},
	{0xA415,0x80},
	{0xA416,0x02},
	{0xA417,0x7F},
	{0xA418,0x00},
	{0xA419,0x90},
	{0xA41A,0x00},
	{0xA41B,0x3F},
	{0xA41C,0xEF},
	{0xA41D,0xF0},
	{0xA41E,0x02},
	{0xA41F,0x89},
	{0xA420,0xD3},
	{0xA421,0x90},
	{0xA422,0x00},
	{0xA423,0x12},
	{0xA424,0xE0},
	{0xA425,0xFF},
	{0xA426,0x70},
	{0xA427,0x0C},
	{0xA428,0x90},
	{0xA429,0x00},
	{0xA42A,0x46},
	{0xA42B,0xE0},
	{0xA42C,0xC3},
	{0xA42D,0x94},
	{0xA42E,0x07},
	{0xA42F,0x40},
	{0xA430,0x03},
	{0xA431,0x75},
	{0xA432,0x2E},
	{0xA433,0x02},
	{0xA434,0xEF},
	{0xA435,0xB4},
	{0xA436,0x01},
	{0xA437,0x0C},
	{0xA438,0x90},
	{0xA439,0x00},
	{0xA43A,0x66},
	{0xA43B,0xE0},
	{0xA43C,0xC3},
	{0xA43D,0x94},
	{0xA43E,0x07},
	{0xA43F,0x40},
	{0xA440,0x03},
	{0xA441,0x75},
	{0xA442,0x2E},
	{0xA443,0x02},
	{0xA444,0x02},
	{0xA445,0xA7},
	{0xA446,0x9E},
	{0xA447,0xC3},
	{0xA448,0x90},
	{0xA449,0x0B},
	{0xA44A,0x8F},
	{0xA44B,0xE0},
	{0xA44C,0x94},
	{0xA44D,0x00},//80
	{0xA44E,0x90},
	{0xA44F,0x0B},
	{0xA450,0x8E},
	{0xA451,0xE0},
	{0xA452,0x94},
	{0xA453,0x41},//44
	{0xA454,0x40},
	{0xA455,0x22},
	{0xA456,0x90},
	{0xA457,0x0B},
	{0xA458,0x91},
	{0xA459,0xE0},
	{0xA45A,0x94},
	{0xA45B,0x00},//80
	{0xA45C,0x90},
	{0xA45D,0x0B},
	{0xA45E,0x90},
	{0xA45F,0xE0},
	{0xA460,0x94},
	{0xA461,0x41}, //44
	{0xA462,0x40},
	{0xA463,0x14},
	{0xA464,0x90},
	{0xA465,0x0B},
	{0xA466,0x93},
	{0xA467,0xE0},
	{0xA468,0x94},
	{0xA469,0x00}, //80
	{0xA46A,0x90},
	{0xA46B,0x0B},
	{0xA46C,0x92},
	{0xA46D,0xE0},
	{0xA46E,0x94},
	{0xA46F,0x41}, //44
	{0xA470,0x40},
	{0xA471,0x06},
	{0xA472,0x90},
	{0xA473,0x01},
	{0xA474,0xA4},
	{0xA475,0x02},
	{0xA476,0x86},
	{0xA477,0x57},
	{0xA478,0x02},
	{0xA479,0x86},
	{0xA47A,0x5C},
	{0xA500,0xF5},
	{0xA501,0x3B},
	{0xA502,0x90},
	{0xA503,0x06},
	{0xA504,0x6C},
	{0xA505,0xE0},
	{0xA506,0xFF},
	{0xA507,0xE5},
	{0xA508,0x3B},
	{0xA509,0xC3},
	{0xA50A,0x9F},
	{0xA50B,0x40},
	{0xA50C,0x03},
	{0xA50D,0x02},
	{0xA50E,0xF6},
	{0xA50F,0x0E},
	{0xA510,0x90},
	{0xA511,0x0B},
	{0xA512,0xC6},
	{0xA513,0xE0},
	{0xA514,0x14},
	{0xA515,0x60},
	{0xA516,0x3C},
	{0xA517,0x14},
	{0xA518,0x60},
	{0xA519,0x6B},
	{0xA51A,0x24},
	{0xA51B,0x02},
	{0xA51C,0x60},
	{0xA51D,0x03},
	{0xA51E,0x02},
	{0xA51F,0xF5},
	{0xA520,0xB5},
	{0xA521,0x90},
	{0xA522,0x0A},
	{0xA523,0x9A},
	{0xA524,0xE0},
	{0xA525,0xFB},
	{0xA526,0xA3},
	{0xA527,0xE0},
	{0xA528,0xFA},
	{0xA529,0xA3},
	{0xA52A,0xE0},
	{0xA52B,0xF9},
	{0xA52C,0x85},
	{0xA52D,0x3B},
	{0xA52E,0x82},
	{0xA52F,0x75},
	{0xA530,0x83},
	{0xA531,0x00},
	{0xA532,0x12},
	{0xA533,0x0A},
	{0xA534,0xB8},
	{0xA535,0xFF},
	{0xA536,0x74},
	{0xA537,0xAB},
	{0xA538,0x25},
	{0xA539,0x3B},
	{0xA53A,0xF5},
	{0xA53B,0x82},
	{0xA53C,0xE4},
	{0xA53D,0x34},
	{0xA53E,0x0A},
	{0xA53F,0xF5},
	{0xA540,0x83},
	{0xA541,0xE0},
	{0xA542,0xFD},
	{0xA543,0xC3},
	{0xA544,0xEF},
	{0xA545,0x9D},
	{0xA546,0xFE},
	{0xA547,0xE4},
	{0xA548,0x94},
	{0xA549,0x00},
	{0xA54A,0x90},
	{0xA54B,0x0B},
	{0xA54C,0xCA},
	{0xA54D,0xF0},
	{0xA54E,0xA3},
	{0xA54F,0xCE},
	{0xA550,0xF0},
	{0xA551,0x80},
	{0xA552,0x62},
	{0xA553,0x90},
	{0xA554,0x0A},
	{0xA555,0x9A},
	{0xA556,0xE0},
	{0xA557,0xFB},
	{0xA558,0xA3},
	{0xA559,0xE0},
	{0xA55A,0xFA},
	{0xA55B,0xA3},
	{0xA55C,0xE0},
	{0xA55D,0xF9},
	{0xA55E,0x85},
	{0xA55F,0x3B},
	{0xA560,0x82},
	{0xA561,0x75},
	{0xA562,0x83},
	{0xA563,0x00},
	{0xA564,0x12},
	{0xA565,0x0A},
	{0xA566,0xB8},
	{0xA567,0xFF},
	{0xA568,0x74},
	{0xA569,0x9D},
	{0xA56A,0x25},
	{0xA56B,0x3B},
	{0xA56C,0xF5},
	{0xA56D,0x82},
	{0xA56E,0xE4},
	{0xA56F,0x34},
	{0xA570,0x0A},
	{0xA571,0xF5},
	{0xA572,0x83},
	{0xA573,0xE0},
	{0xA574,0xFD},
	{0xA575,0xC3},
	{0xA576,0xEF},
	{0xA577,0x9D},
	{0xA578,0xFE},
	{0xA579,0xE4},
	{0xA57A,0x94},
	{0xA57B,0x00},
	{0xA57C,0x90},
	{0xA57D,0x0B},
	{0xA57E,0xCA},
	{0xA57F,0xF0},
	{0xA580,0xA3},
	{0xA581,0xCE},
	{0xA582,0xF0},
	{0xA583,0x80},
	{0xA584,0x30},
	{0xA585,0x90},
	{0xA586,0x0A},
	{0xA587,0x9A},
	{0xA588,0xE0},
	{0xA589,0xFB},
	{0xA58A,0xA3},
	{0xA58B,0xE0},
	{0xA58C,0xFA},
	{0xA58D,0xA3},
	{0xA58E,0xE0},
	{0xA58F,0xF9},
	{0xA590,0x85},
	{0xA591,0x3B},
	{0xA592,0x82},
	{0xA593,0x75},
	{0xA594,0x83},
	{0xA595,0x00},
	{0xA596,0x12},
	{0xA597,0x0A},
	{0xA598,0xB8},
	{0xA599,0xFF},
	{0xA59A,0x74},
	{0xA59B,0xA4},
	{0xA59C,0x25},
	{0xA59D,0x3B},
	{0xA59E,0xF5},
	{0xA59F,0x82},
	{0xA5A0,0xE4},
	{0xA5A1,0x34},
	{0xA5A2,0x0A},
	{0xA5A3,0xF5},
	{0xA5A4,0x83},
	{0xA5A5,0xE0},
	{0xA5A6,0xFD},
	{0xA5A7,0xC3},
	{0xA5A8,0xEF},
	{0xA5A9,0x9D},
	{0xA5AA,0xFE},
	{0xA5AB,0xE4},
	{0xA5AC,0x94},
	{0xA5AD,0x00},
	{0xA5AE,0x90},
	{0xA5AF,0x0B},
	{0xA5B0,0xCA},
	{0xA5B1,0xF0},
	{0xA5B2,0xA3},
	{0xA5B3,0xCE},
	{0xA5B4,0xF0},
	{0xA5B5,0x90},
	{0xA5B6,0x07},
	{0xA5B7,0x83},
	{0xA5B8,0xE0},
	{0xA5B9,0xFF},
	{0xA5BA,0x7E},
	{0xA5BB,0x00},
	{0xA5BC,0x90},
	{0xA5BD,0x0D},
	{0xA5BE,0xF6},
	{0xA5BF,0xEE},
	{0xA5C0,0xF0},
	{0xA5C1,0xA3},
	{0xA5C2,0xEF},
	{0xA5C3,0xF0},
	{0xA5C4,0x90},
	{0xA5C5,0x0B},
	{0xA5C6,0xCA},
	{0xA5C7,0xE0},
	{0xA5C8,0xFC},
	{0xA5C9,0xA3},
	{0xA5CA,0xE0},
	{0xA5CB,0xFD},
	{0xA5CC,0xD3},
	{0xA5CD,0x9F},
	{0xA5CE,0x74},
	{0xA5CF,0x80},
	{0xA5D0,0xF8},
	{0xA5D1,0xEC},
	{0xA5D2,0x64},
	{0xA5D3,0x80},
	{0xA5D4,0x98},
	{0xA5D5,0x40},
	{0xA5D6,0x0C},
	{0xA5D7,0x90},
	{0xA5D8,0x0B},
	{0xA5D9,0xC8},
	{0xA5DA,0xE0},
	{0xA5DB,0x04},
	{0xA5DC,0xF0},
	{0xA5DD,0xA3},
	{0xA5DE,0xE0},
	{0xA5DF,0x04},
	{0xA5E0,0xF0},
	{0xA5E1,0x80},
	{0xA5E2,0x26},
	{0xA5E3,0x90},
	{0xA5E4,0x0D},
	{0xA5E5,0xF6},
	{0xA5E6,0xE0},
	{0xA5E7,0xFE},
	{0xA5E8,0xA3},
	{0xA5E9,0xE0},
	{0xA5EA,0xFF},
	{0xA5EB,0xC3},
	{0xA5EC,0xE4},
	{0xA5ED,0x9F},
	{0xA5EE,0xFF},
	{0xA5EF,0xE4},
	{0xA5F0,0x9E},
	{0xA5F1,0xFE},
	{0xA5F2,0xC3},
	{0xA5F3,0xED},
	{0xA5F4,0x9F},
	{0xA5F5,0xEE},
	{0xA5F6,0x64},
	{0xA5F7,0x80},
	{0xA5F8,0xF8},
	{0xA5F9,0xEC},
	{0xA5FA,0x64},
	{0xA5FB,0x80},
	{0xA5FC,0x98},
	{0xA5FD,0x50},
	{0xA5FE,0x0A},
	{0xA5FF,0x90},
	{0xA600,0x0B},
	{0xA601,0xC8},
	{0xA602,0xE0},
	{0xA603,0x14},
	{0xA604,0xF0},
	{0xA605,0xA3},
	{0xA606,0xE0},
	{0xA607,0x04},
	{0xA608,0xF0},
	{0xA609,0x05},
	{0xA60A,0x3B},
	{0xA60B,0x02},
	{0xA60C,0xF5},
	{0xA60D,0x02},
	{0xA60E,0x90},
	{0xA60F,0x08},
	{0xA610,0x58},
	{0xA611,0x02},
	{0xA612,0x9D},
	{0xA613,0x50},
	{0x9006,0xBA},
	{0x9007,0x75},
	{0x9008,0x00},
	{0x9009,0x00},
	{0x900A,0x02},
	{0x900D,0x01},
	{0x900E,0xA2},
	{0x900F,0x8F},
	{0x9010,0x00},
	{0x9011,0xCB},
	{0x9012,0x03},
	{0x9016,0xE6},
	{0x9017,0x6B},
	{0x9018,0x02},
	{0x9019,0x6B},
	{0x901A,0x02},
	{0x901D,0x01},
	{0x901E,0xAC},
	{0x901F,0x70},
	{0x9020,0x00},
	{0x9021,0xC5},
	{0x9022,0x03},
	{0x9026,0x9C},
	{0x9027,0x5B},
	{0x9028,0x00},
	{0x9029,0xBF},
	{0x902A,0x02},
	{0x902E,0x60},
	{0x902F,0x1C},
	{0x9030,0x01},
	{0x9031,0x37},
	{0x9032,0x02},
	{0x9035,0x01},
	{0x9036,0xBA},
	{0x9037,0x70},
	{0x9038,0x00},
	{0x9039,0x00},
	{0x903A,0x03},
	{0x903E,0x21},
	{0x903F,0x3F},
	{0x9040,0x02},
	{0x9041,0x40},
	{0x9042,0x02},
	{0x9046,0x21},
	{0x9047,0xEA},
	{0x9048,0x02},
	{0x9049,0x43},
	{0x904A,0x02},
	{0x904E,0xA6},
	{0x904F,0x12},
	{0x9050,0x02},
	{0x9051,0x46},
	{0x9052,0x02},
	{0x9056,0x29},
	{0x9057,0xE3},
	{0x9058,0x02},
	{0x9059,0x49},
	{0x905A,0x02},
	{0x905D,0x01},
	{0x905E,0x9C},
	{0x905F,0x6E},
	{0x9060,0x05},
	{0x9061,0x00},
	{0x9062,0x02},
	{0x9065,0x01},
	{0x9066,0xA2},
	{0x9067,0x66},
	{0x9068,0x02},
	{0x9069,0x35},
	{0x906A,0x02},
	{0x906D,0x01},
	{0x906E,0xB5},
	{0x906F,0xC2},
	{0x9070,0x02},
	{0x9071,0x9B},
	{0x9072,0x02},
	{0x9075,0x01},
	{0x9076,0xA2},
	{0x9077,0xD4},
	{0x9078,0x02},
	{0x9079,0xBE},
	{0x907A,0x02},
	{0x907D,0x01},
	{0x907E,0xB7},
	{0x907F,0xEA},
	{0x9080,0x00},
	{0x9081,0x02},
	{0x9082,0x03},
	{0x9086,0x67},
	{0x9087,0x31},
	{0x9088,0x02},
	{0x9089,0xF7},
	{0x908A,0x02},
	{0x908E,0x66},
	{0x908F,0xED},
	{0x9090,0x03},
	{0x9091,0x1D},
	{0x9092,0x02},
	{0x9096,0x67},
	{0x9097,0x73},
	{0x9098,0x03},
	{0x9099,0xD3},
	{0x909A,0x02},
	{0x909E,0x20},
	{0x909F,0x40},
	{0x90A0,0x03},
	{0x90A1,0x3B},
	{0x90A2,0x02},
	{0x90A6,0xC5},
	{0x90A7,0xC0},
	{0x90A8,0x03},
	{0x90A9,0xF0},
	{0x90AA,0x02},
	{0x90AE,0x41},
	{0x90AF,0xB3},
	{0x90B0,0x00},
	{0x90B1,0xA2},
	{0x90B2,0x02},
	{0x90B6,0x44},
	{0x90B7,0xBA},
	{0x90B8,0x00},
	{0x90B9,0xF0},
	{0x90BA,0x03},
	{0x90BE,0x89},
	{0x90BF,0x99},
	{0x90C0,0x04},
	{0x90C1,0x00},
	{0x90C2,0x02},
	{0x90C6,0xA7},
	{0x90C7,0x91},
	{0x90C8,0x04},
	{0x90C9,0x21},
	{0x90CA,0x02},
	{0x90CE,0x3A},
	{0x90CF,0x51},
	{0x90D0,0x00},
	{0x90D1,0xA2},
	{0x90D2,0x02},
	{0x90D6,0x86},
	{0x90D7,0x54},
	{0x90D8,0x04},
	{0x90D9,0x47},
	{0x90DA,0x02},
	{0x9000,0x01},
	{0xFFFF,0x00},
	{REG_DLY,200},//mdelay(200)

	{0x0009,0x16},
	{0x0012,0x00},
	{0x0013,0x00},
	{0x0016,0x00},
	{0x0021,0x00},
	{0x0022,0x01},
	{0x0040,0x01},
	{0x0041,0x04},
	{0x0042,0x05},
	{0x0043,0x00},
	{0x0044,0x03},
	{0x0045,0xC0},
	{0x0046,0x02},
	{0x0060,0x00},
	{0x0061,0x00},
	{0x0066,0x02},
	//{0x0083,0x00},
	//{0x0084,0x00},
	//{0x0085,0x02},
	{0x00B2,0x4f},
	{0x00B3,0x70},
	{0x00B4,0x01},
	//{0x00B5,0x01},
	{0x00E8,0x01},
	//{0x00ED,0x05},
	{0x00EE,0x1E},
	{0x0129,0x00},
	{0x0130,0x00},
	{0x0137,0x00},
	{0x019C,0x4B},
	{0x019D,0xC0},
	{0x01A0,0x01},
	{0x01A1,0x80},
	{0x01A2,0x80},
	{0x01A3,0x80},
	{0x5200,0x01},
	{0x7000,0x0C},
	{0x7101,0x44},//c4
	{0x7102,0x01},
	{0x7103,0x00},
	{0x7104,0x00},
	{0x7105,0x80},
	{0x7158,0x00},
	{0x0143,0x5F},
	{0x0144,0x0D},
	{0x0046,0x00},
	{0x0041,0x00},
	//{0x00B5,0x02},
	{0x7101,0x44},
	{0x00ED,0x0A},
	{0x00EE,0x1E},
	//{0x00B3,0x80},
	{0x019C,0x4B},
	{0x019D,0xC0},
	{0x0129,0x00},
	{0x0130,0xFF},
	{0x0083,0x01},
	{0x0084,0x00},
	{0x01A1,0x80},
	{0x01A2,0x80},
	{0x01A3,0x80},
	{0x01A0,0x01},
	{0x0021,0x00},
	{0x0022,0x01},
	{0x0040,0x01},
	{0x0060,0x00},
	{0x0013,0x00},
	{0x0041,0x04},
	{0x0061,0x00},
	{0x0046,0x02},//02   gong
	{0x0066,0x02},   //02 gong
	{0x0012,0x00},
	{0x7102,0x09},
	{0x7103,0x00},
	{0x7158,0x00},
	{0x00E8,0x01},
	{0x7000,0x2C},
	{0x5200,0x01},
	{0x7000,0x0C},
	{0x02C2,0x00},
	{0x02C3,0xC0},
	{0x015E,0x40},
	{0x015F,0x00},
	{0x0390,0x01},
	{0x0391,0x00},
	{0x0392,0x00},
	{0x03A0,0x14},
	{0x03A1,0x00},
	{0x03A2,0x5A},
	{0x03A3,0xEE},
	{0x03A4,0x69},
	{0x03A5,0x49},
	{0x03A6,0x3E},
	{0x03A7,0x00},
	{0x03A8,0x39},
	{0x03A9,0x33},
	{0x03B0,0x60},
	{0x03B1,0x00},
	{0x03B2,0x5A},
	{0x03B3,0xEE},
	{0x03B4,0x69},
	{0x03B5,0x49},
	{0x03B6,0x3E},
	{0x03B7,0x00},
	{0x03B8,0x3D},
	{0x03B9,0x20},
	{0x03C0,0x10},
	{0x03C1,0x00},
	{0x03C2,0x5A},
	{0x03C3,0xEE},
	{0x03C4,0x69},
	{0x03C5,0x49},
	{0x03C6,0x3A},
	{0x03C7,0x80},
	{0x03D0,0x64},
	{0x03D1,0x00},
	{0x03D2,0x5A},
	{0x03D3,0xEE},
	{0x03D4,0x69},
	{0x03D5,0x49},
	{0x03D6,0x34},
	{0x03D7,0xD1},
	{0x004C,0x08},
	{0x006C,0x08},
	{0x0350,0x00},
	{0x0351,0x5A},
	{0x0352,0xEE},
	{0x0353,0x69},
	{0x0354,0x49},
	{0x0355,0x39},
	{0x0356,0x6D},
	{0x0357,0x19},
	{0x0358,0x00},
	{0x0359,0x3C},
	{0x035A,0x5A},
	{0x035B,0xEE},
	{0x035C,0x69},
	{0x035D,0x49},
	{0x035E,0x39},
	{0x035F,0x85},
	{0x0049,0x14},
	{0x004A,0x0D},
	{0x0069,0x14},
	{0x006A,0x0E},
	{0x0090,0x5A},
	{0x0091,0xEE},
	{0x0092,0x3E},
	{0x0093,0x00},
	{0x0094,0x69},
	{0x0095,0x49},
	{0x0096,0x39},
	{0x0097,0xCF},
	{0x0098,0x01},
	{0x00A0,0x5A},
	{0x00A1,0xEE},
	{0x00A2,0x3E},
	{0x00A3,0x00},
	{0x00A4,0x69},
	{0x00A5,0x49},
	{0x00A6,0x3B},
	{0x00A7,0x80},
	{0x00A8,0x01},
  #if 0
	{0x0420,0x00},  //lsc start
	{0x0421,0x09},
	{0x0422,0xff},
	{0x0423,0x9e},
	{0x0424,0x00},
	{0x0425,0x89},
	{0x0426,0x00},
	{0x0427,0xab},
	{0x0428,0xff},
	{0x0429,0xe9},
	{0x042a,0xff},
	{0x042b,0x8b},
	{0x042c,0x00},
	{0x042d,0x73},
	{0x042E,0xff},
	{0x042f,0xb6},
	{0x0430,0x00},
	{0x0431,0x54},
	{0x0432,0xff},
	{0x0433,0x43},
	{0x0434,0x01},
	{0x0435,0x04},
	{0x0436,0x01},
	{0x0437,0x34},
	{0x0438,0xff},
	{0x0439,0x7c},
	{0x043a,0xfe},
	{0x043b,0xd2},
	{0x043c,0x00},
	{0x043d,0x63},
	{0x043e,0xff},
	{0x043f,0x15},
	{0x0450,0x00},
	{0x0451,0x3b},
	{0x0452,0xff},
	{0x0453,0x98},
	{0x0454,0x00},
	{0x0455,0x6f},
	{0x0456,0x00},
	{0x0457,0x93},
	{0x0458,0xff},
	{0x0459,0xad},
	{0x045a,0xff},
	{0x045b,0x87},
	{0x045c,0x00},
	{0x045d,0x52},
	{0x045E,0xff},
	{0x045f,0xa7},
	{0x0440,0xff},
	{0x0441,0xfd},
	{0x0442,0xff},
	{0x0443,0x6c},
	{0x0444,0x00},
	{0x0445,0x90},
	{0x0446,0x00},
	{0x0447,0xa1},
	{0x0448,0x00},
	{0x0449,0x02},
	{0x044a,0xff},
	{0x044b,0x48},
	{0x044c,0x00},
	{0x044d,0x5b},
	{0x044E,0xff},
	{0x044f,0xb4},
	{0x0460,0xff},
	{0x0461,0x69},
	{0x0462,0xff},
	{0x0463,0xbb},
	{0x0464,0x00},
	{0x0465,0x84},
	{0x0466,0x00},
	{0x0467,0xa3},
	{0x0468,0x00},
	{0x0469,0x0e},
	{0x046A,0x00},
	{0x046b,0x76},
	{0x046C,0xff},
	{0x046d,0xaf},
	{0x046E,0xff},
	{0x046f,0xf5},
	{0x0470,0xff},
	{0x0471,0x8a},
	{0x0472,0xff},
	{0x0473,0x5a},
	{0x0474,0x00},
	{0x0475,0xef},
	{0x0476,0x01},
	{0x0477,0x16},
	{0x0478,0xff},
	{0x0479,0xd4},
	{0x047A,0x00},
	{0x047b,0x02},
	{0x047c,0x00},
	{0x047d,0x2c},
	{0x047E,0xff},
	{0x047f,0x95},
	{0x0490,0xff},
	{0x0491,0x9b},
	{0x0492,0xff},
	{0x0493,0x91},
	{0x0494,0x00},
	{0x0495,0x6f},
	{0x0496,0x00},
	{0x0497,0x95},
	{0x0498,0xff},
	{0x0499,0xd5},
	{0x049a,0x01},
	{0x049b,0x20},
	{0x049C,0xff},
	{0x049d,0xfb},
	{0x049E,0xff},
	{0x049f,0xe1},
	{0x0480,0xff},
	{0x0481,0x5a},
	{0x0482,0xff},
	{0x0483,0x91},
	{0x0484,0x00},
	{0x0485,0x8c},
	{0x0486,0x00},
	{0x0487,0x9f},
	{0x0488,0x00},
	{0x0489,0x29},
	{0x048A,0x00},
	{0x048b,0x53},
	{0x048C,0xff},
	{0x048d,0x80},
	{0x048E,0xff},
	{0x048f,0xf7},
	{0x04A0,0xff},
	{0x04a1,0x6c},
	{0x04a2,0xff},
	{0x04a3,0xb9},
	{0x04A4,0x00},
	{0x04a5,0x81},
	{0x04A6,0x00},
	{0x04a7,0x93},
	{0x04A8,0x00},
	{0x04A9,0x1c},
	{0x04AA,0x00},
	{0x04ab,0x39},
	{0x04AC,0xff},
	{0x04ad,0x9f},
	{0x04ae,0x00},
	{0x04af,0x0e},
	{0x04B0,0xff},
	{0x04b1,0xe0},
	{0x04B2,0xff},
	{0x04b3,0x7b},
	{0x04B4,0x00},
	{0x04b5,0xaa},
	{0x04B6,0x00},
	{0x04b7,0xc8},
	{0x04B8,0xff},
	{0x04b9,0xe1},
	{0x04BA,0x00},
	{0x04bb,0x0e},
	{0x04bc,0x00},
	{0x04bd,0x0b},
	{0x04be,0xff},
	{0x04bf,0xff},
	{0x04D0,0xff},
	{0x04d1,0xac},
	{0x04D2,0xff},
	{0x04d3,0x93},
	{0x04D4,0x00},
	{0x04d5,0x64},
	{0x04D6,0x00},
	{0x04d7,0x83},
	{0x04D8,0xff},
	{0x04d9,0xdb},
	{0x04DA,0x00},
	{0x04db,0xa8},
	{0x04DC,0xff},
	{0x04dd,0xf5},
	{0x04de,0x00},
	{0x04df,0x15},
	{0x04C0,0xff},
	{0x04c1,0x5d},
	{0x04c2,0xff},
	{0x04c3,0x9c},
	{0x04C4,0x00},
	{0x04c5,0x82},
	{0x04C6,0x00},
	{0x04c7,0x96},
	{0x04C8,0x00},
	{0x04c9,0x33},
	{0x04CA,0x00},
	{0x04cb,0x07},
	{0x04CC,0xff},
	{0x04cd,0x71},
	{0x04ce,0x00},
	{0x04cf,0x11},
	{0x04E0,0xff},
	{0x04e1,0x6d},
	{0x04e2,0xff},
	{0x04e3,0xb8},
	{0x04E4,0x00},
	{0x04e5,0x84},
	{0x04E6,0x00},
	{0x04e7,0x96},
	{0x04e8,0xff},
	{0x04e9,0xc0},
	{0x04EA,0x00},
	{0x04eb,0x6d},
	{0x04EC,0xff},
	{0x04ed,0xbb},
	{0x04ee,0x00},
	{0x04ef,0x00},
	{0x04F0,0xff},
	{0x04f1,0xe0},
	{0x04F2,0xff},
	{0x04f3,0x95},
	{0x04F4,0x00},
	{0x04f5,0xa7},
	{0x04F6,0x00},
	{0x04f7,0xc8},
	{0x04F8,0xff},
	{0x04f9,0xde},
	{0x04FA,0x00},
	{0x04fb,0x7e},
	{0x04fc,0x00},
	{0x04fd,0x36},
	{0x04fe,0x00},
	{0x04ff,0x10},
	{0x0510,0xff},
	{0x0511,0xc1},
	{0x0512,0xff},
	{0x0513,0x9f},
	{0x0514,0x00},
	{0x0515,0x6a},
	{0x0516,0x00},
	{0x0517,0x89},
	{0x0518,0xff},
	{0x0519,0xdc},
	{0x051A,0x00},
	{0x051b,0x55},
	{0x051c,0x00},
	{0x051d,0x09},
	{0x051e,0x00},
	{0x051f,0x0d},
	{0x0500,0xff},
	{0x0501,0x60},
	{0x0502,0xff},
	{0x0503,0x9e},
	{0x0504,0x00},
	{0x0505,0x81},
	{0x0506,0x00},
	{0x0507,0x9c},
	{0x0508,0xff},
	{0x0509,0xc0},
	{0x050A,0x00},
	{0x050b,0x40},
	{0x050C,0xff},
	{0x050d,0x8e},
	{0x050e,0x00},
	{0x050f,0x00},
	{0x0561,0x0e},
	{0x0562,0x01},
	{0x0563,0x01},
	{0x0564,0x06},
	{0x0324,0x39},
	{0x0325,0xAE},
	{0x0326,0x3a},
	{0x0327,0x29},
	{0x0328,0x3b},
	{0x0329,0x0A},
	{0x032A,0x3b},
	{0x032B,0x62},
	{0x0320,0x01},
	{0x0321,0x04},
	{0x0322,0x01},
	{0x0323,0x01},
	{0x0330,0x01},
	{0x0384,0x00},
	{0x0337,0x01},
	{0x03EC,0x39},
	{0x03ED,0x85},
	{0x03FC,0x3A},
	{0x03FD,0x14},
	{0x040C,0x3A},
	{0x040D,0xF6},
	{0x041C,0x3B},
	{0x041D,0x9A},
	{0x03E0,0xB6},
	{0x03E1,0x04},
	{0x03E2,0xBB},
	{0x03E3,0xE9},
	{0x03E4,0xBC},
	{0x03E5,0x70},
	{0x03E6,0x37},
	{0x03E7,0x02},
	{0x03E8,0xBC},
	{0x03E9,0x00},
	{0x03EA,0xBF},
	{0x03EB,0x12},
	{0x03F0,0xBA},
	{0x03F1,0x7B},
	{0x03F2,0xBA},
	{0x03F3,0x83},
	{0x03F4,0xBB},
	{0x03F5,0xBC},
	{0x03F6,0x38},
	{0x03F7,0x2D},
	{0x03F8,0xBB},
	{0x03F9,0x23},
	{0x03FA,0xBD},
	{0x03FB,0xAC},
	{0x0400,0xBE},
	{0x0401,0x96},
	{0x0402,0xB9},
	{0x0403,0xBE},
	{0x0404,0xBB},
	{0x0405,0x57},
	{0x0406,0x3A},
	{0x0407,0xBB},
	{0x0408,0xB3},
	{0x0409,0x17},
	{0x040A,0xBE},
	{0x040B,0x66},
	{0x0410,0xBB},
	{0x0411,0x2A},
	{0x0412,0xBA},
	{0x0413,0x00},
	{0x0414,0xBB},
	{0x0415,0x10},
	{0x0416,0xB8},
	{0x0417,0xCD},
	{0x0418,0xB7},
	{0x0419,0x5C},
	{0x041A,0xBB},
	{0x041B,0x6C},
	{0x01f8,0x3c},
	{0x01f9,0x00},
	{0x01FA,0x00},
	{0x02a2,0x3e},
	{0x02a3,0x00},
	{0x02a4,0x3e},
	{0x02a5,0x00},
	{0x02a6,0x3e},
	{0x02a7,0x00},
	{0x02a8,0x3e},
	{0x02a9,0x00},
	{0x056c,0x42},
	{0x056d,0x00},
	{0x056e,0x42},
	{0x056f,0x00},
	{0x0570,0x42},
	{0x0571,0x00},
	{0x0572,0x42},
	{0x0573,0x00},
	{0x0081,0x6E},
	{0x0588,0x00},
	{0x0589,0x5A},
	{0x058A,0xEE},
	{0x058B,0x69},
	{0x058C,0x49},
	{0x058D,0x3D},
	{0x058E,0x3D},
	{0x0080,0x6C},
	{0x0082,0x5A},
	//{0x0010,0x01},
	//{REG_DLY,200},//mdelay(200)

	{0x4708,0x00},
	{0x4709,0x00},
	{0x4710,0x00},
	{0x4711,0x00},
	{0x065A,0x00},
	{0x06C9,0x01},
	{0x06CD,0x01},
	{0x06CE,0xBD},
	{0x06CF,0x00},
	{0x06D0,0x93},
	{0x06D1,0x02},
	{0x06D2,0x30},
	{0x06D3,0xD4},
	{0x06D4,0x01},
	{0x06D5,0x01},
	{0x06D6,0xBD},
	{0x06D7,0x00},
	{0x06D8,0x93},
	{0x06D9,0x00},
	{0x06DA,0x93},
	{0x06DB,0x59},
	{0x06DC,0x0d},
	{0x0730,0x00},
	{0x0731,0x00},
	{0x0732,0x03},
	{0x0733,0xFF},
	{0x0734,0x03},
	{0x0735,0x70},
	{0x0755,0x01},
	{0x0756,0x00},
	{0x075B,0x01},
	{0x075E,0x00},
	{0x0764,0x01},
	{0x0766,0x01},
	{0x0768,0x01},
	{0x076A,0x00},
	{0x0758,0x01},
	{0x075C,0x01},
	{0x0770,0x98},
	{0x0771,0x19},
	{0x0772,0x1B},
	{0x0774,0x01},
	{0x0775,0x4a},
	{0x0777,0x00},
	{0x0778,0x45},
	{0x0779,0x00},
	{0x077A,0x02},
	{0x077D,0x01},
	{0x077E,0x03},
	{0x0783,0x10},
	{0x0785,0x14},
	{0x0788,0x04},
	{0x0846,0x06},
	{0x0847,0x05},
	{0xC41A,0x05},
	{0xC423,0x11},
	{0xC427,0x11},
	{0x300B,0x09},
	{0x0085,0x02},//yuv order
	{0x7000,0x08},
	{0x5200,0x09},
	{0x00B5,0x01},
	{0x0030,0x14},
	{0x0040,0x01},
	{0x0041,0x04},
	{0x00B4,0x01},

	{0x0010,0x01},
	{REG_DLY,100},//mdelay(200)
	{0x070A,0x01},
	//{0x0010,0x01},
	{REG_DLY,100},//mdelay(200)
};

//for capture
static struct regval_list sensor_qsxga_regs[] = { //qsxga: 2592*1936
	//capture 5Mega 5fps
	{0x0010,0x02},
	{0x00ED,0x05},
	//{0x0085,0x02},//yuv order 00
	{0x7000,0x08},
	{0x5200,0x09},
	//{0x00B2,0x50},
	//{0x00B3,0x80},
	//{0x00B5,0x02},
	{0x0030,0x12},
	{0x0040,0x00},
	{0x0041,0x00},
	{0x00B4,0x01},
	{0x0010,0x01},
};

static struct regval_list sensor_qxga_regs[] = { //qxga: 2048*1536
  //capture 3Mega 7.5fps
	{0x0010,0x02},
	{0x7000,0x08},
	{0x5200,0x09},
	{0x00ED,0x05},
	//{0x00B2,0x50},
	//{0x00B3,0x80},
	//{0x00B5,0x02},
	{0x0030,0x12},
	{0x0040,0x00},
	{0x0041,0x0a},
	{0x0042,0x08},
	{0x0043,0x00},
	{0x0044,0x06},
	{0x0045,0x00},
	{0x00B4,0x01},
	{0x0010,0x01},
};

static struct regval_list sensor_uxga_regs[] = { //UXGA: 1600*1200
		//capture 2Mega 5fps
	//{0x0085,0x02},//yuv order 00
	{0x0010,0x02},
	{0x7000,0x08},
	{0x5200,0x09},
	{0x00ED,0x05},
	//{0x00B2,0x50},
	//{0x00B3,0x80},
	//{0x00B5,0x02},
	{0x0030,0x12},
	{0x0040,0x00},
	{0x0041,0x01},
	{0x00B4,0x01},
	{0x0010,0x01},
};
static struct regval_list sensor_sxga_regs[] = { //SXGA: 1280*1024
	//capture 1.3Mega 5fps
	//{0x0085,0x02},//yuv order 00
	{0x0010,0x02},
	{0x7000,0x08},
	{0x5200,0x09},
	{0x00ED,0x05},
	//{0x00B2,0x50},
	//{0x00B3,0x80},
	//{0x00B5,0x02},
	{0x0030,0x12},
	{0x0040,0x00},
	{0x0041,0x02},
	{0x00B4,0x01},
	{0x0010,0x01},
};

static struct regval_list sensor_xga_regs[] = { //XGA: 1024*768
	//capture 1Mega 5fps
	//{0x00,0x85,0x02},//yuv order 00
	{0x0010,0x02},
	{0x7000,0x08},
	{0x5200,0x09},
	{0x00ED,0x05},
	//{0x00B2,0x50},
	//{0x00B3,0x80},
	//{0x00B5,0x02},
	{0x0030,0x12},
	{0x0040,0x01},
	{0x0041,0x0a},
	{0x0042,0x04},
	{0x0043,0x00},
	{0x0044,0x03},
	{0x0045,0x00},

	{0x00B4,0x01},
	{0x0010,0x01},
};

//for video
static struct regval_list sensor_1080p_regs[] = { //1080: 1920*1080
	{0x0010,0x02},
	{0x7000,0x08},
	{0x5200,0x09},
	//{0x00B2,0x50},
	//{0x00B3,0x80},
	//{0x00B5,0x02},
	{0x0030,0x12},
	{0x0040,0x01},
	{0x0041,0x0a},
	{0x0042,0x07},
	{0x0043,0x80},
	{0x0044,0x04},
	{0x0045,0x38},

	{0x00B4,0x01},
	{0x0010,0x01},
};

static struct regval_list sensor_1280x960_regs[] = { //1280*960
	{0x0010,0x02},	 
	{0x7000,0x08},
	{0x00ED,0x0A},//fps
	{0x5200,0x09},
	{0x0030,0x12},
	{0x0040,0x01},
	{0x0041,0x0a},
	{0x0042,0x05},
	{0x0043,0x00},
	{0x0044,0x02},
	{0x0045,0xc0},

	{0x00B4,0x01},
	{0x0010,0x01},
};


static struct regval_list sensor_720p_regs[] = { //1280*720
	{0x0010,0x02},
	{0x7000,0x08},
	{0x00ED,0x0A},//fps
	{0x5200,0x09},
	//{0x00B2,0x50},
	//{0x00B3,0x80},
	//{0x00B5,0x02},
	{0x0030,0x12},
	{0x0040,0x01},
	{0x0041,0x0a},
	{0x0042,0x05},
	{0x0043,0x00},
	{0x0044,0x02},
	{0x0045,0xd0},

	{0x00B4,0x01},
	{0x0010,0x01},
};

static struct regval_list sensor_svga_regs[] = { //SVGA: 800*600
	{0x0010,0x02},
	{0x7000,0x08},
	{0x5200,0x09},
	{0x00ED,0x0A},
	//{0x00B2,0x50},
	//{0x00B3,0x80},
	//{0x00B5,0x02},
	{0x0030,0x12},
	{0x0040,0x00},
	{0x0041,0x03},
	{0x00B4,0x01},
	{0x0010,0x01},
};

static struct regval_list sensor_vga_regs[] = { //VGA:  640*480
	//timing
	//640x480
	//power down
	//{0x0010,0x02},

	//{0x0085,0x02},//yuv order 00
	//{0x0010,0x02},
	{0x7000,0x08},
	{0x5200,0x09},
	{0x00ED,0x0A},
	//{0x00B2,0x50},//50
	//{0x00B3,0xB0},//b0
	//{0x00B5,0x02},//02
	{0x0030,0x12},//11
	{0x0040,0x01},
	{0x0041,0x04},
	{0x00B4,0x01},
	//{0x0010,0x01},
	//{0x070A,0x01},



};


/*
 * The white balance settings
 * Here only tune the R G B channel gain.
 * The white balance enalbe bit is modified in sensor_s_autowb and sensor_s_wb
 */
static struct regval_list sensor_wb_auto_regs[] = {
	{0x01A0,0x01},
};

static struct regval_list sensor_wb_cloud_regs[] = {
	{0x01A0,0x03},//MWB
	{0x01A1,0x62},
	{0x01A2,0x08},
	{0x01A3,0x00},
};

static struct regval_list sensor_wb_daylight_regs[] = {
	{0x01A0,0x03},//MWB
	{0x01A1,0x4f},
	{0x01A2,0x00},
	{0x01A3,0x01},
};

static struct regval_list sensor_wb_incandescence_regs[] = {
	{0x01A0,0x03},//MWB
	{0x01A1,0x10},
	{0x01A2,0x00},
	{0x01A3,0x52},
};

static struct regval_list sensor_wb_fluorescent_regs[] = {
	{0x01A0,0x03},//MWB
	{0x01A1,0x39},
	{0x01A2,0x00},
	{0x01A3,0x59},
};

static struct regval_list sensor_wb_tungsten_regs[] = {
	{0x01A0,0x03},//MWB
	{0x01A1,0x05},
	{0x01A2,0x00},
	{0x01A3,0x7f},
};

/*
 * The color effect settings
 */
static struct regval_list sensor_colorfx_none_regs[] = {
	{0x0380,0x00},
	{0x0381,0x00},
	{0x0382,0x00},
	{0x0384,0x00},
};

static struct regval_list sensor_colorfx_bw_regs[] = {
	{0x0380,0x00},
	{0x0381,0x00},
	{0x0382,0x00},
	{0x0384,0x05},
};

static struct regval_list sensor_colorfx_sepia_regs[] = {
	{0x0380,0x00},
	{0x0381,0x00},
	{0x0382,0x00},
	{0x0384,0x06},
};

static struct regval_list sensor_colorfx_negative_regs[] = {
	{0x0380,0x01},
	{0x0381,0x00},
	{0x0382,0x00},
	{0x0384,0x00},
};

static struct regval_list sensor_colorfx_emboss_regs[] = {

};

static struct regval_list sensor_colorfx_sketch_regs[] = {

};

static struct regval_list sensor_colorfx_sky_blue_regs[] = {
	{0x0380,0x00},
	{0x0381,0x00},
	{0x0382,0x00},
	{0x0384,0x04},
};

static struct regval_list sensor_colorfx_grass_green_regs[] = {
	{0x0380,0x00},
	{0x0381,0x00},
	{0x0382,0x00},
	{0x0384,0x03},
};

static struct regval_list sensor_colorfx_skin_whiten_regs[] = {
//NULL
};

static struct regval_list sensor_colorfx_vivid_regs[] = {
//NULL
};

/*
 * The brightness setttings
 */
static struct regval_list sensor_brightness_neg4_regs[] = {
//NULL
};

static struct regval_list sensor_brightness_neg3_regs[] = {
//NULL
};

static struct regval_list sensor_brightness_neg2_regs[] = {
//NULL
};

static struct regval_list sensor_brightness_neg1_regs[] = {
//NULL
};

static struct regval_list sensor_brightness_zero_regs[] = {
//NULL
};

static struct regval_list sensor_brightness_pos1_regs[] = {
//NULL
};

static struct regval_list sensor_brightness_pos2_regs[] = {
//NULL
};

static struct regval_list sensor_brightness_pos3_regs[] = {
//NULL
};

static struct regval_list sensor_brightness_pos4_regs[] = {
//NULL
};

/*
 * The contrast setttings
 */
static struct regval_list sensor_contrast_neg4_regs[] = {
//NULL
};

static struct regval_list sensor_contrast_neg3_regs[] = {
//NULL
};

static struct regval_list sensor_contrast_neg2_regs[] = {
//NULL
};

static struct regval_list sensor_contrast_neg1_regs[] = {
//NULL
};

static struct regval_list sensor_contrast_zero_regs[] = {
//NULL
};

static struct regval_list sensor_contrast_pos1_regs[] = {
//NULL
};

static struct regval_list sensor_contrast_pos2_regs[] = {
//NULL
};

static struct regval_list sensor_contrast_pos3_regs[] = {
//NULL
};

static struct regval_list sensor_contrast_pos4_regs[] = {
//NULL
};

/*
 * The saturation setttings
 */
static struct regval_list sensor_saturation_neg4_regs[] = {
//NULL
};

static struct regval_list sensor_saturation_neg3_regs[] = {
//NULL
};

static struct regval_list sensor_saturation_neg2_regs[] = {
//NULL
};

static struct regval_list sensor_saturation_neg1_regs[] = {
//NULL
};

static struct regval_list sensor_saturation_zero_regs[] = {
//NULL
};

static struct regval_list sensor_saturation_pos1_regs[] = {
//NULL
};

static struct regval_list sensor_saturation_pos2_regs[] = {
//NULL
};

static struct regval_list sensor_saturation_pos3_regs[] = {
//NULL
};

static struct regval_list sensor_saturation_pos4_regs[] = {
//NULL
};

/*
 * The exposure target setttings
 */

static struct regval_list sensor_ev_neg4_regs[] = {
	{0x0130,0xf6},//-1.3EV
};

static struct regval_list sensor_ev_neg3_regs[] = {
	{0x0130,0xf7},	//-1.0EV
};

static struct regval_list sensor_ev_neg2_regs[] = {
	{0x0130,0xf8},	//-0.7EV
};

static struct regval_list sensor_ev_neg1_regs[] = {
	{0x0130,0xf9},	//-0.3EV
};

static struct regval_list sensor_ev_zero_regs[] = {
	{0x0130,0xfd},		//default
};

static struct regval_list sensor_ev_pos1_regs[] = {
	{0x0130,0x03},	//0.3EV
};

static struct regval_list sensor_ev_pos2_regs[] = {
	{0x0130,0x05},	//0.7EV
};

static struct regval_list sensor_ev_pos3_regs[] = {
	{0x0130,0x06},	//1.0EV
};

static struct regval_list sensor_ev_pos4_regs[] = {
	{0x0130,0x07},	//1.3EV
};


/*
 * Here we'll try to encapsulate the changes for just the output
 * video format.
 *
 */


static struct regval_list sensor_fmt_yuv422_yuyv[] = {
	{0x0085,0x02},	//YUYV
};


static struct regval_list sensor_fmt_yuv422_yvyu[] = {
	{0x0085,0x03},	//YVYU
};

static struct regval_list sensor_fmt_yuv422_vyuy[] = {
	{0x0085,0x01},	//VYUY
};

static struct regval_list sensor_fmt_yuv422_uyvy[] = {
	{0x0085,0x00},	//UYVY
};

//static struct regval_list sensor_fmt_raw[] = {
//	
//};


//static struct regval_list sensor_fmt_raw[] = {
//  
//};

/*
 * Low-level register I/O.
 *
 */


/*
 * On most platforms, we'd rather do straight i2c I/O.
 */
static int sensor_read(struct v4l2_subdev *sd, unsigned short reg,
    unsigned char *value)
{
	int ret=0;
	int cnt=0;
	
  ret = cci_read_a16_d8(sd,reg,value);
 // printk("reg=%d\n",reg);
//  printk("value=%s\n",value);
  while(ret!=0&&cnt<4)
  {
  	ret = cci_read_a16_d8(sd,reg,value);
  	cnt++;
  }
  if(cnt>0)
  	vfe_dev_dbg("sensor read retry=%d\n",cnt);
  
  return ret;
}

static int sensor_write(struct v4l2_subdev *sd, unsigned short reg,
    unsigned char value)
{
	int ret=0;
	int cnt=0;
  
  ret = cci_write_a16_d8(sd,reg,value);
  while(ret!=0&&cnt<2)
  {
  	ret = cci_write_a16_d8(sd,reg,value);
  	cnt++;
  }
  if(cnt>0)
  	vfe_dev_dbg("sensor write retry=%d\n",cnt);
  
  return ret;
}

/*
 * Write a list of register settings;
 */
static int sensor_write_array(struct v4l2_subdev *sd, struct regval_list *regs, int array_size)
{
	int i=0;
	
  if(!regs)
  	return -EINVAL;
  
  while(i<array_size)
  {
    if(regs->addr == REG_DLY) {
      msleep(regs->data);
    } 
    else {  
    	//printk("write 0x%x=0x%x\n", regs->addr, regs->data);
      LOG_ERR_RET(sensor_write(sd, regs->addr, regs->data))
    }
    i++;
    regs++;
  }
  return 0;
}

/*
 * Write a list of continuous register setting;
 */
static int sensor_write_continuous(struct v4l2_subdev *sd, unsigned short addr, unsigned char vals[] , uint size)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct i2c_msg msg;
	unsigned char data[2+32];
	unsigned char *p = vals;
	int ret,i;

	while (size > 0) {
		int len = size > 32 ? 32 : size;
		data[0] = (addr&0xff00) >> 8;
		data[1] = (addr&0x00ff);

		for(i = 2; i < 2+len; i++)
			data[i] = *p++;

		msg.addr = client->addr;
		msg.flags = 0;
		msg.len = 2+len;
		msg.buf = data;

		ret = i2c_transfer(client->adapter, &msg, 1);

		if (ret > 0) {
			ret = 0;
		} else if (ret < 0) {
			vfe_dev_err("sensor_write error!\n");
		}
		addr += len;
		size -= len;
	}
	return ret;
}






/* stuff about auto focus */

static int sensor_download_af_fw(struct v4l2_subdev *sd)
{
	int ret,cnt;
	data_type rdval;
	struct regval_list af_fw_reset_reg[] = {
		{0x3000,0x20},
	};
	struct regval_list af_fw_start_reg[] = {
		{0x3022,0x00},
		{0x3023,0x00},
		{0x3024,0x00},
		{0x3025,0x00},
		{0x3026,0x00},
		{0x3027,0x00},
		{0x3028,0x00},
		{0x3029,0x7f},
		{0x3000,0x00},	//start firmware for af
	};
	return 0;//gong
	//reset sensor MCU
	ret = sensor_write_array(sd, af_fw_reset_reg, ARRAY_SIZE(af_fw_reset_reg));
	if(ret < 0) {
		 vfe_dev_err("reset sensor MCU error\n");
		return ret;
	}

	//download af fw
	ret =sensor_write_continuous(sd, 0x8000, sensor_af_fw_regs, ARRAY_SIZE(sensor_af_fw_regs));
	if(ret < 0) {
		 vfe_dev_err("download af fw error\n");
		return ret;
	}
	//start af firmware
	ret = sensor_write_array(sd, af_fw_start_reg, ARRAY_SIZE(af_fw_start_reg));
	if(ret < 0) {
		 vfe_dev_err("start af firmware error\n");
		return ret;
	}

	mdelay(10);
	//check the af firmware status
	rdval = 0xff;
	cnt = 0;
	while(rdval!=0x70) {
		mdelay(5);
		ret = sensor_read(sd, 0x3029, &rdval);
		if (ret < 0)
		{
			 vfe_dev_err("sensor check the af firmware status err !\n");
			return ret;
		}
		cnt++;
		if(cnt > 200) {
			 vfe_dev_err("AF firmware check status time out !\n");
			return -EFAULT;
		}
	}
	vfe_dev_print("AF firmware check status complete,0x3029 = 0x%x\n",rdval);

#if DEV_DBG_EN == 1
	sensor_read(sd, 0x3000, &rdval);
	vfe_dev_print("0x3000 = 0x%x\n",rdval);

	sensor_read(sd, 0x3004, &rdval);
	vfe_dev_print("0x3004 = 0x%x\n",rdval);

	sensor_read(sd, 0x3001, &rdval);
	vfe_dev_print("0x3001 = 0x%x\n",rdval);

  ret = sensor_read(sd, 0x070A, &rdval); 
  vfe_dev_print("AF firmware check status complete, 0x070A = 0x%x\n",rdval); 

  return 0; 
}

static int sensor_g_single_af(struct v4l2_subdev *sd)
{
	data_type rdval;
	int ret;
	//vfe_dev_dbg("sensor_g_single_af\n");

	rdval = 0xff;

	ret = sensor_read(sd, 0x07ae, &rdval);
	if (ret < 0)
	{
		vfe_dev_err("sensor get af focused status err !\n");
		return ret;
	}

	if(rdval == 1)
	{
		return V4L2_AUTO_FOCUS_STATUS_REACHED;
	}

	return V4L2_AUTO_FOCUS_STATUS_FAILED;
}


static int sensor_g_contin_af(struct v4l2_subdev *sd)
{
	data_type rdval;
	struct sensor_info *info = to_state(sd);

	vfe_dev_dbg("sensor_g_contin_af\n");

	rdval = 0xff;


    info->focus_status = 0; //idle
    sensor_read(sd, 0x07AE, &rdval);
    if(rdval==0)
    {
	    return V4L2_AUTO_FOCUS_STATUS_FAILED;
    }
    else
    {
		return V4L2_AUTO_FOCUS_STATUS_REACHED;
    }

}


static int sensor_g_af_status(struct v4l2_subdev *sd)
{
	int ret=0;
	struct sensor_info *info = to_state(sd);

	if(info->auto_focus==1)
		ret = sensor_g_contin_af(sd);
	else
		ret = sensor_g_single_af(sd);

	return ret;
}


static int sensor_g_3a_lock(struct v4l2_subdev *sd)
{
  //int ret=0;
  struct sensor_info *info = to_state(sd);
  return ( (info->auto_focus==0)?V4L2_LOCK_FOCUS:~V4L2_LOCK_FOCUS |
           (info->autowb==0)?V4L2_LOCK_WHITE_BALANCE:~V4L2_LOCK_WHITE_BALANCE |
           (~V4L2_LOCK_EXPOSURE)
         );
}

static int sensor_s_init_af(struct v4l2_subdev *sd)
{
	int ret;
  struct sensor_info *info = to_state(sd);
	ret=sensor_download_af_fw(sd);
	if(ret==0)
		info->af_first_flag=0;
	//other config
	return ret;
}

static int sensor_s_single_af(struct v4l2_subdev *sd)
{ 
  int ret; 
  struct sensor_info *info = to_state(sd); 
  unsigned char rdval=0xff; 
  unsigned int cnt=0; 
  
  printk("HM5065_sensor_s_single_af\n"); 
  //trig single af 
	
  info->focus_status = 0; //idle   
  
  //sensor_write(sd, 0x3023, 0x01); 
  
  ret = sensor_write(sd, 0x0751, 0x00); 
  ret = sensor_write(sd, 0x070A, 0x03); 
  usleep_range(190000,220000);    //zoemarked 20150806
  ret = sensor_write(sd, 0x070B, 0x01); 
  usleep_range(190000,220000); 
  ret = sensor_write(sd, 0x070B, 0x02); 
  usleep_range(190000,220000);		  // zoemarked 20150806
  if (ret < 0) { 
	vfe_dev_err("sensor tigger single af err !\n"); 
	return ret; 
  } 
  
//	info->contin_focus=0; 
  info->focus_status = 1; //busy 
  info->auto_focus=0; 
  return 0; 
}


static int sensor_g_autofocus_ctrl(struct v4l2_subdev *sd,
		struct v4l2_control *ctrl);

static int sensor_s_continueous_af(struct v4l2_subdev *sd, int value) 
{ 
  struct sensor_info *info = to_state(sd); 
  vfe_dev_err("sensor_s_continueous_af[0x%x]\n",value); 
        if(info->focus_status==1) 
        { 
          vfe_dev_print("continous focus not accepted when single focus\n"); 
          return -1; 
        } 
 /*       if( (info->auto_focus==value) ) 
        { 
          vfe_dev_print("already in same focus mode\n"); 
          return 0; 
        } 
  */      
        if(value==1) 
        { 
          LOG_ERR_RET(sensor_write(sd, 0x070A, 0x01)) 
           info->auto_focus=1; 
        } 

  return 0; 
} 

static int sensor_s_pause_af(struct v4l2_subdev *sd)
{
	int ret;
	//pause af poisition
	vfe_dev_print("sensor_s_pause_af\n");
	mdelay(5);
	return 0;
}

static int sensor_s_release_af(struct v4l2_subdev *sd)
{ 
  //release focus 
  vfe_dev_print("sensor_s_release_af\n"); 
  
  //release single af 
  LOG_ERR_RET(sensor_write(sd, 0x0751, 0x01)) 
  return 0; 
}


static int sensor_s_relaunch_af_zone(struct v4l2_subdev *sd)
{
  printk("HM5065_sensor_s_relaunch_af_zone\n");

  usleep_range(5000,6000);
  return 0;
}


static int sensor_s_af_zone(struct v4l2_subdev *sd,
														struct v4l2_win_coordinate * win_c)
{
	struct sensor_info *info = to_state(sd);
	int ret;

	vfe_dev_print("sensor_s_af_zone\n");
	vfe_dev_dbg("af zone input xc=%d,yc=%d\n",xc,yc);
	return 0;//gong
	if(info->width == 0 || info->height == 0) {
		vfe_dev_err("current width or height is zero!\n");
		return -EINVAL;
	}

	xc = xc * 80 / info->width;
	if((info->width == HD720_WIDTH && info->height == HD720_HEIGHT) || \
			(info->width == HD1080_WIDTH && info->height == HD1080_HEIGHT)) {
		yc = yc * 45 / info->height;
	} else {
		yc = yc * 60 / info->height;
	}

	vfe_dev_dbg("af zone after xc=%d,yc=%d\n",xc,yc);

	//set x center
	ret = sensor_write(sd,0x3024, xc);
	if (ret < 0)
	{
		vfe_dev_err("sensor_s_af_zone_xc error!\n");
		return ret;
	}
	//set y center
	ret = sensor_write(sd, 0x3025, yc);
	if (ret < 0)
	{
		vfe_dev_err("sensor_s_af_zone_yc error!\n");
		return ret;
	}

	//set af zone
	ret = sensor_write(sd, 0x3022, 0x81);
	if (ret < 0)
	{
		vfe_dev_err("sensor_s_af_zone error!\n");
		return ret;
	}
	mdelay(5);
	return 0;
}

static int sensor_s_3a_lock(struct v4l2_subdev *sd, int value) 
{ 
  //struct sensor_info *info = to_state(sd); 
  int ret; 
  
  vfe_dev_dbg("sensor_s_3a_lock=0x%x\n", value); 
  value=!((value&V4L2_LOCK_FOCUS)>>2); 
  if(value==0) 
    ret=sensor_s_pause_af(sd); 
  else 
    ret=sensor_s_relaunch_af_zone(sd);        //0528 
  
  return ret; 
} 


/* *********************************************begin of ******************************************** */


static int sensor_g_hflip(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);
	data_type rdval;

	LOG_ERR_RET(sensor_read(sd, 0x0083, &rdval))
	rdval&= 0x01;

	*value = rdval;

	info->hflip = *value;*/
	return 0;
}

static int sensor_s_hflip(struct v4l2_subdev *sd, int value)
{
	struct sensor_info *info = to_state(sd);
	data_type rdval;

	if(info->hflip == value)
  		  return 0;
	  LOG_ERR_RET(sensor_read(sd, 0x0083, &rdval))


	  switch (value) {
	    case 0:
	      rdval &= 0xfe;
	      break;
	    case 1:
	      rdval |= 0x01;
	      break;
	    default:
	      return -EINVAL;
	  }

 	 LOG_ERR_RET(sensor_write(sd, 0x0083, rdval))

	mdelay(10);

	info->hflip = value;

	return 0;

}

static int sensor_g_vflip(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);
	data_type rdval;

	LOG_ERR_RET(sensor_read(sd, 0x0084, &rdval))


	rdval &= 0x01;

	*value = rdval;

	info->vflip = *value;

	return 0;
}

static int sensor_s_vflip(struct v4l2_subdev *sd, int value)
{
	struct sensor_info *info = to_state(sd);
	data_type rdval;

	if(info->vflip == value)
		return 0;

	LOG_ERR_RET(sensor_read(sd, 0x0084, &rdval))

	switch (value) {
		case 0:
		 	 rdval &= 0xfe;
			break;
		case 1:
			rdval |= 0x01;
			break;
		default:
			return -EINVAL;
	}

  	LOG_ERR_RET(sensor_write(sd, 0x0084, rdval))

  	mdelay(10);
  	info->vflip = value;*/
  	return 0;
}


static int sensor_g_autogain(struct v4l2_subdev *sd, __s32 *value)
{
	return -EINVAL;
}

static int sensor_s_autogain(struct v4l2_subdev *sd, int value)
{
	return -EINVAL;
}


static int sensor_g_exp(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);

	*value = info->exp;
	return 0;
}

static int sensor_s_exp(struct v4l2_subdev *sd, int value)
{
	int ret;
	struct sensor_info *info = to_state(sd);

	switch (value) {
		case -4:
		  ret = sensor_write_array(sd, sensor_ev_neg4_regs, ARRAY_SIZE(sensor_ev_neg4_regs));
			break;
		case -3:
			ret = sensor_write_array(sd, sensor_ev_neg3_regs, ARRAY_SIZE(sensor_ev_neg3_regs));
			break;
		case -2:
			ret = sensor_write_array(sd, sensor_ev_neg2_regs, ARRAY_SIZE(sensor_ev_neg2_regs));
			break;
		case -1:
			ret = sensor_write_array(sd, sensor_ev_neg1_regs, ARRAY_SIZE(sensor_ev_neg1_regs));
			break;
		case 0:
			ret = sensor_write_array(sd, sensor_ev_zero_regs, ARRAY_SIZE(sensor_ev_zero_regs));
			break;
		case 1:
			ret = sensor_write_array(sd, sensor_ev_pos1_regs, ARRAY_SIZE(sensor_ev_pos1_regs));
			break;
		case 2:
			ret = sensor_write_array(sd, sensor_ev_pos2_regs, ARRAY_SIZE(sensor_ev_pos2_regs));
			break;
		case 3:
			ret = sensor_write_array(sd, sensor_ev_pos3_regs, ARRAY_SIZE(sensor_ev_pos3_regs));
			break;
		case 4:
			ret = sensor_write_array(sd, sensor_ev_pos4_regs, ARRAY_SIZE(sensor_ev_pos4_regs));
			break;
		default:
			return -EINVAL;
	}

	if (ret < 0) {
		vfe_dev_err("sensor_write_array err at sensor_s_exp!\n");
		return ret;
	}
	mdelay(10);
	info->exp = value;
	return 0;
}

static int sensor_g_autoexp(struct v4l2_subdev *sd, __s32 *value)
{
	/*
	int ret;
	struct sensor_info *info = to_state(sd);
	struct regval_list regs;
	
	regs.reg_num[0] = 0x30;
	regs.reg_num[1] = 0x13;
	ret = sensor_read(sd, regs.reg_num, regs.value);
	if (ret < 0) {
		 vfe_dev_err("sensor_read err at sensor_g_autoexp!\n");
		return ret;
	}

	regs.value[0] &= 0x01;
	if (regs.value[0] == 0x01) {
		*value = V4L2_EXPOSURE_AUTO;
	}
	else
	{
		*value = V4L2_EXPOSURE_MANUAL;
	}
	
	info->autoexp = *value;
	*/
	return 0;
}

static int sensor_s_autoexp(struct v4l2_subdev *sd,
		enum v4l2_exposure_auto_type value)
{
	/*
	int ret;
	struct sensor_info *info = to_state(sd);
	struct regval_list regs;
	
	regs.reg_num[0] = 0x30;
	regs.reg_num[1] = 0x13;
	ret = sensor_read(sd, regs.reg_num, regs.value);
	if (ret < 0) {
		 vfe_dev_err("sensor_read err at sensor_s_autoexp!\n");
		return ret;
	}

	LOG_ERR_RET(sensor_read(sd, 0x01a0, &rdval))

	rdval &= 0x01;
	rdval = rdval>>1;   //0x3406 bit0 is awb enable

	*value = (rdval == 1)?0:1;
	info->autowb = *value;
	return 0;
}




static int sensor_g_hue(struct v4l2_subdev *sd, __s32 *value)
{
  return -EINVAL;
}

static int sensor_s_hue(struct v4l2_subdev *sd, int value)
{
  return -EINVAL;
}

static int sensor_g_gain(struct v4l2_subdev *sd, __s32 *value)
{
  return -EINVAL;
}

static int sensor_s_gain(struct v4l2_subdev *sd, int value)
{
  return -EINVAL;
}

static int sensor_g_band_filter(struct v4l2_subdev *sd,
    __s32 *value)
{
	struct sensor_info *info = to_state(sd);
	data_type rdval;

	vfe_dev_dbg("%s\n",__func__);
	LOG_ERR_RET(sensor_read(sd, 0x0190, &rdval))

	if(rdval == 0x00)
		info->band_filter = V4L2_CID_POWER_LINE_FREQUENCY_DISABLED;
	else {
		LOG_ERR_RET(sensor_read(sd, 0x019d, &rdval))
		if(rdval == 0x20)
			info->band_filter = V4L2_CID_POWER_LINE_FREQUENCY_50HZ;
		else
			info->band_filter = V4L2_CID_POWER_LINE_FREQUENCY_60HZ;
	}
	return 0;

}

static int sensor_s_band_filter(struct v4l2_subdev *sd,
    enum v4l2_power_line_frequency value)
{
  struct sensor_info *info = to_state(sd);
  unsigned char rdval;

	switch(value) {
		case V4L2_CID_POWER_LINE_FREQUENCY_DISABLED:
			LOG_ERR_RET(sensor_read(sd,0x0190,&rdval))
			LOG_ERR_RET(sensor_write(sd,0x0190,0x00))//turn off band filter
			break;
		case V4L2_CID_POWER_LINE_FREQUENCY_50HZ:
			LOG_ERR_RET(sensor_write(sd,0x019c,0x4b))//50hz
			LOG_ERR_RET(sensor_write(sd,0x019d,0x20))//manual band filter
			LOG_ERR_RET(sensor_read(sd,0x0190,&rdval))
			LOG_ERR_RET(sensor_write(sd,0x0190,0x00))//turn on band filter
			break;
		case V4L2_CID_POWER_LINE_FREQUENCY_60HZ:
			LOG_ERR_RET(sensor_write(sd,0x019c,0x4b))//60hz
			LOG_ERR_RET(sensor_write(sd,0x019d,0xc0))//manual band filter
			LOG_ERR_RET(sensor_read(sd,0x0190,&rdval))
			LOG_ERR_RET(sensor_write(sd,0x0190,0x00))//turn on band filter
			break;
		case V4L2_CID_POWER_LINE_FREQUENCY_AUTO:
			break;
		default:
			break;
	}
	mdelay(10);
	info->band_filter = value;
	return 0;
}




/* stuff about auto focus */


/* *********************************************end of ******************************************** */

static int sensor_g_brightness(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);

	*value = info->brightness;
	return 0;
}

static int sensor_s_brightness(struct v4l2_subdev *sd, int value)
{
	int ret;
	struct sensor_info *info = to_state(sd);

	switch (value) {
		case -4:
		  ret = sensor_write_array(sd, sensor_brightness_neg4_regs, ARRAY_SIZE(sensor_brightness_neg4_regs));
			break;
		case -3:
			ret = sensor_write_array(sd, sensor_brightness_neg3_regs, ARRAY_SIZE(sensor_brightness_neg3_regs));
			break;
		case -2:
			ret = sensor_write_array(sd, sensor_brightness_neg2_regs, ARRAY_SIZE(sensor_brightness_neg2_regs));
			break;
		case -1:
			ret = sensor_write_array(sd, sensor_brightness_neg1_regs, ARRAY_SIZE(sensor_brightness_neg1_regs));
			break;
		case 0:
			ret = sensor_write_array(sd, sensor_brightness_zero_regs, ARRAY_SIZE(sensor_brightness_zero_regs));
			break;
		case 1:
			ret = sensor_write_array(sd, sensor_brightness_pos1_regs, ARRAY_SIZE(sensor_brightness_pos1_regs));
			break;
		case 2:
			ret = sensor_write_array(sd, sensor_brightness_pos2_regs, ARRAY_SIZE(sensor_brightness_pos2_regs));
			break;
		case 3:
			ret = sensor_write_array(sd, sensor_brightness_pos3_regs, ARRAY_SIZE(sensor_brightness_pos3_regs));
			break;
		case 4:
			ret = sensor_write_array(sd, sensor_brightness_pos4_regs, ARRAY_SIZE(sensor_brightness_pos4_regs));
			break;
		default:
			return -EINVAL;
	}

	if (ret < 0) {
		 vfe_dev_err("sensor_write_array err at sensor_s_brightness!\n");
		return ret;
	}
	mdelay(10);
	info->brightness = value;
	return 0;
}


static int sensor_g_contrast(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);

	*value = info->contrast;
	return 0;
}

static int sensor_s_contrast(struct v4l2_subdev *sd, int value)
{
	int ret;
	struct sensor_info *info = to_state(sd);

	switch (value) {
		case -4:
		  ret = sensor_write_array(sd, sensor_contrast_neg4_regs, ARRAY_SIZE(sensor_contrast_neg4_regs));
			break;
		case -3:
			ret = sensor_write_array(sd, sensor_contrast_neg3_regs, ARRAY_SIZE(sensor_contrast_neg3_regs));
			break;
		case -2:
			ret = sensor_write_array(sd, sensor_contrast_neg2_regs, ARRAY_SIZE(sensor_contrast_neg2_regs));
			break;
		case -1:
			ret = sensor_write_array(sd, sensor_contrast_neg1_regs, ARRAY_SIZE(sensor_contrast_neg1_regs));
			break;
		case 0:
			ret = sensor_write_array(sd, sensor_contrast_zero_regs, ARRAY_SIZE(sensor_contrast_zero_regs));
			break;
		case 1:
			ret = sensor_write_array(sd, sensor_contrast_pos1_regs, ARRAY_SIZE(sensor_contrast_pos1_regs));
			break;
		case 2:
			ret = sensor_write_array(sd, sensor_contrast_pos2_regs, ARRAY_SIZE(sensor_contrast_pos2_regs));
			break;
		case 3:
			ret = sensor_write_array(sd, sensor_contrast_pos3_regs, ARRAY_SIZE(sensor_contrast_pos3_regs));
			break;
		case 4:
			ret = sensor_write_array(sd, sensor_contrast_pos4_regs, ARRAY_SIZE(sensor_contrast_pos4_regs));
			break;
		default:
			return -EINVAL;
	}

	if (ret < 0) {
		vfe_dev_err("sensor_write_array err at sensor_s_contrast!\n");
		return ret;
	}
	mdelay(10);
	info->contrast = value;
	return 0;
}

static int sensor_g_saturation(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);

	*value = info->saturation;
	return 0;
}

static int sensor_s_saturation(struct v4l2_subdev *sd, int value)
{
	int ret;
	struct sensor_info *info = to_state(sd);

	switch (value) {
		case -4:
		  ret = sensor_write_array(sd, sensor_saturation_neg4_regs, ARRAY_SIZE(sensor_saturation_neg4_regs));
			break;
		case -3:
			ret = sensor_write_array(sd, sensor_saturation_neg3_regs, ARRAY_SIZE(sensor_saturation_neg3_regs));
			break;
		case -2:
			ret = sensor_write_array(sd, sensor_saturation_neg2_regs, ARRAY_SIZE(sensor_saturation_neg2_regs));
			break;
		case -1:
			ret = sensor_write_array(sd, sensor_saturation_neg1_regs, ARRAY_SIZE(sensor_saturation_neg1_regs));
			break;
		case 0:
			ret = sensor_write_array(sd, sensor_saturation_zero_regs, ARRAY_SIZE(sensor_saturation_zero_regs));
			break;
		case 1:
			ret = sensor_write_array(sd, sensor_saturation_pos1_regs, ARRAY_SIZE(sensor_saturation_pos1_regs));
			break;
		case 2:
			ret = sensor_write_array(sd, sensor_saturation_pos2_regs, ARRAY_SIZE(sensor_saturation_pos2_regs));
			break;
		case 3:
			ret = sensor_write_array(sd, sensor_saturation_pos3_regs, ARRAY_SIZE(sensor_saturation_pos3_regs));
			break;
		case 4:
			ret = sensor_write_array(sd, sensor_saturation_pos4_regs, ARRAY_SIZE(sensor_saturation_pos4_regs));
			break;
		default:
			return -EINVAL;
	}

	if (ret < 0) {
		vfe_dev_err("sensor_write_array err at sensor_s_saturation!\n");
		return ret;
	}
	mdelay(10);
	info->saturation = value;
	return 0;
}

static int sensor_g_wb(struct v4l2_subdev *sd, int *value)
{
	struct sensor_info *info = to_state(sd);
	enum v4l2_auto_n_preset_white_balance *wb_type = (enum v4l2_auto_n_preset_white_balance*)value;

	*wb_type = info->wb;

	return 0;
}
static int sensor_s_autowb(struct v4l2_subdev *sd, int value)
{
	int ret;
	struct sensor_info *info = to_state(sd);

	ret = sensor_write_array(sd, sensor_wb_auto_regs, ARRAY_SIZE(sensor_wb_auto_regs));
	if (ret < 0) {
		vfe_dev_err("sensor_write_array err at sensor_s_autowb!\n");
		return ret;
	}
	/*
	ret = sensor_read(sd, 0x01a0, &rdval);
	if (ret < 0) {
		vfe_dev_err("sensor_read err at sensor_s_autowb!\n");
		return ret;
	}

	switch(value) {
	case 0:
		rdval &= 0xfe;
		break;
	case 1:
		rdval |= 0x01;
		break;
	default:
		break;
	}	
	ret = sensor_write(sd, 0x01a0, rdval);
	if (ret < 0) {
		vfe_dev_err("sensor_write err at sensor_s_autowb!\n");
		return ret;
	}
	*/
	mdelay(10);
	info->autowb = value;

	return 0;
}


static int sensor_s_wb(struct v4l2_subdev *sd,
    enum v4l2_auto_n_preset_white_balance value)
{
	int ret;
	struct sensor_info *info = to_state(sd);
	switch (value) {
		case V4L2_WHITE_BALANCE_AUTO:
		  	LOG_ERR_RET(sensor_write_array(sd, sensor_wb_auto_regs ,ARRAY_SIZE(sensor_wb_auto_regs)))
			break;
		case V4L2_WHITE_BALANCE_CLOUDY:
		  	ret = sensor_write_array(sd, sensor_wb_cloud_regs, ARRAY_SIZE(sensor_wb_cloud_regs));
			break;
		case V4L2_WHITE_BALANCE_DAYLIGHT:
			ret = sensor_write_array(sd, sensor_wb_daylight_regs, ARRAY_SIZE(sensor_wb_daylight_regs));
			break;
		case V4L2_WHITE_BALANCE_INCANDESCENT:
			ret = sensor_write_array(sd, sensor_wb_incandescence_regs, ARRAY_SIZE(sensor_wb_incandescence_regs));
			break;
		case V4L2_WHITE_BALANCE_FLUORESCENT_H:
			ret = sensor_write_array(sd, sensor_wb_fluorescent_regs, ARRAY_SIZE(sensor_wb_fluorescent_regs));
			break;
		//case V4L2_WB_TUNGSTEN:
		//	ret = sensor_write_array(sd, sensor_wb_tungsten_regs, ARRAY_SIZE(sensor_wb_tungsten_regs));
		//	break;
		default:
			return -EINVAL;
	}

	if (ret < 0) {
		 vfe_dev_err("sensor_s_wb error, return %x!\n",ret);
		return ret;
	}

	mdelay(10);
	info->wb = value;
	return 0;
}

static int sensor_g_colorfx(struct v4l2_subdev *sd,
    __s32 *value)
{
	struct sensor_info *info = to_state(sd);
	enum v4l2_colorfx *clrfx_type = (enum v4l2_colorfx*)value;

	*clrfx_type = info->clrfx;
	return 0;
}

static int sensor_s_colorfx(struct v4l2_subdev *sd,
    enum v4l2_colorfx value)
{
	int ret;
	struct sensor_info *info = to_state(sd);

	switch (value) {
	case V4L2_COLORFX_NONE:
	  ret = sensor_write_array(sd, sensor_colorfx_none_regs, ARRAY_SIZE(sensor_colorfx_none_regs));
		break;
	case V4L2_COLORFX_BW:
		ret = sensor_write_array(sd, sensor_colorfx_bw_regs, ARRAY_SIZE(sensor_colorfx_bw_regs));
		break;
	case V4L2_COLORFX_SEPIA:
		ret = sensor_write_array(sd, sensor_colorfx_sepia_regs, ARRAY_SIZE(sensor_colorfx_sepia_regs));
		break;
	case V4L2_COLORFX_NEGATIVE:
		ret = sensor_write_array(sd, sensor_colorfx_negative_regs, ARRAY_SIZE(sensor_colorfx_negative_regs));
		break;
	case V4L2_COLORFX_EMBOSS:
		ret = sensor_write_array(sd, sensor_colorfx_emboss_regs, ARRAY_SIZE(sensor_colorfx_emboss_regs));
		break;
	case V4L2_COLORFX_SKETCH:
		ret = sensor_write_array(sd, sensor_colorfx_sketch_regs, ARRAY_SIZE(sensor_colorfx_sketch_regs));
		break;
	case V4L2_COLORFX_SKY_BLUE:
		ret = sensor_write_array(sd, sensor_colorfx_sky_blue_regs, ARRAY_SIZE(sensor_colorfx_sky_blue_regs));
		break;
	case V4L2_COLORFX_GRASS_GREEN:
		ret = sensor_write_array(sd, sensor_colorfx_grass_green_regs, ARRAY_SIZE(sensor_colorfx_grass_green_regs));
		break;
	case V4L2_COLORFX_SKIN_WHITEN:
		ret = sensor_write_array(sd, sensor_colorfx_skin_whiten_regs, ARRAY_SIZE(sensor_colorfx_skin_whiten_regs));
		break;
	case V4L2_COLORFX_VIVID:
		ret = sensor_write_array(sd, sensor_colorfx_vivid_regs, ARRAY_SIZE(sensor_colorfx_vivid_regs));
		break;
	default:
		return -EINVAL;
	}

	if (ret < 0) {
		 vfe_dev_err("sensor_s_colorfx error, return %x!\n",ret);
		return ret;
	}
  info->clrfx = value;
  return 0;
}
/*
static int sensor_g_flash_mode(struct v4l2_subdev *sd,
    __s32 *value)
{
  struct sensor_info *info = to_state(sd);
  enum v4l2_flash_led_mode *flash_mode = (enum v4l2_flash_led_mode*)value;
  
  *flash_mode = info->flash_mode;
  return 0;
}

static int sensor_s_flash_mode(struct v4l2_subdev *sd,
    enum v4l2_flash_led_mode value)
{
	struct sensor_info *info = to_state(sd);
	struct  vfe_dev *dev=(struct  vfe_dev *)dev_get_drvdata(sd->v4l2_dev->dev);
	int flash_on,flash_off;
	
	flash_on = (dev->flash_pol!=0)?1:0;
	flash_off = (flash_on==1)?0:1;
	
	switch (value) {
	case V4L2_FLASH_MODE_OFF:
		csi_gpio_write(sd,&dev->flash_io,flash_off);
		break;
	case V4L2_FLASH_MODE_AUTO:
		return -EINVAL;
		break;  
	case V4L2_FLASH_MODE_ON:
		csi_gpio_write(sd,&dev->flash_io,flash_on);
		break;   
	case V4L2_FLASH_MODE_TORCH:
		return -EINVAL;
		break;
	case V4L2_FLASH_MODE_RED_EYE:   
		return -EINVAL;
		break;
	default:
		return -EINVAL;
	}
	
	info->flash_mode = value;
	return 0;
  return 0;
}
*/
/*
 * Stuff that knows about the sensor.
 */
 
static int sensor_power(struct v4l2_subdev *sd, int on)
{
	switch(on)
	{
		case CSI_SUBDEV_STBY_ON:
			vfe_dev_dbg("CSI_SUBDEV_STBY_ON!\n");
			vfe_dev_print("disalbe oe!\n");
			cci_lock(sd);
			vfe_gpio_write(sd,PWDN,CSI_GPIO_HIGH);
			cci_unlock(sd);
			vfe_set_mclk(sd,OFF);
			break;
		case CSI_SUBDEV_STBY_OFF:
			vfe_dev_dbg("CSI_SUBDEV_STBY_OFF!\n");
			cci_lock(sd);
			vfe_set_mclk_freq(sd,MCLK);
			vfe_set_mclk(sd,ON);
			usleep_range(10000,12000);
			vfe_gpio_write(sd,PWDN,CSI_GPIO_LOW);
			usleep_range(10000,12000);
			vfe_gpio_write(sd,RESET,CSI_GPIO_HIGH);
			usleep_range(10000,12000);
			vfe_gpio_write(sd,RESET,CSI_GPIO_LOW);
			usleep_range(10000,12000);
			vfe_gpio_write(sd,RESET,CSI_GPIO_HIGH);
			usleep_range(10000,12000);
			cci_unlock(sd);
			break;
		case CSI_SUBDEV_PWR_ON:
			vfe_dev_dbg("CSI_SUBDEV_PWR_ON!\n");
			cci_lock(sd);
			// vfe_gpio_set_status(sd,PWDN,1);//set the gpio to output
			// vfe_gpio_set_status(sd,RESET,1);//set the gpio to output
			vfe_gpio_write(sd,PWDN,CSI_GPIO_HIGH);
			vfe_gpio_write(sd,RESET,CSI_GPIO_LOW);
			usleep_range(1000,1200);
			vfe_set_mclk_freq(sd,MCLK);
			vfe_set_mclk(sd,ON);
			usleep_range(10000,12000);
			vfe_gpio_write(sd,POWER_EN,CSI_GPIO_HIGH);
			vfe_set_pmu_channel(sd,IOVDD,ON);
			vfe_set_pmu_channel(sd,AVDD,ON);
			vfe_set_pmu_channel(sd,DVDD,ON);
			vfe_set_pmu_channel(sd,AFVDD,ON);
			vfe_gpio_write(sd,PWDN,CSI_GPIO_LOW);
			usleep_range(10000,12000);
			vfe_gpio_write(sd,RESET,CSI_GPIO_HIGH);
			usleep_range(30000,31000);
			cci_unlock(sd);
			break;
		case CSI_SUBDEV_PWR_OFF:
			vfe_dev_dbg("CSI_SUBDEV_PWR_OFF!\n");
			cci_lock(sd);
			vfe_set_mclk(sd,OFF);
			vfe_gpio_write(sd,POWER_EN,CSI_GPIO_LOW);
			vfe_set_pmu_channel(sd,AFVDD,OFF);
			vfe_set_pmu_channel(sd,DVDD,OFF);
			vfe_set_pmu_channel(sd,AVDD,OFF);
			vfe_set_pmu_channel(sd,IOVDD,OFF);
			usleep_range(10000,12000);
			vfe_gpio_write(sd,PWDN,CSI_GPIO_LOW);
			vfe_gpio_write(sd,RESET,CSI_GPIO_LOW);
			//  vfe_gpio_set_status(sd,RESET,0);//set the gpio to input
			//  vfe_gpio_set_status(sd,PWDN,0);//set the gpio to input
			cci_unlock(sd);
			break;
		default:
			return -EINVAL;
	}

  return 0;
}

static int sensor_reset(struct v4l2_subdev *sd, u32 val)
{
	switch(val)
	{
		case 0:
			vfe_gpio_write(sd,RESET,CSI_GPIO_HIGH);
			mdelay(10);
			break;
		case 1:
			vfe_gpio_write(sd,RESET,CSI_GPIO_LOW);
			mdelay(10);
			break;
		default:
			return -EINVAL;
	}

	return 0;
}

static int sensor_detect(struct v4l2_subdev *sd)
{
	data_type rdval;

	LOG_ERR_RET(sensor_read(sd, 0x0000, &rdval))

	if(rdval != 0x03)
		return -ENODEV;


	return 0;
}

static int sensor_init(struct v4l2_subdev *sd, u32 val)
{
	int ret;
	struct sensor_info *info = to_state(sd);

	vfe_dev_dbg("sensor_init 0x%x\n",val);

	/*Make sure it is a target sensor*/
	ret = sensor_detect(sd);
	if (ret) {
		vfe_dev_err("chip found is not an target chip.\n");
		return ret;
	}

	vfe_get_standby_mode(sd,&info->stby_mode);

	if((info->stby_mode == HW_STBY || info->stby_mode == SW_STBY) \
	  && info->init_first_flag == 0) {
		vfe_dev_print("stby_mode and init_first_flag = 0\n");
		return 0;
	}

	info->focus_status = 0;
	info->low_speed = 0;
	info->width = 0;
	info->height = 0;
	info->brightness = 0;
	info->contrast = 0;
	info->saturation = 0;
	info->hue = 0;
	info->hflip = 0;
	info->vflip = 0;
	info->gain = 0;
	info->autogain = 1;
	info->exp_bias = 0;
	info->autoexp = 0;
  info->autowb = 1;
  info->clrfx = V4L2_COLORFX_NONE;
  info->band_filter = V4L2_CID_POWER_LINE_FREQUENCY_50HZ;

	info->tpf.numerator = 1;
	info->tpf.denominator = 30;    /* 30fps */

	ret = sensor_write_array(sd, sensor_default_regs, ARRAY_SIZE(sensor_default_regs));
	if(ret < 0) {
		vfe_dev_err("write sensor_default_regs error\n");
		return ret;
	}

	sensor_s_band_filter(sd, V4L2_CID_POWER_LINE_FREQUENCY_50HZ);

	if(info->stby_mode == 0)
		info->init_first_flag = 0;

	info->preview_first_flag = 1;

	return 0;
}

static int sensor_g_exif(struct v4l2_subdev *sd, struct sensor_exif_attribute *exif)
{
	int ret = 0;//, gain_val, exp_val;
	unsigned  int  temp=0,shutter=0, gain = 0;
	unsigned char val;

	sensor_write(sd, 0xfe, 0x00);
	//sensor_write(sd, 0xb6, 0x02);

	/*read shutter */
	sensor_read(sd, 0x03, &val);
	temp |= (val<< 8);
	sensor_read(sd, 0x04, &val);
	temp |= (val & 0xff);
	shutter=temp;
	
	sensor_read(sd, 0xb1, &val);
	gain = val;
	exif->fnumber = 280;
	exif->focal_length = 425;
	exif->brightness = 125;
	exif->flash_fire = 0;
	//exif->iso_speed = 50*((50 + CLIP(gain-0x20, 0, 0xff)*5)/50);
	exif->iso_speed = 50*gain / 16;

	exif->exposure_time_num = 1;
	exif->exposure_time_den = 16000/shutter;
	return ret;
}


static long sensor_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	int ret=0;
	//struct sensor_info *info = to_state(sd);
	switch(cmd) {
		case GET_SENSOR_EXIF:
			sensor_g_exif(sd, (struct sensor_exif_attribute *)arg);
			break;
		default:
			return -EINVAL;
	}
	return ret;
}


/*
 * Store information about the video data format.
 */
static struct sensor_format_struct {
  __u8 *desc;
  //__u32 pixelformat;
  enum v4l2_mbus_pixelcode mbus_code;
  struct regval_list *regs;
  int regs_size;
  int bpp;   /* Bytes per pixel */
} sensor_formats[] = {
  {
    .desc   = "YUYV 4:2:2",
    .mbus_code  = V4L2_MBUS_FMT_YUYV8_2X8,
    .regs     = sensor_fmt_yuv422_yuyv,
    .regs_size = ARRAY_SIZE(sensor_fmt_yuv422_yuyv),
    .bpp    = 2,
  },
  {
    .desc   = "YVYU 4:2:2",
    .mbus_code  = V4L2_MBUS_FMT_YVYU8_2X8,
    .regs     = sensor_fmt_yuv422_yvyu,
    .regs_size = ARRAY_SIZE(sensor_fmt_yuv422_yvyu),
    .bpp    = 2,
  },
  {
    .desc   = "UYVY 4:2:2",
    .mbus_code  = V4L2_MBUS_FMT_UYVY8_2X8,
    .regs     = sensor_fmt_yuv422_uyvy,
    .regs_size = ARRAY_SIZE(sensor_fmt_yuv422_uyvy),
    .bpp    = 2,
  },
  {
    .desc   = "VYUY 4:2:2",
    .mbus_code  = V4L2_MBUS_FMT_VYUY8_2X8,
    .regs     = sensor_fmt_yuv422_vyuy,
    .regs_size = ARRAY_SIZE(sensor_fmt_yuv422_vyuy),
    .bpp    = 2,
  },
//  {
//    .desc   = "Raw RGB Bayer",
//    .mbus_code  = V4L2_MBUS_FMT_SBGGR8_1X8,
//    .regs     = sensor_fmt_raw,
//    .regs_size = ARRAY_SIZE(sensor_fmt_raw),
//    .bpp    = 1
//  },
};
#define N_FMTS ARRAY_SIZE(sensor_formats)



/*
 * Then there is the issue of window sizes.  Try to capture the info here.
 */


static struct sensor_win_size sensor_win_sizes[] = {
  /* qsxga: 2592*1936 */
  {
    .width      = QSXGA_WIDTH,
    .height     = QSXGA_HEIGHT,
    .hoffset    = 0,
    .voffset    = 0,
    .regs       = sensor_qsxga_regs,
    .regs_size  = ARRAY_SIZE(sensor_qsxga_regs),
    .set_size   = NULL,
  },
  #if 0
  /* qxga: 2048*1536 */
  {
    .width      = QXGA_WIDTH,
    .height     = QXGA_HEIGHT,
    .hoffset    = 0,
    .voffset    = 0,
    .regs       = sensor_qxga_regs,
    .regs_size  = ARRAY_SIZE(sensor_qxga_regs),
    .set_size   = NULL,
  },
  #endif
  /* 1080P */
  {
    .width      = HD1080_WIDTH,
    .height     = HD1080_HEIGHT,
    .hoffset    = 0,
    .voffset    = 0,
    .regs       = sensor_1080p_regs,
    .regs_size  = ARRAY_SIZE(sensor_1080p_regs),
    .set_size   = NULL,
  },
  /* UXGA */
  {
    .width      = UXGA_WIDTH,
    .height     = UXGA_HEIGHT,
    .hoffset    = 0,
    .voffset    = 0,
    .regs       = sensor_uxga_regs,
    .regs_size  = ARRAY_SIZE(sensor_uxga_regs),
    .set_size   = NULL,
  },
  /* SXGA */
  {
    .width      = SXGA_WIDTH,
    .height     = SXGA_HEIGHT,
    .hoffset    = 0,
    .voffset    = 0,
    .regs       = sensor_sxga_regs,
    .regs_size  = ARRAY_SIZE(sensor_sxga_regs),
    .set_size   = NULL,
  },
  /* 720p */
  {
    .width      = HD720_WIDTH,
    .height     = HD720_HEIGHT,
    .hoffset    = 0,
    .voffset    = 0,
    .regs       = sensor_720p_regs,
    .regs_size  = ARRAY_SIZE(sensor_720p_regs),
    .set_size   = NULL,
  },
  /* XGA */
  {
    .width      = XGA_WIDTH,
    .height     = XGA_HEIGHT,
    .hoffset    = 0,
    .voffset    = 0,
    .regs       = sensor_xga_regs,
    .regs_size  = ARRAY_SIZE(sensor_xga_regs),
    .set_size   = NULL,
  },
  /* SVGA */
  {
    .width      = SVGA_WIDTH,
    .height     = SVGA_HEIGHT,
    .hoffset    = 0,
    .voffset    = 0,
    .regs       = sensor_svga_regs,
    .regs_size  = ARRAY_SIZE(sensor_svga_regs),
    .set_size   = NULL,
  },
  /* VGA */
  {
    .width      = VGA_WIDTH,
    .height     = VGA_HEIGHT,
    .hoffset    = 0,
    .voffset    = 0,
    .regs       = sensor_vga_regs,
    .regs_size  = ARRAY_SIZE(sensor_vga_regs),
    .set_size   = NULL,
  },
};

#define N_WIN_SIZES (ARRAY_SIZE(sensor_win_sizes))

static int sensor_enum_fmt(struct v4l2_subdev *sd, unsigned index,
                 enum v4l2_mbus_pixelcode *code)
{
  if (index >= N_FMTS)
    return -EINVAL;

  *code = sensor_formats[index].mbus_code;
  return 0;
}

static int sensor_enum_size(struct v4l2_subdev *sd,
                            struct v4l2_frmsizeenum *fsize)
{
	if(fsize->index > N_WIN_SIZES-1)
		return -EINVAL;

	fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
	fsize->discrete.width = sensor_win_sizes[fsize->index].width;
	fsize->discrete.height = sensor_win_sizes[fsize->index].height;

	return 0;
}


static int sensor_try_fmt_internal(struct v4l2_subdev *sd,
    struct v4l2_mbus_framefmt *fmt,
    struct sensor_format_struct **ret_fmt,
    struct sensor_win_size **ret_wsize)
{
  int index;
  struct sensor_win_size *wsize;

	if (index >= N_FMTS)
		return -EINVAL;

	if (ret_fmt != NULL)
		*ret_fmt = sensor_formats + index;

	/*
	* Fields: the sensor devices claim to be progressive.
	*/
	fmt->field = V4L2_FIELD_NONE;

	/*
	* Round requested image size down to the nearest
	* we support, but not below the smallest.
	*/
	for (wsize = sensor_win_sizes; wsize < sensor_win_sizes + N_WIN_SIZES; wsize++)
		if (fmt->width >= wsize->width && fmt->height >= wsize->height)
			break;

	if (wsize >= sensor_win_sizes + N_WIN_SIZES)
		wsize--;   /* Take the smallest one */
	if (ret_wsize != NULL)
		*ret_wsize = wsize;
	/*
	* Note the size we'll actually handle.
	*/
	fmt->width = wsize->width;
	fmt->height = wsize->height;

  return 0;
}

static int sensor_try_fmt(struct v4l2_subdev *sd,
             struct v4l2_mbus_framefmt *fmt)
{
  return sensor_try_fmt_internal(sd, fmt, NULL, NULL);
}

static int sensor_g_mbus_config(struct v4l2_subdev *sd,
           struct v4l2_mbus_config *cfg)
{
	cfg->type = V4L2_MBUS_PARALLEL;
	cfg->flags = V4L2_MBUS_MASTER | VREF_POL | HREF_POL | CLK_POL ;

	return 0;
}

/*
 * Set fps
 */
#if 0
static int sensor_s_fps(struct v4l2_subdev *sd)
{
  struct sensor_info *info = to_state(sd);
  unsigned char div,sys_div;
  unsigned char band_50_high,band_50_low,band_60_high,band_60_low;
  unsigned char band_50_step,band_60_step,vts_high,vts_low;
  int band_50,band_60,vts;

  struct regval_list regs_fr[] = {
		{0x3035,0xee},
		{0x3a08,0xee},//50HZ step MSB 
		{0x3a09,0xee},//50HZ step LSB 
		{0x3a0a,0xee},//60HZ step MSB 
		{0x3a0b,0xee},//60HZ step LSB 
		{0x3a0e,0xee},//50HZ step max 
		{0x3a0d,0xee},//60HZ step max 
  	//{REG_TERM,VAL_TERM},
  };

  vfe_dev_dbg("sensor_s_fps\n");
  
  if (info->tpf.numerator == 0)
    return -EINVAL;
    
  div = info->tpf.numerator;
  
  //power down
//  ret = sensor_write(sd, 0x3008, 0x42);
//  if(ret<0) {
//    vfe_dev_err("power down error at sensor_s_parm!\n");
//    return ret;
//  }
  
  LOG_ERR_RET(sensor_read(sd, 0x3035, &sys_div))  
  LOG_ERR_RET(sensor_read(sd, 0x3a08, &band_50_high))
  LOG_ERR_RET(sensor_read(sd, 0x3a09, &band_50_low))
  
  band_50 = band_50_high*256+band_50_low;
  
  LOG_ERR_RET(sensor_read(sd, 0x3a0a, &band_60_high))
  LOG_ERR_RET(sensor_read(sd, 0x3a0b, &band_60_low))
    
  band_60 = band_60_high*256+band_60_low;
  
  LOG_ERR_RET(sensor_read(sd, 0x380e, &vts_high)) 
  LOG_ERR_RET(sensor_read(sd, 0x380f, &vts_low))
  
  vts = vts_high*256+vts_low;
  
//  vfe_dev_dbg("sys_div=%x,band50=%x,band_60=%x\n",sys_div,band_50,band_60);
  
  sys_div = (sys_div & 0x0f) | ((sys_div & 0xf0)*div);
  band_50 = band_50/div;
  band_60 = band_60/div;
  band_50_step = vts/band_50;
  band_60_step = vts/band_60;
  
//  vfe_dev_dbg("sys_div=%x,band50=%x,band_60=%x,band_50_step=%x,band_60_step=%x\n",sys_div,band_50,band_60,band_50_step,band_60_step);
  
  regs_fr[0].data = sys_div;
  regs_fr[1].data = (band_50&0xff00)>>8;
  regs_fr[2].data = (band_50&0x00ff)>>0;
  regs_fr[3].data = (band_60&0xff00)>>8;
  regs_fr[4].data = (band_60&0x00ff)>>0;
  regs_fr[5].data = band_50_step;
  regs_fr[6].data = band_60_step;
  
  LOG_ERR_RET(sensor_write_array(sd, regs_fr, ARRAY_SIZE(regs_fr)))
  
//#if DEV_DBG_EN == 1 
//  {
//    int i;  
//    for(i=0;i<7;i++) {
//      sensor_read(sd,regs_fr[i].reg_num,regs_fr[i].value);
//      vfe_dev_print("address 0x%2x%2x = %4x",regs_fr[i].reg_num[0],regs_fr[i].reg_num[1],regs_fr[i].value[0]);
//    }
//  }
//#endif
	
//	//release power down
//  ret = sensor_write(sd, 0x3008, 0x02);
//  if(ret<0) {
//    vfe_dev_err("release power down error at sensor_s_parm!\n");
//    return ret;
//  }
  
  //msleep(500);
  vfe_dev_dbg("set frame rate %d\n",info->tpf.denominator/info->tpf.numerator);
  
  return 0;
}
#endif
/*
 * Set a format.
 */
static int sensor_s_fmt(struct v4l2_subdev *sd,
             struct v4l2_mbus_framefmt *fmt)//linux-3.0
{
	int ret;
	struct sensor_format_struct *sensor_fmt;
	struct sensor_win_size *wsize;
	struct sensor_info *info = to_state(sd);
	int cnt;
	data_type retval,pos_l=0,pos_h=0;

	vfe_dev_print(" sensor_s_fmt\n");

	ret = sensor_try_fmt_internal(sd, fmt, &sensor_fmt, &wsize);
	if (ret)
		return ret;
	if(info->capture_mode == V4L2_MODE_IMAGE)
	{
		LOG_ERR_RET(sensor_write(sd, 0x070B, 0x01))
		LOG_ERR_RET(sensor_write(sd, 0x070A, 0x03))
		mdelay(200);
		LOG_ERR_RET(sensor_write(sd, 0x070B, 0x02))
		for(cnt=0;cnt<20;cnt++)
		{
	  		LOG_ERR_RET(sensor_read(sd, 0x07AE, &retval))
			if(retval==1)
				break;
		}
		 LOG_ERR_RET(sensor_read(sd, 0x06F0, &pos_h))
		 LOG_ERR_RET(sensor_read(sd, 0x06F1, &pos_l))


	}
	else if(info->capture_mode == V4L2_MODE_PREVIEW)
		sensor_s_continueous_af(sd);

	sensor_write_array(sd, sensor_fmt->regs , sensor_fmt->regs_size);//add by xiongbiao on 20130403
	ret = 0;
	if (wsize->regs)
	{
		 vfe_dev_print(" 22222222222222\n");
		ret = sensor_write_array(sd, wsize->regs , wsize->regs_size);
		if (ret < 0)
			return ret;
	}
	if(info->capture_mode == V4L2_MODE_VIDEO || info->capture_mode == V4L2_MODE_PREVIEW)
	{
	/*
		if(info->auto_focus==1) 
		{
			sensor_s_continueous_af(sd, 1); 
			msleep(100); 
		} 
		else 
		{ 
			msleep(150); 
		} 

		 vfe_dev_print("info->capture_mode == V4L2_MODE_VIDEO\n");
		 */
	}
	else if(info->capture_mode == V4L2_MODE_IMAGE)
	{
		vfe_dev_print("info->capture_mode == V4L2_MODE_IMAGE\n");
		LOG_ERR_RET(sensor_write(sd, 0x070A, 0x00))
		LOG_ERR_RET(sensor_write(sd, 0x0734, pos_h))
		LOG_ERR_RET(sensor_write(sd, 0x0735, pos_l))
		LOG_ERR_RET(sensor_write(sd, 0x070c, 0x00))
		mdelay(200);
		LOG_ERR_RET(sensor_write(sd, 0x070c, 0x05))
		mdelay(200);

	}
	if (wsize->set_size)
	{
		ret = wsize->set_size(sd);
		if (ret < 0)
			return ret;
	}

#if 0	
	if(info->capture_mode == V4L2_MODE_VIDEO)
	{
		//video
		if(info->af_mode != V4L2_AF_FIXED) {

#if 0
			if(info->af_mode != V4L2_AF_TOUCH && info->af_mode != V4L2_AF_FACE) {				
				ret = sensor_s_relaunch_af_zone(sd);	//set af zone to default zone
				if (ret < 0) {
					 vfe_dev_err("sensor_s_relaunch_af_zone err !\n");
					return ret;
				}	
			}
#endif

			if(info->af_mode != V4L2_AF_INFINITY) {
				ret = sensor_s_continueous_af(sd);		//set continueous af
				if (ret < 0) {
					 vfe_dev_err("sensor_s_continueous_af err !\n");
					return ret;
				}
			}
		}
		sensor_s_fps(sd);	
	}
#endif	
#if 0//#if DEV_DBG_EN == 1		//gong
	{
		int i;
		struct regval_list dbg_regs[] = {
			{{0x30,0x34},{0xee}},
			{{0x30,0x35},{0xee}},
			{{0x30,0x36},{0xee}},
			{{0x30,0x37},{0xee}},
			{{0x31,0x08},{0xee}},
			{{0x38,0x24},{0xee}},
		};
		for(i=0;i<6;i++) {
			sensor_read(sd,dbg_regs[i].reg_num,dbg_regs[i].value);
			 vfe_dev_print("address 0x%2x%2x = %4x",dbg_regs[i].reg_num[0],dbg_regs[i].reg_num[1],dbg_regs[i].value[0]);
		}
	}
#endif
		
	info->fmt = sensor_fmt;
	info->width = wsize->width;
	info->height = wsize->height;
	vfe_dev_print("s_fmt=%s set width = %d, height = %d\n",sensor_fmt->desc,wsize->width,wsize->height);

	return 0;
}


/*
 * Implement G/S_PARM.  There is a "high quality" mode we could try
 * to do someday; for now, we just do the frame rate tweak.
 */
static int sensor_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	struct v4l2_captureparm *cp = &parms->parm.capture;
	struct sensor_info *info = to_state(sd);

	if (parms->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	memset(cp, 0, sizeof(struct v4l2_captureparm));
	cp->capability = V4L2_CAP_TIMEPERFRAME;
	cp->capturemode = info->capture_mode;

	cp->timeperframe.numerator = info->tpf.numerator;
	cp->timeperframe.denominator = info->tpf.denominator;

	return 0;
}

static int sensor_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	struct v4l2_captureparm *cp = &parms->parm.capture;
	struct v4l2_fract *tpf = &cp->timeperframe;
	struct sensor_info *info = to_state(sd);
	data_type div;

	vfe_dev_dbg("sensor_s_parm\n");

	if (parms->type != V4L2_BUF_TYPE_VIDEO_CAPTURE){
		vfe_dev_dbg("parms->type!=V4L2_BUF_TYPE_VIDEO_CAPTURE\n");
		return -EINVAL;
	}

	if (info->tpf.numerator == 0){
		vfe_dev_dbg("info->tpf.numerator == 0\n");
		return -EINVAL;
	}

	info->capture_mode = cp->capturemode;

	if (info->capture_mode == V4L2_MODE_IMAGE) {
		vfe_dev_dbg("capture mode is not video mode,can not set frame rate!\n");
		return 0;
	}

	if (tpf->numerator == 0 || tpf->denominator == 0) {
		tpf->numerator = 1;
		tpf->denominator = SENSOR_FRAME_RATE;/* Reset to full rate */
		vfe_dev_err("sensor frame rate reset to full rate!\n");
	}

	div = SENSOR_FRAME_RATE/(tpf->denominator/tpf->numerator);
	if(div > 15 || div == 0)
	{
		vfe_dev_print("SENSOR_FRAME_RATE=%d\n",SENSOR_FRAME_RATE);
		vfe_dev_print("tpf->denominator=%d\n",tpf->denominator);
		vfe_dev_print("tpf->numerator=%d\n",tpf->numerator);
		return -EINVAL;
	}

	vfe_dev_dbg("set frame rate %d\n",tpf->denominator/tpf->numerator);

	info->tpf.denominator = SENSOR_FRAME_RATE;
	info->tpf.numerator = div;

	if(info->tpf.denominator/info->tpf.numerator < 30)
		info->low_speed = 1;

	return 0;
}


/*
 * Code for dealing with controls.
 * fill with different sensor module
 * different sensor module has different settings here
 * if not support the follow function ,retrun -EINVAL
 */

/* *********************************************begin of ******************************************** */
static int sensor_queryctrl(struct v4l2_subdev *sd,
    struct v4l2_queryctrl *qc)
{
	/* Fill in min, max, step and default value for these controls. */
	/* see include/linux/videodev2.h for details */
	/* see sensor_s_parm and sensor_g_parm for the meaning of value */

	switch (qc->id) {
	case V4L2_CID_BRIGHTNESS:
		return v4l2_ctrl_query_fill(qc, -4, 4, 1, 1);
	case V4L2_CID_CONTRAST:
		return v4l2_ctrl_query_fill(qc, -4, 4, 1, 1);
	case V4L2_CID_SATURATION:
		return v4l2_ctrl_query_fill(qc, -4, 4, 1, 1);
	case V4L2_CID_HUE:
		return v4l2_ctrl_query_fill(qc, -180, 180, 5, 0);
	case V4L2_CID_VFLIP:
	case V4L2_CID_HFLIP:
		return v4l2_ctrl_query_fill(qc, 0, 1, 1, 0);
	case V4L2_CID_GAIN:
		return v4l2_ctrl_query_fill(qc, 0, 255, 1, 128);
	case V4L2_CID_AUTOGAIN:
		return v4l2_ctrl_query_fill(qc, 0, 1, 1, 1);
	case V4L2_CID_EXPOSURE:
	case V4L2_CID_AUTO_EXPOSURE_BIAS:
		return v4l2_ctrl_query_fill(qc, -4, 4, 1, -1);
	case V4L2_CID_EXPOSURE_AUTO:
		return v4l2_ctrl_query_fill(qc, -4, 4, 1, -1);
	case V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE:
		return v4l2_ctrl_query_fill(qc, 0, 9, 1, 1);
	case V4L2_CID_AUTO_WHITE_BALANCE:
		return v4l2_ctrl_query_fill(qc, 0, 9, 1, 1);
	case V4L2_CID_COLORFX:
		return v4l2_ctrl_query_fill(qc, 0, 15, 1, 0);
	case V4L2_CID_FLASH_LED_MODE:
		return v4l2_ctrl_query_fill(qc, 0, 4, 1, 0);
	case V4L2_CID_3A_LOCK:
		return v4l2_ctrl_query_fill(qc, 0, V4L2_LOCK_FOCUS, 1, 0);
//	case V4L2_CID_AUTO_FOCUS_RANGE:
//		return v4l2_ctrl_query_fill(qc, 0, 0, 0, 0);//only auto
	case V4L2_CID_AUTO_FOCUS_INIT:
	case V4L2_CID_AUTO_FOCUS_RELEASE:
	case V4L2_CID_AUTO_FOCUS_START:
	case V4L2_CID_AUTO_FOCUS_STOP:
	case V4L2_CID_AUTO_FOCUS_STATUS:
		return v4l2_ctrl_query_fill(qc, 0, 0, 0, 0);
	case V4L2_CID_FOCUS_AUTO:
		return v4l2_ctrl_query_fill(qc, 0, 1, 1, 0);
	}
	return -EINVAL;
}

static int sensor_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	switch (ctrl->id) {
	case V4L2_CID_BRIGHTNESS:
		return sensor_g_brightness(sd, &ctrl->value);
	case V4L2_CID_CONTRAST:
		return sensor_g_contrast(sd, &ctrl->value);
	case V4L2_CID_SATURATION:
		return sensor_g_saturation(sd, &ctrl->value);
	case V4L2_CID_HUE:
		return sensor_g_hue(sd, &ctrl->value);
	case V4L2_CID_VFLIP:
		return sensor_g_vflip(sd, &ctrl->value);
	case V4L2_CID_HFLIP:
		return sensor_g_hflip(sd, &ctrl->value);
	case V4L2_CID_GAIN:
		return sensor_g_gain(sd, &ctrl->value);
	case V4L2_CID_AUTOGAIN:
		return sensor_g_autogain(sd, &ctrl->value);
	case V4L2_CID_EXPOSURE:
	case V4L2_CID_AUTO_EXPOSURE_BIAS:
		return sensor_g_exp(sd, &ctrl->value);
	case V4L2_CID_EXPOSURE_AUTO:
		return sensor_g_autoexp(sd, &ctrl->value);
	case V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE:
		return sensor_g_wb(sd, &ctrl->value);
	case V4L2_CID_AUTO_WHITE_BALANCE:
		return sensor_g_autowb(sd, &ctrl->value);
	case V4L2_CID_COLORFX:
		return sensor_g_colorfx(sd, &ctrl->value);
//	case V4L2_CID_FLASH_LED_MODE:
//		return sensor_g_flash_mode(sd, &ctrl->value);
	case V4L2_CID_POWER_LINE_FREQUENCY:
		return sensor_g_band_filter(sd, &ctrl->value);
	case V4L2_CID_3A_LOCK:
		return sensor_g_3a_lock(sd);
//	case V4L2_CID_AUTO_FOCUS_RANGE:
//		ctrl->value=0;//only auto
//		return 0;
//	case V4L2_CID_AUTO_FOCUS_INIT:
//	case V4L2_CID_AUTO_FOCUS_RELEASE:
//	case V4L2_CID_AUTO_FOCUS_START:
//	case V4L2_CID_AUTO_FOCUS_STOP:
	case V4L2_CID_AUTO_FOCUS_STATUS:
		return sensor_g_af_status(sd);
//	case V4L2_CID_FOCUS_AUTO:
	}
	return -EINVAL;
}

static int sensor_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	switch (ctrl->id) {
	case V4L2_CID_BRIGHTNESS:
		return sensor_s_brightness(sd, ctrl->value);
	case V4L2_CID_CONTRAST:
		return sensor_s_contrast(sd, ctrl->value);
	case V4L2_CID_SATURATION:
		return sensor_s_saturation(sd, ctrl->value);
	case V4L2_CID_HUE:
		return sensor_s_hue(sd, ctrl->value);
	case V4L2_CID_VFLIP:
		return sensor_s_vflip(sd, ctrl->value);
	case V4L2_CID_HFLIP:
		return sensor_s_hflip(sd, ctrl->value);
	case V4L2_CID_GAIN:
		return sensor_s_gain(sd, ctrl->value);
	case V4L2_CID_AUTOGAIN:
		return sensor_s_autogain(sd, ctrl->value);
	case V4L2_CID_EXPOSURE:
	case V4L2_CID_AUTO_EXPOSURE_BIAS:
		return sensor_s_exp(sd, ctrl->value);
	case V4L2_CID_EXPOSURE_AUTO:
	return sensor_s_autoexp(sd, (enum v4l2_exposure_auto_type) ctrl->value);
	case V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE:
		return sensor_s_wb(sd, (enum v4l2_auto_n_preset_white_balance) ctrl->value);
	case V4L2_CID_AUTO_WHITE_BALANCE:
		return sensor_s_autowb(sd, ctrl->value);
	case V4L2_CID_COLORFX:
		return sensor_s_colorfx(sd, (enum v4l2_colorfx) ctrl->value);
//	case V4L2_CID_FLASH_LED_MODE:
//		return sensor_s_flash_mode(sd, (enum v4l2_flash_led_mode) ctrl->value);
	case V4L2_CID_POWER_LINE_FREQUENCY:
	return sensor_s_band_filter(sd, (enum v4l2_power_line_frequency) ctrl->value);
//	case V4L2_CID_3A_LOCK:
//		return sensor_s_3a_lock(sd, ctrl->value);
//	case V4L2_CID_AUTO_FOCUS_RANGE:
//		return 0;
	case V4L2_CID_AUTO_FOCUS_INIT:
		return sensor_s_init_af(sd);
	case V4L2_CID_AUTO_FOCUS_RELEASE:
		return sensor_s_release_af(sd);
	case V4L2_CID_AUTO_FOCUS_START:
		return sensor_s_single_af(sd);
	case V4L2_CID_AUTO_FOCUS_STOP:
		return sensor_s_pause_af(sd);
	case V4L2_CID_AUTO_FOCUS_STATUS:
	case V4L2_CID_FOCUS_AUTO:
		return sensor_s_continueous_af(sd);
	}
	return -EINVAL;
}


static int sensor_g_chip_ident(struct v4l2_subdev *sd,
    struct v4l2_dbg_chip_ident *chip)
{
  struct i2c_client *client = v4l2_get_subdevdata(sd);

  return v4l2_chip_ident_i2c_client(client, chip, V4L2_IDENT_SENSOR, 0);
}


/* ----------------------------------------------------------------------- */

static const struct v4l2_subdev_core_ops sensor_core_ops = {
  .g_chip_ident = sensor_g_chip_ident,
  .g_ctrl = sensor_g_ctrl,
  .s_ctrl = sensor_s_ctrl,
  .queryctrl = sensor_queryctrl,
  .reset = sensor_reset,
  .init = sensor_init,
  .s_power = sensor_power,
  .ioctl = sensor_ioctl,
};

static const struct v4l2_subdev_video_ops sensor_video_ops = {
  .enum_mbus_fmt = sensor_enum_fmt,
  .enum_framesizes = sensor_enum_size,
  .try_mbus_fmt = sensor_try_fmt,
  .s_mbus_fmt = sensor_s_fmt,
  .s_parm = sensor_s_parm,
  .g_parm = sensor_g_parm,
  .g_mbus_config = sensor_g_mbus_config,
};

static const struct v4l2_subdev_ops sensor_ops = {
  .core = &sensor_core_ops,
  .video = &sensor_video_ops,
};

/* ----------------------------------------------------------------------- */
static struct cci_driver cci_drv = {
	.name = SENSOR_NAME,
};


static int sensor_probe(struct i2c_client *client,
      const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct sensor_info *info;
	info = kzalloc(sizeof(struct sensor_info), GFP_KERNEL);
	if (info == NULL)
		return -ENOMEM;
	sd = &info->sd;
	glb_sd = sd;
	cci_dev_probe_helper(sd, client, &sensor_ops, &cci_drv);
	info->fmt = &sensor_formats[0];
	info->af_first_flag = 1;
	info->init_first_flag = 1;
	info->auto_focus = 0;
	return 0;
}

static int sensor_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd;
	sd = cci_dev_remove_helper(client, &cci_drv);
	printk("sensor_remove ov5640 sd = %p!\n",sd);
	kfree(to_state(sd));
	return 0;
}

static const struct i2c_device_id sensor_id[] = {
  { SENSOR_NAME, 0 },
  { }
};
MODULE_DEVICE_TABLE(i2c, sensor_id);

static struct i2c_driver sensor_driver = {
  .driver = {
    .owner = THIS_MODULE,
  .name = SENSOR_NAME,
  },
  .probe = sensor_probe,
  .remove = sensor_remove,
  .id_table = sensor_id,
};
static __init int init_sensor(void)
{
	return cci_dev_init_helper(&sensor_driver);
}

static __exit void exit_sensor(void)
{
	cci_dev_exit_helper(&sensor_driver);
}

module_init(init_sensor);
module_exit(exit_sensor);

