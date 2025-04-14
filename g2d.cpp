
#include "g2d.h"
#define TJE_IMPLEMENTATION
#include "./tiny_jpeg.h"
using namespace g2dapi;

G2d::G2d()
{
    Create_Path();
}

G2d::~G2d()
{
}


void G2d::Create_Path()
{
    pcompPicPath0=Config::GetTime_Path();
    pcompPicPath0+=Config::Config::Get("Path","Montage");
    bool created = std::filesystem::create_directories(pcompPicPath0);
    if (created) {
        std::cout << "Directory created: " <<pcompPicPath0<< std::endl;
    }
}

int G2d::decode_jpeg(const char *filename, paramStruct_t *pops)
{
     struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;

    FILE* infile = fopen(filename, "rb");
    if (!infile) {
        fprintf(stderr, "Error opening JPEG file: %s\n", filename);
        return 0;
    }

    ALOGD("fopen %s OK ", filename);

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, infile);
    jpeg_read_header(&cinfo, TRUE);
    jpeg_start_decompress(&cinfo);
    int width = cinfo.output_width;
    int height = cinfo.output_height;
    int channels = cinfo.output_components;

    unsigned char* rgb_data = (unsigned char*)malloc(iWidth * iHeight * 3);
    unsigned char* row_buffer = (unsigned char*)malloc(iWidth * 3);
    unsigned char* yuv_data = (unsigned char*)malloc(iWidth * iHeight * 3 / 2);

    while (cinfo.output_scanline < cinfo.output_height) {
        jpeg_read_scanlines(&cinfo, &row_buffer, 1);
        memcpy(rgb_data + (cinfo.output_scanline - 1) * iWidth * 3, row_buffer, iWidth * 3);
    }

    rgb_to_yuv420(rgb_data,yuv_data,iWidth,iHeight);

    int iRet = 0;
    iRet = allocOpen(MEM_TYPE_CDX_NEW, pops, NULL);
    if (iRet < 0) {
        ALOGD("ion_alloc_open failed");
        return iRet;
    }
    pops->size = iSubWidth * iSubHeight * 3 / 2;
    iRet = allocAlloc(MEM_TYPE_CDX_NEW, pops, NULL);
    if (iRet < 0) {
        ALOGD("allocAlloc failed");
        return iRet;
    }


    unsigned char *dst_yuv_data = (unsigned char *)malloc(iSubWidth * iSubHeight * 3 / 2);
    memset(dst_yuv_data,0,iSubWidth * iSubHeight * 3 / 2);

    // 定义跨度参数
    const int src_stride_y = iWidth;
    const int src_stride_uv = iWidth;
    const int dst_stride_y = iSubWidth;
    const int dst_stride_uv = iSubWidth;

    // 获取UV平面指针
    uint8_t* src_uv = yuv_data + iWidth * iHeight;
    uint8_t* dst_uv = dst_yuv_data + iSubWidth * iSubHeight;

   
    // 执行NV12缩放
    int ret = libyuv::NV12Scale(
        yuv_data,      src_stride_y,   // 源Y
        src_uv,         src_stride_uv,  // 源UV
        iWidth,         iHeight,        // 源分辨率
        dst_yuv_data,    dst_stride_y,   // 目标Y
        dst_uv,         dst_stride_uv,  // 目标UV
        iSubWidth,      iSubHeight,     // 目标分辨率
        libyuv::kFilterBox              // 缩放模式
    );
    printf("压缩完成\n");
    // 将缩放后的数据拷贝到 pops->vir
    memcpy((void *)pops->vir, dst_yuv_data, pops->size);

    
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    fclose(infile);
    free(rgb_data);
    free(row_buffer);
    free(yuv_data);
    free(dst_yuv_data);

    flushCache(MEM_TYPE_CDX_NEW, pops, NULL);
    return 0;
}

void G2d::rgb_to_yuv420(unsigned char *rgb, unsigned char *yuv, int width, int height)
{
     int frameSize = width*height;
    int yIndex = 0;
    int uvIndex = frameSize;
  
    int R, G, B, Y, U, V;
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            B = rgb[(i * width + j) * 3 + 0];
            G = rgb[(i * width + j) * 3 + 1];
            R = rgb[(i * width + j) * 3 + 2];
  
            //RGB to    
            Y = ((66 * R + 129 * G + 25 * B + 128) >> 8) + 16;
            U = ((-38 * R - 74 * G + 112 * B + 128) >> 8) + 128;
            V = ((112 * R - 94 * G - 18 * B + 128) >> 8) + 128;
  
            yuv[yIndex++] = (unsigned char)((Y < 0) ? 0 : ((Y > 255) ? 255 : Y));
            if (i % 2 == 0 && j % 2 == 0)
            {
                yuv[uvIndex++] = (unsigned char)((U < 0) ? 0 : ((U > 255) ? 255 : U));
                yuv[uvIndex++] = (unsigned char)((V < 0) ? 0 : ((V > 255) ? 255 : V));
            }
        }
    }
}

// 检查文件扩展名是否为图片格式
bool G2d::is_image_file(const std::string& filename) {
    // 这里只检查了一些常见的图片扩展名，你可以根据需要添加更多
    std::string ext = filename.substr(filename.find_last_of(".") + 1);
    return (ext == "jpg") || (ext == "jpeg") || (ext == "png") || (ext == "bmp") || (ext == "gif");
}

int G2d::allocPicMem(paramStruct_t *pops, int size)
{
    int iRet = 0;

    iRet = allocOpen(MEM_TYPE_CDX_NEW, pops, NULL);
    if (iRet < 0) {
        printf("ion_alloc_open failed");
        return iRet;
    }
    pops->size = size;
    iRet = allocAlloc(MEM_TYPE_CDX_NEW, pops, NULL);
    if (iRet < 0) {
        printf("allocAlloc failed");
        return iRet;
    }

    return 0;
}

void G2d::NV21_TO_RGB24(unsigned char *data, unsigned char *rgb, int width, int height)
{
    int index = 0;

    unsigned char *ybase = data;
    unsigned char *ubase = &data[width * height];
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            //YYYYYYYYVUVU
            u_char Y = ybase[x + y * width];
            u_char U = ubase[y / 2 * width + (x / 2) * 2 + 1];
            u_char V = ubase[y / 2 * width + (x / 2) * 2];

            rgb[index++] = Y + 1.402 * (V - 128); //R
            rgb[index++] = Y - 0.34413 * (U - 128) - 0.71414 * (V - 128); //G
            rgb[index++] = Y + 1.772 * (U - 128); //B
        }
    }
}

int G2d::freePicMem(paramStruct_t *pops)
{
    allocFree(MEM_TYPE_CDX_NEW, pops, NULL);

    return 0;
}

void G2d::montage_jpeg()
{
    std::string folder_path = Config::Get("Path","Tmp");
    string sava_path;
    // 检查路径是否存在且为目录
    if (!std::filesystem::exists(folder_path) || !std::filesystem::is_directory(folder_path)) {
        std::cerr << "The specified folder does not exist or is not a directory." << std::endl;
        return ;
    }

     for (const auto& entry : std::filesystem::directory_iterator(folder_path)) {
        // 检查是否是文件且扩展名为图片
        if (std::filesystem::is_regular_file(entry.status()) && is_image_file(entry.path().filename().string())) {
            // 将图片路径添加到向量中
            image_paths.push_back(entry.path().string());
        }
    }

    // if (Config::client_map.size() != image_paths.size())
    // {
    //     printf("client=%d,image=%d\n",Config::client_map.size(),image_paths.size());
    //     return ;
    // }
    int image_size = image_paths.size();
    printf("文件夹中图片的数量:%d\n",image_size);

    for (int  i = 0; i < image_size; i++)
    {
        decode_jpeg(image_paths[i].c_str(),&m_DispMemOpsArray[i]);
    }
    
    allocPicMem(&m_DispMemOps, 1920 * 1080 * 3 / 2);

    //compose
    int g2dfd = g2dInit();

    t1.tv_sec = t1.tv_usec = 0;
    t2.tv_sec = t2.tv_usec = 0;
    gettimeofday(&t1, NULL);
    int outfd = m_DispMemOps.ion_buffer.fd_data.aw_fd;
    int infd[9];
    for (int  i = 0; i < image_paths.size(); i++)
    {
        infd[i] =m_DispMemOpsArray[i].ion_buffer.fd_data.aw_fd;
    }
    

    int ret = -1;
    int flag = (int)G2D_ROT_0;
    int fmt = (int)G2D_FORMAT_YUV420UVC_U1V1U0V0;
    
    for (int i = 0; i < image_size; i++)
    {
        int row = i / 3;
        int col = i % 3; 

        int dst_x = col * iSubWidth; 
        int dst_y = row * iSubHeight;

        ret |= g2dClipByFd(g2dfd, infd[i], flag, fmt, iSubWidth, iSubHeight, 0, 0, iSubWidth, iSubHeight,
                        outfd, fmt, iSubWidth, iSubHeight, dst_x, dst_y, 1920, 1080);

    }

    gettimeofday(&t2, NULL);
    printf("use time =%lld us\n ",int64_t(t2.tv_sec) * 1000000LL + int64_t(t2.tv_usec) - (int64_t(t1.tv_sec) * 1000000LL + int64_t(t1.tv_usec)));
    
    unsigned char *RGBStreamPack = (unsigned char*)malloc(1920 * 1080 * 3);
    memset(RGBStreamPack, 0, 1920 * 1080 * 3);

    NV21_TO_RGB24(m_DispMemOps.vir,RGBStreamPack,1920,1080); 

    time_t now = time(nullptr);
    tm local_time;
    localtime_r(&now, &local_time);
    ostringstream oss;
    oss << "/"<< setw(4) << (local_time.tm_year + 1900)<<"_"
        << setw(2) << setfill('0') << (local_time.tm_mon + 1)<<"_"
        << setw(2) << setfill('0') << local_time.tm_mday<<"_montage.jpeg";
    sava_path += pcompPicPath0;
    sava_path+=oss.str();
    cout << "拼接图片保存路径："<< sava_path <<endl;
    tje_encode_to_file(sava_path.c_str(), 1920,1080, 3, RGBStreamPack);

    
    // for(int  i = 0; i < image_size; i++)
    // {
    //     try
    //     {
    //         if (!std::filesystem::exists(image_paths[i])) 
    //         {
    //            std::cerr << "Skipped (file not found): " << image_paths[i] << std::endl;
    //         }

    //          if(!std::filesystem::remove(image_paths[i]))
    //         {
    //             std::cerr << "Failed to delete: " << image_paths[i] << " (unknown error)";
    //         }
    //     }
    //     catch(const std::filesystem::filesystem_error& e)
    //     {
    //         std::cerr << "Error deleting " << image_paths[i] << ": " << e.what() << std::endl;
    //     }
    // }



    g2dUnit(g2dfd);
    image_paths.clear();
    free(RGBStreamPack);
    freePicMem(&m_DispMemOps);
    for (int  i = 0; i < image_size; i++)
    {
        freePicMem(&m_DispMemOpsArray[i]);
    }
}


