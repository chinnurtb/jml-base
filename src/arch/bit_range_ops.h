/* bit_range_ops.h                                                 -*- C++ -*-
   Jeremy Barnes, 23 March 2009
   Copyright (c) 2009 Jeremy Barnes.  All rights reserved.

   Operations for operating over a range of bits.
*/

#ifndef __arch__bit_range_ops_h__
#define __arch__bit_range_ops_h__

#include "arch/arch.h"
#include "compiler/compiler.h"
#include <cstddef>
#include <stdint.h>
#include <algorithm>

namespace ML {

typedef uint32_t shift_t;

/** Performs the same as the shrd instruction in x86 land: shifts a low and
    a high value together and returns a middle set of bits.
    
    2n                n                    0
    +-----------------+--------------------+
    |      high       |       low          |
    +--------+--------+-----------+--------+
             |     result         |<-bits---
             +--------------------+
                                     
    NOTE: bits MUST be LESS than the number of bits in the size.  The result
    is undefined if it's equal or greater.

*/
template<typename T>
JML_ALWAYS_INLINE JML_COMPUTE_METHOD
T shrd_emulated(T low, T high, shift_t bits)
{
    enum { TBITS = sizeof(T) * 8 };
    //
    //if (JML_UNLIKELY(bits >= TBITS)) return high >> (bits - TBITS);
    //if (JML_UNLIKELY(bits == TBITS)) return high;
    low >>= bits;
    high <<= (TBITS - bits);
    return low | high;
}

#if defined( JML_INTEL_ISA ) && ! defined(JML_COMPILER_NVCC)

template<typename T>
JML_ALWAYS_INLINE JML_PURE_FN JML_COMPUTE_METHOD
T shrd(T low, T high, shift_t bits)
{
    enum { TBITS = sizeof(T) * 8 };
    if (JML_UNLIKELY(bits > TBITS)) return 0;

    __asm__("shrd   %[bits], %[high], %[low] \n\t"
            : [low] "+r,r" (low)
            : [bits] "J,c" ((uint8_t)bits), [high] "r,r" (high)
            : "cc"
            );

    return low;

}

// There's no 8 bit shrd instruction available
JML_ALWAYS_INLINE JML_COMPUTE_METHOD
unsigned char shrd(unsigned char low, unsigned char high, shift_t bits)
{
    return shrd_emulated(low, high, bits);
}

// There's no 8 bit shrd instruction available
JML_ALWAYS_INLINE JML_COMPUTE_METHOD
signed char shrd(signed char low, signed char high, shift_t bits)
{
    return shrd_emulated(low, high, bits);
}

#else // no SHRD instruction available

template<typename T>
JML_ALWAYS_INLINE JML_COMPUTE_METHOD
T shrd(T low, T high, shift_t bits)
{
    return shrd_emulated(low, high, bits);
}

#endif // JML_INTEL_ISA

/** Extract the bits from bit to bit+numBits starting at the pointed to
    address.  There can be a maximum of one word extracted in this way.

    NO ADJUSTMENT IS DONE ON THE ADDRESS; bit is expected to already be
    there.

    Algorithm:
    1.  If it fits in a single word: copy it, shift it right
    2.  Otherwise, do a double shift right across the two memory locations
*/
template<typename Data>
JML_ALWAYS_INLINE JML_COMPUTE_METHOD
Data extract_bit_range(const Data * p, size_t bit, shift_t bits)
{
    Data result = p[0];

    enum { DBITS = sizeof(Data) * 8 };
    
    if (bit + bits > DBITS) {
        // We need to extract from the two values
        result = shrd(result, p[1], bit);
    }
    else result >>= bit;

    result *= Data(bits != 0);  // bits == 0: should return 0; mask doesn't work

    return result & ((Data(1) << bits) - 1);
}

/** Same, but the low and high values are passed it making it pure. */
template<typename Data>
JML_ALWAYS_INLINE JML_PURE_FN JML_COMPUTE_METHOD
Data extract_bit_range(Data p0, Data p1, size_t bit, shift_t bits)
{
    if (JML_UNLIKELY(bits == 0)) return 0;

    Data result
        = (shrd(p0, p1, bit) // extract
           * Data(bits != 0)) // zero if bits == 0
        & ((Data(1) << bits) - 1); // mask
    
    return result;
}

/** Set the given range of bits in out to the given value.  Note that val
    mustn't have bits set outside bits, and it must entirely fit within
    the value. */
template<typename Data>
JML_ALWAYS_INLINE JML_PURE_FN
Data set_bits(Data in, Data val, shift_t bit, shift_t bits)
{
    // Create a mask with the bits to modify
    Data mask = (Data(1) << bits) - 1;
    mask <<= bit;
    val  <<= bit;

#if 0
    using namespace std;
    cerr << "set_bits: in = " << in << " val = " << val
         << " bit = " << (int)bit << " mask = " << mask
         << endl;
#endif

    return (in & ~mask) | (val & mask);
}

template<typename Data>
void set_bit_range(Data * p, Data val, shift_t bit, shift_t bits)
{
    if (JML_UNLIKELY(bits == 0)) return;

    /* There's some part of the first and some part of the second
       value (both zero or more bits) that need to be replaced by the
       low and high bits respectively. */

    enum { DBITS = sizeof(Data) * 8 };

    int bits0 = std::min<int>(bits, DBITS - bit);
    int bits1 = bits - bits0;

#if 0
    using namespace std;
    cerr << "bits0 = " << bits0 << endl;
    cerr << "bits1 = " << bits1 << endl;
    cerr << "val = " << val << " bit = " << (int)bit << " bits = " << (int)bits
         << endl;
#endif

    p[0] = set_bits<Data>(p[0], val, bit, bits0);
    if (bits1) p[1] = set_bits<Data>(p[1], val >> bits0, 0, bits1);
}



/*****************************************************************************/
/* UTILITY FUNCTIONS                                                         */
/*****************************************************************************/

/** Sign extend the sign from the given bit into the rest of the sign bits
    of the given type. */
template<typename T>
T sign_extend(T raw, int sign_bit)
{
    int sign = (raw & (1 << sign_bit)) != 0;
    T new_bits = T(-sign) << sign_bit;
    return raw | new_bits;
}

// TODO: ix86 can be optimized by doing a shl and then a sar

template<typename T>
T fixup_extract(T extracted, shift_t bits)
{
    return extracted;
}

JML_ALWAYS_INLINE signed char fixup_extract(signed char e, shift_t bits)
{
    return sign_extend(e, bits);
}

JML_ALWAYS_INLINE signed short fixup_extract(signed short e, shift_t bits)
{
    return sign_extend(e, bits);
} 

JML_ALWAYS_INLINE signed int fixup_extract(signed int e, shift_t bits)
{
    return sign_extend(e, bits);
} 

JML_ALWAYS_INLINE signed long fixup_extract(signed long e, shift_t bits)
{
    return sign_extend(e, bits);
} 

JML_ALWAYS_INLINE signed long long fixup_extract(signed long long e, shift_t bits)
{
    return sign_extend(e, bits);
}

/*****************************************************************************/
/* MEM_BUFFER                                                                */
/*****************************************************************************/

/** This is a buffer that handles sequential access to memory.  It ensures
    that there are always N words available, and can skip.  Memory is read
    in such a way that memory is always read as few times as possible and
    with a whole buffer at once.
*/

template<typename Data>
struct Simple_Mem_Buffer {
    Simple_Mem_Buffer(const Data * data)
        : data(data)
    {
    }

    JML_ALWAYS_INLINE Data curr() const { return data[0]; }
    JML_ALWAYS_INLINE Data next() const { return data[1]; }
    
    void operator += (int offset) { data += offset; }

private:
    const Data * data;  // always aligned to 2 * alignof(Data)
};

template<typename Data>
struct Buffered_Mem_Buffer {
    Buffered_Mem_Buffer(const Data * data)
        : data(data)
    {
        b0 = data[0];
        b1 = data[1];
    }

    JML_ALWAYS_INLINE Data curr() const { return b0; }
    JML_ALWAYS_INLINE Data next() const { return b1; }
    
    void operator += (int offset)
    {
        data += offset;
        b0 = (offset == 0 ? b0 : (offset == 1 ? b1 : data[0]));
        b1 = (offset == 0 ? b1 : data[1]);
    }

    void operator ++ (int)
    {
        b0 = b1;
        b1 = data[1];
    }

private:
    const Data * data;  // always aligned to 2 * alignof(Data)
    Data b0, b1;
};


/*****************************************************************************/
/* BIT_BUFFER                                                                */
/*****************************************************************************/

template<typename Data, class MemBuf = Simple_Mem_Buffer<Data> >
struct Bit_Buffer {
    Bit_Buffer(const Data * data)
        : data(data), bit_ofs(0)
    {
    }

    Data extract(int bits)
    {
        Data result = extract_bit_range(data.curr(), data.next(), bit_ofs,
                                        bits);
        advance(bits);
        return result;
    }

    void advance(int bits)
    {
        bit_ofs += bits;
        data += (bit_ofs / (sizeof(Data) * 8));
        bit_ofs %= sizeof(Data) * 8;
    }

private:
    MemBuf data;
    int bit_ofs;     // number of bits from start
};


/*****************************************************************************/
/* BIT_EXTRACTOR                                                             */
/*****************************************************************************/

/** A class to perform extraction of bits from a range of memory.  The goal
    is to support the streaming case efficiently (where we stream across a
    lot of memory, extracting different numbers of bits all the time).

    This class will:
    - Read memory in as large a chunks as possible, with each chunk aligned
      properly (in order to minimise the memory bandwidth used);
    - Perform the minimum number of memory reads necessary;
    - Keep enough state information around to allow multiple extractions to
      be performed efficiently.

    Note that it is not designed to be used where there is lots of random
    access to the fields.  For that, use set_bit_range and extract_bit_range
    instead.

    Extraction is done as integers, respecting the endianness of the current
    machine.  Signed integers will be automatically sign extended, but other
    manipulations will need to be done manually (or fixup_extract specialized).
*/

template<typename Data, typename Buffer = Bit_Buffer<Data> >
struct Bit_Extractor {

    JML_COMPUTE_METHOD
    Bit_Extractor(const Data * data)
        : buf(data), bit_ofs(0)
    {
    }

    template<typename T>
    JML_COMPUTE_METHOD
    T extract(int num_bits)
    {
        return buf.extract(num_bits);
    }

    template<typename T>
    JML_COMPUTE_METHOD
    void extract(T & where, int num_bits)
    {
        where = buf.extract(num_bits);
    }

    template<typename T1, typename T2>
    JML_COMPUTE_METHOD
    void extract(T1 & where1, int num_bits1,
                 T2 & where2, int num_bits2)
    {
        where1 = buf.extract(num_bits1);
        where2 = buf.extract(num_bits2);
    }

    template<typename T1, typename T2, typename T3>
    JML_COMPUTE_METHOD
    void extract(T1 & where1, int num_bits1,
                 T2 & where2, int num_bits2,
                 T3 & where3, int num_bits3)
    {
        where1 = buf.extract(num_bits1);
        where2 = buf.extract(num_bits2);
        where3 = buf.extract(num_bits3);
    }

    template<typename T1, typename T2, typename T3, typename T4>
    JML_COMPUTE_METHOD
    void extract(T1 & where1, int num_bits1,
                 T2 & where2, int num_bits2,
                 T3 & where3, int num_bits3,
                 T4 & where4, int num_bits4)
    {
        where1 = buf.extract(num_bits1);
        where2 = buf.extract(num_bits2);
        where3 = buf.extract(num_bits3);
        where4 = buf.extract(num_bits4);
    }

    JML_COMPUTE_METHOD
    void advance(int bits)
    {
        buf.advance(bits);
    }

    template<typename T, class OutputIterator>
    OutputIterator extract(int num_bits, size_t num_objects,
                           OutputIterator where);

private:
    Buffer buf;
    int bit_ofs;
};


/*****************************************************************************/
/* BIT_WRITER                                                                */
/*****************************************************************************/

template<class Data>
struct Bit_Writer {
    Bit_Writer(Data * data)
        : data(data), bit_ofs(0)
    {
    }

    void write(Data val, int bits)
    {
        set_bit_range(data, val, bit_ofs, bits);
        bit_ofs += bits;
        data += (bit_ofs / (sizeof(Data) * 8));
        bit_ofs %= sizeof(Data) * 8;
    }

    Data * data;
    int bit_ofs;
};

} // namespace ML


#endif /* __arch__bit_range_ops_h__ */