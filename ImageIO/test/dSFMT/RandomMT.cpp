#include "RandomMT.h"

// Hard-coded Mersenne Twister definitions
#define DSFMT_DO_NOT_USE_OLD_NAMES
#define DHAVE_SSE2 1 
#define DSFMT_MEXP 19937

#ifdef __cplusplus
extern "C" {
#endif
#include "dSFMT.h"
#ifdef __cplusplus
}
#endif

#include <cassert>
#include <ctime>
#include <cmath>


// Actual helper struct definition
struct RandomMT::Data
{
public:

    dsfmt_t& operator() () {
        return st;
    }

    const dsfmt_t& operator() () const {
        return st;
    }

    dsfmt_t* ptr() {
        return &st;
    }

private:
    // Mersenne Twister state
    dsfmt_t st;
};



void
RandomMT::init(const unsigned int seed_data[], size_t len)
{
    assert (m_data == NULL);
    m_data = new Data;
    assert (m_data != NULL);

    dsfmt_init_by_array (m_data->ptr(), 
        const_cast<unsigned int *>(seed_data), static_cast<int>(len));
}



RandomMT::RandomMT() : m_data(NULL), m_haveNextNextGaussian(false)
{
    clock_t clk = clock();
    init (reinterpret_cast<const unsigned int *>(&clk), 
        sizeof(clk)/sizeof(unsigned int));
}



RandomMT::~RandomMT()
{
    if (m_data) {
        delete m_data;
        m_data = NULL;
    }
}



void
RandomMT::setSeed(unsigned int seed)
{
    assert (m_data != NULL);
    dsfmt_init_gen_rand (m_data->ptr(), seed);
}



void
RandomMT::setSeed(const unsigned int seed_data[], size_t len)
{
    assert (m_data != NULL);
    dsfmt_init_by_array (m_data->ptr(), 
        const_cast<unsigned int *>(seed_data), static_cast<int>(len));
}



bool
RandomMT::nextBoolean()
{
    assert (m_data != NULL);
    return dsfmt_genrand_close_open(m_data->ptr()) > 0.5;
}



double
RandomMT::nextDouble()
{
    assert (m_data != NULL);
    return dsfmt_genrand_close_open(m_data->ptr());
}



float
RandomMT::nextFloat()
{
    assert (m_data != NULL);
    return static_cast<float>(dsfmt_genrand_close_open(m_data->ptr()));
}



double
RandomMT::nextGaussian()
{
    assert (m_data != NULL);

    // Implementation from 
    //http://java.sun.com/javase/6/docs/api/java/util/Random.html#nextGaussian()
    if (m_haveNextNextGaussian) {
        m_haveNextNextGaussian = false;
        return m_nextNextGaussian;
    } else {
        double v[2]; double s;
        do {
            v[0] = 2 * nextDouble() - 1;   // between -1.0 and 1.0
            v[1] = 2 * nextDouble() - 1;   // between -1.0 and 1.0
            s = v[0] * v[0] + v[1] * v[1];
        } while (s >= 1 || s == 0);
        double multiplier = sqrt(-2 * log(s)/s);
        m_nextNextGaussian = v[1] * multiplier;
        m_haveNextNextGaussian = true;
        return v[0] * multiplier;
    }
}



unsigned int
RandomMT::nextInt(unsigned int n)
{
    assert (m_data != NULL);
    const double v = dsfmt_genrand_close_open(m_data->ptr());
    assert ((unsigned int) (v * n) < n);
    return (unsigned int) (v * n);
}
