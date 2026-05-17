#pragma once
#include <cstdint>
#include <cstring>
#include <semaphore.h>

constexpr char SHM_NAME[] = "/mp_cli_shm";
constexpr char SEM_PROD_NAME[] = "/mp_cli_sem_prod";
constexpr char SEM_CONS_NAME[] = "/mp_cli_sem_cons";

constexpr size_t MAX_PAYLOAD_SIZE = 1024 * 1024;
constexpr size_t BUFFER_SLOTS = 16;

struct PacketHeader {
    uint64_t timestamp_ns;
    uint32_t seq_number;
    uint32_t payload_size;
    uint32_t checksum;
};

struct BufferSlot {
    PacketHeader header;
    uint8_t payload[MAX_PAYLOAD_SIZE];
};

struct SharedBuffer {
    uint32_t head;
    uint32_t tail;
    BufferSlot slots[BUFFER_SLOTS];
};

inline uint32_t calculate_checksum(const uint8_t* data, size_t len) {
    uint64_t checksum = 0;
    size_t words = len / 8;
    size_t remainder = len % 8;

    for (size_t i = 0; i < words; ++i) {
        uint64_t word;
        std::memcpy(&word, data + i * 8, sizeof(word));
        checksum ^= word;
    }

    for (size_t j = 0; j < remainder; ++j) {
        checksum ^= static_cast<uint64_t>(data[words * 8 + j]) << (j * 8);
    }

    return static_cast<uint32_t>(checksum ^ (checksum >> 32));
}
