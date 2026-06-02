#pragma once

#include <juce_core/juce_core.h>
#include <vector>
#include <atomic>

// ─────────────────────────────────────────────────────────────────────────────
// FIFOManager
//
// Lock-free SPSC ring buffer.
// Producer: audio thread (push)
// Consumer: analysis thread (pop)
//
// Uses power-of-two sizing + atomic indices for wait-free push/pop.
// ─────────────────────────────────────────────────────────────────────────────
class FIFOManager
{
public:
    explicit FIFOManager(int capacity);
    ~FIFOManager() = default;

    // Called from prepareToPlay (not realtime — may allocate)
    void reset(int capacity);

    // Audio thread — push samples; drops oldest if full
    void push(const float* data, int numSamples);

    // Analysis thread — pop up to numSamples; returns actual count popped
    int  pop(float* dest, int numSamples);

    // Analysis thread — peek without consuming
    int  getNumReady() const noexcept;
    int  getCapacity() const noexcept { return m_mask + 1; }

private:
    std::vector<float>   m_buffer;
    int                  m_mask = 0;       // capacity - 1 (power of two)

    std::atomic<int>     m_writePos { 0 };
    std::atomic<int>     m_readPos  { 0 };

    // Force each atomic onto its own cache line to avoid false sharing
    char                 m_pad1[64 - sizeof(std::atomic<int>)];
    char                 m_pad2[64 - sizeof(std::atomic<int>)];

    static int nextPow2(int v) noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FIFOManager)
};
