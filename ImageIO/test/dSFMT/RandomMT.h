#pragma once
#ifndef PCG_RANDOMMT_H
#define PCG_RANDOMMT_H

// Helper wrapper class to use dSFMT
class RandomMT
{
public:
    RandomMT();
    RandomMT(unsigned int seed) : m_data(0), m_haveNextNextGaussian(false) {
        init (&seed, 1);
    }
    RandomMT(const unsigned int seed_data[], size_t len) : 
    m_data(0), m_haveNextNextGaussian(false)  {
        init (seed_data, len);
    }

    template <size_t N>
    RandomMT(const unsigned int (&init_data)[N]) : 
    m_data(0), m_haveNextNextGaussian(false) {
        init (seed_data, N);
    }

    virtual ~RandomMT();


    void setSeed(unsigned int seed);
    void setSeed(const unsigned int seed_data[], size_t len);
    template <size_t N>
    void setSeed(const unsigned int (&seed_data)[N]) {
        setSeed(seed_data, N);
    }


    bool nextBoolean();
    double nextDouble();
    float nextFloat();
    double nextGaussian();
    unsigned int nextInt(unsigned int n = 0xFFFFFFFF);

private:

    // State data
    struct Data;
    Data* m_data;

    double m_nextNextGaussian;
    bool m_haveNextNextGaussian;

    void init(const unsigned int seed_data[], size_t len);
};

#endif /* PCG_RANDOMMT_H */
