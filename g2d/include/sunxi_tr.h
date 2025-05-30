//*********************************************************************************************************************
//  All Winner Tech, All Right Reserved. 2014-2015 Copyright (c)
//
//  File name   :   sunxi_tr.h
//
//  Description :   display engine 2.0 rotation processing base functions implement
//
//  History     :   2014/04/08  iptang  v0.1  Initial version
//
//*********************************************************************************************************************

#ifndef __SUNXI_TR_H__
#define __SUNXI_TR_H__

typedef enum {
    TR_FORMAT_ARGB_8888                    = 0x00,//MSB  A-R-G-B  LSB
    TR_FORMAT_ABGR_8888                    = 0x01,
    TR_FORMAT_RGBA_8888                    = 0x02,
    TR_FORMAT_BGRA_8888                    = 0x03,
    TR_FORMAT_XRGB_8888                    = 0x04,
    TR_FORMAT_XBGR_8888                    = 0x05,
    TR_FORMAT_RGBX_8888                    = 0x06,
    TR_FORMAT_BGRX_8888                    = 0x07,
    TR_FORMAT_RGB_888                      = 0x08,
    TR_FORMAT_BGR_888                      = 0x09,
    TR_FORMAT_RGB_565                      = 0x0a,
    TR_FORMAT_BGR_565                      = 0x0b,
    TR_FORMAT_ARGB_4444                    = 0x0c,
    TR_FORMAT_ABGR_4444                    = 0x0d,
    TR_FORMAT_RGBA_4444                    = 0x0e,
    TR_FORMAT_BGRA_4444                    = 0x0f,
    TR_FORMAT_ARGB_1555                    = 0x10,
    TR_FORMAT_ABGR_1555                    = 0x11,
    TR_FORMAT_RGBA_5551                    = 0x12,
    TR_FORMAT_BGRA_5551                    = 0x13,

    /* SP: semi-planar, P:planar, I:interleaved
     * UVUV: U in the LSBs;     VUVU: V in the LSBs */
    TR_FORMAT_YUV444_I_AYUV                = 0x40,//MSB  A-Y-U-V  LSB, reserved
    TR_FORMAT_YUV444_I_VUYA                = 0x41,//MSB  V-U-Y-A  LSB
    TR_FORMAT_YUV422_I_YVYU                = 0x42,//MSB  Y-V-Y-U  LSB
    TR_FORMAT_YUV422_I_YUYV                = 0x43,//MSB  Y-U-Y-V  LSB
    TR_FORMAT_YUV422_I_UYVY                = 0x44,//MSB  U-Y-V-Y  LSB
    TR_FORMAT_YUV422_I_VYUY                = 0x45,//MSB  V-Y-U-Y  LSB
    TR_FORMAT_YUV444_P                     = 0x46,//MSB  P3-2-1-0 LSB,  YYYY UUUU VVVV, reserved
    TR_FORMAT_YUV422_P                     = 0x47,//MSB  P3-2-1-0 LSB   YYYY UU   VV
    TR_FORMAT_YUV420_P                     = 0x48,//MSB  P3-2-1-0 LSB   YYYY U    V
    TR_FORMAT_YUV411_P                     = 0x49,//MSB  P3-2-1-0 LSB   YYYY U    V
    TR_FORMAT_YUV422_SP_UVUV               = 0x4a,//MSB  V-U-V-U  LSB
    TR_FORMAT_YUV422_SP_VUVU               = 0x4b,//MSB  U-V-U-V  LSB
    TR_FORMAT_YUV420_SP_UVUV               = 0x4c,
    TR_FORMAT_YUV420_SP_VUVU               = 0x4d,
    TR_FORMAT_YUV411_SP_UVUV               = 0x4e,
    TR_FORMAT_YUV411_SP_VUVU               = 0x4f,
} tr_pixel_format;

typedef enum {
    TR_ROT_0        = 0x0,//rotate clockwise 0 ROTgree
    TR_ROT_90       = 0x1,//rotate clockwise 90 ROTgree
    TR_ROT_180      = 0x2,//rotate clockwise 180 ROTgree
    TR_ROT_270      = 0x3,//rotate clockwise 270 ROTgree
    TR_HFLIP        = 0x4,//horizontal flip
    TR_HFLIP_ROT_90 = 0x5,//first rotate clockwise 90 ROTgree then horizontal flip
    TR_VFLIP        = 0x6,//vertical flip
    TR_VFLIP_ROT_90 = 0x7,//first rotate clockwise 90 ROTgree then vertical flip
} tr_mode;

typedef struct {
    int x;
    int y;
    unsigned int w;
    unsigned int h;
} tr_rect;

typedef struct {
    unsigned char fmt;
    unsigned char haddr[3];
    unsigned int  laddr[3];
    unsigned int  pitch[3]; /* line stride of fb */
    unsigned int  height[3];
} tr_frame;

typedef struct {
    tr_mode mode;

    tr_frame   src_frame;
    tr_rect    src_rect;

    tr_frame   dst_frame;
    tr_rect    dst_rect;

    int fd;
} tr_info;

typedef enum tag_TR_CMD {
    TR_REQUEST = 0x03,
    TR_RELEASE = 0x04,
    TR_COMMIT = 0x05,
    TR_QUERY = 0x06,
    TR_SET_TIMEOUT = 0x07,
} tr_cmd_t;

#endif
