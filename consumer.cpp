#include <iostream>
#include <chrono>
#include <atomic>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <signal.h>
#include "shared_common.h"

std::atomic<bool> is_running{true};

void handle_signal(int signum) {
    if (signum == SIGUSR1) {
        is_running = !is_running;
        const char* msg = is_running ? "\n[Consumer] Resumed\n" : "\n[Consumer] Paused (Backpressure active)\n";
        write(STDOUT_FILENO, msg, strlen(msg));
    }
}

int main() {
    struct sigaction sa{};
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGUSR1, &sa, nullptr);

    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd < 0) {
        std::cerr << "Error: Failed to open shared memory. Run Producer first!\n";
        return 1;
    }

    auto* shared_data = static_cast<SharedBuffer*>(mmap(
        nullptr, sizeof(SharedBuffer), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0));

    sem_t* sem_prod = sem_open(SEM_PROD_NAME, 0);
    sem_t* sem_cons = sem_open(SEM_CONS_NAME, 0);

    std::cout << "Consumer started. PID: " << getpid() << "\n";
    std::cout << "To pause/resume execute: kill -SIGUSR1 " << getpid() << "\n";

    uint64_t total_packages = 0;
    uint64_t period_packages = 0;
    uint64_t period_bytes = 0;

    auto last_report_time = std::chrono::steady_clock::now();

    while (true) {
        if (!is_running) {
            usleep(50000);
            continue;
        }

        sem_wait(sem_cons);

        uint32_t current_tail = shared_data->tail;
        const BufferSlot& slot = shared_data->slots[current_tail];

        const uint32_t payload_size = slot.header.payload_size;
        if (payload_size > MAX_PAYLOAD_SIZE) {
            std::cerr << "[Consumer] Invalid payload size at packet " << slot.header.seq_number << "\n";
        } else {
            const uint32_t calc_checksum = calculate_checksum(slot.payload, payload_size);
            if (calc_checksum != slot.header.checksum) {
                std::cerr << "[Consumer] Integrity error! Checksum mismatch at packet "
                          << slot.header.seq_number << "\n";
            } else {
                total_packages++;
                period_packages++;
                period_bytes += payload_size + sizeof(PacketHeader);
            }
        }

        shared_data->tail = (current_tail + 1) % BUFFER_SLOTS;
        sem_post(sem_prod);

        auto now = std::chrono::steady_clock::now();
        const std::chrono::duration<double> elapsed = now - last_report_time;
        if (elapsed.count() >= 1.0) {
            const double mbytes_per_sec = (period_bytes / (1024.0 * 1024.0)) / elapsed.count();
            std::cout << "[Report] Total: " << total_packages
                      << " | Packets/sec: " << static_cast<uint64_t>(period_packages / elapsed.count())
                      << " | Throughput: " << mbytes_per_sec << " MB/s\n";

            period_packages = 0;
            period_bytes = 0;
            last_report_time = now;
        }
    }
}
