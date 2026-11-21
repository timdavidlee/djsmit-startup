// Fuzz harness for parse().
// Build and run from this folder:
//   clang++ -g -O1 -fsanitize=fuzzer,address,undefined -Isrc fuzz.cpp src/*.cpp -o fuzz && ./fuzz
// On a Mac, use Homebrew's Clang: replace clang++ with $(brew --prefix llvm)/bin/clang++
#include "packet.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    Packet out{};
    parse(data, size, out);
    return 0;
}
