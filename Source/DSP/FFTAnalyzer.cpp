#include "FFTAnalyzer.h"
#include <cmath>
#include <algorithm>
#include <numeric>

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────
FFTAnalyzer::FFTAnalyzer(int fftSize)
    : m_fftSize(fftSize)
{
    prepare(fftSize, 44100.0);
}

// ─────────────────────────────────────────────────────────────────────────────
// prepare
// ─────────────────────────────────────────────────────────────────────────────
void FFTAnalyzer::prepare(int fftSize, double sampleRate)
{
    m_sampleRate = sampleRate;
    m_fftSize    = fftSize;
    m_displayBins = fftSize / 2;
    m_hopSize     = fftSize / 2;   // 50% overlap

    // KFR real FFT plan
    m_fftPlan = std::make_unique<kfr::dft_plan_real<float>>(
        static_cast<kfr::size_t>(m_fftSize));

    // Scratch buffer required by KFR
    m_scratchBuf.resize(m_fftPlan->temp_size);

    // Allocate buffers
    m_inputBuffer.assign(m_fftSize, 0.f);
    m_fftOutput.resize(m_fftSize / 2 + 1);
    m_magnitudeDB.assign(m_displayBins, -120.f);
    m_peakHoldDB.assign(m_displayBins, -120.f);
    m_smoothedDB.assign(m_displayBins, -120.f);
    m_peakHoldAge.assign(m_displayBins, 0.f);

    buildWindow();

    m_inputWritePos    = 0;
    m_samplesUntilNext = m_hopSize;
    m_rmsDB            = -120.f;
    m_peakDB           = -120.f;
}

// ─────────────────────────────────────────────────────────────────────────────
// push — called from analysis thread
// ─────────────────────────────────────────────────────────────────────────────
void FFTAnalyzer::push(const float* samples, int numSamples)
{
    for (int i = 0; i < numSamples; ++i)
    {
        m_inputBuffer[m_inputWritePos] = samples[i];
        m_inputWritePos = (m_inputWritePos + 1) % m_fftSize;
        --m_samplesUntilNext;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// process — run FFT when we have enough samples
// ─────────────────────────────────────────────────────────────────────────────
void FFTAnalyzer::process()
{
    rebuildIfNeeded();

    if (m_samplesUntilNext > 0)
        return;   // Not enough new samples yet

    m_samplesUntilNext = m_hopSize;

    // Build windowed frame (unwrap ring buffer)
    static thread_local std::vector<float> frame;
    frame.resize(m_fftSize);

    const int start = m_inputWritePos;
    for (int i = 0; i < m_fftSize; ++i)
    {
        const int idx = (start + i) % m_fftSize;
        frame[i] = m_inputBuffer[idx] * m_window[i];
    }

    // Compute RMS on un-windowed frame for level metering
    computeRMS(frame.data(), m_fftSize);

    // ── KFR real FFT ──────────────────────────────────────────────────────
    kfr::univector<float> kfrInput(m_fftSize);
    std::copy(frame.begin(), frame.end(), kfrInput.begin());

    kfr::univector<kfr::complex<float>> kfrOutput(m_fftSize / 2 + 1);

    m_fftPlan->execute(
    kfrOutput.data(),
    kfrInput.data(),
    m_scratchBuf.data());

    // ── Magnitude → dB ────────────────────────────────────────────────────
    const float gainLinear = std::pow(10.f, m_gainDB.load() / 20.f);
    const float normFactor  = 2.f / static_cast<float>(m_fftSize);

    for (int bin = 0; bin < m_displayBins; ++bin)
    {
        const float re  = kfrOutput[bin].real();
        const float im  = kfrOutput[bin].imag();
        const float mag = std::sqrt(re * re + im * im) * normFactor * gainLinear;
        m_magnitudeDB[bin] = 20.f * safeLog10(mag);
    }

    applySmoothing();
    updatePeakHold(static_cast<float>(m_hopSize) / static_cast<float>(m_sampleRate));
}

// ─────────────────────────────────────────────────────────────────────────────
// getMagnitudesDB — copy out display data
// ─────────────────────────────────────────────────────────────────────────────
void FFTAnalyzer::getMagnitudesDB(std::vector<float>& out) const
{
    out = m_smoothedDB;
}

void FFTAnalyzer::getPeakHold(std::vector<float>& out) const
{
    out = m_peakHoldDB;
}

// ─────────────────────────────────────────────────────────────────────────────
// buildWindow
// ─────────────────────────────────────────────────────────────────────────────
void FFTAnalyzer::buildWindow()
{
    m_window.resize(m_fftSize);
    const int  N   = m_fftSize;
    const auto wt  = static_cast<WindowType>(m_windowType.load());
    const float pi = juce::MathConstants<float>::pi;

    for (int i = 0; i < N; ++i)
    {
        const float n = static_cast<float>(i);
        switch (wt)
        {
            case WindowType::Hann:
                m_window[i] = 0.5f * (1.f - std::cos(2.f * pi * n / (N - 1)));
                break;
            case WindowType::Hamming:
                m_window[i] = 0.54f - 0.46f * std::cos(2.f * pi * n / (N - 1));
                break;
            case WindowType::Blackman:
                m_window[i] = 0.42f
                    - 0.5f  * std::cos(2.f * pi * n / (N - 1))
                    + 0.08f * std::cos(4.f * pi * n / (N - 1));
                break;
            case WindowType::Kaiser:
            {
                // Kaiser β=8 approximation
                const float beta = 8.f;
                const float half = (N - 1) * 0.5f;
                const float arg  = 1.f - ((n - half) / half) * ((n - half) / half);
                // I0 approximation via series (sufficient for audio)
                float sum = 1.f, x = (beta * std::sqrt(std::max(arg, 0.f)));
                float term = 1.f;
                for (int k = 1; k <= 10; ++k)
                {
                    term *= (x * 0.5f) * (x * 0.5f) / (static_cast<float>(k * k));
                    sum += term;
                }
                m_window[i] = sum;  // Will normalize below
                break;
            }
            case WindowType::FlatTop:
                m_window[i] = 0.21557895f
                    - 0.41663158f * std::cos(2.f * pi * n / (N - 1))
                    + 0.27726316f * std::cos(4.f * pi * n / (N - 1))
                    - 0.08357895f * std::cos(6.f * pi * n / (N - 1))
                    + 0.00694737f * std::cos(8.f * pi * n / (N - 1));
                break;
        }
    }

    // Normalize Kaiser window
    if (wt == WindowType::Kaiser)
    {
        const float maxVal = *std::max_element(m_window.begin(), m_window.end());
        if (maxVal > 0.f)
            for (auto& w : m_window) w /= maxVal;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// rebuildIfNeeded — check if FFT size changed
// ─────────────────────────────────────────────────────────────────────────────
void FFTAnalyzer::rebuildIfNeeded()
{
    if (!m_needsRebuild.exchange(false))
        return;

    const int newSize = m_pendingFFTSize.load();
    prepare(newSize, m_sampleRate);
}

void FFTAnalyzer::setFFTSize(int size)
{
    m_pendingFFTSize.store(size);
    m_needsRebuild.store(true);
}

// ─────────────────────────────────────────────────────────────────────────────
// computeRMS
// ─────────────────────────────────────────────────────────────────────────────
void FFTAnalyzer::computeRMS(const float* frame, int n)
{
    float sum = 0.f;
    float peak = 0.f;
    for (int i = 0; i < n; ++i)
    {
        sum  += frame[i] * frame[i];
        peak  = std::max(peak, std::abs(frame[i]));
    }
    m_rmsDB  = 10.f * safeLog10(sum / static_cast<float>(n));
    m_peakDB = 20.f * safeLog10(peak);
}

// ─────────────────────────────────────────────────────────────────────────────
// applySmoothing — exponential IIR per bin
// ─────────────────────────────────────────────────────────────────────────────
void FFTAnalyzer::applySmoothing()
{
    const float alpha = m_smoothing.load();
    const float inv   = 1.f - alpha;

    for (int bin = 0; bin < m_displayBins; ++bin)
    {
        // Attack faster than release
        const float cur = m_magnitudeDB[bin];
        const float old = m_smoothedDB[bin];

        if (cur > old)
            m_smoothedDB[bin] = cur * (1.f - alpha * 0.5f) + old * (alpha * 0.5f);
        else
            m_smoothedDB[bin] = cur * inv + old * alpha;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// updatePeakHold
// ─────────────────────────────────────────────────────────────────────────────
void FFTAnalyzer::updatePeakHold(float dt)
{
    const float holdTime  = m_peakHoldTime.load();
    const float decayRate = 30.f;  // dB per second decay after hold

    for (int bin = 0; bin < m_displayBins; ++bin)
    {
        const float cur = m_smoothedDB[bin];
        if (cur >= m_peakHoldDB[bin])
        {
            m_peakHoldDB[bin] = cur;
            m_peakHoldAge[bin] = 0.f;
        }
        else
        {
            m_peakHoldAge[bin] += dt;
            if (m_peakHoldAge[bin] > holdTime)
                m_peakHoldDB[bin] -= decayRate * dt;

            m_peakHoldDB[bin] = std::max(m_peakHoldDB[bin], -120.f);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Utilities
// ─────────────────────────────────────────────────────────────────────────────
float FFTAnalyzer::binToFrequency(int bin, int fftSize, double sr) noexcept
{
    return static_cast<float>(bin) * static_cast<float>(sr) / static_cast<float>(fftSize);
}

float FFTAnalyzer::safeLog10(float v) noexcept
{
    return (v > 1e-30f) ? std::log10(v) : -30.f;
}