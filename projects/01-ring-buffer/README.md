# Project 1: Fixed-capacity ring buffer

Build a circular buffer whose capacity is a template parameter and whose storage lives entirely inside the object. This is the most common data structure in firmware: sensor samples, UART bytes and event logs all sit in one.

**Starting interface**

```cpp
template <typename T, std::size_t N>
class RingBuffer {
public:
    bool push(const T& value);        // false if full
    std::optional<T> pop();           // empty optional if empty
    const T& peek(std::size_t i) const; // i-th oldest element
    std::size_t size() const;
    bool empty() const;
    bool full() const;
    static constexpr std::size_t capacity() { return N; }
private:
    std::array<T, N> data_{};
    std::size_t head_ = 0;
    std::size_t count_ = 0;
};
```

**Tasks**

1. Implement the interface above. `sizeof(RingBuffer<uint16_t, 64>)` should be 128 bytes plus the two indices and padding.
2. Add a second policy, `push_overwrite`, that drops the oldest sample when full. An ECG trace (a recording of the heart's electrical signal) wants this; an event log does not.
3. Add `static_assert(N > 0)` and a specialization or `constexpr` branch that uses a bitmask instead of `%` when `N` is a power of two.
4. Write tests for wraparound, full, empty, and push-pop interleaving over at least `3 * N` operations.
5. Compare the generated assembly for the `%` and bitmask versions on [Compiler Explorer](https://godbolt.org) at `-O2`.

**Check yourself**

- Where does a `RingBuffer` declared inside a function live? What about one declared `static`? What is the risk of putting a large one on the stack?
- Why does a template go in the header?
- What does `const` after a member function promise, and what does the compiler do if you break it?
- Why is `std::array` acceptable here when `std::vector` is not?

**Stretch:** make it work for types with no default constructor by storing raw bytes (`alignas(T) std::byte storage_[N * sizeof(T)]`). That needs placement new, which is Project 2.
