/*
 *  Copyright 2012 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "conductor.h"

#include <iostream>
#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <utility>
#include <vector>

#include "absl/memory/memory.h"
#include "absl/types/optional.h"
#include "api/audio/audio_mixer.h"
#include "api/audio_codecs/audio_decoder_factory.h"
#include "api/audio_codecs/audio_encoder_factory.h"
#include "api/audio_codecs/builtin_audio_decoder_factory.h"
#include "api/audio_codecs/builtin_audio_encoder_factory.h"
#include "api/audio_options.h"
#include "api/create_peerconnection_factory.h"
#include "api/rtp_sender_interface.h"
#include "api/video_codecs/builtin_video_decoder_factory.h"
#include "api/video_codecs/builtin_video_encoder_factory.h"
#include "api/video_codecs/video_decoder_factory.h"
#include "api/video_codecs/video_encoder_factory.h"
//#include "defaults.h"
#include "modules/audio_device/include/audio_device.h"
#include "modules/audio_processing/include/audio_processing.h"
#include "modules/video_capture/video_capture.h"
#include "modules/video_capture/video_capture_factory.h"
#include "p2p/base/port_allocator.h"
#include "pc/video_track_source.h"
#include "rtc_base/checks.h"
#include "rtc_base/logging.h"
#include "rtc_base/ref_counted_object.h"
#include "rtc_base/rtc_certificate_generator.h"
#include "rtc_base/strings/json.h"
#include "test/vcm_capturer.h"
#include "rtc_base/arraysize.h"


#include <libyuv.h>
#include <thread>

namespace {
// Names used for a IceCandidate JSON object.
const char kCandidateSdpMidName[] = "sdpMid";
const char kCandidateSdpMlineIndexName[] = "sdpMLineIndex";
const char kCandidateSdpName[] = "candidate";

// Names used for a SessionDescription JSON object.
const char kSessionDescriptionTypeName[] = "type";
const char kSessionDescriptionSdpName[] = "sdp";
bool esc =false;  //判断是否另外一端退出
class DummySetSessionDescriptionObserver
    : public webrtc::SetSessionDescriptionObserver {
 public:
  static DummySetSessionDescriptionObserver* Create() {
    return new rtc::RefCountedObject<DummySetSessionDescriptionObserver>();
  }
  virtual void OnSuccess() { RTC_LOG(LS_INFO) << __FUNCTION__; }
  virtual void OnFailure(webrtc::RTCError error) {
    RTC_LOG(LS_INFO) << __FUNCTION__ << " " << ToString(error.type()) << ": "
                     << error.message();
  }
};

class CapturerTrackSource : public webrtc::VideoTrackSource {
 public:
  static rtc::scoped_refptr<CapturerTrackSource> Create() {
    const size_t kWidth = 640;
    const size_t kHeight = 480;
    const size_t kFps = 30;
    std::unique_ptr<webrtc::test::VcmCapturer> capturer;
    std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> info(
        webrtc::VideoCaptureFactory::CreateDeviceInfo());
    if (!info) {
      return nullptr;
    }
    int num_devices = info->NumberOfDevices();
    for (int i = 0; i < num_devices; ++i) {
      capturer = absl::WrapUnique(
          webrtc::test::VcmCapturer::Create(kWidth, kHeight, kFps, i));
      if (capturer) {
        return new rtc::RefCountedObject<CapturerTrackSource>(
            std::move(capturer));
      }
    }

    return nullptr;
  }

 protected:
  explicit CapturerTrackSource(
      std::unique_ptr<webrtc::test::VcmCapturer> capturer)
      : VideoTrackSource(/*remote=*/false), capturer_(std::move(capturer)) {}

 private:
  rtc::VideoSourceInterface<webrtc::VideoFrame>* source() override {
    return capturer_.get();
  }
  std::unique_ptr<webrtc::test::VcmCapturer> capturer_;
};

}  // namespace

Conductor::Conductor(PeerConnectionClient* client, MainWindow* main_wnd)
    : peer_id_(-1), loopback_(false), client_(client), main_wnd_(main_wnd) {
    client_->RegisterObserver(this);
    main_wnd->RegisterObserver(this);
}

Conductor::~Conductor() {
    RTC_DCHECK(!peer_connection_);
}

bool Conductor::connection_active() const {
    return peer_connection_ != nullptr;
}

void Conductor::Close() {
    client_->SignOut();
    DeletePeerConnection();
}

bool Conductor::InitializePeerConnection() {
    RTC_DCHECK(!peer_connection_factory_);
    RTC_DCHECK(!peer_connection_);

    if (!signaling_thread_.get()) {
        signaling_thread_ = rtc::Thread::CreateWithSocketServer();
        signaling_thread_->Start();
    }
    peer_connection_factory_ = webrtc::CreatePeerConnectionFactory(
        nullptr /* network_thread */, nullptr /* worker_thread */,
        signaling_thread_.get(), nullptr /* default_adm */,
        webrtc::CreateBuiltinAudioEncoderFactory(),
        webrtc::CreateBuiltinAudioDecoderFactory(),
        webrtc::CreateBuiltinVideoEncoderFactory(),
        webrtc::CreateBuiltinVideoDecoderFactory(), nullptr /* audio_mixer */,
        nullptr /* audio_processing */);

    if (!peer_connection_factory_) {
        //main_wnd_->MessageBox("Error", "Failed to initialize PeerConnectionFactory",true);
        printf("Failed to initialize PeerConnectionFactory\n");
        DeletePeerConnection();
        return false;
    }

    if (!CreatePeerConnection()) {
        printf("Failed CreatePeerConnection\n");
        //main_wnd_->MessageBox("Error", "CreatePeerConnection failed", true);
        DeletePeerConnection();
    }

    AddTracks();

    return peer_connection_ != nullptr;
}

bool Conductor::ReinitializePeerConnectionForLoopback() {
    loopback_ = true;
    std::vector<rtc::scoped_refptr<webrtc::RtpSenderInterface>> senders =
        peer_connection_->GetSenders();
    peer_connection_ = nullptr;
    // Loopback is only possible if encryption is disabled.
    webrtc::PeerConnectionFactoryInterface::Options options;
    options.disable_encryption = true;
    peer_connection_factory_->SetOptions(options);
    if (CreatePeerConnection()) {
        for (const auto& sender : senders) {
        peer_connection_->AddTrack(sender->track(), sender->stream_ids());
        }
        peer_connection_->CreateOffer(
            this, webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
    }
    options.disable_encryption = false;
    peer_connection_factory_->SetOptions(options);
    return peer_connection_ != nullptr;
}

bool Conductor::CreatePeerConnection() {
    RTC_DCHECK(peer_connection_factory_);
    RTC_DCHECK(!peer_connection_);

    webrtc::PeerConnectionInterface::RTCConfiguration config;
    config.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;
    webrtc::PeerConnectionInterface::IceServer server;
    server.uri = GetPeerConnectionString();
    config.servers.push_back(server);

    peer_connection_ = peer_connection_factory_->CreatePeerConnection(
        config, nullptr, nullptr, this);
    return peer_connection_ != nullptr;
}

void Conductor::DeletePeerConnection() {
    main_wnd_->StopLocalRenderer();
    main_wnd_->StopRemoteRenderer();
    peer_connection_ = nullptr;
    peer_connection_factory_ = nullptr;
    peer_id_ = -1;
    loopback_ = false;
}

void Conductor::EnsureStreamingUI() {
  // RTC_DCHECK(peer_connection_);
  // if (main_wnd_->IsWindow()) {
  //   if (main_wnd_->current_ui() != MainWindow::STREAMING)
  //     main_wnd_->SwitchToStreamingUI();
  // }
}

//
// PeerConnectionObserver implementation.
//

void Conductor::OnAddTrack(
    rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
    const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>&
        streams) 
{
  // RTC_LOG(LS_INFO) << __FUNCTION__ << " " << receiver->id();
  // main_wnd_->QueueUIThreadCallback(NEW_TRACK_ADDED,
  //                                  receiver->track().release());
}

void Conductor::OnRemoveTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) 
{
    RTC_LOG(LS_INFO) << __FUNCTION__ << " " << receiver->id();
    main_wnd_->QueueUIThreadCallback(TRACK_REMOVED, receiver->track().release());
}

void Conductor::OnIceCandidate(const webrtc::IceCandidateInterface* candidate) {
    //RTC_LOG(LS_INFO) << __FUNCTION__ << " " << candidate->sdp_mline_index();
    //printf("OnIceCandidate = %d\n", candidate->sdp_mline_index()); 
    // For loopback test. To save some connecting delay.
    if (loopback_) {
        if (!peer_connection_->AddIceCandidate(candidate)) {
        //RTC_LOG(LS_WARNING) << "Failed to apply the received candidate";
        printf("Failed to apply the received candidate\n");
        }
        return;
    }

    Json::StyledWriter writer;
    Json::Value jmessage;

    jmessage[kCandidateSdpMidName] = candidate->sdp_mid();
    jmessage[kCandidateSdpMlineIndexName] = candidate->sdp_mline_index();
    std::string sdp;
    if (!candidate->ToString(&sdp)) {
        RTC_LOG(LS_ERROR) << "Failed to serialize candidate";
        return;
    }
    jmessage[kCandidateSdpName] = sdp;
    SendMessage(writer.write(jmessage));
}

//
// PeerConnectionClientObserver implementation.
//

std::string Conductor::GetPeerName()
{
    char computer_name[256];
    std::string ret(GetEnvVarOrDefault("USERNAME", "user"));
    ret += '@';
    if (gethostname(computer_name, arraysize(computer_name)) == 0) {
        ret += computer_name;
    } else {
        ret += "host";
    }
    return ret;
}

std::string Conductor::GetPeerConnectionString()
{
    return GetEnvVarOrDefault("WEBRTC_CONNECT", "stun:stun.l.google.com:19302");
}

std::string Conductor::GetEnvVarOrDefault(const char *env_var_name, const char *default_value)
{
    std::string value;
    const char* env_var = getenv(env_var_name);
    if (env_var)
        value = env_var;

    if (value.empty())
        value = default_value;

    return value;
}

void Conductor::OnSignedIn()
{
    // RTC_LOG(LS_INFO) << __FUNCTION__;
    // main_wnd_->SwitchToPeerList(client_->peers());
}

void Conductor::OnDisconnected() {
    RTC_LOG(LS_INFO) << __FUNCTION__;

    DeletePeerConnection();

    // if (main_wnd_->IsWindow())
    //   main_wnd_->SwitchToConnectUI();
}

void Conductor::OnPeerConnected(int id, const std::string& name) {
    // RTC_LOG(LS_INFO) << __FUNCTION__;
    // // Refresh the list if we're showing it.
    // if (main_wnd_->current_ui() == MainWindow::LIST_PEERS)
    //   main_wnd_->SwitchToPeerList(client_->peers());
}

void Conductor::OnPeerDisconnected(int id) {
    RTC_LOG(LS_INFO) << __FUNCTION__;
    if (id == peer_id_) {
        RTC_LOG(LS_INFO) << "Our peer disconnected";
        esc = true;
        printf("有人断开\n");
        //DeletePeerConnection();
        main_wnd_->QueueUIThreadCallback(PEER_CONNECTION_CLOSED, NULL);
    } 
    // else {
    //   // Refresh the list if we're showing it.
    //   if (main_wnd_->current_ui() == MainWindow::LIST_PEERS)
    //     main_wnd_->SwitchToPeerList(client_->peers());
    // }
}

//接收到服务器转发的信息
void Conductor::OnMessageFromPeer(int peer_id, const std::string& message) {
    RTC_DCHECK(peer_id_ == peer_id || peer_id_ == -1);
    RTC_DCHECK(!message.empty());

    //printf("peer_id=%d\n",peer_id_);
    //被动端第一次
    if (!peer_connection_.get()) {
        RTC_DCHECK(peer_id_ == -1);
        peer_id_ = peer_id;

        if (!InitializePeerConnection()) {
        //RTC_LOG(LS_ERROR) << "Failed to initialize our PeerConnection instance";
        printf("Failed to initialize\n");
        client_->SignOut();
        return;
        }
    } else if (peer_id != peer_id_) {
        //RTC_DCHECK(peer_id_ != -1);
        //RTC_LOG(LS_WARNING) << "Received a message from unknown peer while already in a ""conversation with a different peer.";
        printf("通讯id不一样\n");
        return;
    }

    Json::Reader reader;
    Json::Value jmessage;
    if (!reader.parse(message, jmessage)) {
        //RTC_LOG(LS_WARNING) << "Received unknown message. " << message;
        printf("接收到的信息不对\n");
        return;
    }
    std::string type_str;
    std::string json_object;

    rtc::GetStringFromJsonObject(jmessage, kSessionDescriptionTypeName,
                                &type_str);

    //接收到的信息中有 type 字段
    if (!type_str.empty()) {
        if (type_str == "offer-loopback") {
        // This is a loopback call.
        // Recreate the peerconnection with DTLS disabled.
        if (!ReinitializePeerConnectionForLoopback()) {
            RTC_LOG(LS_ERROR) << "Failed to initialize our PeerConnection instance";
            DeletePeerConnection();
            client_->SignOut();
        }
        return;
        }
        absl::optional<webrtc::SdpType> type_maybe =webrtc::SdpTypeFromString(type_str);
        if (!type_maybe) {
        //RTC_LOG(LS_ERROR) << "Unknown SDP type: " << type_str;

        return;
        }
        webrtc::SdpType type = *type_maybe;
        std::string sdp;
        if (!rtc::GetStringFromJsonObject(jmessage, kSessionDescriptionSdpName,
                                        &sdp)) {
        //RTC_LOG(LS_WARNING) << "Can't parse received session description message.";
        printf("获取sdp键值信息失败\n");
        return;
        }
        webrtc::SdpParseError error;
        std::unique_ptr<webrtc::SessionDescriptionInterface> session_description =
            webrtc::CreateSessionDescription(type, sdp, &error);
        if (!session_description) {
        // RTC_LOG(LS_WARNING)
        //     << "Can't parse received session description message. "
        //        "SdpParseError was: "
        //     << error.description;
        printf("无法解析获取到的描述符\n");
        return;
        }
        RTC_LOG(LS_INFO) << " Received session description :" << message;
        //std::cout << "The message is: " << message << std::endl;
        peer_connection_->SetRemoteDescription(DummySetSessionDescriptionObserver::Create(),session_description.release());

        //判断是否为offer
        if (type == webrtc::SdpType::kOffer) {
        peer_connection_->CreateAnswer(this, webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());//是的话发送answer
        }
    } else {
        std::string sdp_mid;
        int sdp_mlineindex = 0;
        std::string sdp;
        if (!rtc::GetStringFromJsonObject(jmessage, kCandidateSdpMidName,
                                        &sdp_mid) ||
            !rtc::GetIntFromJsonObject(jmessage, kCandidateSdpMlineIndexName,
                                    &sdp_mlineindex) ||
            !rtc::GetStringFromJsonObject(jmessage, kCandidateSdpName, &sdp)) {
        //RTC_LOG(LS_WARNING) << "Can't parse received message.";
        printf("获取到的键值信息不对\n");
        return;
        }
        webrtc::SdpParseError error;
        std::unique_ptr<webrtc::IceCandidateInterface> candidate(
            webrtc::CreateIceCandidate(sdp_mid, sdp_mlineindex, sdp, &error));
        if (!candidate.get()) {
        // RTC_LOG(LS_WARNING) << "Can't parse received candidate message. "
        //                        "SdpParseError was: "
        //                     << error.description;
        printf("解析候选者失败\n");
        return;
        }
        if (!peer_connection_->AddIceCandidate(candidate.get())) {
        //RTC_LOG(LS_WARNING) << "Failed to apply the received candidate";
        printf("添加候选者信息失败\n");
        return;
        }
        //std::cout << "Received candidate :" << message << std::endl;
        RTC_LOG(LS_INFO) << " Received candidate :" << message;
    }
}

void Conductor::OnMessageSent(int err) {
    // Process the next pending message if any.
    main_wnd_->QueueUIThreadCallback(SEND_MESSAGE_TO_PEER, NULL);
}

void Conductor::OnServerConnectionFailure() {
    printf("连接服务器失败\n");
    //main_wnd_->MessageBox("Error", ("Failed to connect to " + server_).c_str(),true);
}

//
// MainWndCallback implementation.
//

void Conductor::StartLogin(const std::string& server, int port) {
    if (client_->is_connected())
        return;
    server_ = server;
    port_ = port;
    //printf("server=%s, port=%d\n", server.c_str(), port);
    client_->Connect(server, port, GetPeerName());
}

void Conductor::DisconnectFromServer() {

    //该步骤只有在退出连接界面时才会执行，在发送信息时因为在没有连接成功时会执行置空操作，所以 is_connected 判断为false
    if (client_->is_connected())
    {
        client_->SignOut();
    }
    else
    {
        printf("重新连接\n");
        client_->Connect(server_, port_, GetPeerName());
    }
}

void Conductor::ConnectToPeer(int peer_id) {
    RTC_DCHECK(peer_id_ == -1);
    RTC_DCHECK(peer_id != -1);

    if (peer_connection_.get()) {
        printf("We only support connecting to one peer at a time\n");
        //main_wnd_->MessageBox("Error", "We only support connecting to one peer at a time", true);
        return;
    }

    if (InitializePeerConnection()) {
        peer_id_ = peer_id;
        peer_connection_->CreateOffer(
            this, webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
    } else {
        printf("Failed to initialize PeerConnection\n");
        //main_wnd_->MessageBox("Error", "Failed to initialize PeerConnection", true);
    }
}

void Conductor::AddTracks() {
    if (!peer_connection_->GetSenders().empty()) {
        return;  // Already added tracks.
    }

    rtc::scoped_refptr<webrtc::AudioTrackInterface> audio_track(
        peer_connection_factory_->CreateAudioTrack(
            kAudioLabel, peer_connection_factory_->CreateAudioSource(
                            cricket::AudioOptions())));
    auto result_or_error = peer_connection_->AddTrack(audio_track, {kStreamId});
    if (!result_or_error.ok()) {
        RTC_LOG(LS_ERROR) << "Failed to add audio track to PeerConnection: "
                        << result_or_error.error().message();
    }

#if 0
    rtc::scoped_refptr<CapturerTrackSource> video_device =
        CapturerTrackSource::Create();
    if (video_device) {
        rtc::scoped_refptr<webrtc::VideoTrackInterface> video_track_(
            peer_connection_factory_->CreateVideoTrack(kVideoLabel, video_device));
        main_wnd_->StartLocalRenderer(video_track_);

        result_or_error = peer_connection_->AddTrack(video_track_, {kStreamId});
        if (!result_or_error.ok()) {
        RTC_LOG(LS_ERROR) << "Failed to add video track to PeerConnection: "
                            << result_or_error.error().message();
        }
    } else {
        RTC_LOG(LS_ERROR) << "OpenVideoCaptureDevice failed";
    }
#else
    rtc::scoped_refptr<MyCapturer> video_device = new rtc::RefCountedObject<MyCapturer>();

    if (video_device) {
        video_device->startCapturer();
        rtc::scoped_refptr<webrtc::VideoTrackInterface> video_track_(peer_connection_factory_->CreateVideoTrack(kVideoLabel, video_device));
        main_wnd_->StartLocalRenderer(video_track_);
        auto result_or_error = peer_connection_->AddTrack(video_track_, {kStreamId});
        if (!result_or_error.ok()) 
        {
            printf("Failed to add video track to PeerConnection");
        }
    } else {
        printf("OpenVideoCaptureDevice failed\n");
    }
#endif
  //main_wnd_->SwitchToStreamingUI();
}

void Conductor::DisconnectFromCurrentPeer() {
    RTC_LOG(LS_INFO) << __FUNCTION__;
    if (peer_connection_.get()) {
        client_->SendHangUp(peer_id_);
        DeletePeerConnection();
    }

    // if (main_wnd_->IsWindow())
    //     main_wnd_->SwitchToPeerList(client_->peers());
}

void Conductor::UIThreadCallback(int msg_id, void* data) {
    switch (msg_id) {
        case PEER_CONNECTION_CLOSED:
        RTC_LOG(LS_INFO) << "PEER_CONNECTION_CLOSED";
        DeletePeerConnection();

        if (main_wnd_->IsWindow()) {
            if (client_->is_connected()) {
            //main_wnd_->SwitchToPeerList(client_->peers());
            } else {
            //printf("my_id = %d\n", client_->is_connected());
            //main_wnd_->SwitchToConnectUI();
            }
        } else {
            printf("开始退出\n");
            DisconnectFromServer();
        }
        break;

        case SEND_MESSAGE_TO_PEER: {
        printf("SEND_MESSAGE_TO_PEER\n");
        RTC_LOG(LS_INFO) << "SEND_MESSAGE_TO_PEER";
        std::string* msg = reinterpret_cast<std::string*>(data);
        if (msg) {
            // For convenience, we always run the message through the queue.
            // This way we can be sure that messages are sent to the server
            // in the same order they were signaled without much hassle.
            pending_messages_.push_back(msg);
        }

        //判断队列中是否有数据以及是否在发送数据
        if (!pending_messages_.empty() && !client_->IsSendingMessage()) {
            msg = pending_messages_.front();//取出队列中的数据
            pending_messages_.pop_front();//将取出的数据移除
            printf("获取队列数据\n");
            if (!client_->SendToPeer(peer_id_, *msg) && peer_id_ != -1) {
            RTC_LOG(LS_ERROR) << "SendToPeer failed";
            printf("SendToPeer failed\n");
            DisconnectFromServer();
            
            }
            delete msg;
        }

        if (!peer_connection_.get())
            peer_id_ = -1;

        break;
        }
        //产生新轨道
        case NEW_TRACK_ADDED: {
        auto* track = reinterpret_cast<webrtc::MediaStreamTrackInterface*>(data);
        if (track->kind() == webrtc::MediaStreamTrackInterface::kVideoKind) {
            auto* video_track = static_cast<webrtc::VideoTrackInterface*>(track);
            main_wnd_->StartRemoteRenderer(video_track);//生成远端的渲染器
        }
        track->Release();
        break;
        }

        case TRACK_REMOVED: {
        // Remote peer stopped sending a track.
        auto* track = reinterpret_cast<webrtc::MediaStreamTrackInterface*>(data);
        track->Release();
        break;
        }

        default:
        RTC_DCHECK_NOTREACHED();
        break;
    }
}

void Conductor::OnSuccess(webrtc::SessionDescriptionInterface* desc) {
    peer_connection_->SetLocalDescription(DummySetSessionDescriptionObserver::Create(), desc);

    //printf("Connected CreateOffer successfully\n");
    
    std::string sdp;
    desc->ToString(&sdp);

    // 只有在tyep中的键值为offer-loopback才执行
    // For loopback test. To save some connecting delay.
    if (loopback_) {
        // Replace message type from "offer" to "answer"
        std::unique_ptr<webrtc::SessionDescriptionInterface> session_description =
            webrtc::CreateSessionDescription(webrtc::SdpType::kAnswer, sdp);
        peer_connection_->SetRemoteDescription(
            DummySetSessionDescriptionObserver::Create(),
            session_description.release());
        return;
    }

    Json::StyledWriter writer;
    Json::Value jmessage;
    jmessage[kSessionDescriptionTypeName] =
        webrtc::SdpTypeToString(desc->GetType());
    jmessage[kSessionDescriptionSdpName] = sdp;
    SendMessage(writer.write(jmessage));
}

void Conductor::OnFailure(webrtc::RTCError error) {
    RTC_LOG(LS_ERROR) << ToString(error.type()) << ": " << error.message();
}

void Conductor::SendMessage(const std::string& json_object) {
    std::string* msg = new std::string(json_object);
    main_wnd_->QueueUIThreadCallback(SEND_MESSAGE_TO_PEER, msg);
}






MyCapturer::MyCapturer() {

}



void MyCapturer::startThread()
{
    //printf("创建线程成功\n");
    const char * filename = "./data/test.yuv";
    FILE* fp = fopen(filename, "rb");
    if(!fp){
        printf("Failed to open: %s\n", filename);
        return;
    }

    // 读取文件大小
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    int frame_width = 1080;
    int frame_height = 606;
    int frame_buffer = frame_width * frame_height* 3/2;
    int frame_count = file_size/frame_buffer; //轮次
    int last_frame_size = file_size % frame_count;//最后一帧剩余大小
    //printf("frame_count =%d,last=%d\n",frame_count,last_frame_size);

    //unsigned char *frame = (unsigned char *)malloc(frame_buffer);//申请对应大小的内存
    if (!i420_buffer_.get() ||i420_buffer_->width() * i420_buffer_->height() < frame_width, frame_height) 
    {
        i420_buffer_ = webrtc::I420Buffer::Create(frame_width, frame_height);
    }

    uint8_t* dst_y = i420_buffer_->MutableDataY();
    int dst_stride_y = i420_buffer_->StrideY();
    uint8_t* dst_u = i420_buffer_->MutableDataU();
    int dst_stride_u = i420_buffer_->StrideU();
    uint8_t* dst_v = i420_buffer_->MutableDataV();
    int dst_stride_v = i420_buffer_->StrideV();

    unsigned char *frame = (unsigned char *)malloc(frame_buffer);//申请对应大小的内存
    for (int  i = 0; i < frame_count; i++)
    {
        if (esc ==true)
        {
        esc = false;
        return ;
        }
        
        int size=fread(frame, frame_buffer, 1, fp);
        // 假设I420Copy函数已经正确实现，这里直接调用
        // 注意：src_stride_y, src_stride_u, src_stride_v应该是frame_width和frame_width/2
        libyuv::I420Copy(frame, frame_width,
                frame + frame_width * frame_height, frame_width / 2,
                frame + frame_width * frame_height * 5 / 4, frame_width / 2,
                dst_y, dst_stride_y,
                dst_u, dst_stride_u,
                dst_v, dst_stride_v,
                frame_width, frame_height);
        OnFrame(webrtc::VideoFrame(i420_buffer_, 0, 0, webrtc::kVideoRotation_0));
        usleep(40 * 1000);
    }
    free(frame);
    if (last_frame_size > 0) {
        unsigned char *last_frame = (unsigned char *)malloc(last_frame_size);

        fread(last_frame, 1, last_frame_size, fp);
        
        libyuv::I420Copy(last_frame, frame_width,
                last_frame + frame_width * frame_height, frame_width / 2,
                last_frame + frame_width * frame_height * 5 / 4, frame_width / 2,
                dst_y, dst_stride_y,
                dst_u, dst_stride_u,
                dst_v, dst_stride_v,
                frame_width, frame_height);
        OnFrame(webrtc::VideoFrame(i420_buffer_, 0, 0, webrtc::kVideoRotation_0));
        free(last_frame);
    }
 
}

void MyCapturer::startCapturer() {
#if 1
    std::thread start_Thread(&MyCapturer::startThread, this);
    start_Thread.detach();
#else 
    auto options = webrtc::DesktopCaptureOptions::CreateDefault();
    capturer_ = webrtc::DesktopCapturer::CreateScreenCapturer(options);

    capturer_->Start(this);
    CaptureFrame();
#endif
}

webrtc::MediaSourceInterface::SourceState MyCapturer::state() const {
    return webrtc::MediaSourceInterface::kLive;
}

bool MyCapturer::remote() const {
    return false;
}

bool MyCapturer::is_screencast() const {
    return true;
}

absl::optional<bool> MyCapturer::needs_denoising() const {
    return false;
}


//捕获一帧视频后调用
void MyCapturer::OnCaptureResult(webrtc::DesktopCapturer::Result result,
                                 std::unique_ptr<webrtc::DesktopFrame> frame) {
    if (result != webrtc::DesktopCapturer::Result::SUCCESS)
        return;

    int width = frame->size().width();
    int height = frame->size().height();

    if (!i420_buffer_.get() ||
        i420_buffer_->width() * i420_buffer_->height() < width * height) {
        i420_buffer_ = webrtc::I420Buffer::Create(width, height);
    }
    
    libyuv::ConvertToI420(frame->data(), 0, i420_buffer_->MutableDataY(),
                            i420_buffer_->StrideY(), i420_buffer_->MutableDataU(),
                            i420_buffer_->StrideU(), i420_buffer_->MutableDataV(),
                            i420_buffer_->StrideV(), 0, 0, width, height, width,
                            height, libyuv::kRotate0, libyuv::FOURCC_ARGB);
    OnFrame(webrtc::VideoFrame(i420_buffer_, 0, 0, webrtc::kVideoRotation_0));
}

void MyCapturer::OnMessage(rtc::Message* msg) {
    if (msg->message_id == 0)
    {
        CaptureFrame();
    }
}

void MyCapturer::CaptureFrame() {
    capturer_->CaptureFrame();
    
    //fps=30
    rtc::Thread::Current()->PostDelayed(RTC_FROM_HERE, 30, this, 0);
}