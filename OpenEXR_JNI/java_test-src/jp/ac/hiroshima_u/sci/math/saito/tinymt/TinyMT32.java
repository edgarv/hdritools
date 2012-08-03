// Copyright (c) 2012 Mutsuo Saito, Makoto Matsumoto, Hiroshima
// University and The University of Tokyo. All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// 
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//     * Neither the name of the Hiroshima University, The University of
//       Tokyo nor the names of its contributors may be used to endorse
//       or promote products derived from this software without specific
//       prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

package jp.ac.hiroshima_u.sci.math.saito.tinymt;

import java.io.IOException;
import java.math.BigInteger;

/**
 * TinyMT is a pseudo random number generator.
 * <p>
 * To get an instance, call {@link TinyMT32#getDefault()},
 * {@link TinyMT32#getDefault(long)}, {@link TinyMT32#getDefault(String)}
 * or {@link TinyMT32#getDefault(int[])}.
 * </p>
 * <p>
 * This class is <strong>not synchronized</strong>, One way to use TinyMT in
 * concurrent environment is to use an instance per a thread.
 * </p>
 * <p>
 * This class supports jump function. User can get an array of pseudo random
 * number generators by calling
 * {@link TinyMT32#getDefaultArray(int, long, long)}, or
 * {@link TinyMT32#getDefaultArray(int, String, long)}.
 * </p>
 * <p>
 * This class supports discrete characteristic polynomial generators. User can
 * get an array of pseudo random number generators whose characteristic
 * polynomials are discrete by calling
 * {@link TinyMT32#getTinyMTArray(int, long)}.
 * </p>
 * @author M. Saito
 * @see <a href=
 *      "http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/TINYMT/index.html">
 *      TinyMT web page</a>
 */
public final class TinyMT32 extends java.util.Random implements Cloneable {
    /** bit size of int. */
    private static final int INT_SIZE = 32;
    /** least long over int. */
    private static final long LONG_LIMIT = 0x100000000L;
    /** initialize shift. */
    private static final int INITIALIZE_SHIFT = 27;
    /** initialize shift. */
    private static final int INITIALIZE_SHIFT2 = 30;
    /** magic number. */
    private static final int MAGIC_NUMBER1 = 1664525;
    /** magic number. */
    private static final int MAGIC_NUMBER2 = 1566083941;
    /** magic number. */
    private static final int MAGIC_NUMBER3 = 1812433253;
    /** int to float shift. */
    private static final int INT_TO_FLOAT_SHIFT = 9;
    /** long to double shift. */
    private static final int LONG_TO_DOUBLE_SHIFT = 12;
    /** hexadecimal base. */
    private static final int HEXA_DECIMAL_BASE = 16;
    /** int to unsigned long mask. */
    private static final long INT_TO_LONG_MASK = 0xffffffffL;
    /** long to double mask. */
    private static final long LONG_TO_DOUBLE_MASK = 0x3ff0000000000000L;
    /** basic jump step.
     * every jump step is a multiple of this step.
     */
    private static final BigInteger BASIC_JUMP_STEP 
        = new BigInteger("2").pow(64);
    /** mask pattern to limit internal size. */
    private static final int MASK = 0x7fffffff;
    /** fixed shift 0. */
    private static final int SH0 = 1;
    /** fixed shift 1. */
    private static final int SH1 = 10;
    /** fixed 8 bit shift. */
    private static final int SH8 = 8;
    /** pre loop before generation. */
    private static final int MIN_LOOP = 8;
    /** internal state 0. */
    private int st0;
    /** internal state 1. */
    private int st1;
    /** internal state 2. */
    private int st2;
    /** internal state 3. */
    private int st3;
    /** parameters for this generator. */
    private final TinyMT32Parameter parameter;
    
    /**
     * Cached random normal value.  The default implementation for
     * {@link #nextGaussian} generates pairs of values and this field caches the
     * second value so that the full algorithm is not executed for every
     * activation.  The value {@code Double.NaN} signals that there is
     * no cached value.  Use {@link #clear} to clear the cached value.
     */
    private double cachedNormalDeviate = Double.NaN;

    /**
     * Constructor from a parameter.
     * 
     * @param param
     *            a parameter generated by TinyMTDC
     */
    private TinyMT32(final TinyMT32Parameter param) {
        this.parameter = param;
    }

    /**
     * Copy constructor.
     * 
     * @param that
     *            source
     */
    private TinyMT32(final TinyMT32 that) {
        this.parameter = that.parameter;
        this.st0 = that.st0;
        this.st1 = that.st1;
        this.st2 = that.st2;
        this.st3 = that.st3;
    }

    /**
     * returns 32-bit integer.
     * @return next int
     */
    @Override
    public int nextInt() {
        nextState();
        return output();
    }
    
    /**
     * Returns a pseudorandom, uniformly distributed {@code int} value between
     * 0 (inclusive) and the specified value (exclusive), drawn from this
     * random number generator's sequence. This used the same method as
     * {@link java.util.Random#next(int) }.
     * 
     * @param n the bound on the random number to be returned. Must be positive
     * @return the next pseudorandom, uniformly distributed {@code int} value
     *         between 0 (inclusive) and the specified value (exclusive), drawn
     *         from this random number generator's sequence.
     * @throws IllegalArgumentException if n is not positive
     */
    @Override
    public int nextInt(int n) {
        if (n <= 0)
            throw new IllegalArgumentException("n must be positive");

        if ((n & -n) == n)  // i.e., n is a power of 2
            return (int)((n * (long)(nextInt() & 0x7fffffff)) >> 31);

        int bits, val;
        do {
            bits = nextInt() & 0x7fffffff;
            val = bits % n;
        } while (bits - val + (n-1) < 0);
        return val;
    }

    /**
     * returns 64-bit integer.
     * @return next long
     */
    @Override
    public long nextLong() {
        long x = nextInt();
        x = x << INT_SIZE;
        x |= nextInt() & INT_TO_LONG_MASK;
        return x;
    }

    /**
     * initialize internal state by seed.
     * @param seed seed of randomness
     */
    @Override
    public void setSeed(final long seed) {
        if ((seed >= 0) && (seed < LONG_LIMIT)) {
            setSeed((int) seed);
        } else {
            int[] tmp = new int[2];
            tmp[0] = (int) (seed & 0xffffffff);
            tmp[1] = (int) (seed >>> INT_SIZE);
            setSeed(tmp);
        }
    }

    /**
     * seeding by string, This will be convenient. This function does not
     * compatible from other language implementation because coding of
     * characters are different.
     * 
     * @param seed
     *            seed of pseudo random numbers
     */
    public void setSeed(final String seed) {
        int[] intSeeds = new int[seed.length()];
        for (int i = 0; i < intSeeds.length; i++) {
            intSeeds[i] = seed.charAt(i);
        }
        setSeed(intSeeds);
    }

    /**
     * seeding by array of integers. This seeding is compatible with other
     * language implementation.
     * 
     * @param seeds
     *            seeds of pseudo random numbers.
     */
    public void setSeed(final int[] seeds) {
        //  Needed if this is a subclass of java.util.Random. (Before Java 7)  
        if (parameter == null) {
            return;
        }
        
        final int lag = 1;
        final int mid = 1;
        final int size = 4;
        int i, j;
        int count;
        int r;
        int keyLength = seeds.length;
        int[] status = { 0,
                parameter.getMat1(),
                parameter.getMat2(),
                parameter.getTmat() };
        if (keyLength + 1 > MIN_LOOP) {
            count = keyLength + 1;
        } else {
            count = MIN_LOOP;
        }
        r = iniFunc1(status[0] ^ status[mid % size]
                    ^ status[(size - 1) % size]);
        status[mid % size] += r;
        r += keyLength;
        status[(mid + lag) % size] += r;
        status[0] = r;
        count--;
        for (i = 1, j = 0; (j < count) && (j < keyLength); j++) {
            r = iniFunc1(status[i % size] ^ status[(i + mid) % size]
                    ^ status[(i + size - 1) % size]);
            status[(i + mid) % size] += r;
            r += seeds[j] + i;
            status[(i + mid + lag) % size] += r;
            status[i % size] = r;
            i = (i + 1) % size;
        }
        for (; j < count; j++) {
            r = iniFunc1(status[i % size] ^ status[(i + mid) % size]
                    ^ status[(i + size - 1) % size]);
            status[(i + mid) % size] += r;
            r += i;
            status[(i + mid + lag) % size] += r;
            status[i % size] = r;
            i = (i + 1) % size;
        }
        for (j = 0; j < size; j++) {
            r = iniFunc2(status[i % size] + status[(i + mid) % size]
                    + status[(i + size - 1) % size]);
            status[(i + mid) % size] ^= r;
            r -= i;
            status[(i + mid + lag) % size] ^= r;
            status[i % size] = r;
            i = (i + 1) % size;
        }
        st0 = status[0];
        st1 = status[1];
        st2 = status[2];
        st3 = status[3];
        periodCertification();
        for (i = 0; i < MIN_LOOP; i++) {
            nextState();
        }
        clear();
    }

    /**
     * sub function of initialization.
     * 
     * @param x
     *            input number
     * @return scrambled integer
     */
    private int iniFunc1(final int x) {
        return (x ^ (x >>> INITIALIZE_SHIFT)) * MAGIC_NUMBER1;
    }

    /**
     * sub function of initialization.
     * 
     * @param x
     *            input number
     * @return scrambled integer
     */
    private int iniFunc2(final int x) {
        return (x ^ (x >>> INITIALIZE_SHIFT)) * MAGIC_NUMBER2;
    }

    /**
     * internal set seed function This seeding is compatible with C language
     * implementation.
     * 
     * @param seed
     *            seed of pseudo random numbers
     */
    public void setSeed(final int seed) {
        int counterMask = 3;
        int[] status = new int[4];
        status[0] = seed;
        status[1] = parameter.getMat1();
        status[2] = parameter.getMat2();
        status[3] = parameter.getTmat();
        for (int i = 1; i < MIN_LOOP; i++) {
            status[i & counterMask] ^= i + MAGIC_NUMBER3
                    * (status[(i - 1) & counterMask]
                            ^ (status[(i - 1) & counterMask]
                                    >>> INITIALIZE_SHIFT2));
        }
        st0 = status[0];
        st1 = status[1];
        st2 = status[2];
        st3 = status[3];
        periodCertification();
        for (int i = 0; i < MIN_LOOP; i++) {
            nextState();
        }
        clear();
    }

    /**
     * Avoiding all zero status is sufficient for certificating the period of
     * 2<sup>127</sup> - 1 for TinyMT.
     */
    private void periodCertification() {
        if (((st0 & MASK) == 0) && (st1 == 0) && (st2 == 0) && (st3 == 0)) {
            st0 = 'T';
            st1 = 'I';
            st2 = 'N';
            st3 = 'Y';
        }
    }

    /**
     * The state transition function. This function is F<sub>2</sub>-linear.
     */
    private void nextState() {
        int x;
        int y;
        y = st3;
        x = (st0 & MASK) ^ st1 ^ st2;
        x ^= (x << SH0);
        y ^= (y >>> SH0) ^ x;
        st0 = st1;
        st1 = st2;
        st2 = x ^ (y << SH1);
        st3 = y;
        st1 ^= parameter.getMat1(y);
        st2 ^= parameter.getMat2(y);
    }

    /**
     * The output function.
     * 
     * @return pseudo random number
     */
    private int output() {
        int t0;
        int t1;
        t0 = st3;
        t1 = st0 + (st2 >>> SH8);
        t0 ^= t1;
        t0 ^= parameter.getTmat(t1);
        return t0;
    }

    /**
     * make float random.
     * @return float output.
     */
    private float outputFloat() {
        int t0;
        int t1;
        t0 = st3;
        t1 = st0 + (st2 >>> SH8);
        t0 ^= t1;
        t0 = (t0 >>> INT_TO_FLOAT_SHIFT) ^ parameter.getTmatFloat(t1);
        return Float.intBitsToFloat(t0) - 1.0f;
    }

    /**
     * Addition as F<sub>2</sub> vector.
     * 
     * @param that
     *            vector which added to this vector
     */
    private void add(final TinyMT32 that) {
        this.st0 ^= that.st0;
        this.st1 ^= that.st1;
        this.st2 ^= that.st2;
        this.st3 ^= that.st3;
    }

    /**
     * Factory method which returns the TinyMT with the first generated
     * parameter of TinyMTDC.
     * 
     * @param seed
     *            seed of pseudo random numbers.
     * @return TinyMT with the first parameter.
     */
    public static TinyMT32 getDefault(final String seed) {
        TinyMT32Parameter defaultParameter = TinyMT32Parameter
                .getDefaultParameter();
        TinyMT32 tiny = new TinyMT32(defaultParameter);
        tiny.setSeed(seed);
        return tiny;
    }

    /**
     * Factory method which returns the TinyMT with the first generated
     * parameter of TinyMTDC.
     * 
     * @param seed
     *            seed of pseudo random numbers.
     * @return TinyMT with the first parameter.
     */
    public static TinyMT32 getDefault(final long seed) {
        TinyMT32Parameter defaultParameter = TinyMT32Parameter
                .getDefaultParameter();
        TinyMT32 tiny = new TinyMT32(defaultParameter);
        tiny.setSeed(seed);
        return tiny;
    }

    /**
     * get default TinyMT32 with seeding by array.
     * @param seeds seeds for initialization.
     * @return random number generator TinyMT32
     */
    public static TinyMT32 getDefault(final int[] seeds) {
        TinyMT32Parameter defaultParameter = TinyMT32Parameter
                .getDefaultParameter();
        TinyMT32 tiny = new TinyMT32(defaultParameter);
        tiny.setSeed(seeds);
        return tiny;
    }

    /**
     * Factory method which returns the TinyMT with the first generated
     * parameter of TinyMTDC. {@link System#nanoTime()} and
     * {@link Thread#getId()} are used for seed.
     * 
     * @return TinyMT with the first parameter.
     */
    public static TinyMT32 getDefault() {
        int[] seed = new int[4];
        long time = System.nanoTime();
        long threadId = Thread.currentThread().getId();
        seed[0] = (int) (time >>> INT_SIZE);
        seed[1] = (int) time;
        seed[2] = (int) (threadId >>> INT_SIZE);
        seed[3] = (int) threadId;
        TinyMT32Parameter defaultParameter = TinyMT32Parameter
                .getDefaultParameter();
        TinyMT32 tiny = new TinyMT32(defaultParameter);
        tiny.setSeed(seed);
        return tiny;
    }

    /**
     * jump function.
     * 
     * @param pol
     *            jump polynomial
     * @return jumped new TinyMT
     */
    private TinyMT32 jump(final F2Polynomial pol) {
        TinyMT32 src = new TinyMT32(this);
        TinyMT32 that = getZero();
        int degree = pol.degree();
        for (int i = 0; i <= degree; i++) {
            if (pol.getCoefficient(i) == 1) {
                that.add(src);
            }
            src.nextState();
        }
        return that;
    }

    /**
     * Make and return all zero status.
     * 
     * @return all zero status
     */
    private TinyMT32 getZero() {
        TinyMT32 that = new TinyMT32(this);
        that.st0 = 0;
        that.st1 = 0;
        that.st2 = 0;
        that.st3 = 0;
        return that;
    }

    /**
     * make and return an array of TinyMT. Each element has the same
     * characteristic polynomial with TinyMT gotten by getDefaultMT. Especially,
     * the first element is just same as default TinyMT. The second element has
     * the state of <b>jump</b> * 2<sup>64</sup> steps after the first element.
     * In other word, the first element will generate the same sequence with the
     * second element, after <b>jump</b> * 2<sup>64</sup> pseudo random number
     * generation.
     * 
     * @param count
     *            number of TinyMT to be created.
     * @param seed
     *            seed of first element
     * @param jump
     *            step is jump * 2<sup>64</sup>
     * @return array of TinyMT
     */
    public static TinyMT32[] getDefaultArray(final int count, final long seed,
            final long jump) {
        TinyMT32 tiny = getDefault(seed);
        return tiny.getJumpedArray(count, jump);
    }

    /**
     * Make and return an array of TinyMT. Each element has the same
     * characteristic polynomial with TinyMT gotten by getDefaultMT. Especially,
     * the first element is just same as default TinyMT. The second element has
     * the state of <b>jump</b> * 2<sup>64</sup> steps after the first element.
     * In other word, the first element will generate the same sequence with the
     * second element, after <b>jump</b> * 2<sup>64</sup> pseudo random number
     * generation.
     * 
     * This is equals to TinyMT32.getDefault(seed).getJumpedArray(count, jump);
     * 
     * @param count
     *            number of TinyMT to be created.
     * @param seed
     *            seed of first element
     * @param jump
     *            step is jump * 2<sup>64</sup>
     * @return array of TinyMT
     */
    public static TinyMT32[] getDefaultArray(final int count,
            final String seed, final long jump) {
        TinyMT32 tiny = getDefault(seed);
        return tiny.getJumpedArray(count, jump);
    }

    /**
     * Make and return an array of TinyMT. Each element of the array has the
     * same characteristic polynomial with this. Especially, the first element
     * is just same as this. The second element has the state of <b>jump</b> *
     * 2<sup>64</sup> steps after the first element. In other word, the first
     * element will generate the same sequence with the second element, after
     * <b>jump</b> * 2<sup>64</sup> pseudo random number generation.
     * 
     * <p>
     * Note: Do not call any setSeed methods after jump. Seeding will cancel
     * the effect of jump.
     * </p>
     * @param count number of arrays 
     * @param jump jump step
     * @return jumped array of TinyMT32.
     */
    public TinyMT32[] getJumpedArray(final int count, final long jump) {
        TinyMT32[] tiny = new TinyMT32[count];
        tiny[0] = this;
        final F2Polynomial poly = tiny[0].parameter.getCharacteristic();
        final BigInteger pow = BASIC_JUMP_STEP.multiply(
                new BigInteger(Long.toString(jump)));
        final F2Polynomial jumpPoly = F2Polynomial.X.powerMod(pow, poly);
        for (int i = 1; i < count; i++) {
            tiny[i] = tiny[i - 1].jump(jumpPoly);
        }
        return tiny;
    }

    /**
     * Make and return an array of TinyMT. Each element of the array has
     * discrete characteristic polynomial. This function does not return
     * defaultTinyMT. User can call {@link TinyMT32#getJumpedArray(int, long)}
     * using each element and can get more TinyMTs.
     * <p>
     * Calling setSeed for each element is O.K and is a nice idea
     * to assure independency. 
     * </p>
     * 
     * @param count
     *            number of TinyMT you want.
     * @param seed
     *            seed of each element
     * @return array of TinyMT, length may be smaller than {@code count}
     * @throws IOException when can't read resource file.
     */
    public static TinyMT32[] getTinyMTArray(final int count, final long seed)
            throws IOException {
        TinyMT32Parameter[] params = TinyMT32Parameter.getParameters(count);
        TinyMT32[] tiny = new TinyMT32[params.length];
        for (int i = 0; i < params.length; i++) {
            tiny[i] = new TinyMT32(params[i]);
            tiny[i].setSeed(seed);
        }
        return tiny;
    }

    /**
     * return TinyMT32 instance whose parameter has ID = 1.
     * @param threadId thread ID
     * @return TinyMT32 instance
     */
    static TinyMT32 getThreadLlocal(final long threadId) {
        return new TinyMT32(TinyMT32Parameter
                    .getThreadLocalParameter(threadId));
    }

    /**
     * returns double r, 0 <= r < 1.0.
     * @return next double 
     */
    @Override
    public double nextDouble() {
        long x = (nextLong() >>> LONG_TO_DOUBLE_SHIFT) | LONG_TO_DOUBLE_MASK;
        return Double.longBitsToDouble(x) - 1.0;
    }

    /**
     * returns float r, 0 <= r < 1.0.
     * @return next float
     */
    @Override
    public float nextFloat() {
        nextState();
        return outputFloat();
    }
    
    /**
     * Returns the next pseudorandom, Gaussian ("normally") distributed
     * {@code double} value with mean {@code 0.0} and standard
     * deviation {@code 1.0} from this random number generator's sequence.
     * <p>
     * The default implementation uses the <em>Polar Method</em>
     * due to G.E.P. Box, M.E. Muller and G. Marsaglia, as described in
     * D. Knuth, <u>The Art of Computer Programming</u>, 3.4.1C.</p>
     * <p>
     * The algorithm generates a pair of independent random values.  One of
     * these is cached for reuse, so the full algorithm is not executed on each
     * activation.  Implementations that do not override this method should
     * make sure to call {@link #clear} to clear the cached value in the
     * implementation of {@link #setSeed(long)}.</p>
     *
     * @return  the next pseudorandom, Gaussian ("normally") distributed
     * {@code double} value with mean {@code 0.0} and
     * standard deviation {@code 1.0} from this random number
     *  generator's sequence
     */
    @Override
    public double nextGaussian() {
        if (!Double.isNaN(cachedNormalDeviate)) {
            double dev = cachedNormalDeviate;
            cachedNormalDeviate = Double.NaN;
            return dev;
        }
        double v1 = 0;
        double v2 = 0;
        double s = 1;
        while (s >=1 ) {
            v1 = 2 * nextDouble() - 1;
            v2 = 2 * nextDouble() - 1;
            s = v1 * v1 + v2 * v2;
        }
        if (s != 0) {
            s = Math.sqrt(-2 * Math.log(s) / s);
        }
        cachedNormalDeviate = v2 * s;
        return v1 * s;
    }
    
    /**
     * Clears the cache used by the default implementation of
     * {@link #nextGaussian}. Implementations that do not override the
     * default implementation of {@code nextGaussian} should call this
     * method in the implementation of {@link #setSeed(long)}
     */
    public void clear() {
        cachedNormalDeviate = Double.NaN;
    }

    /**
     * return characteristic polynomial in hexadecimal format.
     * @return characteristic polynomial
     */
    public String getCharacteristic() {
        return parameter.getCharacteristic().toString(HEXA_DECIMAL_BASE);
    }
    /**
     * return ID of TinyMT. ID is not unique in TinyMT.
     * @return ID
     */
    public int getId() {
        return parameter.getId();
    }

    /**
     * return Delta of TinyMT.
     * 
     * @return Delta
     */
    public int getDelta() {
        return parameter.getDelta();
    }

    /**
     * return Hamming weight of characteristic polynomial of TinyMT.
     * 
     * @return Hamming weight
     */
    public int getWeight() {
        return parameter.getWeight();
    }

    /**
     * Returns a new instance of the random generator with the same state as
     * the calling instance.
     * 
     * @return a new <code>TinyMT32</code> with the same state.
     */
    @Override
    protected Object clone() {
        return new TinyMT32(this);
    }

    /**
     * Returns a new instance of the random generator with the same state as
     * the calling instance.
     * 
     * @return a new <code>TinyMT32</code> with the same state.
     */
    public TinyMT32 cloneRandom() {
        return new TinyMT32(this);
    }
    
    

}
