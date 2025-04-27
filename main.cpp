#include <glib.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include "absl/flags/internal/parse.h"
#include "absl/flags/parse.h"
#include "api/scoped_refptr.h"
#include "peer_connection_client.h"
#include "rtc_base/physical_socket_server.h"
#include "rtc_base/ref_counted_object.h"
#include "rtc_base/ssl_adapter.h"
#include "rtc_base/thread.h"
#include "system_wrappers/include/field_trial.h"
#include "test/field_trial.h"

#include <thread>
#include "conductor.h"
#include "listen_socket.h"
#include "SipClient.h"
#include <vector>
#include "Public.h"

class CustomSocketServer : public rtc::PhysicalSocketServer {
 public:
  explicit CustomSocketServer(P2P* wnd)
      : wnd_(wnd), conductor_(NULL), client_(NULL) {
      }
  virtual ~CustomSocketServer() {}

  void SetMessageQueue(rtc::Thread* queue) override { message_queue_ = queue; }

  void set_client(PeerConnectionClient* client) { client_ = client;}
  void set_conductor(Conductor* conductor) { conductor_ = conductor;}

  // Override so that we can also pump the GTK message loop.
  bool Wait(int cms, bool process_io) override {
    // Pump GTK events.
    // TODO(henrike): We really should move either the socket server or UI to a
    // different thread.  Alternatively we could look at merging the two loops
    // by implementing a dispatcher for the socket server and/or use
    // g_main_context_set_poll_func.
   
    // while (gtk_events_pending())
    //   gtk_main_iteration();

     if (!wnd_->IsWindow() && !conductor_->connection_active() &&
        client_ != NULL && !client_->is_connected()) {
      printf("关闭线程\n");
      message_queue_->Quit();
    }
    
    return rtc::PhysicalSocketServer::Wait(0 /*cms == -1 ? 1 : cms*/,
                                           process_io);
  }

 protected:
  rtc::Thread* message_queue_;
  P2P* wnd_;
  Conductor* conductor_;
  PeerConnectionClient* client_;
};



int main() {

    SipClient sipClient;
    
    sipClient.gb28181_client_start();
    Public::Init();
    // P2P p2p("192.168.30.193",11111);
    // CustomSocketServer socket_server(&p2p);
    // rtc::AutoSocketServerThread thread(&socket_server);

    

    // rtc::InitializeSSL();
    
    // //Must be constructed after we set the socketserver.
    // PeerConnectionClient client;
    
    // rtc::scoped_refptr<Conductor> conductor(
    //     new rtc::RefCountedObject<Conductor>(&client, &p2p));
    // socket_server.set_client(&client);
    // socket_server.set_conductor(conductor);
    
    
    // thread.Run();

    int port = 8888;
    ListeningSocket listener;
    if (!listener.Listen(port)) {
        printf("Failed to listen on server socket\n");
        return -1;
    }

    while (1)
    {
        fd_set socket_set;
        FD_ZERO(&socket_set);//重置添加的套接字
        int listener_socket = listener.Getsocket();
        FD_SET(listener_socket, &socket_set);
        int max_fd = listener_socket;

        // 2. 添加所有客户端套接字到集合
        for (int sock : listener.sockets) {
            FD_SET(sock, &socket_set);
            if (sock > max_fd) max_fd = sock;
        }

        // 3. 设置超时并调用 select
        struct timeval timeout = {10, 0};
        int activity = select(max_fd + 1, &socket_set, NULL, NULL, &timeout);
        if (activity < 0) {
            perror("select error");
            continue;
        }

        // 4. 优先处理新连接
        if (FD_ISSET(listener_socket, &socket_set)) {
            int new_sock = listener.Accept();
            if (new_sock != -1) {
                if (listener.sockets.size() < 50) {
                    listener.sockets.push_back(new_sock);
                    printf("New connection\n");
                } else {
                    close(new_sock);
                    printf("Connection limit reached\n");
                }
            }
        }
    }


    return 0;
}


//export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/zhang/T527_Server/osip/lib
//export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/mnt/hgfs/IPC/T527_Server/osip/lib