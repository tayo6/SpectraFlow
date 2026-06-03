#pragma once

#include <juce_core/juce_core.h>
#include <vector>
#include <array>
#include <atomic>
#include <complex>

// KFR forward declarations — include the KFR headers carefully
// (KFR must be included before any JUCE headers that redefine complex)
#include <kfr/dft.hpp>
#include <kfr/dsp.hpp>
#include <kfr/math.hpp>

// ─────────────────────────────────────────────────────────────────────────────
// FFTAnalyzer
//
// Responsibilities:
//   - Accept samples pushed from the audio thread (via FIFOManager)
//   - When enough samples are available, run a windowed KFR FFT
//   - Store magnitude spectrum in dB with smoothing and peak-hold
//   - Thread: runs on the analysis thread, never on audio or GUI thread
// ─────────────────────────────────────────────────────────────────────────────
class FFTAnalyzer
{
public:
    enum class WindowType
    {
        Hann = 0,
        Hamming,
        Blackman,
        Kaiser,
        FlatTop
    };

    enum class FrequencyScale
    {
        Logarithmic = 0,
        Linear,
        Mel,
        Bark
    };

    explicit FFTAnalyzer(int fftSize = 2048);
    ~FFTAnalyzer() = default;

    // Called from audio thread prepareToPlay path
    void prepare(int fftSize, double sampleRate);

    // Called from analysis thread
    void push(const float* samples, int numSamples);
    void process();   // Run FFT if enough samples are buffered

    // Results — safe to read from analysis thread after process()
    void getMagnitudesDB(std::vector<float>& out) const;
    void getPeakHold(std::vector<float>& out) const;
    float getRMS()  const noexcept { return m_rmsDB; }
    float getPeak() const noexcept { return m_peakDB; }

    // Settings (can be changed from GUI thread atomically)
    void setFFTSize(int size);
    void setWindowType(WindowType w)       { m_windowType.store(static_cast<int>(w)); }
    void setFrequencyScale(FrequencyScale s) { m_freqScale.store(static_cast<int>(s)); }
    void setSmoothing(float alpha)         { m_smoothing.store(alpha); }
    void setPeakHoldTime(float seconds)    { m_peakHoldTime.store(seconds); }
    void setGain(float gainDB)             { m_gainDB.store(gainDB); }

    int getFFTSize()     const noexcept { return m_fftSize; }
    int getDisplayBins() const noexcept { return m_displayBins; }

private:
    // ── KFR FFT plan ────────────────────────────────────────────────────────
    std::unique_ptr<kfr::dft_plan_real<float>> m_fftPlan;

    // ── Buffers (analysis thread only) ──────────────────────────────────────
    std::vector<float>                m_inputBuffer;   // Ring-style accumulator
    std::vector<float>                m_window;        // Pre-computed window
    std::vector<kfr::complex<float>>  m_fftOutput;     // KFR complex output
    std::vector<float>                m_magnitudeDB;   // dB magnitudes
    std::vector<float>                m_peakHoldDB;    // Peak-hold per bin
    std::vector<float>                m_smoothedDB;    // Smoothed display
    std::vector<float>                m_peakHoldAge;   // Age in seconds per bin
    std::vector<kfr::u8>              m_scratchBuf;    // KFR temp scratch

    int    m_fftSize          = 2048;
    int    m_displayBins      = 1024;
    int    m_inputWritePos    = 0;
    int    m_hopSize          = 0;
    int    m_samplesUntilNext = 0;
    double m_sampleRate       = 44100.0;

    float  m_rmsDB   = -120.f;
    float  m_peakDB  = -120.f;

    // ── Atomic parameters (GUI → analysis thread) ───────────────────────────
    std::atomic<int>   m_pendingFFTSize { 2048 };
    std::atomic<int>   m_windowType     { static_cast<int>(WindowType::Hann) };
    std::atomic<int>   m_freqScale      { static_cast<int>(FrequencyScale::Logarithmic) };
    std::atomic<float> m_smoothing      { 0.8f };
    std::atomic<float> m_peakHoldTime   { 2.0f };
    std::atomic<float> m_gainDB         { 0.f };
    std::atomic<bool>  m_needsRebuild   { false };

    // ── Private helpers ──────────────────────────────────────────────────────
    void buildWindow();
    void rebuildIfNeeded();
    void computeRMS(const float* frame, int n);
    void applySmoothing();
    void updatePeakHold(float dt);

    static float binToFrequency(int bin, int fftSize, double sr) noexcept;
    static float safeLog10(float v) noexcept;
};
