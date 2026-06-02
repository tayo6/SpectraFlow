#include "WaveformAnalyzer.h"
#include <cmath>
#include <algorithm>

WaveformAnalyzer::WaveformAnalyzer()
{
    prepare(44100.0, 3.0f);
}

void WaveformAnalyzer::prepare(double sampleRate, float historySeconds)
{
    m_sampleRate = sampleRate;
    m_capacity   = static_cast<int>(sampleRate * historySeconds);

    m_ring.assign(m_capacity, 0.f);
    m_writePos = 0;

    // RMS envelope: ~10 ms release
    const float releaseMs = 10.f;
    m_envCoeff = std::exp(-1.f / (static_cast<float>(sampleRate) * releaseMs * 0.001f));
    m_envState = 0.f;
}

void WaveformAnalyzer::push(const float* samples, int numSamples)
{
    const int cap = m_capacity;
    const auto mode = static_cast<Mode>(m_mode.load());

    for (int i = 0; i < numSamples; ++i)
    {
        float s = samples[i];

        if (mode == Mode::Envelope)
        {
            const float abs_s = std::abs(s);
            m_envState = (abs_s > m_envState)
                ? abs_s
                : m_envState * m_envCoeff;
            s = m_envState;
        }
        else if (mode == Mode::Loudness)
        {
            m_envState = m_envState * m_envCoeff + s * s * (1.f - m_envCoeff);
            s = std::sqrt(m_envState);
        }

        m_ring[m_writePos] = s;
        if (++m_writePos >= cap)
            m_writePos = 0;
    }
}

void WaveformAnalyzer::getSnapshot(std::vector<float>& out) const
{
    const int outSize = static_cast<int>(out.size());
    if (outSize <= 0) return;

    const int cap  = m_capacity;
    const float zoom = m_zoom.load();

    // How many ring-buffer samples map to the display width
    const int viewLen = static_cast<int>(cap / std::max(zoom, 0.1f));
    const float step  = static_cast<float>(viewLen) / static_cast<float>(outSize);

    // Start reading from the oldest visible sample
    int readStart = m_writePos - viewLen;
    if (readStart < 0) readStart += cap;

    for (int i = 0; i < outSize; ++i)
    {
        // Simple linear interpolation between ring buffer samples
        const float floatIdx = i * step;
        const int   idx0     = static_cast<int>(floatIdx);
        const float frac     = floatIdx - static_cast<float>(idx0);

        const int pos0 = (readStart + idx0)     % cap;
        const int pos1 = (readStart + idx0 + 1) % cap;

        out[i] = m_ring[pos0] * (1.f - frac) + m_ring[pos1] * frac;
    }
}
