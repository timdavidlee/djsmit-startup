# Project 5: Interrupt-style ECG pipeline

Build a two-thread pipeline: one thread plays the role of a hardware interrupt producing heart-signal samples, the other filters them, detects beats and reports heart rate. It combines Projects 1 to 4 and adds the hardest embedded topic, sharing data safely between an interrupt and the main loop.

*Medical context:* an **ECG** (electrocardiogram) is the heart's electrical signal plotted over time. Each beat shows a tall, narrow spike called the **R wave**, the peak of the **QRS complex**, which marks the ventricles contracting. The time between two R waves is the **R-R interval**, and heart rate in beats per minute is 60,000 divided by the R-R interval in milliseconds. A resting rate below 60 is called **bradycardia** (the condition pacemakers treat) and above 100 is **tachycardia**.

Build this one with `-DSANITIZER=thread` as well as the default.

**Tasks**

1. **Signal source.** Generate a synthetic ECG at 250 samples per second as `int16_t`: a flat baseline, a spike of about 1000 counts every 200 samples (75 beats per minute), and ±50 counts of pseudo-random noise. Use a fixed seed.
2. **SPSC queue.** Turn the Project 1 ring buffer into a single-producer, single-consumer lock-free queue. Make `head_` and `tail_` `std::atomic<std::size_t>`; the producer writes only `tail_`, the consumer only `head_`. Use `memory_order_release` on the store and `memory_order_acquire` on the load of the other side's index.
3. **Producer thread.** Stands in for an ADC (analog-to-digital converter) interrupt. It pushes one sample at a time and must never block, allocate or lock. If the queue is full, it increments an atomic overrun counter.
4. **Filter.** In the consumer, apply an 8-sample moving average using integers only: keep a running sum, subtract the oldest sample, add the newest, shift right by 3.
5. **Beat detector.** Detect an R wave when the filtered signal crosses a threshold upward, then ignore crossings for 200 ms. Reuse the state machine pattern from Project 4.
6. **Heart rate.** Compute beats per minute from the mean of the last 8 R-R intervals, using integer math. Flag bradycardia and tachycardia.
7. **Verify.** The reported rate must be 75 ±1. Then build with the thread sanitizer and confirm zero data races. Temporarily make an index non-atomic and watch ThreadSanitizer catch it.
8. **Report.** Every second, emit a Project 3 telemetry frame carrying the heart rate.

**Check yourself**

- Why is a mutex unacceptable inside an interrupt handler?
- Why does the lock-free queue only work with exactly one producer and one consumer?
- A thread is not a real interrupt. On a single-core microcontroller, what are the two classic ways to protect data shared with an interrupt handler?
- What is the worst-case execution time of your consumer loop per sample, and why do firmware engineers care about worst case more than average?
- How much RAM does the whole pipeline use? Add it up from `sizeof`.

**Stretch: C# interop.** The job also involves C# and .NET, likely for tools that sit beside the firmware. Expose the pipeline through a C interface (`extern "C" int32_t ecg_process(const int16_t* samples, int32_t count, int32_t* bpm_out)`), build it as a shared library, and call it from a small .NET console app with `[DllImport]`. Then have the C# side parse the telemetry frames with `BinaryPrimitives` and check the CRC. Note what crosses the boundary cleanly (fixed-width integers, pointers to plain arrays) and what does not (C++ classes, templates, `std::` types).
