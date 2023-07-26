#pragma once

#include "file_parser.h"

#include <optional>
#include <vector>
#include <cstddef>

namespace Streamer
{
    class H264FileParser: public FileParser
    {
        using units_t = std::vector<std::byte>;
    public:
        H264FileParser(std::string directory, uint32_t fps, bool loop);
        void load_next_sample() override;
        [[nodiscard]] std::vector<std::byte> initial_nal_units();

    private:
        std::optional<units_t> previous_unit_type5 = std::nullopt;
        std::optional<units_t> previous_unit_type7 = std::nullopt;
        std::optional<units_t> previous_unit_type8 = std::nullopt;
    };
}