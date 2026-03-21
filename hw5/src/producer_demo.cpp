#include "mpsc_shm_queue.h"

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

int main(int argc, char** argv) {
    const std::string shm_name = argc > 1 ? argv[1] : "/hw5_mpsc_demo";

    try {
        hw5::ProducerNode producer(shm_name, 1U << 16U);

        for (int i = 0; i < 20; ++i) {
            const std::uint32_t type = (i % 2 == 0) ? 1 : 2;
            const std::string payload = "msg-" + std::to_string(i);

            while (!producer.Send(type, payload)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
            }

            std::cout << "sent type=" << type << " payload=" << payload << '\n';
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
        }

        std::cout << "producer finished\n";
    } catch (const std::exception& ex) {
        std::cerr << "producer error: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}
