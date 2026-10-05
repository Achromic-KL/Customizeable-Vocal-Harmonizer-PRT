#pragma once
#include <vector>
#include <cmath>

// YIN pitch detection algorithm (de Cheveigne & Kawahara, 2002).
// Chosen over basic autocorrelation because it's much more robust to
// octave errors on voice signals, which is what will bite you first
// if you try naive autocorrelation on a human voice.
class PitchDetector
{
public:
    PitchDetector (double sampleRateIn, int analysisSizeIn)
        : sampleRate (sampleRateIn), analysisSize (analysisSizeIn)
    {
        // We only search tau up to half the window; can't detect
        // periods longer than that with this window size.
        diff.resize (analysisSize / 2, 0.0f);
    }

    // Returns detected frequency in Hz, or -1.0f if no clear pitch found
    // (e.g. silence, noise, unvoiced consonant).
    float detectPitch (const float* buffer)
    {
        differenceFunction (buffer);
        cumulativeMeanNormalizedDifference();

        int tauEstimate = absoluteThreshold();
        if (tauEstimate == -1)
            return -1.0f;

        float betterTau = parabolicInterpolation (tauEstimate);
        if (betterTau <= 0.0f)
            return -1.0f;

        return (float) (sampleRate / betterTau);
    }

    void setThreshold (float newThreshold) { threshold = newThreshold; }

private:
    double sampleRate;
    int analysisSize;
    std::vector<float> diff;
    float threshold = 0.15f; // lower = stricter (fewer false positives, may miss quiet notes)

    // Step 1: difference function d(tau) = sum((x[i] - x[i+tau])^2)
    void differenceFunction (const float* buf)
    {
        const int n = (int) diff.size();
        for (int tau = 0; tau < n; ++tau)
        {
            float sum = 0.0f;
            for (int i = 0; i < n; ++i)
            {
                float delta = buf[i] - buf[i + tau];
                sum += delta * delta;
            }
            diff[(size_t) tau] = sum;
        }
    }

    // Step 2: cumulative mean normalized difference function.
    // This is what makes YIN better than raw autocorrelation-based
    // methods: it normalizes away the bias toward tau=0 and low tau values.
    void cumulativeMeanNormalizedDifference()
    {
        diff[0] = 1.0f;
        float runningSum = 0.0f;
        for (int tau = 1; tau < (int) diff.size(); ++tau)
        {
            runningSum += diff[(size_t) tau];
            diff[(size_t) tau] *= (float) tau / runningSum;
        }
    }

    // Step 3: find first tau where the normalized difference dips below
    // threshold (i.e. a strong periodicity candidate), then walk forward
    // to the local minimum.
    int absoluteThreshold()
    {
        for (int tau = 2; tau < (int) diff.size(); ++tau)
        {
            if (diff[(size_t) tau] < threshold)
            {
                while (tau + 1 < (int) diff.size() && diff[(size_t) (tau + 1)] < diff[(size_t) tau])
                    ++tau;
                return tau;
            }
        }
        return -1; // no periodicity found -> treat as unvoiced/silence
    }

    // Step 4: parabolic interpolation around the estimated tau to get
    // sub-sample precision. Without this, pitch estimates are quantized
    // to whole sample periods and sound noticeably out of tune.
    float parabolicInterpolation (int tauEstimate)
    {
        int x0 = (tauEstimate < 1) ? tauEstimate : tauEstimate - 1;
        int x2 = (tauEstimate + 1 < (int) diff.size()) ? tauEstimate + 1 : tauEstimate;

        if (x0 == tauEstimate) return (diff[(size_t) tauEstimate] <= diff[(size_t) x2]) ? (float) tauEstimate : (float) x2;
        if (x2 == tauEstimate) return (diff[(size_t) tauEstimate] <= diff[(size_t) x0]) ? (float) tauEstimate : (float) x0;

        float s0 = diff[(size_t) x0];
        float s1 = diff[(size_t) tauEstimate];
        float s2 = diff[(size_t) x2];

        float denom = (s0 - 2.0f * s1 + s2);
        if (std::abs (denom) < 1.0e-9f)
            return (float) tauEstimate;

        return (float) tauEstimate + 0.5f * (s0 - s2) / denom;
    }
};
