# SpectraFlow — Realtime Multichannel Spectral Visualization VST3

A CPU-optimised multichannel spectral analysis plugin built with JUCE 7.0.12

## Architecture

```
Audio Thread   →  FIFOManager (lock-free SPSC)
Analysis Thread ←  FFTAnalyzer (KFR FFT, windowing, smoothing, peak-hold)
                   WaveformAnalyzer (ring buffer, envelope, loudness)
                   Double-buffer snapshot publish
GUI Thread     ←  UIController (timer 30–120 FPS)
                   SpectrumRenderer (cached paths, log mapping, glow)
                   WaveformRenderer
```

### Key design decisions

| Concern | Solution |
|---|---|
| Audio thread safety | Lock-free SPSC FIFO; audio thread only writes samples |
| CPU: 12 FFTs per frame | Staggered batching — 3 channels per tick (4 ticks = full cycle) |
| GUI repaint cost | Dirty-region repaint of spectrum + waveform areas only |
| Path allocation | Pre-allocated `juce::Path` objects, `clear()` + reuse each frame |
| Bin→pixel mapping | Cached `m_binToX[]` array, rebuilt only on resize or param change |
| Thread safety | Atomic double-buffer flip; no mutexes on hot path |

## Buses

| Index | Default Name | Notes |
|---|---|---|
| 0 | Bass | Main input — always active |
| 1 | Lead Vocal | Optional sidechain |
| 2 | Backup Vocal | Optional sidechain |
| 3 | Drums | Optional sidechain |
| 4 | Synth | Optional sidechain |
| 5 | FX | Optional sidechain |
| 6 | Percussion | Optional sidechain |
| 7 | Pads | Optional sidechain |
| 8 | Master | Optional sidechain |
| 9 | Reference | Optional sidechain |
| 10 | Sidechain | Optional sidechain |
| 11 | Utility | Optional sidechain |

## Building locally (Windows)

```bat
git clone <repo>
cd SpectraFlow
cmake -B build -S . -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target SpectraFlow_VST3
```

The `.vst3` bundle appears under `build/SpectraFlow_artefacts/Release/VST3/`.

## GitHub Actions

Push to `main` or `develop` to trigger the Windows build. The VST3 bundle is uploaded as artifact `SpectraFlow-VST3-Windows-x64`.

## Dependencies (auto-fetched by CMake FetchContent)

## CPU budget estimates (per analysis tick, 12 channels staggered)

| Operation | Approx cost |
|---|---|
| 3× KFR FFT-2048 (AVX2) | ~0.3 ms |
| Windowing + smoothing (SIMD) | ~0.1 ms |
| Path generation (3 channels) | ~0.2 ms |
| Total per 16 ms tick | < 1 ms |

license:
MIT
