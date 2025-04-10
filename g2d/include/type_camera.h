#include <stdint.h>
#include "videodev2_new.h"
#ifndef __LIB__CAMERA__TYPE__H__
#define __LIB__CAMERA__TYPE__H__

typedef struct PREVIEWINFO_t {
    unsigned int left;
    unsigned int top;
    unsigned int width;          // preview width
    unsigned int height;         // preview height
} PREVIEWINFO_t, RECT_t;

typedef struct V4L2BUF_t {
    unsigned long   addrPhyY;       // physical Y address of this frame
    unsigned long   addrPhyC;       // physical Y address of this frame
    uint8_t*   addrVirY;       // virtual Y address of this frame
    uint8_t*   addrVirC;       // virtual Y address of this frame
    unsigned int    width;
    unsigned int    height;
    int             index;          // DQUE id number
    long long       timeStamp;      // time stamp of this frame
    RECT_t          crop_rect;
    int             format;
    void*           overlay_info;

    // thumb
    unsigned char   isThumbAvailable;
    unsigned char   thumbUsedForPreview;
    unsigned char   thumbUsedForPhoto;
    unsigned char   thumbUsedForVideo;
    unsigned int    thumbAddrPhyY;      // physical Y address of thumb buffer
    uint8_t*    thumbAddrVirY;      // virtual Y address of thumb buffer
    unsigned int    thumbWidth;
    unsigned int    thumbHeight;
    RECT_t          thumb_crop_rect;
    int             thumbFormat;

    int             refCnt;         // used for releasing this frame
    unsigned int    bytesused;      // used by compressed source
    int             dmafd;
    struct v4l2_buffer buf;
} V4L2BUF_t;

typedef enum MEDIA_SRC_MODE {
    MEDIA_SRC_PUSH_MODE,
    MEDIA_SRC_PULL_MODE
} MEDIA_SRC_MODE;

typedef struct VIDEOINFO_t {
    int video_source;
    int src_height;
    int src_width;
    int height;         // camcorder video frame height
    int width;          // camcorder video frame width
    int frameRate;      // camcorder video frame rate
    int bitRate;        // camcorder video bitrate
    short profileIdc;
    short levelIdc;

    int geo_available;
    int latitudex10000;
    int longitudex10000;

    // rotate
    int rotate_degree;      // only support 0, 90, 180 and 270

    // for video encoder
    unsigned int picEncmode; //0 for frame encoding 1: for field encoding 2:field used for frame encoding
    unsigned int qp_max;
    unsigned int qp_min;

    int is_compress_source; // 0 for common source 1: for mjpeg source 2: for h264 source
} VIDEOINFO_t;

typedef struct AUDIOINFO_t {
    int sampleRate;
    int channels;
    int bitRate;
    int bitsPerSample;
    int audioEncType;  // 0: aac, 1: LPCM
} AUDIOINFO_t;

typedef struct ENCEXTRADATAINFO_t { //don't touch it, because it also defined in type.h
    char *data;
    int length;
} ENCEXTRADATAINFO_t;

typedef struct ENC_BUFFER_t {
    uint8_t* addrY;
    uint8_t* addrCb;
    uint8_t* addrCr;
    int width;
    int height;
    RECT_t crop_rect;
    int force_keyframe;
    void*  overlay_info;
    int format;
} ENC_BUFFER_t;

typedef enum JPEG_COLOR_FORMAT {
    JPEG_COLOR_YUV444,
    JPEG_COLOR_YUV422,
    JPEG_COLOR_YUV420,
    JPEG_COLOR_YUV411,
    JPEG_COLOR_YUV420_NV12,
    JPEG_COLOR_YUV420_NV21,
    JPEG_COLOR_TILE_32X32,
    JPEG_COLOR_CSIARGB,
    JPEG_COLOR_CSIRGBA,
    JPEG_COLOR_CSIABGR,
    JPEG_COLOR_CSIBGRA
} JPEG_COLOR_FORMAT;

typedef struct JPEG_ENC_t {
    unsigned int             src_w;
    unsigned int             src_h;
    unsigned int             pic_w;
    unsigned int             pic_h;
    uint8_t*             addrY;
    uint8_t*             addrC;
    int             colorFormat;
    int             quality;
    int             rotate;

    int             scale_factor;
    double          focal_length;

    int             thumbWidth;
    int             thumbHeight;

    unsigned char   enable_crop;
    int             crop_x;
    int             crop_y;
    int             crop_w;
    int             crop_h;

    // gps exif
    unsigned char   enable_gps;
    double          gps_latitude;
    double          gps_longitude;
    long            gps_altitude;
    long            gps_timestamp;
    char            gps_processing_method[100];
    int             whitebalance;
    char            CameraMake[64];//for the cameraMake name
    char            CameraModel[64];//for the cameraMode
    char            DateTime[21];//for the data and time
    void*           pover_overlay;
} JPEG_ENC_t;


typedef struct thumb_buffer {
    int            id;
    unsigned int   width;
    unsigned int   height;
    unsigned char* y_vir;
    unsigned char* uv_vir;
    unsigned int   y_phy;
    unsigned int   uv_phy;
    int size_y;
    int size_uv;
    long long pts;
} thumb_buffer;

// typedef void (*notify_callback)(int32_t msgType,
//                                 int32_t ext1,
//                                 int32_t ext2,
//                                 void* user);

// typedef void (*data_callback)(int32_t msgType,
//                               char *dataPtr,
//                               camera_frame_metadata_t *metadata,
//                               void* user);

// typedef void (*data_callback_timestamp)(nsecs_t timestamp,
//                                         int32_t msgType,
//                                         char *dataPtr,
//                                         void *user);

// typedef status_t (*usr_data_cb)(int32_t msgType,
//                                 char *dataPtr, int dalen,
//                                 void *user);
#endif // __LIB__CAMERA__TYPE__H__

