#include "file_parser.h"

#include <limits>
#include <fstream>
#include <cstddef>
#include <filesystem>

using namespace Streamer;

FileParser::FileParser(std::string directory, std::string extension, uint32_t samples_per_second, bool loop)
    : m_directory(std::move(directory)), m_extension(std::move(extension)),
      m_sample_duration_us(1000'000 / samples_per_second), m_loop(loop)
{
}

FileParser ::~FileParser()
{
    stop();
}

void FileParser::start()
{
    m_sample_time_us = std::numeric_limits<uint64_t>::max() - m_sample_duration_us + 1;
    load_next_sample();
}

void FileParser::stop()
{
    m_sample = {};
    m_sample_time_us = 0;
    m_counter = -1;
}

void FileParser::load_next_sample()
{
    auto path =
        (std::filesystem::path(m_directory) / ("sample-" + std::to_string(++m_counter))).replace_extension(m_extension);
    std::ifstream source(path, std::ios_base::binary);
    if (!source)
    {
        if (m_loop && m_counter > 0)
        {
            m_loop_time_stamp_offset = m_sample_time_us;
            m_counter = -1;
            load_next_sample();
            return;
        }
        m_sample = {};
        return;
    }
    std::vector<uint8_t> file_contents((std::istreambuf_iterator<char>(source)), std::istreambuf_iterator<char>());
    m_sample = *reinterpret_cast<std::vector<std::byte>*>(&file_contents);
    m_sample_time_us += m_sample_duration_us;
}

uint64_t FileParser::get_sample_time_us() const
{
    return m_sample_time_us;
}

uint64_t FileParser::get_sample_duration_us() const
{
    return m_sample_duration_us;
}

rtc::binary FileParser::get_sample() const
{
    return m_sample;
}
