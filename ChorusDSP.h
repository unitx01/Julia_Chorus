#pragma once
// ============================================================================
//  Caesar Chorus - DSP model
//
//  Signal flow (mirrors the PedalPCB schematic):
//
//    in --+--------------------------------------------- dry ---+
//         |                                                     |
//         +--> pre-filter --> BBD delay --> post-filter --> wet +--> Blend --> out
//                              ^
//                              | clock speed
//                   LFO (Rate, Shape) * Depth  around  Lag (base delay)
//
//  - Pre/post filters: the Sallen-Key lowpass stages around Q3 and Q5.
//  - BBD: V3207 (1024 stages) clocked by a V3102. Delay = stages / (2 * clock).
//    The LFO modulates the CLOCK FREQUENCY, not the delay directly, so the
//    delay sweep is slightly asymmetric - like the real chip.
//  - Shape: triangle, or a rounded "sine" made by soft-clipping the triangle
//    (the circuit does this with the D1/D2 LEDs).
//
//  This file has no JUCE dependency so it can be tested on its own.
// ============================================================================

#include <algorithm>
#include <cmath>
#include <vector>

namespace caesar
{

constexpr double kPi = 3.14159265358979323846;

// ---------------------------------------------------------------------------
// RBJ biquad lowpass (transposed direct form II)
// ---------------------------------------------------------------------------
struct Biquad
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    double z1 = 0, z2 = 0;

    void setLowpass (double sampleRate, double cutoffHz, double q)
    {
        cutoffHz = std::min (cutoffHz, 0.45 * sampleRate);
        const double w0 = 2.0 * kPi * cutoffHz / sampleRate;
        const double cw = std::cos (w0);
        const double alpha = std::sin (w0) / (2.0 * q);
        const double a0 = 1.0 + alpha;

        b0 = (1.0 - cw) * 0.5 / a0;
        b1 = (1.0 - cw) / a0;
        b2 = b0;
        a1 = -2.0 * cw / a0;
        a2 = (1.0 - alpha) / a0;
    }

    void reset() { z1 = z2 = 0.0; }

    float process (float x)
    {
        const double y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return static_cast<float> (y);
    }
};

// ---------------------------------------------------------------------------
// Circular delay line with 4-point Hermite interpolation
// ---------------------------------------------------------------------------
struct DelayLine
{
    std::vector<float> buffer;
    unsigned mask = 0;
    unsigned writePos = 0;

    void prepare (int minSamples)
    {
        unsigned size = 1;
        while (size < static_cast<unsigned> (minSamples))
            size <<= 1;
        buffer.assign (size, 0.0f);
        mask = size - 1;
        writePos = 0;
    }

    void reset()
    {
        std::fill (buffer.begin(), buffer.end(), 0.0f);
        writePos = 0;
    }

    void push (float x)
    {
        buffer[writePos] = x;
        writePos = (writePos + 1) & mask;
    }

    // Read the sample from 'delaySamples' ago (call after push). Must be >= 2.
    float read (double delaySamples) const
    {
        const double readPos = static_cast<double> (writePos) - 1.0 - delaySamples;
        const double floorPos = std::floor (readPos);
        const auto i = static_cast<long long> (floorPos);
        const float f = static_cast<float> (readPos - floorPos);

        auto at = [this] (long long k) { return buffer[static_cast<unsigned> (k) & mask]; };

        const float xm1 = at (i - 1), x0 = at (i), x1 = at (i + 1), x2 = at (i + 2);

        const float c1 = 0.5f * (x1 - xm1);
        const float c2 = xm1 - 2.5f * x0 + 2.0f * x1 - 0.5f * x2;
        const float c3 = 0.5f * (x2 - xm1) + 1.5f * (x0 - x1);
        return ((c3 * f + c2) * f + c1) * f + x0;
    }
};

// ---------------------------------------------------------------------------
// One-pole parameter smoother (prevents zipper noise when knobs move)
// ---------------------------------------------------------------------------
struct Smoother
{
    double coeff = 0.0, value = 0.0;

    void prepare (double sampleRate, double timeMs, double initial)
    {
        coeff = std::exp (-1.0 / (0.001 * timeMs * sampleRate));
        value = initial;
    }

    double next (double target)
    {
        value = target + coeff * (value - target);
        return value;
    }
};

// ---------------------------------------------------------------------------
// The pedal
// ---------------------------------------------------------------------------
class ChorusEngine
{
public:
    struct Params
    {
        float rateHz = 1.0f; // Rate knob
        float depth  = 0.5f; // Depth knob, 0..1
        float lag    = 0.3f; // Lag knob, 0..1 (base delay time)
        float blend  = 0.5f; // Blend knob, 0 = dry, 1 = wet (vibrato)
        bool  sine   = false; // Shape switch
    };

    // ---- Tunable constants (adjust these to taste) ----
    static constexpr int    kBbdStages      = 1024;  // V3207
    static constexpr double kMinDelayMs     = 1.5;   // Lag fully CCW
    static constexpr double kMaxDelayMs     = 12.0;  // Lag fully CW
    static constexpr double kMaxClockMod    = 0.5;   // Depth fully CW: clock swings +/-50%
    static constexpr double kPreCutoffHz    = 7000.0;
    static constexpr double kPostCutoffHz   = 6500.0;
    static constexpr double kFilterQ        = 0.85;
    static constexpr double kSineRounding   = 1.8;   // higher = rounder "sine"
    static constexpr float  kWetDrive       = 1.15f; // gentle BBD-style saturation

    void prepare (double newSampleRate, int numChannels)
    {
        sampleRate = newSampleRate;
        channels.assign (static_cast<size_t> (std::max (1, numChannels)), Channel{});

        // Longest possible delay: max base delay with the clock slowed to (1 - kMaxClockMod)
        const double maxDelaySec = (kMaxDelayMs / 1000.0) / (1.0 - kMaxClockMod);
        const int delaySize = static_cast<int> (maxDelaySec * sampleRate) + 16;

        for (auto& ch : channels)
        {
            ch.pre.setLowpass (sampleRate, kPreCutoffHz, kFilterQ);
            ch.post.setLowpass (sampleRate, kPostCutoffHz, kFilterQ);
            ch.delay.prepare (delaySize);
        }

        depthSmooth.prepare (sampleRate, 30.0, 0.5);
        lagSmooth.prepare (sampleRate, 60.0, 0.3);
        blendSmooth.prepare (sampleRate, 30.0, 0.5);
        lfoPhase = 0.0;
        needsSnap = true;
    }

    void reset()
    {
        for (auto& ch : channels)
        {
            ch.pre.reset();
            ch.post.reset();
            ch.delay.reset();
        }
        lfoPhase = 0.0;
        needsSnap = true;
    }

    void process (float* const* audio, int numChannels, int numSamples, const Params& p)
    {
        numChannels = std::min (numChannels, static_cast<int> (channels.size()));
        const double phaseInc = p.rateHz / sampleRate;
        const double sineNorm = std::tanh (kSineRounding);

        // Jump straight to the knob positions on the first block (no fade-in from defaults)
        if (needsSnap)
        {
            depthSmooth.value = p.depth;
            lagSmooth.value   = p.lag;
            blendSmooth.value = p.blend;
            needsSnap = false;
        }

        for (int n = 0; n < numSamples; ++n)
        {
            // ---- LFO ----
            double lfo = 4.0 * std::abs (lfoPhase - 0.5) - 1.0; // triangle, -1..1
            if (p.sine)
                lfo = std::tanh (kSineRounding * lfo) / sineNorm;

            lfoPhase += phaseInc;
            if (lfoPhase >= 1.0)
                lfoPhase -= 1.0;

            // ---- Smoothed controls ----
            const double depth = depthSmooth.next (p.depth);
            const double lag   = lagSmooth.next (p.lag);
            const double blend = blendSmooth.next (p.blend);

            // ---- BBD clock -> delay time ----
            // Lag sets the base delay; the matching clock is stages / (2 * delay).
            // The LFO speeds up / slows down that clock, and delay follows 1/clock.
            const double baseDelaySec = (kMinDelayMs + lag * (kMaxDelayMs - kMinDelayMs)) / 1000.0;
            const double baseClockHz  = kBbdStages / (2.0 * baseDelaySec);
            const double clockHz      = baseClockHz * (1.0 + kMaxClockMod * depth * lfo);
            const double delaySamples = std::max (2.0, (kBbdStages / (2.0 * clockHz)) * sampleRate);

            const float dryGain = static_cast<float> (1.0 - blend);
            const float wetGain = static_cast<float> (blend);

            for (int c = 0; c < numChannels; ++c)
            {
                auto& ch = channels[static_cast<size_t> (c)];
                const float dry = audio[c][n];

                ch.delay.push (ch.pre.process (dry));
                float wet = ch.post.process (ch.delay.read (delaySamples));
                wet = std::tanh (kWetDrive * wet) / kWetDrive;

                audio[c][n] = dryGain * dry + wetGain * wet;
            }
        }
    }

private:
    struct Channel
    {
        Biquad pre, post;
        DelayLine delay;
    };

    std::vector<Channel> channels;
    double sampleRate = 44100.0;
    double lfoPhase = 0.0;
    bool needsSnap = true;
    Smoother depthSmooth, lagSmooth, blendSmooth;
};

} // namespace caesar
