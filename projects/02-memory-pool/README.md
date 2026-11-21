# Project 2: Memory pool allocator

Build a fixed-block pool that hands out and reclaims objects of one type from a static buffer, in constant time, with no heap. Firmware avoids `malloc` because fragmentation and variable timing are unacceptable in a device that must run for years; pools are the standard replacement.

**Starting interface**

```cpp
template <typename T, std::size_t N>
class Pool {
public:
    template <typename... Args>
    T* create(Args&&... args);   // nullptr if exhausted
    void destroy(T* p);          // runs ~T(), returns block to free list
    std::size_t available() const;
private:
    union Slot {
        Slot* next;
        alignas(T) std::byte storage[sizeof(T)];
    };
    Slot slots_[N];
    Slot* free_ = nullptr;
};
```

**Tasks**

1. Thread an intrusive free list through the unused slots in the constructor. `create` pops the head and constructs with placement new; `destroy` calls the destructor explicitly and pushes the slot back.
2. Prove both operations are O(1) and that no bytes outside `slots_` are ever touched. Run under AddressSanitizer.
3. Delete the copy constructor and copy assignment. Explain why copying a pool would be a bug.
4. Write a `PoolPtr<T>`: a small move-only RAII handle that calls `pool.destroy` in its destructor, so a block cannot leak.
5. Add debug-only checks behind an `#ifndef NDEBUG`: double free, and a pointer that does not belong to this pool.
6. Test with a type whose constructor and destructor increment counters, and confirm they balance after 10,000 random create/destroy operations.

**Check yourself**

- What is the difference between allocating memory and constructing an object? Which one does placement new do?
- What does `alignas(T)` prevent? What happens on an ARM Cortex-M0 if a `uint32_t` is read from an odd address?
- Rule of five: which of the five special member functions does `PoolPtr` need, and which must be deleted?
- Why does `std::forward<Args>(args)...` appear in `create`?
- Stack, static storage, heap: where does each object in your test program live?

**Stretch:** write a bump (arena) allocator that serves mixed sizes and is reset all at once. Compare when you would pick an arena over a pool.
