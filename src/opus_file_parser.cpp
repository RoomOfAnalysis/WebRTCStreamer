#include "opus_file_parser.h"

using namespace Streamer;

OPUSFileParser::OPUSFileParser(std::string directory, bool loop, uint32_t samples_per_second)
    : FileParser(directory, ".opus", samples_per_second, loop)
{
}
