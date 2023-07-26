#include "client_manager.h"
#include "h264_file_parser.h"
#include "opus_file_parser.h"

#include <nlohmann/json.hpp>

#include <iostream>
#include <variant>

using namespace Streamer;

ClientManager::ClientManager(rtc::Configuration const& config, std::string h264_samples_directory,
                             std::string opus_samples_directory)
    : m_config(config), m_h264_samples_directory{std::move(h264_samples_directory)}, m_opus_samples_directory{std::move(
                                                                                         opus_samples_directory)}
{
}

std::shared_ptr<Client> ClientManager::create_client(rtc::Configuration const& config,
                                                     std::weak_ptr<rtc::WebSocket> wws, std::string id)
{
    auto pc = std::make_shared<rtc::PeerConnection>(config);
    auto client = std::make_shared<Client>(pc);

    pc->onStateChange([id, this](rtc::PeerConnection::State state) {
        std::cout << "state: " << state << std::endl;
        if (state == rtc::PeerConnection::State::Disconnected || state == rtc::PeerConnection::State::Failed ||
            state == rtc::PeerConnection::State::Closed)
            // remove disconnected client
            m_main_thread.dispatch([id, this]() { m_clients.erase(id); });
    });

    pc->onGatheringStateChange([wpc = std::weak_ptr(pc), id, wws](rtc::PeerConnection::GatheringState state) {
        std::cout << "gathering state: " << state << std::endl;
        if (state == rtc::PeerConnection::GatheringState::Complete)
            if (auto pc = wpc.lock())
            {
                auto description = pc->localDescription();
                nlohmann::json msg = {
                    {"id", id}, {"type", description->typeString()}, {"sdp", std::string(description.value())}};
                // gathering complete, send answer
                if (auto ws = wws.lock()) ws->send(msg.dump());
            }
    });

    client->set_video("stream1", [id, wc = std::weak_ptr(client), this] {
        m_main_thread.dispatch([wc, this] {
            if (auto c = wc.lock()) add_to_stream(c, true);
        });
        std::cout << "video from " << id << " opened" << std::endl;
    });

    client->set_audio("stream1", [id, wc = std::weak_ptr(client), this] {
        m_main_thread.dispatch([wc, this] {
            if (auto c = wc.lock()) add_to_stream(c, false);
        });
        std::cout << "audio from " << id << " opened" << std::endl;
    });

    auto data_channel = pc->createDataChannel("ping-pong");
    data_channel->onOpen([id, wdc = std::weak_ptr(data_channel)] {
        if (auto dc = wdc.lock()) dc->send("Ping");
    });
    data_channel->onMessage(nullptr, [id, wdc = std::weak_ptr(data_channel)] (std::string msg) {
        std::cout << "message from " << id << " received: " << msg << std::endl;
        if (auto dc = wdc.lock()) dc->send("Ping");
    });
    client->data_channel = data_channel;

    pc->setLocalDescription();
    return client;
}

std::shared_ptr<rtc::WebSocket> ClientManager::create_ws()
{
    auto ws = std::make_shared<rtc::WebSocket>();
    ws->onOpen([] { std::cout << "websocket connected, signaling ready" << std::endl; });
    ws->onClosed([] { std::cout << "websocket closed" << std::endl; });
    ws->onError([](std::string const& err) { std::cout << "websocket failed: " << err << std::endl; });
    ws->onMessage([ws, this](std::variant<rtc::binary, std::string> data) {
        if (!std::holds_alternative<std::string>(data)) return;
        auto msg = nlohmann::json::parse(std::get<std::string>(data));
        m_main_thread.dispatch([msg, ws, this] {
            auto it = msg.find("id");
            if (it == msg.end()) return;
            std::string id = it->get<std::string>();
            it = msg.find("type");
            if (it == msg.end()) return;
            std::string type = it->get<std::string>();
            if (type == "request")
                m_clients.emplace(id, create_client(m_config, std::weak_ptr(ws), id));
            else if (type == "answer")
            {
                if (auto jt = m_clients.find(id); jt != m_clients.end())
                {
                    auto pc = jt->second->get_peer_connection();
                    auto sdp = msg["sdp"].get<std::string>();
                    auto des = rtc::Description(sdp, type);
                    pc->setRemoteDescription(des);
                }
            }
        });
    });
    return ws;
}

std::shared_ptr<Stream> ClientManager::create_stream(std::string h264_samples, unsigned fps, std::string opus_samples)
{
    auto video = std::make_shared<H264FileParser>(h264_samples, fps, true);
    auto audio = std::make_shared<OPUSFileParser>(opus_samples, true);
    auto stream = std::make_shared<Stream>(video, audio);

    stream->on_sample([ws = std::weak_ptr(stream), this](Stream::StreamSourceType type, uint64_t sample_time,
                                                         rtc::binary sample) {
        std::vector<ClientTrack> tracks{};
        std::string stream_type = (type == Stream::StreamSourceType::Video ? "video" : "audio");
        std::function<std::optional<std::shared_ptr<ClientTrackData>>(std::shared_ptr<Client>)> get_track_data =
            [type](std::shared_ptr<Client> client) {
                return type == Stream::StreamSourceType::Video ? client->video : client->audio;
            };
        // get all clients with ready state
        for (auto const& [id, client] : m_clients)
        {
            auto track_data = get_track_data(client);
            if (client->get_state() == Client::State::Ready && track_data.has_value())
                tracks.push_back(ClientTrack{id, track_data.value()});
        }
        if (!tracks.empty())
            for (auto const& [id, track_data] : tracks)
            {
                auto rtp_config = track_data->sender->rtpConfig;
                rtp_config->timestamp = rtp_config->startTimestamp + rtp_config->secondsToTimestamp(sample_time / 1e6);
                // get elapsed time in clock rate from last RTCP sender report
                // check if last report was at least 1 second ago
                if (auto report_elapsed_timestamp = rtp_config->timestamp - track_data->sender->lastReportedTimestamp();
                    rtp_config->timestampToSeconds(report_elapsed_timestamp) > 1)
                    track_data->sender->setNeedsToReport();
                // send sample
                try
                {
                    track_data->track->send(sample);
                }
                catch (std::exception const& e)
                {
                    std::cerr << "Unable to send " << stream_type << " packet: " << e.what() << std::endl;
                }
            }

        m_main_thread.dispatch([ws, this] {
            if (m_clients.empty())
                if (auto stream = ws.lock(); stream) stream->stop();
        });
    });
    return stream;
}

void ClientManager::start_stream()
{
    if (m_stream.has_value())
    {
        if (m_stream.value()->is_running()) return;
    }
    else
        m_stream = create_stream(m_h264_samples_directory, 30, m_opus_samples_directory);
    m_stream.value()->start();
}

void ClientManager::send_init_nal_units(std::shared_ptr<Stream> stream, std::shared_ptr<ClientTrackData> video)
{
    auto h264 = dynamic_cast<H264FileParser*>(stream->get_video_source().get());
    auto initial_nal_units = h264->initial_nal_units();

    // send previous NALU key frame so users don't have to wait to see stream works
    if (!initial_nal_units.empty())
    {
        auto const frame_timestamp_duration =
            video->sender->rtpConfig->secondsToTimestamp(h264->get_sample_duration_us() / 1e6);
        video->sender->rtpConfig->timestamp = video->sender->rtpConfig->startTimestamp - frame_timestamp_duration * 2;
        video->track->send(initial_nal_units);
        video->sender->rtpConfig->timestamp += frame_timestamp_duration;
        // send initial NAL units again to start stream in firefox browser
        video->track->send(initial_nal_units);
    }
}

void ClientManager::add_to_stream(std::shared_ptr<Client> client, bool is_adding_video)
{
    if (client->get_state() == Client::State::Waiting)
        client->set_state(is_adding_video ? Client::State::WaitingForAudio : Client::State::WaitingForVideo);
    else if ((client->get_state() == Client::State::WaitingForAudio && !is_adding_video) ||
        (client->get_state() == Client::State::WaitingForVideo && is_adding_video))
    {
        assert(client->video.has_value() && client->audio.has_value());

        auto video = client->video.value();
        if (m_stream.has_value()) send_init_nal_units(m_stream.value(), video);
        client->set_state(Client::State::Ready);
    }
    if (client->get_state() == Client::State::Ready) start_stream();
}
