#include "stream.h"

#include <chrono>
#include <thread>

using namespace Streamer;

uint64_t current_time_in_us()
{
    return std::chrono::duration_cast<std::chrono::microseconds>(
               std::chrono::high_resolution_clock::now().time_since_epoch())
        .count();
}

Stream::Stream(std::shared_ptr<StreamSource> video, std::shared_ptr<StreamSource> audio)
    : std::enable_shared_from_this<Stream>(), m_video(video), m_audio(audio)
{
}

Stream::~Stream()
{
    stop();
}

void Stream::start()
{
    std::lock_guard<std::mutex> lock(m_mtx);
    if (m_is_running) return;
    m_is_running = true;
    m_start_time = current_time_in_us();
    m_video->start();
    m_audio->start();
    m_dispatch_queue.dispatch([this] { this->send_sample(); });
}

void Stream::stop()
{
    std::lock_guard<std::mutex> lock(m_mtx);
    if (!m_is_running) return;
    m_is_running = false;
    m_dispatch_queue.remove_pending();
    m_video->stop();
    m_audio->stop();
}

void Stream::on_sample(std::function<void(StreamSourceType, uint64_t, rtc::binary)> handler)
{
    m_sample_handler = handler;
}

bool Stream::is_running() const
{
    return m_is_running;
}

std::shared_ptr<StreamSource> Stream::get_video_source() const
{
    return m_video;
}

std::shared_ptr<StreamSource> Stream::get_audio_source() const
{
    return m_audio;
}

std::pair<std::shared_ptr<StreamSource>, Stream::StreamSourceType> Stream::unsafe_prepare_for_sample()
{
    std::shared_ptr<StreamSource> ss;
    StreamSourceType sst = StreamSourceType::Video;
    uint64_t next_time = 0;
    if (m_audio->get_sample_time_us() < m_video->get_sample_time_us())
    {
        ss = m_audio;
        sst = StreamSourceType::Audio;
    }
    else
        ss = m_video;
    next_time = ss->get_sample_time_us();
    if (auto wait_time = next_time - (current_time_in_us() - m_start_time); wait_time > 0)
    {
        m_mtx.unlock();
        std::this_thread::sleep_for(std::chrono::microseconds{wait_time});
        m_mtx.lock();
    }
    return std::make_pair(ss, sst);
}

void Stream::send_sample()
{
    std::lock_guard<std::mutex> lock(m_mtx);
    if (!m_is_running) return;
    auto [ss, sst] = unsafe_prepare_for_sample();
    m_sample_handler(sst, ss->get_sample_time_us(), ss->get_sample());
    ss->load_next_sample();
    m_dispatch_queue.dispatch([this] { this->send_sample(); });
}
