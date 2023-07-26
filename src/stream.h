#pragma once

#include <rtc/rtc.hpp>

#include "dispatch_queue.h"

#include <cstdint>
#include <memory>
#include <functional>
#include <mutex>
#include <utility>

namespace Streamer
{
	class StreamSource
	{
    public:
        virtual void start() = 0;
        virtual void stop() = 0;
        virtual void load_next_sample() = 0;
        virtual uint64_t get_sample_time_us() const = 0;
        virtual uint64_t get_sample_duration_us() const = 0;
        virtual rtc::binary get_sample() const = 0;
	};

    class Stream : std::enable_shared_from_this<Stream>
    {
    public:
        enum class StreamSourceType
        {
            Video,
            Audio
        };

    public:
        Stream(std::shared_ptr<StreamSource> video, std::shared_ptr<StreamSource> audio);
        ~Stream();

        void start();
        void stop();
        void on_sample(std::function<void(StreamSourceType, uint64_t, rtc::binary)> handler);
        [[nodiscard]] bool is_running() const;

        [[nodiscard]] std::shared_ptr<StreamSource> get_video_source() const;
        [[nodiscard]] std::shared_ptr<StreamSource> get_audio_source() const;

    private:
        [[nodiscard]] std::pair<std::shared_ptr<StreamSource>, StreamSourceType> unsafe_prepare_for_sample();
        void send_sample();

    private:
        std::shared_ptr<StreamSource> m_video;
        std::shared_ptr<StreamSource> m_audio;
        rtc::synchronized_callback<StreamSourceType, uint64_t, rtc::binary> m_sample_handler;

        uint64_t m_start_time = 0;
        std::mutex m_mtx;
        DispatchQueue m_dispatch_queue{"StreamQueue"};
        bool m_is_running = false;
    };
}
