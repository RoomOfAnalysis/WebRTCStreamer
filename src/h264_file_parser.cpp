#include "h264_file_parser.h"

#include <rtc/rtc.hpp>

#include <cassert>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

using namespace Streamer;

H264FileParser::H264FileParser(std::string directory, uint32_t fps, bool loop)
    : FileParser(directory, ".h264", fps, loop)
{
}

void H264FileParser::load_next_sample()
{
    FileParser::load_next_sample();

    std::size_t i = 0;
    while (i < m_sample.size())
    {
        assert(i + 4 < m_sample.size());

        auto length_ptr = (uint32_t*)(m_sample.data() + i);
        uint32_t length = ntohl(*length_ptr);
        auto nal_units_start_idx = i + 4;
        auto nal_units_end_idx = nal_units_start_idx + length;

        assert(nal_units_end_idx <= m_sample.size());

        auto header = reinterpret_cast<rtc::NalUnitHeader*>(m_sample.data() + nal_units_start_idx);
        switch (header->unitType())
        {
        case 7:
            previous_unit_type7 = {m_sample.begin() + i, m_sample.begin() + nal_units_end_idx};
            break;
        case 8:
            previous_unit_type8 = {m_sample.begin() + i, m_sample.begin() + nal_units_end_idx};
            break;
        case 5:
            previous_unit_type5 = {m_sample.begin() + i, m_sample.begin() + nal_units_end_idx};
            break;
        default:
            break;
        }
        i = nal_units_end_idx;
    }
}

std::vector<std::byte> H264FileParser::initial_nal_units()
{
    units_t units{};
    if (previous_unit_type7.has_value())
    {
        auto const& nalu = previous_unit_type7.value();
        units.insert(units.end(), nalu.begin(), nalu.end());
    }
    if (previous_unit_type8.has_value())
    {
        auto const& nalu = previous_unit_type8.value();
        units.insert(units.end(), nalu.begin(), nalu.end());
    }
    if (previous_unit_type5.has_value())
    {
        auto const& nalu = previous_unit_type5.value();
        units.insert(units.end(), nalu.begin(), nalu.end());
    }
    return units;
}
