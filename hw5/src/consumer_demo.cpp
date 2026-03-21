#include "mpsc_shm_queue.h"

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

int main(int argc, char** argv) {
    const std::string shm_name = argc > 1 ? argv[1] : "/hw5_mpsc_demo";
    const std::uint32_t wanted_type = argc > 2 ? static_cast<std::uint32_t>(std::stoul(argv[2])) : 1;

    try {
        hw5::ConsumerNode consumer(shm_name);

        int received = 0;
        while (received < 10) {
            hw5::Message msg;
            if (consumer.TryRecvByType(wanted_type, msg)) {
                std::string payload(msg.payload.begin(), msg.payload.end());
                std::cout << "recv type=" << msg.type << " payload=" << payload << '\n';
                ++received;
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        }

        std::cout << "consumer finished\n";
    } catch (const std::exception& ex) {
        std::cerr << "consumer error: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}
