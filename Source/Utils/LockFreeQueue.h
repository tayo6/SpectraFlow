#pragma once

#include <atomic>
#include <vector>
#include <optional>

// ─────────────────────────────────────────────────────────────────────────────
// LockFreeQueue<T>
//
// Single-producer single-consumer lock-free queue using a power-of-two
// ring buffer.  T must be trivially copyable.
// ─────────────────────────────────────────────────────────────────────────────
template <typename T>
class LockFreeQueue
{
public:
    explicit LockFreeQueue(int capacity)
    {
        const int pow2 = nextPow2(capacity);
        m_buffer.resize(pow2);
        m_mask = pow2 - 1;
    }

    // Producer thread
    bool push(const T& item)
    {
        const int wp = m_writePos.load(std::memory_order_relaxed);
        const int next = (wp + 1) & m_mask;
        if (next == m_readPos.load(std::memory_order_acquire))
            return false;   // full

        m_buffer[wp] = item;
        m_writePos.store(next, std::memory_order_release);
        return true;
    }

    // Consumer thread
    std::optional<T> pop()
    {
        const int rp = m_readPos.load(std::memory_order_relaxed);
        if (rp == m_writePos.load(std::memory_order_acquire))
            return std::nullopt;   // empty

        T item = m_buffer[rp];
        m_readPos.store((rp + 1) & m_mask, std::memory_order_release);
        return item;
    }

    bool isEmpty() const noexcept
    {
        return m_readPos.load(std::memory_order_relaxed)
            == m_writePos.load(std::memory_order_relaxed);
    }

    int size() const noexcept
    {
        const int wp = m_writePos.load(std::memory_order_relaxed);
        const int rp = m_readPos.load(std::memory_order_relaxed);
        return (wp - rp) & m_mask;
    }

private:
    std::vector<T>    m_buffer;
    int               m_mask = 0;
    std::atomic<int>  m_writePos { 0 };
    std::atomic<int>  m_readPos  { 0 };

    static int nextPow2(int v) noexcept
    {
        if (v <= 1) return 2;
        --v;
        v |= v >> 1; v |= v >> 2; v |= v >> 4; v |= v >> 8; v |= v >> 16;
        return v + 1;
    }
};
