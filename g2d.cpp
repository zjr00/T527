
#include "g2d.h"
#define TJE_IMPLEMENTATION
#include "./tiny_jpeg.h"
using namespace g2dapi;
#include <set>


/*功能：t527硬件拼图---》九宫格*/

G2d::G2d()
{
    
}

G2d::~G2d()
{
}


void G2d::Create_Path()
{
    ftp.ftp_init();
    pcompPicPath0=Config::GetTime_Path();
    pcompPicPath0+=Config::Get("Path","Montage");
    bool created = filesystem::create_directories(pcompPicPath0);
    if (created) {
        cout << "Directory created: " <<pcompPicPath0<< endl;
    }
}



void G2d::set_scok_size(int size)
{
    if (size >8 )
    {
        sock_size = 8;

        return;
    }
    
    sock_size =size;

    return;
}

int G2d::decode_jpeg(const char *filename, paramStruct_t *pops)
{
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    int ret = -1;

    unsigned char *dst_yuv_data = (unsigned char *)malloc(iSubWidth * iSubHeight * 3 / 2);
    memset(dst_yuv_data,0,iSubWidth * iSubHeight * 3 / 2);

    // 定义跨度参数
    const int src_stride_y = iWidth;
    const int src_stride_uv = iWidth;
    const int dst_stride_y = iSubWidth;
    const int dst_stride_uv = iSubWidth;

    FILE* infile = fopen(filename, "rb");
    if (!infile) {
        fprintf(stderr, "Error opening JPEG file: %s\n", filename);
        return -1;
    }

    LOGD("fopen %s OK", filename);

    cinfo.err = jpeg_std_error(&jerr);
    


    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, infile);
    jpeg_read_header(&cinfo, TRUE);
    jpeg_start_decompress(&cinfo);
    int width = cinfo.output_width;
    int height = cinfo.output_height;
    int channels = cinfo.output_components;
    LOGI("width=%d,height=%d,channels=%d\n",width,height,channels);


    unsigned char* rgb_data = (unsigned char*)malloc(width * height * 3);
    unsigned char* row_buffer = (unsigned char*)malloc(width * 3);
    unsigned char* yuv_data = (unsigned char*)malloc(width * height * 3 / 2);

    while (cinfo.output_scanline < cinfo.output_height) {
        jpeg_read_scanlines(&cinfo, &row_buffer, 1);
        memcpy(rgb_data + (cinfo.output_scanline -1) * width * 3, row_buffer, width * 3);
    }



    rgb_to_yuv420(rgb_data,yuv_data,width,height);
    // 获取UV平面指针
    uint8_t*src_uv = yuv_data + width * height;
    uint8_t*dst_uv = dst_yuv_data + iSubWidth * iSubHeight;
   
    // 执行NV12缩放
    ret = libyuv::NV12Scale(
        yuv_data,      src_stride_y,   // 源Y
        src_uv,         src_stride_uv,  // 源UV
        width,         height,        // 源分辨率
        dst_yuv_data,    dst_stride_y,   // 目标Y
        dst_uv,         dst_stride_uv,  // 目标UV
        iSubWidth,      iSubHeight,     // 目标分辨率
        libyuv::kFilterBox              // 缩放模式
    );
    printf("压缩完成\n");
    // 将缩放后的数据拷贝到 pops->vir
    
    memcpy((void *)pops->vir, dst_yuv_data, pops->size);
 
    if (cinfo.global_state) {
        jpeg_finish_decompress(&cinfo);
        jpeg_destroy_decompress(&cinfo);
    }
    
    if (infile) {
        fclose(infile);
    }
    
    free(rgb_data);
    free(row_buffer);
    free(yuv_data);
    free(dst_yuv_data);


    flushCache(MEM_TYPE_CDX_NEW, pops, NULL);//数据同步
    return 1;
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
bool G2d::is_image_file(const string& filename) {
    // 这里只检查了一些常见的图片扩展名，你可以根据需要添加更多
    string ext = filename.substr(filename.find_last_of(".") + 1);
    return (ext == "jpg") || (ext == "jpeg") || (ext == "png") || (ext == "bmp") || (ext == "gif");
}

int G2d::allocPicMem(paramStruct_t *pops, int size)
{
    // memset(&pops, 0, sizeof(paramStruct_t));
    //printf("size = %d\n",size);

    int iRet = 0;

    iRet = allocOpen(MEM_TYPE_CDX_NEW, pops, NULL);
    if (iRet < 0) {
        LOGE("ion_alloc_open failed");
        return iRet;
    }
    pops->size = size;
    iRet = allocAlloc(MEM_TYPE_CDX_NEW, pops, NULL);
    if (iRet < 0) {
        LOGE("allocAlloc failed");
        return iRet;
    }
   //LOGD("allocPicMem: vir=%p, phy=0x%lx, size=%d", pops->vir, pops->phy, size);
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

void G2d::save_yuv_file(unsigned char *yuv_data, int width, int height)
{
    char save_filename[256];
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    
    snprintf(save_filename, sizeof(save_filename), 
             "/tmp/IPC/%04d%02d%02d_%02d%02d%02d.yuv",
             t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
             t->tm_hour, t->tm_min, t->tm_sec);
    
    FILE* yuv_file = fopen(save_filename, "wb");
    if (yuv_file) {
        size_t yuv_size = width * height * 3 / 2;
        fwrite(yuv_data, 1, yuv_size, yuv_file);
        fclose(yuv_file);
        LOGD("Saved compressed YUV to: %s (Size: %dx%d)", 
             save_filename, width, height);
    }
}

int G2d::freePicMem(paramStruct_t *pops)
{
    if (!pops) {
        return -1;
    }
    
     int iRet = 0;
    //LOGD("freePicMem: vir=%p, phy=0x%lx, size=%d", pops->vir, pops->phy, pops->size);
    // 检查是否有有效的内存需要释放
    if (pops->vir != nullptr && pops->phy != 0 && pops->size != 0) { 
        allocFree(MEM_TYPE_CDX_NEW, pops, NULL);
        allocClose(MEM_TYPE_CDX_NEW, pops, NULL);
        // 重置结构体
        memset(pops, 0, sizeof(paramStruct_t));
    }
    
    return 0;
}

int G2d::extract_index(const string &filename)
{
    // 找到文件名中的数字部分
    size_t dot_pos = filename.find('.');
    if (dot_pos == string::npos) return -1;
    
    string num_str = filename.substr(0, dot_pos);
    try {
        return stoi(num_str);
    } catch (...) {
        return -1;
    }
}

void G2d::mark_client_completed(int sockfd)
{
    if (is_resetting.load()) return;

    bool should_start_timer = false;
    unique_lock<std::mutex> lock(clients_mutex);
    auto it_comp =find(completed_clients.begin(), completed_clients.end(), sockfd);
    if (it_comp == completed_clients.end()) {
        completed_clients.push_back(sockfd);

    }

    if (completed_clients.size() == 1&& !is_first) {
        printf("这是第一个\n");
        first_completion_time = chrono::steady_clock::now();
        should_start_timer =true;
        is_first = true;
        
    }

    bool trigger_montage = false;
    {
        if (!montage_triggered.load() && 
            !completed_clients.empty() && 
            completed_clients.size() == sock_size) {
            montage_triggered.store(true);
            trigger_montage = true;
        }
    }

    lock.unlock(); 
    
    if(should_start_timer)
    {
        should_start_timer =false;
        start_montage_timer();
    }

    // 检查是否所有客户端都已完成
    //check_all_clients_completed();

    if (trigger_montage) {
        montage_jpeg();
    }
   
}

void G2d::timer_thread_func()
{
    unique_lock<std::mutex> lock(timer_mutex);
        
    // 使用条件变量等待，但设置超时
    auto status = timer_cv.wait_for(lock, chrono::seconds(15), [this] {
        return stop_timer.load() || montage_triggered.load() || is_resetting.load();
    });
    
    // 检查是否需要触发拼图
    if (!status && !montage_triggered.load() && !is_resetting.load()) {
        printf("超时触发拼图\n");
        montage_triggered.store(true);
        lock.unlock();  // 重要：先释放锁再执行耗时操作
        montage_jpeg();
    }
}

void G2d::start_montage_timer()
{
    if (montage_triggered.load()  || is_resetting.load()) {
            return;
    }

    lock_guard<std::mutex> lock(timer_mutex);
    // 启动新计时器
    stop_timer.store(false);
    timer_thread = thread([this]() {
                this->timer_thread_func();
            });

    
    timer_thread.detach();
    return;
    
}

// 停止计时器
void G2d::stop_timer_thread()
{   
   
    stop_timer = true;
    {
        lock_guard<std::mutex> lock(timer_mutex);
        timer_cv.notify_all();
    }

    // if (timer_thread.joinable()) {
    //     // 设置超时，避免永久阻塞
    //     auto start = chrono::steady_clock::now();
    //     while (chrono::steady_clock::now() - start < chrono::seconds(2)) {
    //         if (!timer_thread.joinable()) break;
    //         this_thread::sleep_for(chrono::milliseconds(10));
    //     }
        
    //     if (timer_thread.joinable()) {
    //         printf("强制分离计时器线程\n");
    //         timer_thread.detach();
    //     }
    // }
}

void G2d::check_all_clients_completed()
{
    printf("sock_size = %d,size = %d\n",sock_size,completed_clients.size());

        
   
}

void G2d::reset_state()
{   
    printf("开始回收资源\n");
    is_resetting.store(true);
    // 先停止计时器
    stop_timer_thread();
    
    // 然后清理状态
    {
        unique_lock<std::mutex> lock(clients_mutex);
        completed_clients.clear();
        montage_triggered.store(false);
    }
    is_resetting.store(false);
    printf("回收资源完成\n");
}

void G2d::montage_jpeg()
{


    // 检查是否应该继续
    if (is_resetting.load()) {
        printf("重置过程中，取消拼图\n");
        return;
    }

    printf("开始拼图\n");
    struct timeval total_start, total_end;  // 添加总耗时计时
    long long total_time;

    gettimeofday(&total_start, NULL);       // 记录开始时间

    string folder_path = Config::Get("Path","Tmp");
    string sava_path;
    vector<string> tmp_paths;
    map<int, string> image_map; // 用map自动按序号排序
    int g2dfd = g2dInit();
    unsigned char *RGBStreamPack = nullptr;
    time_t now = time(nullptr);
    tm local_time;
    ostringstream oss;
    int outfd = -1;
    int infd[9];
    
    int ret = -1;
    int flag = (int)G2D_ROT_0;
    int fmt = (int)G2D_FORMAT_YUV420UVC_U1V1U0V0;

    vector<string> jpegExtensions = {".jpg", ".jpeg", ".jpe", ".JPG", ".JPEG", ".JPE"};

    // 检查路径是否存在且为目录
    if (!filesystem::exists(folder_path) || !filesystem::is_directory(folder_path)) {
        cerr << "The specified folder does not exist or is not a directory." << endl;
        return ;
    }

    allocPicMem(&m_DispMemOps, iWidth * iHeight * 3 / 2);//申请九宫格图片内存
    
    
    // 收集所有图片并按序号存储
    for (const auto& entry : filesystem::directory_iterator(folder_path)) {
        if (filesystem::is_regular_file(entry.status()) && is_image_file(entry.path().filename().string())) {
            int index = extract_index(entry.path().filename().string());
            if (index >= 0) {
                image_map[index] = entry.path().string();
            }
        }
    }

    for (int i = 0; i < 9; ++i) {
        int expected_index = i + 1;
        auto it = image_map.find(expected_index);
        if (it != image_map.end()) {
            tmp_paths.push_back(it->second);
        } else {
            tmp_paths.push_back("./data/green.jpeg");
        }
    }
    

    image_paths = move(tmp_paths);
    tmp_paths.clear();
    for (int  i = 0; i < image_paths.size(); i++)
    {
        allocPicMem(m_DispMemOpsArray[i],iSubWidth * iSubHeight * 3 / 2);
        int ret =decode_jpeg(image_paths[i].c_str(),m_DispMemOpsArray[i]);//解码图片
        if(ret <0)
        {
            LOGE("Failed to decode image %d: %s", i, image_paths[i].c_str());
            goto cleanup;
        }
    }
    
    outfd = m_DispMemOps.ion_buffer.fd_data.aw_fd;
    for (int  i = 0; i < 9; i++)
    {
        infd[i] =m_DispMemOpsArray[i]->ion_buffer.fd_data.aw_fd;
    }
    
    
    for (int i = 0; i < 9; i++)
    {
        int row = i / 3;
        int col = i % 3; 

        int dst_x = col * iSubWidth; 
        int dst_y = row * iSubHeight;

        ret |= g2dClipByFd(g2dfd, infd[i], flag, fmt, iSubWidth, iSubHeight, 0, 0, iSubWidth, iSubHeight,
                        outfd, fmt, iSubWidth, iSubHeight, dst_x, dst_y, iWidth, iHeight);

    }

    RGBStreamPack = (unsigned char*)malloc(iWidth * iHeight * 3);
    if (!RGBStreamPack) {
        LOGE("Failed to allocate RGB buffer");
        goto cleanup;
    }
    memset(RGBStreamPack, 0, iWidth * iHeight * 3);

    NV21_TO_RGB24(m_DispMemOps.vir,RGBStreamPack,iWidth,iHeight); 


    localtime_r(&now, &local_time);
    
    oss << "/"<< setw(4) << (local_time.tm_year + 1900)
        << setw(2) << setfill('0') << (local_time.tm_mon + 1) 
        << setw(2) << setfill('0') << local_time.tm_mday 
        << setw(2) << setfill('0') << local_time.tm_hour
        << setw(2) << setfill('0') << local_time.tm_min
        << setw(2) << setfill('0') << local_time.tm_sec << "_montage.jpeg";
    sava_path += pcompPicPath0;
    sava_path+=oss.str();

    LOGI("拼接图片保存路径：%s",sava_path.c_str());
    tje_encode_to_file(sava_path.c_str(), iWidth,iHeight, 3, RGBStreamPack);


    gettimeofday(&total_end, NULL);//结束时间
    
    total_time = (int64_t(total_end.tv_sec) * 1000000LL + int64_t(total_end.tv_usec)) - 
                          (int64_t(total_start.tv_sec) * 1000000LL + int64_t(total_start.tv_usec));

    printf("总耗时:          %lld us (%.3f ms)\n", total_time, total_time / 1000.0);     

    ftp.upload_file_start(sava_path);        
    // for (const auto& [index, filepath] : image_map)
    // {
    //     try
    //     {
        
    //         if (!filesystem::exists(filepath)) 
    //         {
    //             LOGE("Skipped (file not found): %s",filepath);
    //            //cerr << "Skipped (file not found): " << filepath << endl;
    //         }

    //          if(!filesystem::remove(filepath))
    //         {
    //             LOGE("Failed to delete:  %s",filepath);
    //             //cerr << "Failed to delete: " << filepath << " (unknown error)"<<endl;
    //         }
    //     }
    //     catch(const filesystem::filesystem_error& e)
    //     {
    //         LOGE("Error deleting :  %s",filepath);
    //         //cerr << "Error deleting " << filepath << ": " << e.what() << endl;
    //     }
    // }

     
        
    // for (const auto& entry : filesystem::recursive_directory_iterator(folder_path)) {
    //     if (entry.is_regular_file()) {
    //         string extension = entry.path().extension().string();
            
    //         // 转换为小写比较（可选）
    //         string lowerExtension = extension;
    //         transform(lowerExtension.begin(), lowerExtension.end(), lowerExtension.begin(), ::tolower);
            
    //         // 检查是否是JPEG文件
    //         for (const auto& ext : jpegExtensions) {
    //             if (lowerExtension == ext) {
    //                 //cout << "删除文件: " << entry.path() << endl;
    //                 filesystem::remove(entry.path());
    //                 break;
    //             }
    //         }
    //     }
    // }
cleanup:
    
    is_first =false;
    if (g2dfd >= 0) {
        g2dUnit(g2dfd);
    }

    if (m_DispMemOps.vir) {
        freePicMem(&m_DispMemOps);
    }

    for (int  i = 0; i < 9; i++)
    {
        freePicMem(m_DispMemOpsArray[i]);
    }
    
    reset_state();
    image_paths.clear();
    free(RGBStreamPack);
    
    return ;
}



