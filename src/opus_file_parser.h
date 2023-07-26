#pragma once

#include "file_parser.h"

namespace Streamer
{
    class OPUSFileParser: public FileParser
    {
    public:
        OPUSFileParser(std::string directory, bool loop,
                       uint32_t samples_per_second = OPUSFileParser::default_samples_per_second);

    private:
        static const uint32_t default_samples_per_second = 50;
    };
}
