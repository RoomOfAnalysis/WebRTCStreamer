#pragma once

#include <rtc/rtc.hpp>

#include <memory>
#include <string>
#include <optional>
#include <shared_mutex>
#include <functional>

namespace Streamer
{
    struct ClientTrackData
    {
        ClientTrackData(std::shared_ptr<rtc::Track> track, std::shared_ptr<rtc::RtcpSrReporter> sender);

        std::shared_ptr<rtc::Track> track = nullptr;
        std::shared_ptr<rtc::RtcpSrReporter> sender = nullptr;
    };

    struct ClientTrack
    {
        ClientTrack(std::string id, std::shared_ptr<ClientTrackData> track_data);

        std::string id;
        std::shared_ptr<ClientTrackData> track_data;
    };

    class Client
    {
    public:
        enum class State
        {
            Waiting,
            WaitingForVideo,
            WaitingForAudio,
            Ready
        };

        explicit Client(std::shared_ptr<rtc::PeerConnection> pc);

        void set_state(State state);
        void set_video(std::string const msid, std::function<void(void)> const on_open, uint8_t const payload_type = 102,
                       uint32_t const ssrc = 1, std::string const cname = "video-stream");
        void set_audio(std::string const msid, std::function<void(void)> const on_open, uint8_t const payload_type = 111,
                       uint32_t const ssrc = 2, std::string const cname = "audio-stream");

        [[nodiscard]] State get_state();
        [[nodiscard]] std::shared_ptr<rtc::PeerConnection> const& get_peer_connection() const;

    private:
        [[nodiscard]] static std::shared_ptr<ClientTrackData> add_video(std::shared_ptr<rtc::PeerConnection> const& pc,
                                                                        uint8_t const payload_type, uint32_t const ssrc,
                                                                        std::string const cname, std::string const msid,
                                                                        std::function<void(void)> const on_open);

        [[nodiscard]] static std::shared_ptr<ClientTrackData> add_audio(std::shared_ptr<rtc::PeerConnection> const& pc,
                                                                        uint8_t const payload_type, uint32_t const ssrc,
                                                                        std::string const cname, std::string const msid,
                                                                        std::function<void(void)> const on_open);

    public:
        std::optional<std::shared_ptr<ClientTrackData>> video;
        std::optional<std::shared_ptr<ClientTrackData>> audio;
        std::optional<std::shared_ptr<rtc::DataChannel>> data_channel;

    private:
        std::shared_mutex m_mtx;
        State m_state = State::Waiting;
        std::string m_id;
        std::shared_ptr<rtc::PeerConnection> m_pc;
    };
}
