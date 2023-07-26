#pragma once

#include "stream.h"

#include <string>
#include <cstdint>
#include <vector>

namespace Streamer
{
	class FileParser : public StreamSource
	{
    public:
        FileParser(std::string directory, std::string extension, uint32_t samples_per_second, bool loop);
        virtual ~FileParser();

        virtual void start() override;
        virtual void stop() override;
        virtual void load_next_sample() override;

        virtual [[nodiscard]] uint64_t get_sample_time_us() const override;
        virtual [[nodiscard]] uint64_t get_sample_duration_us() const override;
        virtual [[nodiscard]] rtc::binary get_sample() const override;

    protected:
        rtc::binary m_sample = {};

    private:
        std::string m_directory;
        std::string m_extension;
        uint64_t m_sample_duration_us;
        uint64_t m_sample_time_us = 0;
        int64_t m_counter = -1;
        bool m_loop;
        uint64_t m_loop_time_stamp_offset = 0;
	};
}