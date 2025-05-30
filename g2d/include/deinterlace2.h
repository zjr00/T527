
#ifndef __DEINTERLACE_H__
#define __DEINTERLACE_H__

#include <stdio.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/types.h>

enum __di_pixel_fmt_t {
    DI_FORMAT_NV12 = 0x00, /* 2-plane */
    DI_FORMAT_NV21 = 0x01, /* 2-plane */
    DI_FORMAT_MB32_12 = 0x02, /* NOT SUPPORTED, UV mapping like NV12 */
    DI_FORMAT_MB32_21 = 0x03, /* NOT SUPPORTED, UV mapping like NV21 */
    DI_FORMAT_YV12 = 0x04, /* 3-plane */
    DI_FORMAT_YUV422_SP_UVUV = 0x08, /* 2-plane, New in DI_V2.2 */
    DI_FORMAT_YUV422_SP_VUVU = 0x09, /* 2-plane, New in DI_V2.2 */
    DI_FORMAT_YUV422P = 0x0c, /* 3-plane, New in DI_V2.2 */
    DI_FORMAT_MAX,
};

enum __di_intp_mode_t {
    DI_MODE_WEAVE = 0x0, /* Copy source to destination */
    DI_MODE_INTP = 0x1, /* Use current field to interpolate another field */
    DI_MODE_MOTION = 0x2, /* Use 4-field to interpolate another field */
};

enum __di_updmode_t {
    DI_UPDMODE_FIELD = 0x0, /* Output 2 frames when updated 1 input frame */
    DI_UPDMODE_FRAME = 0x1, /* Output 1 frame when updated 1 input frame */
};

struct __di_rectsz_t {
    unsigned int width;
    unsigned int height;
};

struct __di_fb_t {
    void    *addr[2];              /* frame buffer address */
    struct  __di_rectsz_t       size;           /* size pixel */
    enum    __di_pixel_fmt_t    format;
};

struct __di_para_t {
    struct __di_fb_t       input_fb;          /* current frame fb */
    struct __di_fb_t       pre_fb;            /* previous frame fb */
    struct __di_rectsz_t   source_regn;    /* current frame and
                        * previous frame process region
                        */
    struct __di_fb_t       output_fb;     /* output frame fb */
    struct __di_rectsz_t   out_regn;          /* output frame region */
    __u32           field;                  /* process field <0-top field ;
                         * 1-bottom field>
                         */
    __u32          top_field_first;        /* video infomation <0-is not
                        * top_field_first; 1-is top_
                        * field_first>
                        */
};

/* di_format_attr - display format attribute
 *
 * @format: pixel format
 * @bits: bits of each component
 * @hor_rsample_u: reciprocal of horizontal sample rate
 * @hor_rsample_v: reciprocal of horizontal sample rate
 * @ver_rsample_u: reciprocal of vertical sample rate
 * @hor_rsample_v: reciprocal of vertical sample rate
 * @uvc: 1: u & v component combined
 * @interleave: 0: progressive, 1: interleave
 * @factor & div: bytes of pixel = factor / div (bytes)
 *
 * @addr[out]: address for each plane
 * @trd_addr[out]: address for each plane of right eye buffer
 */
struct di_format_attr {
    enum __di_pixel_fmt_t format;
    unsigned int bits;
    unsigned int hor_rsample_u;
    unsigned int hor_rsample_v;
    unsigned int ver_rsample_u;
    unsigned int ver_rsample_v;
    unsigned int uvc;
    unsigned int interleave;
    unsigned int factor;
    unsigned int div;
};

struct __di_fb_t2 {
    int fd;
    unsigned long long addr[3]; /* frame buffer address */
    struct __di_rectsz_t size; /* size (in pixel) */
    enum __di_pixel_fmt_t format;
};

struct __di_para_t2 {
    struct __di_fb_t2 input_fb; /* current frame fb */
    struct __di_fb_t2 pre_fb; /* previous frame fb */
    struct __di_fb_t2 next_fb; /* next frame fb */
    struct __di_rectsz_t source_regn; /* current frame /previous frame and
                        next frame process region */
    struct __di_fb_t2 output_fb; /* output frame fb */
    struct __di_rectsz_t out_regn; /* output frame region */
    unsigned int field; /* process field <0-first field ; 1-second field> */
    unsigned int top_field_first; /* video infomation <0-is not
                top_field_first; 1-is top_field_first> */
    /* unsigned int update_mode; */
    /* update buffer mode <0-update 1 frame,
    output 2 frames; 1-update 1 frame, output 1 frame> */
    int id;
};

/* New in DI_2.X */
struct __di_mode_t {
    enum __di_intp_mode_t di_mode;
    enum __di_updmode_t update_mode;
};

struct __di_mem_t {
    unsigned int size;
    void *v_addr;
    unsigned long p_addr;
};

#define DI_IOC_MAGIC        'D'
#define DI_IOCSTART     _IOWR(DI_IOC_MAGIC, 0, struct __di_rectsz_t)
#define DI_IOCSTART2    _IOWR(DI_IOC_MAGIC, 1, struct __di_rectsz_t)
#define DI_IOCSETMODE   _IOWR(DI_IOC_MAGIC, 2, struct __di_mode_t)
#define DI_REQUEST  _IOWR(DI_IOC_MAGIC, 3, struct __di_rectsz_t)
#define DI_RELEASE  _IOWR(DI_IOC_MAGIC, 4, struct __di_rectsz_t)

enum {
    DEBUG_INIT = 1U << 0,
    DEBUG_INT = 1U << 1,
    DEBUG_DATA_INFO = 1U << 2,
    DEBUG_SUSPEND = 1U << 3,
    DEBUG_TEST = 1U << 4,
};

#endif
