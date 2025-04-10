#include "P2P.h"
#include <thread>
#include <cairo.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <glib-object.h>
#include <glib.h>
#include <gobject/gclosure.h>
#include <gtk/gtk.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cstdint>
#include <map>
#include <utility>

#include "api/video/i420_buffer.h"
#include "api/video/video_frame_buffer.h"
#include "api/video/video_rotation.h"
#include "api/video/video_source_interface.h"
#include "rtc_base/checks.h"
#include "rtc_base/logging.h"
#include "third_party/libyuv/include/libyuv/convert.h"
#include "third_party/libyuv/include/libyuv/convert_from.h"

#include <signal.h>
#include <unistd.h>
bool shouldExit =false;

namespace {

struct UIThreadCallbackData {
  explicit UIThreadCallbackData(MainWndCallback* cb, int id, void* d)
      : callback(cb), msg_id(id), data(d) {}
  MainWndCallback* callback;
  int msg_id;
  void* data;
};


std::queue<UIThreadCallbackData*> callback_queue_;
std::mutex mutex_;
std::condition_variable condition_;

}  


P2P::P2P(const char *server, int port)
{
    server_ = server;
    port_ = port;
    std::thread uiThread(&P2P::HandleUIThreadCallback, this);
    uiThread.detach();
    // signal(SIGINT,handle_signal);
    // std::thread exitThread([&]() {
    //             this->Destroy_signal();
    //         });
    // exitThread.detach();
}

P2P::~P2P()
{
    RTC_DCHECK(!IsWindow());
}

bool P2P::IsWindow()
{
    if (shouldExit == true)
    {
        return false;
    }
    
    return true;
}

void P2P::RegisterObserver(MainWndCallback *callback)
{
    callback_ = callback;
    callback_->StartLogin(server_, port_);
}

MainWindow::UI P2P::current_ui()
{
    if (vbox_)
    return CONNECT_TO_SERVER;

    if (peer_list_)
        return LIST_PEERS;

    return STREAMING;
}



P2P::VideoRenderer::VideoRenderer(P2P* main_wnd,webrtc::VideoTrackInterface* track_to_render)
    : width_(0),
      height_(0),
      main_wnd_(main_wnd),
      rendered_track_(track_to_render) {
  rendered_track_->AddOrUpdateSink(this, rtc::VideoSinkWants());
}

P2P::VideoRenderer::~VideoRenderer() {
  rendered_track_->RemoveSink(this);
}


void P2P::StartLocalRenderer(webrtc::VideoTrackInterface *local_video)
{
    local_renderer_.reset(new VideoRenderer(this, local_video));//本地渲染器指针
}


void P2P::StartRemoteRenderer(webrtc::VideoTrackInterface *remote_video)
{
    remote_renderer_.reset(new VideoRenderer(this, remote_video));//生成远端渲染器
}



//当生成对端的渲染器器后，有视频帧会回调该函数，生成对端的画面
void P2P::VideoRenderer::OnFrame(const webrtc::VideoFrame& video_frame) {

}


void P2P::QueueUIThreadCallback(int msg_id, void *data)
{
    std::unique_lock<std::mutex> lock(mutex_);
    callback_queue_.push(new UIThreadCallbackData(callback_, msg_id, data));
    lock.unlock();
    condition_.notify_one();
}

bool P2P::Destroy()
{
    window_ = NULL;

    return true;
}

void P2P::Destroy_signal()
{
    while (!shouldExit)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1)); 
    }
    
    callback_->Close();
    exit(0);
}


void P2P::StopRemoteRenderer()
{
    remote_renderer_.reset();
}



void P2P::StopLocalRenderer()
{
    local_renderer_.reset();
}


void P2P::handle_signal(int signum)
{
    shouldExit =true;
}

void P2P::HandleUIThreadCallback()
{
   while (true) 
  {
    std::unique_lock<std::mutex> lock(mutex_);
    condition_.wait(lock, [this] { return!callback_queue_.empty(); });
    UIThreadCallbackData* cb_data = callback_queue_.front();//取出队列中的一个
    callback_queue_.pop();
    lock.unlock();
    cb_data->callback->UIThreadCallback(cb_data->msg_id, cb_data->data);
    printf("callback_queue\n");
    delete cb_data;
  }
}
