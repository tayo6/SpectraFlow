#pragma once

#include <juce_core/juce_core.h>
#include <vector>
#include <atomic>

// ─────────────────────────────────────────────────────────────────────────────
// WaveformAnalyzer
//
// Maintains a rolling ring buffer of audio samples for waveform display.
// Supports oscilloscope, envelope (RMS), and stereo-scope modes.
// Thread: push() from analysis thread; getSnapshot() from analysis thread
//         before publishing to GUI snapshot.
// ─────────────────────────────────────────────────────────────────────────────
class WaveformAnalyzer
{
public:
    enum class Mode
    {
        Oscilloscope = 0,
        Envelope,
        Loudness
    };

    WaveformAnalyzer();
    ~WaveformAnalyzer() = default;

    void prepare(double sampleRate, float historySeconds);
    void push(const float* samples, int numSamples);

    // Fill 'out' with a downsampled snapshot sized to out.size()
    void getSnapshot(std::vector<float>& out) const;

    void setMode(Mode m)        { m_mode.store(static_cast<int>(m)); }
    void setZoom(float zoom)    { m_zoom.store(zoom); }

    int  getCapacity()  const noexcept { return static_cast<int>(m_ring.size()); }

private:
    std::vector<float>  m_ring;
    int                 m_writePos   = 0;
    int                 m_capacity   = 0;
    double              m_sampleRate = 44100.0;

    // RMS envelope state
    float               m_envState   = 0.f;
    float               m_envCoeff   = 0.f;

    std::atomic<int>    m_mode { static_cast<int>(Mode::Oscilloscope) };
    std::atomic<float>  m_zoom { 1.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformAnalyzer)
};
