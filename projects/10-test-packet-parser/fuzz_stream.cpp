// Fuzz harness for FrameReader and decode_command().
// Build and run from this folder:
//   clang++ -g -O1 -fsanitize=fuzzer,address,undefined -Isrc fuzz_stream.cpp src/*.cpp \
//       -o fuzz_stream && ./fuzz_stream
// On a Mac, use Homebrew's Clang: replace clang++ with $(brew --prefix llvm)/bin/clang++
//
// The fuzzer's input is used as a script: one control byte that picks a chunk size from 1 to
// 16, then that many bytes fed to the reader in a single call, then at most one packet is
// popped and decoded.
#include <memory>

#include "command.h"
#include "frame_reader.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    // On the heap at its exact size, like the buffers in the unit tests, so AddressSanitizer
    // checks its bounds.
    std::unique_ptr<FrameReader> reader(new FrameReader);

    size_t pos = 0;
    while (pos < size) {
        size_t chunk = 1u + data[pos] % 16u;
        ++pos;
        if (chunk > size - pos) chunk = size - pos;
        reader->feed(data + pos, chunk);
        pos += chunk;

        Packet packet{};
        if (reader->pop(packet)) {
            Command command{};
            decode_command(packet, command);
        }
    }
    return 0;
}
