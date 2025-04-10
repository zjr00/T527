#ifndef P2P_H
#define P2P_H

#include <stdint.h>
#include <memory>
#include <string>

#include "api/media_stream_interface.h"
#include "api/scoped_refptr.h"
#include "api/video/video_frame.h"
#include "api/video/video_sink_interface.h"
#include "peer_connection_client.h"

#include <mutex>
#include <condition_variable>
#include <queue>
typedef std::map<int, std::string> Peers;
// Forward declarations.
typedef struct _GtkWidget GtkWidget;
typedef struct _GdkEventKey GdkEventKey;
typedef struct _GtkTreeView GtkTreeView;
typedef struct _GtkTreePath GtkTreePath;
typedef struct _GtkTreeViewColumn GtkTreeViewColumn;
typedef struct _cairo cairo_t;
typedef union _GdkEvent GdkEvent;


class MainWndCallback {
 public:
  virtual void StartLogin(const std::string& server, int port) = 0;
  virtual void DisconnectFromServer() = 0;
  virtual void ConnectToPeer(int peer_id) = 0;
  virtual void DisconnectFromCurrentPeer() = 0;
  virtual void UIThreadCallback(int msg_id, void* data) = 0;
  virtual void Close() = 0;

 protected:
  virtual ~MainWndCallback() {}
};


class MainWindow {
 public:
  virtual ~MainWindow() {}

  enum UI {
    CONNECT_TO_SERVER,
    LIST_PEERS,
    STREAMING,
  };

  virtual void RegisterObserver(MainWndCallback* callback) = 0;

  virtual bool IsWindow() = 0;

  virtual UI current_ui() = 0;

  virtual void StartLocalRenderer(webrtc::VideoTrackInterface* local_video) = 0;
  virtual void StopLocalRenderer() = 0;
  virtual void StartRemoteRenderer(
      webrtc::VideoTrackInterface* remote_video) = 0;
  virtual void StopRemoteRenderer() = 0;

  virtual void QueueUIThreadCallback(int msg_id, void* data) = 0;
};



class P2P: public MainWindow
{
public:
  P2P(const char* server, int port);
  ~P2P();
  virtual void RegisterObserver(MainWndCallback* callback);
  virtual bool IsWindow();
  virtual MainWindow::UI current_ui();
  virtual void StartLocalRenderer(webrtc::VideoTrackInterface* local_video);
  virtual void StopLocalRenderer();
  virtual void StartRemoteRenderer(webrtc::VideoTrackInterface* remote_video);
  virtual void StopRemoteRenderer();

  virtual void QueueUIThreadCallback(int msg_id, void* data);

  bool Destroy();
  // connection.



  void HandleUIThreadCallback();
  static void handle_signal(int signum);

  void Destroy_signal();

protected:
  class VideoRenderer : public rtc::VideoSinkInterface<webrtc::VideoFrame> {
   public:
    VideoRenderer(P2P* main_wnd,webrtc::VideoTrackInterface* track_to_render);
    virtual ~VideoRenderer();
    void OnFrame(const webrtc::VideoFrame& frame) override;
    const uint8_t* image() const { return image_.get(); }

    int width() const { return width_; }

    int height() const { return height_; }

   protected:
    std::unique_ptr<uint8_t[]> image_;
    int width_;
    int height_;
    P2P* main_wnd_;
    rtc::scoped_refptr<webrtc::VideoTrackInterface> rendered_track_;
  };

protected:
  GtkWidget* window_;     // Our main window.
  GtkWidget* draw_area_;  // The drawing surface for rendering video streams.
  GtkWidget* vbox_;       // Container for the Connect UI.
  GtkWidget* server_edit_;
  GtkWidget* port_edit_;
  GtkWidget* peer_list_;  // The list of peers.
  MainWndCallback* callback_;
  std::string server_;
  int port_;
  std::unique_ptr<VideoRenderer> local_renderer_;
  std::unique_ptr<VideoRenderer> remote_renderer_;
  int width_;
  int height_;
  std::unique_ptr<uint8_t[]> draw_buffer_;
  int draw_buffer_size_;
};


#endif