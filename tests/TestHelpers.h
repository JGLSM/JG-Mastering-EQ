#pragma once

// Test toolkit for the JG Mastering EQ.
//
// Everything here runs the real plugin processor offline (no DAW, no audio
// device) and measures what comes out. As each DSP stage lands, its tests are
// built from these pieces:
//
//   Render     push a signal through the plugin in blocks, with chosen settings
//   Signals    sine, noise, log sweep, impulse (all deterministic)
//   Measure    peak/RMS, null depth, frequency response, THD, aliasing, clicks
//   Goldens    compare a measured response or a render against a stored file
//
// Keep it header-only and free of UI code.

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_dsp/juce_dsp.h>

#include "PluginProcessor.h"

#include <cmath>
#include <complex>
#include <map>
#include <vector>

namespace jgtest
{
// ---------------------------------------------------------------------------
// Setup
// ---------------------------------------------------------------------------
struct JuceGuard
{
    juce::ScopedJuceInitialiser_GUI init;
};

using Settings = std::map<juce::String, float>;   // parameter ID -> plain value

inline juce::RangedAudioParameter& param (JGMasteringEQAudioProcessor& p, const juce::String& id)
{
    auto* rp = p.apvts.getParameter (id);
    if (rp == nullptr)
        throw std::runtime_error (("unknown parameter: " + id).toStdString());
    return *rp;
}

inline void setPlain (JGMasteringEQAudioProcessor& p, const juce::String& id, float plain)
{
    auto& rp = param (p, id);
    rp.setValueNotifyingHost (rp.convertTo0to1 (plain));
}

inline float getPlain (JGMasteringEQAudioProcessor& p, const juce::String& id)
{
    auto& rp = param (p, id);
    return rp.convertFrom0to1 (rp.getValue());
}

inline void applySettings (JGMasteringEQAudioProcessor& p, const Settings& s)
{
    for (const auto& [id, v] : s)
        setPlain (p, id, v);
}

// The brief's null-test condition: input stage and Smart EQ bypassed, every
// band flat, output gain at 0 dB, oversampling off.
inline Settings nullSettings()
{
    Settings s { { "in_bypass", 1.0f }, { "seq_bypass", 1.0f }, { "out_gain", 0.0f }, { "glb_oversampling", 0.0f } };
    for (int b = 1; b <= 7; ++b)
        s["peq_b" + juce::String (b) + "_gain"] = 0.0f;
    return s;
}

// ---------------------------------------------------------------------------
// Signals (all deterministic)
// ---------------------------------------------------------------------------
template <typename Sample = float>
juce::AudioBuffer<Sample> noise (int channels, int samples, double amp = 0.5, juce::int64 seed = 1234)
{
    juce::Random rng (seed);
    juce::AudioBuffer<Sample> b (channels, samples);
    for (int ch = 0; ch < channels; ++ch)
        for (int i = 0; i < samples; ++i)
            b.setSample (ch, i, (Sample) (amp * (rng.nextDouble() * 2.0 - 1.0)));
    return b;
}

inline juce::AudioBuffer<float> sine (int channels, int samples, double sr, double hz, double dBFS = -6.0)
{
    juce::AudioBuffer<float> b (channels, samples);
    const double a = juce::Decibels::decibelsToGain (dBFS);
    for (int i = 0; i < samples; ++i)
    {
        const auto v = (float) (a * std::sin (juce::MathConstants<double>::twoPi * hz * i / sr));
        for (int ch = 0; ch < channels; ++ch)
            b.setSample (ch, i, v);
    }
    return b;
}

inline juce::AudioBuffer<float> impulse (int channels, int samples, int at = 0)
{
    juce::AudioBuffer<float> b (channels, samples);
    b.clear();
    for (int ch = 0; ch < channels; ++ch)
        b.setSample (ch, at, 1.0f);
    return b;
}

// Exponential (log) sine sweep, f0 -> f1 over the whole buffer.
inline juce::AudioBuffer<float> logSweep (int channels, int samples, double sr, double f0 = 20.0, double f1 = 20000.0, double dBFS = -12.0)
{
    juce::AudioBuffer<float> b (channels, samples);
    const double a = juce::Decibels::decibelsToGain (dBFS);
    const double T = samples / sr, k = std::log (f1 / f0);
    for (int i = 0; i < samples; ++i)
    {
        const double t = i / sr;
        const auto v = (float) (a * std::sin (juce::MathConstants<double>::twoPi * f0 * T / k * (std::exp (t / T * k) - 1.0)));
        for (int ch = 0; ch < channels; ++ch)
            b.setSample (ch, i, v);
    }
    return b;
}

// ---------------------------------------------------------------------------
// Render
// ---------------------------------------------------------------------------
struct RenderOptions
{
    double sampleRate = 48000.0;
    int blockSize = 512;
    bool nonRealtime = false;          // offline/bounce mode
    // Called before each block with the block's start sample: use it to
    // automate parameters mid-render.
    std::function<void (JGMasteringEQAudioProcessor&, int)> beforeBlock;
};

template <typename Sample>
juce::AudioBuffer<Sample> render (JGMasteringEQAudioProcessor& p, const juce::AudioBuffer<Sample>& input, const RenderOptions& o = {})
{
    const int chans = input.getNumChannels();
    p.setPlayConfigDetails (chans, chans, o.sampleRate, o.blockSize);
    p.setNonRealtime (o.nonRealtime);
    p.setProcessingPrecision (std::is_same_v<Sample, double> ? juce::AudioProcessor::doublePrecision
                                                             : juce::AudioProcessor::singlePrecision);
    p.prepareToPlay (o.sampleRate, o.blockSize);

    auto out = input;
    juce::MidiBuffer midi;
    for (int start = 0; start < out.getNumSamples(); start += o.blockSize)
    {
        if (o.beforeBlock)
            o.beforeBlock (p, start);
        const int n = std::min (o.blockSize, out.getNumSamples() - start);
        juce::AudioBuffer<Sample> view (out.getArrayOfWritePointers(), chans, start, n);
        p.processBlock (view, midi);
    }
    p.releaseResources();
    return out;
}

// ---------------------------------------------------------------------------
// Measure
// ---------------------------------------------------------------------------
inline double toDb (double gain) { return gain > 1e-20 ? 20.0 * std::log10 (gain) : -400.0; }

template <typename Sample>
double peakDb (const juce::AudioBuffer<Sample>& b)
{
    double m = 0.0;
    for (int ch = 0; ch < b.getNumChannels(); ++ch)
        m = std::max (m, (double) b.getMagnitude (ch, 0, b.getNumSamples()));
    return toDb (m);
}

template <typename Sample>
double rmsDb (const juce::AudioBuffer<Sample>& b, int start = 0, int len = -1)
{
    if (len < 0) len = b.getNumSamples() - start;
    double sum = 0.0;
    for (int ch = 0; ch < b.getNumChannels(); ++ch)
        for (int i = start; i < start + len; ++i)
            sum += (double) b.getSample (ch, i) * (double) b.getSample (ch, i);
    return toDb (std::sqrt (sum / std::max (1, b.getNumChannels() * len)));
}

// Largest sample difference between two buffers, in dB (-400 = bit-identical).
template <typename Sample>
double nullDepthDb (const juce::AudioBuffer<Sample>& a, const juce::AudioBuffer<Sample>& b, int latency = 0)
{
    double m = 0.0;
    for (int ch = 0; ch < a.getNumChannels(); ++ch)
        for (int i = 0; i + latency < b.getNumSamples(); ++i)
            m = std::max (m, std::abs ((double) a.getSample (ch, i) - (double) b.getSample (ch, i + latency)));
    return toDb (m);
}

// FFT size used by the distortion measurements.
inline constexpr int kAnalysisOrder = 15;

// Nearest frequency that fits a whole number of cycles in the analysis window,
// so a pure tone lands in exactly one FFT bin (no window leakage). Use it for
// every tone fed to thdDb / nonHarmonicDb.
inline double coherentHz (double hz, double sr, int order = kAnalysisOrder)
{
    const double n = double (1 << order);
    // odd bin numbers keep harmonics from landing on the same bins as aliases
    double k = std::round (hz * n / sr);
    if (std::fmod (k, 2.0) == 0.0) k += 1.0;
    return k * sr / n;
}

// Magnitude spectrum (linear) of one channel, rectangular window (pair with
// coherentHz). Computed in double precision for a deep noise floor.
inline std::vector<double> spectrum (const juce::AudioBuffer<float>& b, int ch, int start, int order)
{
    const int n = 1 << order;
    std::vector<std::complex<double>> x ((size_t) n);
    for (int i = 0; i < n; ++i)
        x[(size_t) i] = (start + i < b.getNumSamples()) ? (double) b.getSample (ch, start + i) : 0.0;
    // iterative radix-2 FFT
    for (int i = 1, j = 0; i < n; ++i)
    {
        int bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) std::swap (x[(size_t) i], x[(size_t) j]);
    }
    for (int len = 2; len <= n; len <<= 1)
    {
        const auto w = std::polar (1.0, -juce::MathConstants<double>::twoPi / len);
        for (int i = 0; i < n; i += len)
        {
            std::complex<double> wn (1.0);
            for (int k = 0; k < len / 2; ++k, wn *= w)
            {
                auto u = x[(size_t) (i + k)], v = x[(size_t) (i + k + len / 2)] * wn;
                x[(size_t) (i + k)] = u + v;
                x[(size_t) (i + k + len / 2)] = u - v;
            }
        }
    }
    std::vector<double> mag ((size_t) n / 2);
    for (int k = 0; k < n / 2; ++k) mag[(size_t) k] = std::abs (x[(size_t) k]) * 2.0 / n;
    return mag;
}

// Frequency response from an impulse: gain in dB at each requested frequency.
inline std::vector<double> frequencyResponseDb (JGMasteringEQAudioProcessor& p, const std::vector<double>& freqs,
                                                double sr = 48000.0, int order = 16, int latency = 0)
{
    const int n = 1 << order;
    auto out = render (p, impulse (1, n, 0), { sr, 512 });
    juce::dsp::FFT fft (order);
    std::vector<float> data ((size_t) n * 2, 0.0f);
    for (int i = 0; i < n; ++i)
        data[(size_t) i] = out.getSample (0, (i + latency) % n);
    fft.performRealOnlyForwardTransform (data.data());
    std::vector<double> res;
    for (double f : freqs)
    {
        const double bin = f / sr * n;
        const int k = (int) std::floor (bin);
        const double t = bin - k;
        auto mag = [&] (int idx) { return std::hypot ((double) data[(size_t) (2 * idx)], (double) data[(size_t) (2 * idx + 1)]); };
        res.push_back (toDb (mag (k) * (1.0 - t) + mag (k + 1) * t));
    }
    return res;
}

inline std::vector<double> logFrequencies (int count = 64, double lo = 20.0, double hi = 20000.0)
{
    std::vector<double> f;
    for (int i = 0; i < count; ++i)
        f.push_back (lo * std::pow (hi / lo, i / double (count - 1)));
    return f;
}

// Total harmonic distortion (+noise excluded) of a steady sine, in dB relative
// to the fundamental. Harmonics 2..maxHarmonic below Nyquist.
inline double thdDb (const juce::AudioBuffer<float>& out, double sr, double hz, int maxHarmonic = 10)
{
    const int order = kAnalysisOrder;
    const auto mag = spectrum (out, 0, out.getNumSamples() - (1 << order), order);
    const int n = 1 << order;
    auto peakNear = [&] (double f)
    {
        const int c = (int) std::round (f / sr * n);
        double m = 0.0;
        for (int k = std::max (1, c - 1); k <= std::min ((int) mag.size() - 1, c + 1); ++k)
            m = std::max (m, mag[(size_t) k]);
        return m;
    };
    const double fund = peakNear (hz);
    double sum = 0.0;
    for (int h = 2; h <= maxHarmonic && h * hz < sr / 2 - 100.0; ++h)
        sum += std::pow (peakNear (h * hz), 2.0);
    return toDb (std::sqrt (sum) / std::max (fund, 1e-30));
}

// Energy that isn't the fundamental or one of its harmonics (e.g. aliasing
// folded back from saturation), in dB relative to the fundamental.
inline double nonHarmonicDb (const juce::AudioBuffer<float>& out, double sr, double hz)
{
    const int order = kAnalysisOrder, n = 1 << order;
    const auto mag = spectrum (out, 0, out.getNumSamples() - n, order);
    const double binHz = sr / n;
    double fund = 0.0, worst = 0.0;
    for (size_t k = 2; k < mag.size(); ++k)
    {
        const double f = k * binHz;
        const double h = f / hz;
        const bool nearHarmonic = std::abs (h - std::round (h)) * hz < 1.5 * binHz && std::round (h) >= 1;
        if (std::abs (f - hz) < 1.5 * binHz) fund = std::max (fund, mag[k]);
        else if (! nearHarmonic && f > 20.0) worst = std::max (worst, mag[k]);
    }
    return toDb (worst / std::max (fund, 1e-30));
}

// Clicks/zipper noise: the largest jump in the second difference of the
// output, compared with the same measure on the input. Smooth automation keeps
// this close to the input's value.
inline double worstSecondDifference (const juce::AudioBuffer<float>& b)
{
    double m = 0.0;
    for (int ch = 0; ch < b.getNumChannels(); ++ch)
        for (int i = 2; i < b.getNumSamples(); ++i)
            m = std::max (m, std::abs ((double) b.getSample (ch, i) - 2.0 * b.getSample (ch, i - 1) + b.getSample (ch, i - 2)));
    return m;
}

// ---------------------------------------------------------------------------
// Goldens: stored references in tests/golden/. Set JGEQ_UPDATE_GOLDENS=1 to
// (re)write them after a change Josh has approved by ear.
// ---------------------------------------------------------------------------
inline juce::File goldenDir() { return juce::File (JGEQ_GOLDEN_DIR); }
inline bool updatingGoldens() { return juce::SystemStats::getEnvironmentVariable ("JGEQ_UPDATE_GOLDENS", {}) == "1"; }

// Compares a response curve (one value per line) with a stored golden file.
// Returns the largest deviation in dB; writes the file when updating.
inline double compareCurveGolden (const juce::String& name, const std::vector<double>& curve)
{
    auto f = goldenDir().getChildFile (name + ".txt");
    if (updatingGoldens() || ! f.existsAsFile())
    {
        juce::StringArray lines;
        for (double v : curve) lines.add (juce::String (v, 4));
        f.getParentDirectory().createDirectory();
        f.replaceWithText (lines.joinIntoString ("\n") + "\n");
        return 0.0;
    }
    juce::StringArray lines;
    lines.addLines (f.loadFileAsString());
    lines.removeEmptyStrings();
    if ((size_t) lines.size() != curve.size())
        return 1.0e9;
    double worst = 0.0;
    for (size_t i = 0; i < curve.size(); ++i)
        worst = std::max (worst, std::abs (curve[i] - lines[(int) i].getDoubleValue()));
    return worst;
}

inline bool writeWav (const juce::File& f, const juce::AudioBuffer<float>& b, double sr)
{
    f.deleteFile();
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::OutputStream> os (f.createOutputStream().release());
    if (os == nullptr) return false;
    auto opts = juce::AudioFormatWriterOptions{}.withSampleRate (sr).withNumChannels (b.getNumChannels()).withBitsPerSample (32)
                    .withSampleFormat (juce::AudioFormatWriterOptions::SampleFormat::floatingPoint);
    auto w = wav.createWriterFor (os, opts);
    return w != nullptr && w->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
}

inline juce::AudioBuffer<float> readWav (const juce::File& f, double* srOut = nullptr)
{
    juce::AudioFormatManager fm;
    fm.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> r (fm.createReaderFor (f));
    juce::AudioBuffer<float> b;
    if (r == nullptr) return b;
    b.setSize ((int) r->numChannels, (int) r->lengthInSamples);
    r->read (&b, 0, (int) r->lengthInSamples, 0, true, true);
    if (srOut) *srOut = r->sampleRate;
    return b;
}

// Renders `input` and compares with tests/golden/<name>.wav. Returns the null
// depth in dB (-400 = identical). Writes the golden when updating.
inline double compareRenderGolden (const juce::String& name, const juce::AudioBuffer<float>& rendered, double sr)
{
    auto f = goldenDir().getChildFile (name + ".wav");
    if (updatingGoldens() || ! f.existsAsFile())
    {
        f.getParentDirectory().createDirectory();
        writeWav (f, rendered, sr);
        return -400.0;
    }
    auto ref = readWav (f);
    if (ref.getNumSamples() != rendered.getNumSamples() || ref.getNumChannels() != rendered.getNumChannels())
        return 0.0;
    return nullDepthDb (ref, rendered);
}
}
