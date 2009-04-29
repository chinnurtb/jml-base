/* compact_size_types.cc
   Jeremy Barnes, 17 March 2005
   Copyright (c) 2005 Jeremy Barnes.  All rights reserved.
   
   This file is part of "Jeremy's Machine Learning Library", copyright (c)
   1999-2005 Jeremy Barnes.
   
   This program is available under the GNU General Public License, the terms
   of which are given by the file "license.txt" in the top level directory of
   the source code distribution.  If this file is missing, you have no right
   to use the program; please contact the author.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
   for more details.

   ---

   Implementation of persistence for compact size types.
*/

#include "compact_size_types.h"
#include "compiler/compiler.h"
#include "arch/bitops.h"
#include "persistent.h"
#include <stdint.h>
#include <iomanip>


using namespace std;


namespace ML {
namespace DB {

const compact_size_t compact_const(unsigned val)
{
    return compact_size_t(val);
}

/*****************************************************************************/
/* UNSIGNED VERSIONS                                                         */
/*****************************************************************************/

/* byte1     extra     min     max  max32bit   
   0 xxxxxxx     0    0     2^7-1
   10 xxxxxx     1    2^7   2^14-1
   110 xxxxx     2    2^14  2^21-1
   1110 xxxx     3    2^21  2^28-1
   11110 xxx     4    2^28  2^35-1  (2^32-1)
   111110 xx     5    2^35  2^42-1
   1111110 x     6    2^42  2^49-1
   11111110      7    2^49  2^56-1
   11111111      8    2^56  2^64-1
*/

void encode_compact(Store_Writer & store, unsigned long val)
{
    char buf[9];

    /* Length depends upon highest bit / 7 */
    int highest = highest_bit(val);
    int idx = highest / 7;
    int len = idx + 1;

    //cerr << "val = " << val << " highest = " << highest << " len = "
    //     << len << endl;

    /* Pack it into the back bytes. */
    for (int i = len-1;  i >= 0;  --i) {
        //cerr << "i = " << i << endl;
        buf[i] = val & 0xff;
        val >>= 8;
    }

    /* Add the indicator to the first byte. */
    uint32_t indicator = ~((1 << (8-idx)) - 1);
    buf[0] |= indicator;
    
    store.save_binary(buf, len);
}

unsigned long decode_compact(Store_Reader & store)
{
    /* Find the first zero bit in the marker.  We do this by bit flipping
       and finding the first 1 bit in the result. */
    store.must_have(1);

    char marker = *store;
    int len = 8 - highest_bit((char)~marker);// no bits set=-1, so len=9 as reqd

    /* Make sure this data is available. */
    store.must_have(len);

    /* Construct our value from the bytes. */
    unsigned long result = 0;
    for (int i = 0;  i < len;  ++i) {
        result <<= 8;
        result |= store[i]; 
    }

    /* Filter off the top bits, which told us the length. */
    if (len == 9) ;
    else {
        int bits = len * 7;
        result &= (~((1 << bits)-1));
    }

    /* Skip the data.  Makes sure we are in sync even if we throw. */
    store.skip(len);

    return result;
}


/*****************************************************************************/
/* COMPACT_SIZE_T                                                            */
/*****************************************************************************/

compact_size_t::compact_size_t(Store_Reader & store)
{
    size_ = decode_compact(store);
}
    
void compact_size_t::serialize(Store_Writer & store) const
{
    encode_compact(store, size_);
}

void compact_size_t::reconstitute(Store_Reader & store)
{
    size_ = decode_compact(store);
}

void compact_size_t::serialize(std::ostream & stream) const
{
    Store_Writer writer(stream);
    serialize(writer);
}

std::ostream & operator << (std::ostream & stream, const compact_size_t & s)
{
    stream << s.size_;
    return stream;
}


/*****************************************************************************/
/* SIGNED VERSIONS                                                           */
/*****************************************************************************/

/* byte1      byte2    others  range
   0 s xxxxxx          0       2^6
   10 s xxxxx xxxxxxxx 0       2^13
   110 s xxxx xxxxxxxx 1       2^20
   1110 s xxx xxxxxxxx 2       2^27
   11110 s xx xxxxxxxx 3       2^34 (2^31)
   111110 s x xxxxxxxx 4       2^41 
   1111110 s  xxxxxxxx 5       2^48
   11111110   sxxxxxxx 6       2^55
   11111111   sxxxxxxx 7       2^63
*/

void encode_signed_compact(Store_Reader & store, signed long val)
{
    throw Exception("not implemented");
}

signed long decode_signed_compact(Store_Reader & store)
{
    throw Exception("not implemented");
}



} // namespace DB
} // namespace ML
