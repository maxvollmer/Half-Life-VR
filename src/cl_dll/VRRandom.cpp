
#include "VRRandom.h"

#include <random>

namespace
{
    std::random_device random_device;

    std::default_random_engine seeder{ random_device() };

    std::default_random_engine rng{ random_device() };

    std::default_random_engine rngbackup = rng;

    std::uniform_real_distribution<float> floatdistribution(0.f, 1.f);
}

void VRRandomResetSeed(unsigned int seed/* = 0*/)
{
    if (seed)
    {
        rng.seed(seed);
    }
    else
    {
        static std::uniform_int_distribution<long> distribution((std::random_device::min)(), (std::random_device::max)());
        rng.seed(distribution(seeder));
    }
}

void VRRandomBackupSeed()
{
    rngbackup = rng;
}

void VRRandomRestoreSeed()
{
    rng = rngbackup;
}

float VRRandomFloat(float low, float high)
{
    return floatdistribution(rng) * (high - low) + low;
}

long VRRandomLong(long low, long high)
{
    return static_cast<long>(floatdistribution(rng) * (high + 1 - low)) + low;
}
