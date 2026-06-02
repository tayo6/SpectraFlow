#include "FIFOManager.h"
#include <algorithm>
#include <cstring>

FIFOManager::FIFOManager(int capacity)
{
    reset(capacity);
}

int FIFOManager::nextPow2(int v) noexcept
{
    if (v <= 0) return 1;
    --v;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    return v + 1;
}

void FIFOManager::reset(int capacity)
{
    const int pow2 = nextPow2(capacity);
    m_buffer.assign(pow2, 0.f);
    m_mask     = pow2 - 1;
    m_writePos.store(0, std::memory_order_relaxed);
    m_readPos.store(0,  std::memory_order_relaxed);
}

void FIFOManager::push(const float* data, int numSamples)
{
    const int cap  = m_mask + 1;
    int wp         = m_writePos.load(std::memory_order_relaxed);
    const int rp   = m_readPos.load(std::memory_order_acquire);

    for (int i = 0; i < numSamples; ++i)
    {
        const int nextWp = (wp + 1) & m_mask;
        if (nextWp == rp)
        {
            // Buffer full — advance read pointer to drop oldest sample
            m_readPos.store((rp + 1) & m_mask, std::memory_order_release);
        }
        m_buffer[wp] = data[i];
        wp = (wp + 1) & m_mask;
        (void)cap;
    }

    m_writePos.store(wp, std::memory_order_release);
}

int FIFOManager::pop(float* dest, int numSamples)
{
    int rp       = m_readPos.load(std::memory_order_relaxed);
    const int wp = m_writePos.load(std::memory_order_acquire);

    int available = (wp - rp) & m_mask;
    const int toRead = std::min(available, numSamples);

    for (int i = 0; i < toRead; ++i)
    {
        dest[i] = m_buffer[rp];
        rp = (rp + 1) & m_mask;
    }

    m_readPos.store(rp, std::memory_order_release);
    return toRead;
}

int FIFOManager::getNumReady() const noexcept
{
    const int wp = m_writePos.load(std::memory_order_acquire);
    const int rp = m_readPos.load(std::memory_order_relaxed);
    return (wp - rp) & m_mask;
}
