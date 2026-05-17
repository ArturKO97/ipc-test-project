#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <cstring>
#include <atomic>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include "shared_common.h"

std::atomic<bool> is_running{true};

void handle_signal(int signum) {
    if (signum == SIGUSR1) {
        is_running = !is_running;
        const char* msg = is_running ? "\n[Producer] Resumed\n" : "\n[Producer] Paused\n";
        write(STDOUT_FILENO, msg, strlen(msg));
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <payload_size_bytes>\n";
        return 1;
    }

    size_t payload_size = std::stoul(argv[1]);
    if (payload_size > MAX_PAYLOAD_SIZE) {
        std::cerr << "Error: Max payload size is " << MAX_PAYLOAD_SIZE << " bytes\n";
        return 1;
    }

    struct sigaction sa{};
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGUSR1, &sa, nullptr);

    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(SharedBuffer));
    auto* shared_data = static_cast<SharedBuffer*>(mmap(
        nullptr, sizeof(SharedBuffer), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0));

    sem_t* sem_prod = sem_open(SEM_PROD_NAME, O_CREAT, 0666, BUFFER_SLOTS);
    sem_t* sem_cons = sem_open(SEM_CONS_NAME, O_CREAT, 0666, 0);

    std::cout << "Producer started. PID: " << getpid() << "\n";
    std::cout << "To pause/resume execute: kill -SIGUSR1 " << getpid() << "\n";

    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<uint8_t> dist(0, 255);
    std::vector<uint8_t> dummy_payload(payload_size);
    for (size_t i = 0; i < payload_size; ++i) {
        dummy_payload[i] = dist(rng);
    }
    const uint32_t payload_checksum = calculate_checksum(dummy_payload.data(), payload_size);

    uint32_t seq_num = 0;

    while (true) {
        if (!is_running) {
            usleep(50000);
            continue;
        }

        sem_wait(sem_prod);

        uint32_t current_head = shared_data->head;
        BufferSlot& slot = shared_data->slots[current_head];

        slot.header.seq_number = seq_num++;
        slot.header.payload_size = static_cast<uint32_t>(payload_size);
        slot.header.timestamp_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        slot.header.checksum = payload_checksum;

        std::memcpy(slot.payload, dummy_payload.data(), payload_size);

        shared_data->head = (current_head + 1) % BUFFER_SLOTS;
        sem_post(sem_cons);
    }
}
