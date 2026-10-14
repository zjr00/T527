// #include <glib.h>
// #include <gtk/gtk.h>
// #include <stdio.h>
// #include "absl/flags/internal/parse.h"
// #include "absl/flags/parse.h"
// #include "api/scoped_refptr.h"
// #include "peer_connection_client.h"
// #include "rtc_base/physical_socket_server.h"
// #include "rtc_base/ref_counted_object.h"
// #include "rtc_base/ssl_adapter.h"
// #include "rtc_base/thread.h"
// #include "system_wrappers/include/field_trial.h"
// #include "test/field_trial.h"
//#include "conductor.h"

#include <thread>

#include "listen_socket.h"

#include <vector>
#include "Public.h"

// #define MACRO_LOG_COLOR  LOG_DISABLE




// class CustomSocketServer : public rtc::PhysicalSocketServer {
//  public:
//   explicit CustomSocketServer(P2P* wnd)
//       : wnd_(wnd), conductor_(NULL), client_(NULL) {
//       }
//   virtual ~CustomSocketServer() {}

//   void SetMessageQueue(rtc::Thread* queue) override { message_queue_ = queue; }

//   void set_client(PeerConnectionClient* client) { client_ = client;}
//   void set_conductor(Conductor* conductor) { conductor_ = conductor;}

//   // Override so that we can also pump the GTK message loop.
//   bool Wait(int cms, bool process_io) override {
//     // Pump GTK events.
//     // TODO(henrike): We really should move either the socket server or UI to a
//     // different thread.  Alternatively we could look at merging the two loops
//     // by implementing a dispatcher for the socket server and/or use
//     // g_main_context_set_poll_func.
   
//     // while (gtk_events_pending())
//     //   gtk_main_iteration();

//      if (!wnd_->IsWindow() && !conductor_->connection_active() &&
//         client_ != NULL && !client_->is_connected()) {
//       printf("关闭线程\n");
//       message_queue_->Quit();
//     }
    
//     return rtc::PhysicalSocketServer::Wait(0 /*cms == -1 ? 1 : cms*/,
//                                            process_io);
//   }

//  protected:
//   rtc::Thread* message_queue_;
//   P2P* wnd_;
//   Conductor* conductor_;
//   PeerConnectionClient* client_;
// };



int main() {

    //Public::Init();
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
#if 0
    int port = 8888;
    ListeningSocket listener;
    listener.Listen_Init();
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
        if (FD_ISSET(listener_socket, &socket_set)) 
        {
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
#else
    
    
    ListeningSocket listener;
    listener.Listen_Init();
    int port = stoi(Config::Get("IPC","port1"));
    if (!listener.Listen_2Server(port,port+1,port+2)) {
        printf("Failed to listen on server socket\n");
        return -1;
    }

    while (1)
    {
        fd_set socket_set;
        FD_ZERO(&socket_set);//重置添加的套接字

        int server_socket1 = listener.Getsocket();
        int server_socket2 = listener.Getsocket2();
        int server_socket3 = listener.Getsocket_gb();

        FD_SET(server_socket1, &socket_set);
        FD_SET(server_socket2, &socket_set);
        FD_SET(server_socket3, &socket_set);

        // 正确计算max_fd
        int max_fd = -1;
        if (server_socket1 > max_fd) max_fd = server_socket1;
        if (server_socket2 > max_fd) max_fd = server_socket2;
        if (server_socket3 > max_fd) max_fd = server_socket3;

        // 2. 添加所有客户端套接字到集合
        for (int sock : listener.sockets) {
            FD_SET(sock, &socket_set);
            if (sock > max_fd) max_fd = sock;
        }

        // 3. 设置超时并调用 select
        struct timeval timeout = {10, 0};
        int activity = select(max_fd + 1, &socket_set, NULL, NULL, &timeout);
        if (activity < 0) {
            // if (errno == EBADF) {
            //     printf("select错误: 坏的文件描述符，进行清理\n");
            //     listener.cleanup_clients();
            // } else {
            //     perror("select error");
            // }
            continue;
        }
        
        // 4. 优先处理新连接
        if (FD_ISSET(server_socket1, &socket_set)) //处理第一个server
        {
            int new_sock = listener.Accept_client();
            if (new_sock != -1) {
                if (listener.sockets.size() < 50) {
                    listener.sockets.push_back(new_sock);
                    listener.client_ptrhread(new_sock);
                    listener.addSocket(new_sock,Config::getClientIP(new_sock),1);
                    //LOGI("接收报文服务器 New connection: %s",Config::getClientIP(new_sock).c_str());
                    // cout<<"接收报文 New connection:"<<Config::getClientIP(new_sock)<<endl;

                } else {
                    close(new_sock);
                    LOGE("Connection limit reached");
                    //printf("Connection limit reached\n");
                }
            }
        }

        if (FD_ISSET(server_socket2, &socket_set)) //处理第二个server
        {
            int file_sock = listener.Accept_clientfile();
            if (file_sock != -1) {
                if (listener.sockets.size() < 50) {
                    listener.sockets.push_back(file_sock);
                    listener.clientfile_ptrhread(file_sock);
                    listener.addSocket(file_sock,Config::getClientIP(file_sock),2);

                    //cout<<"接收文件 New connection:"<<Config::getClientIP(file_sock)<<endl;
                    //LOGI("接收文件服务器 New connection: %s",Config::getClientIP(file_sock).c_str());
                    // #if TARGET_MODE == 2
                    // listener.g2d.set_scok_size((listener.sockets.size())/3);
                    // #endif
                    
                } else {
                    close(file_sock);
                    LOGE("Connection limit reached");
                    //printf("Connection limit reached\n");
                }
            }
        }

        if (FD_ISSET(server_socket3, &socket_set)) //处理第三个server
        {
            int video_sock = listener.Accept_clientVideo();
            if (video_sock != -1) {
                if (listener.sockets.size() < 50) {
                    listener.sockets.push_back(video_sock);
                    listener.clientVideo_ptrhread(video_sock);
                    listener.addSocket(video_sock,Config::getClientIP(video_sock),3);
                } else {
                    close(video_sock);
                    LOGE("Connection limit reached");
                    //printf("Connection limit reached\n");
                }
            }
        }

    }

#endif
    
    return 0;
}


//export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/zhang/T527_Server/osip/lib
//export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/mnt/hgfs/IPC/T527_Server/osip/lib
//export http_proxy=http://192.168.30.170:7897
// export https_proxy=http://192.168.30.170:7897