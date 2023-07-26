#include "client.h"

using namespace Streamer;

ClientTrackData::ClientTrackData(std::shared_ptr<rtc::Track> track, std::shared_ptr<rtc::RtcpSrReporter> sender)
    : track{track}, sender{sender}
{
}

ClientTrack::ClientTrack(std::string id, std::shared_ptr<ClientTrackData> track_data): id{id}, track_data{track_data} {}

Client::Client(std::shared_ptr<rtc::PeerConnection> pc)
{
    m_pc = pc;
}

void Client::set_state(State state)
{
    std::unique_lock lock(m_mtx);
    m_state = state;
}

void Client::set_video(std::string const msid, std::function<void(void)> const on_open, uint8_t const payload_type,
                       uint32_t const ssrc, std::string const cname)
{
    video = add_video(m_pc, payload_type, ssrc, cname, msid, on_open);
}

void Client::set_audio(std::string const msid, std::function<void(void)> const on_open, uint8_t const payload_type,
                       uint32_t const ssrc, std::string const cname)
{
    audio = add_audio(m_pc, payload_type, ssrc, cname, msid, on_open);
}

Client::State Client::get_state()
{
    std::shared_lock lock(m_mtx);
    return m_state;
}

std::shared_ptr<rtc::PeerConnection> const& Client::get_peer_connection() const
{
    return m_pc;
}

std::shared_ptr<ClientTrackData> Client::add_video(std::shared_ptr<rtc::PeerConnection> const& pc,
                                                   uint8_t const payload_type, uint32_t const ssrc,
                                                   std::string const cname, std::string const msid,
                                                   std::function<void(void)> const on_open)
{
    auto video = rtc::Description::Video(cname);
    video.addH264Codec(payload_type);
    video.addSSRC(ssrc, cname, msid, cname);
    auto track = pc->addTrack(video);
    // create RTP configuration
    auto rtp_config = std::make_shared<rtc::RtpPacketizationConfig>(ssrc, cname, payload_type,
                                                                    rtc::H264RtpPacketizer::defaultClockRate);
    // create packetizer
    auto packetizer = std::make_shared<rtc::H264RtpPacketizer>(rtc::H264RtpPacketizer::Separator::Length, rtp_config);
    // create H264 handler
    auto h264_handler = std::make_shared<rtc::H264PacketizationHandler>(packetizer);
    // add RTCP SR handler
    auto sr_handler = std::make_shared<rtc::RtcpSrReporter>(rtp_config);
    h264_handler->addToChain(sr_handler);
    // add RTCP NACK handler
    auto nack_handler = std::make_shared<rtc::RtcpNackResponder>();
    h264_handler->addToChain(nack_handler);
    // set handler
    track->setMediaHandler(h264_handler);
    track->onOpen(on_open);
    auto track_data = std::make_shared<ClientTrackData>(track, sr_handler);
    return track_data;
}

std::shared_ptr<ClientTrackData> Client::add_audio(std::shared_ptr<rtc::PeerConnection> const& pc,
                                                   uint8_t const payload_type, uint32_t const ssrc,
                                                   std::string const cname, std::string const msid,
                                                   std::function<void(void)> const on_open)
{
    auto audio = rtc::Description::Audio(cname);
    audio.addOpusCodec(payload_type);
    audio.addSSRC(ssrc, cname, msid, cname);
    auto track = pc->addTrack(audio);
    // create RTP configuration
    auto rtp_config = std::make_shared<rtc::RtpPacketizationConfig>(ssrc, cname, payload_type,
                                                                    rtc::OpusRtpPacketizer::defaultClockRate);
    // create packetizer
    auto packetizer = std::make_shared<rtc::OpusRtpPacketizer>(rtp_config);
    // create opus handler
    auto opus_handler = std::make_shared<rtc::OpusPacketizationHandler>(packetizer);
    // add RTCP SR handler
    auto sr_handler = std::make_shared<rtc::RtcpSrReporter>(rtp_config);
    opus_handler->addToChain(sr_handler);
    // add RTCP NACK handler
    auto nack_handler = std::make_shared<rtc::RtcpNackResponder>();
    opus_handler->addToChain(nack_handler);
    // set handler
    track->setMediaHandler(opus_handler);
    track->onOpen(on_open);
    auto track_data = std::make_shared<ClientTrackData>(track, sr_handler);
    return track_data;
}
