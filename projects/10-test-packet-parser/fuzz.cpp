// Build: clang++ -g -O1 -fsanitize=fuzzer,address,undefined fuzz.cpp -o fuzz && ./fuzz
#include "packet.h"
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    Packet out{};
    parse(data, size, out);
    return 0;
}
