#pragma once

#include <cstdint>
#include <vector>

namespace melodee
{

/** Small deterministic PRNG (PCG32). Identical sequences on every platform and compiler,
    so a seed always reproduces the same melody. */
class Rng
{
public:
    explicit Rng (std::uint64_t seed = 0x853c49e6748fea9bULL) noexcept
    {
        state = 0;
        next();
        state += seed;
        next();
    }

    std::uint32_t next() noexcept
    {
        const std::uint64_t old = state;
        state = old * 6364136223846793005ULL + 1442695040888963407ULL;
        const auto xorshifted = static_cast<std::uint32_t> (((old >> 18u) ^ old) >> 27u);
        const auto rot = static_cast<std::uint32_t> (old >> 59u);
        return (xorshifted >> rot) | (xorshifted << ((~rot + 1u) & 31u));
    }

    /** Uniform double in [0, 1). */
    double uniform() noexcept { return (next() >> 8) * (1.0 / 16777216.0); }

    double uniform (double lo, double hi) noexcept { return lo + (hi - lo) * uniform(); }

    /** Uniform integer in [lo, hi] (inclusive). */
    int range (int lo, int hi) noexcept
    {
        if (hi <= lo)
            return lo;
        const auto span = static_cast<std::uint32_t> (hi - lo + 1);
        return lo + static_cast<int> (next() % span);
    }

    bool chance (double probability) noexcept { return uniform() < probability; }

    /** Picks an index with probability proportional to its weight. Returns -1 if all weights are zero. */
    int weighted (const std::vector<double>& weights) noexcept
    {
        double total = 0.0;
        for (double w : weights)
            total += w > 0.0 ? w : 0.0;

        if (total <= 0.0)
            return -1;

        double r = uniform() * total;
        for (size_t i = 0; i < weights.size(); ++i)
        {
            const double w = weights[i] > 0.0 ? weights[i] : 0.0;
            if (r < w)
                return static_cast<int> (i);
            r -= w;
        }

        for (size_t i = weights.size(); i-- > 0;)
            if (weights[i] > 0.0)
                return static_cast<int> (i);

        return -1;
    }

    template <typename T>
    const T& pick (const std::vector<T>& items) noexcept
    {
        return items[static_cast<size_t> (range (0, static_cast<int> (items.size()) - 1))];
    }

    template <typename T>
    void shuffle (std::vector<T>& items) noexcept
    {
        for (size_t i = items.size(); i > 1; --i)
        {
            const auto j = static_cast<size_t> (next() % static_cast<std::uint32_t> (i));
            std::swap (items[i - 1], items[j]);
        }
    }

private:
    std::uint64_t state;
};

/** Mixes a base seed with an index so variation N always gets the same seed. */
inline std::uint32_t mixSeed (std::uint32_t seed, std::uint32_t index) noexcept
{
    std::uint64_t z = (static_cast<std::uint64_t> (seed) << 32) ^ (index + 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    z ^= z >> 31;
    return static_cast<std::uint32_t> (z ^ (z >> 32));
}

} // namespace melodee
