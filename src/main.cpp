#include "client_manager.h"

using namespace std::chrono_literals;

using namespace Streamer;

int main(int argc, char* argv[])
{
    if (argc != 3) std::cerr << "required h264 samples directory and opus samples directory as input" << std::endl;

    rtc::Configuration config;

    std::string stun_server = "stun:stun.l.google.com:19302";
    std::cout << "STUN server is " << stun_server << std::endl;
    config.iceServers.emplace_back(stun_server);
    config.disableAutoNegotiation = true;

    std::string h264_samples_directory = argv[1];
    std::string opus_samples_directory = argv[2];

    ClientManager cm(config, h264_samples_directory, opus_samples_directory);

    std::string local_id = "server";
    std::cout << "The local ID is: " << local_id << std::endl;

    std::string ip_address = "127.0.0.1";
    uint16_t port = 8000;
    std::string url = "ws://" + ip_address + ":" + std::to_string(port) + "/" + local_id;
    std::cout << "URL is " << url << std::endl;

    auto ws = cm.create_ws();
    ws->open(url);

    std::cout << "Waiting for signaling to be connected..." << std::endl;
    while (!ws->isOpen())
    {
        if (ws->isClosed()) return 1;
        std::this_thread::sleep_for(100ms);
    }

    while (true)
    {
        std::string id;
        std::cout << "Enter to exit" << std::endl;
        std::cin >> id;
        std::cin.ignore();
        std::cout << "exiting" << std::endl;
        break;
    }

    std::cout << "Cleaning up..." << std::endl;
    return 0;

    return 0;
}