# IPC test task

Two small C++ CLI apps: one sends data packets, the other receives them.

They talk through POSIX shared memory and semaphores. There is a ring buffer with 16 slots. Each packet has a header (timestamp, sequence number, size, checksum) and a payload (up to 1 MB).

No extra libraries — just the standard toolchain.

## Build

You need CMake and a C++ compiler (Clang or GCC).

```bash
./rebuild.sh
```

Binaries end up in `build/`.

## Run

Start the consumer first, then the producer (the producer creates the shared memory):

```bash
# terminal 1
./build/consumer

# terminal 2
./build/producer 1048576
```

The number is the payload size in bytes. `1048576` is 1 MB. You can use something smaller; the max is 1048576.

The producer generates random bytes once at startup and keeps sending them. The consumer prints stats every second: total packets, packets/sec, and MB/s.

## Pause / resume

Both apps toggle pause with `SIGUSR1` (same signal again to resume):

```bash
kill -SIGUSR1 <pid>
```

While paused, the producer just sleeps. The consumer stops reading, the buffer fills up, and the producer blocks when it is full. That is expected — basic backpressure.

## What's in the repo

- `producer.cpp` — sends packets
- `consumer.cpp` — receives and prints stats
- `shared_common.h` — shared structs and checksum
