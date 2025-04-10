/*
 *  Copyright 2012 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef EXAMPLES_PEERCONNECTION_CLIENT_CONDUCTOR_H_
#define EXAMPLES_PEERCONNECTION_CLIENT_CONDUCTOR_H_


#include <deque>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "api/media_stream_interface.h"
#include "api/peer_connection_interface.h"
#include "P2P.h"
#include "peer_connection_client.h"
#include "rtc_base/thread.h"
#include "media/base/adapted_video_track_source.h"
#include "modules/desktop_capture/desktop_capturer.h"
#include "modules/desktop_capture/desktop_capture_options.h"
#include "api/video/i420_buffer.h"

const char kAudioLabel[] = "audio_label";
const char kVideoLabel[] = "video_label";
const char kStreamId[] = "stream_id";

namespace webrtc {
class VideoCaptureModule;
}  // namespace webrtc

namespace cricket {
class VideoRenderer;
}  // namespace cricket

class Conductor : public webrtc::PeerConnectionObserver,
                  public webrtc::CreateSessionDescriptionObserver,
                  public PeerConnectionClientObserver,
                  public MainWndCallback {
 public:
    enum CallbackID {
        MEDIA_CHANNELS_INITIALIZED = 1,
        PEER_CONNECTION_CLOSED,
        SEND_MESSAGE_TO_PEER,
        NEW_TRACK_ADDED,
        TRACK_REMOVED,
    };

    Conductor(PeerConnectionClient* client, MainWindow* main_wnd);

    bool connection_active() const;

    void Close() override;

 protected:
    ~Conductor();
    bool InitializePeerConnection();
    bool ReinitializePeerConnectionForLoopback();
    bool CreatePeerConnection();
    void DeletePeerConnection();
    void EnsureStreamingUI();
    void AddTracks();

    //
    // PeerConnectionObserver implementation.
    //

    void OnSignalingChange(
        webrtc::PeerConnectionInterface::SignalingState new_state) override {}
    void OnAddTrack(
        rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
        const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>&
            streams) override;
    void OnRemoveTrack(
        rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) override;
    void OnDataChannel(
        rtc::scoped_refptr<webrtc::DataChannelInterface> channel) override {}
    void OnRenegotiationNeeded() override {}
    void OnIceConnectionChange(
        webrtc::PeerConnectionInterface::IceConnectionState new_state) override {}
    void OnIceGatheringChange(
        webrtc::PeerConnectionInterface::IceGatheringState new_state) override {}
    void OnIceCandidate(const webrtc::IceCandidateInterface* candidate) override;
    void OnIceConnectionReceivingChange(bool receiving) override {}

  //
  // PeerConnectionClientObserver implementation.
  //

    std::string GetEnvVarOrDefault(const char* env_var_name,
                               const char* default_value);
    std::string GetPeerConnectionString();

    std::string GetPeerName();

    void OnSignedIn() override;

    void OnDisconnected() override;

    void OnPeerConnected(int id, const std::string& name) override;

    void OnPeerDisconnected(int id) override;

    void OnMessageFromPeer(int peer_id, const std::string& message) override;

    void OnMessageSent(int err) override;

    void OnServerConnectionFailure() override;

    //
    // MainWndCallback implementation.
    //

    void StartLogin(const std::string& server, int port) override;

    void DisconnectFromServer() override;

    void ConnectToPeer(int peer_id) override;

    void DisconnectFromCurrentPeer() override;

    void UIThreadCallback(int msg_id, void* data) override;

    // CreateSessionDescriptionObserver implementation.
    void OnSuccess(webrtc::SessionDescriptionInterface* desc) override;
    void OnFailure(webrtc::RTCError error) override;
    protected:
    // Send a message to the remote peer.
    void SendMessage(const std::string& json_object);

    int peer_id_;
    bool loopback_;
    std::unique_ptr<rtc::Thread> signaling_thread_;
    rtc::scoped_refptr<webrtc::PeerConnectionInterface> peer_connection_;
    rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface>
        peer_connection_factory_;
    PeerConnectionClient* client_;
    MainWindow* main_wnd_;
    std::deque<std::string*> pending_messages_;
    std::string server_;
    int port_;

};


class MyCapturer : public rtc::AdaptedVideoTrackSource,
                   public rtc::MessageHandler,
                   public webrtc::DesktopCapturer::Callback {
 public:
    MyCapturer();

    void startCapturer();

    void CaptureFrame();

    bool is_screencast() const override;

    absl::optional<bool> needs_denoising() const override;

    webrtc::MediaSourceInterface::SourceState state() const override;

    bool remote() const override;

    void OnCaptureResult(webrtc::DesktopCapturer::Result result,
                                std::unique_ptr<webrtc::DesktopFrame> frame) override;
    void OnMessage(rtc::Message* msg) override;
    //int getNextNalu(FILE* inpf, unsigned char* buf);
    void startThread();
private:
    std::unique_ptr<webrtc::DesktopCapturer> capturer_;
    rtc::scoped_refptr<webrtc::I420Buffer> i420_buffer_;

  //mutable volatile int ref_count_;
};

#endif  // EXAMPLES_PEERCONNECTION_CLIENT_CONDUCTOR_H_
