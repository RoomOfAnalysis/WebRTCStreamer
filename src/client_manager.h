#pragma once

#include "client.h"
#include "dispatch_queue.h"
#include "stream.h"

#include <rtc/rtc.hpp>

#include <unordered_map>
#include <optional>
#include <string>

namespace Streamer
{
    class ClientManager
    {
    public:
        explicit ClientManager(rtc::Configuration const& config, std::string h264_samples_directory,
                               std::string opus_samples_directory);
        ~ClientManager() = default;

        std::shared_ptr<Client> create_client(rtc::Configuration const& config, std::weak_ptr<rtc::WebSocket> wws, std::string id);
        std::shared_ptr<rtc::WebSocket> create_ws();

        ClientManager(ClientManager const&) = delete;
        ClientManager& operator=(ClientManager const&) = delete;
        ClientManager(ClientManager&&) = delete;
        ClientManager& operator=(ClientManager&&) = delete;

    private:
        [[nodiscard]] std::shared_ptr<Stream> create_stream(std::string h264_samples, unsigned fps, std::string opus_samples);
        void start_stream();
        void send_init_nal_units(std::shared_ptr<Stream> stream, std::shared_ptr<ClientTrackData> video);
        void add_to_stream(std::shared_ptr<Client> client, bool is_adding_video);

    private:
        rtc::Configuration m_config;
        std::unordered_map<std::string, std::shared_ptr<Client>> m_clients{};
        std::optional<std::shared_ptr<Stream>> m_stream = std::nullopt;
        std::string m_h264_samples_directory{};
        std::string m_opus_samples_directory{};
        DispatchQueue m_main_thread{"Main"};
    };
}
