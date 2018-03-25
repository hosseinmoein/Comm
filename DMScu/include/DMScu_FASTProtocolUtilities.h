// Hossein Moein
// March 25, 2018
// Copyright (C) 2018-2019 Hossein Moein
// Distributed under the BSD Software License (see file License)

// ----------------------------------------------------------------------------

#pragma once

#include <limits.h>
#include <cmath>

#include <DMScu_FixedSizeString.h>

#if defined (DMS_SunOS_GCC64__) || defined (DMS_SunOS_GCC32__)
#include <ieeefp.h>
#endif // defined (DMS_SunOS_GCC64__) || defined (DMS_SunOS_GCC32__)

// ----------------------------------------------------------------------------

class   DMScu_FASTProtocolUtilities  {

    private:

        typedef unsigned char   uchar;

    public:

        typedef unsigned int    size_type;

        static  const   unsigned    short   AND_4 = 0xf;
        static  const   unsigned    short   AND_7 = 0x7f;
        static  const   unsigned    short   AND_8 = 0xff;

        static inline bool is_final (uchar c) throw ()  {

            return ((c & 0x80) != 0);
        }

        static inline size_type
        bytes_required (size_type size, bool signed_value = true) throw ()  {

            if (size < 1)
                return (0);

            if (signed_value)
                return (size + size / 7 + 1);
            else
               // return (size + ceil (size / 7))
               //
                return (size + (size % 7 == 0 ? size / 7 : size / 7 + 1));
        }

        static inline size_type
        encode_uchar (uchar *buffer, uchar c) throw ()  {

            buffer [0] = c;
            return (1);
        }

        static inline size_type
        decode_uchar (const uchar *buffer, uchar c) throw ()  {

            c = buffer [0];
            return (1);
        }

        template<class cu_TYPE>
        static inline size_type
        encode_uinteger (uchar *buffer, cu_TYPE x) throw ()  {

            cu_TYPE     y = 0x80;
            size_type   bytes = 1;
            const   size_type       max_bytes =
                bytes_required (sizeof (cu_TYPE), false);

            while (x >= y && bytes < max_bytes)  {
                y = y << 7;
                bytes += 1;
            }

            size_type   idx = 0;

            for (int shift = (bytes - 1) * 7; shift > 0; shift -= 7)
                buffer [idx++] = static_cast<uchar>((x >> shift) & AND_7);
            buffer [idx++] = static_cast<uchar> ((x & AND_7) | 0x80);

            return (idx);
        }

        template<class cu_TYPE>
        static inline size_type
        decode_uinteger (const uchar *buffer, cu_TYPE &x) throw ()  {

            x = 0;

            size_type   idx = 0;
            bool        had_stop = false;
            const   size_type       max_bytes =
                bytes_required (sizeof (cu_TYPE), false);

            while (! had_stop)  {
                if ((buffer [idx] & 0x80) != 0)
                    had_stop = true;

                x = (x << 7) | (buffer [idx] & AND_7);
                idx += 1;

                if (idx > max_bytes)
                    break;
            }

            return (idx);
        }

        template<class cu_TYPE>
        static inline size_type
        actual_bytes_required_int (cu_TYPE x) throw ()  {

            size_type   bytes = 1;
            const   size_type       max_bytes =
                bytes_required (sizeof (cu_TYPE), true);
            cu_TYPE     y =
                x > 0 ? 0x40 : ~(static_cast<cu_TYPE>(0x3f));

            while ((x > 0 ? x >= y : x <= y) && bytes < max_bytes)  {
                y = y << 7;
                bytes += 1;
            }

            return (bytes);
        }

        template<class cu_TYPE>
        static inline size_type
        encode_integer (uchar *buffer, cu_TYPE x) throw ()  {

            size_type   bytes = actual_bytes_required_int (x);
            int         shift = (bytes - 1 ) * 7;
            size_type   idx = 0;

            if (x >= 0 ||
                bytes != bytes_required (sizeof (cu_TYPE), true) ||
                sizeof (cu_TYPE) * 8 % 7 == 0)  {
                for (; shift > 0; shift -= 7)
                    buffer [idx++] = static_cast<uchar>((x >> shift) & AND_7);
                buffer [idx++] = static_cast<uchar>((x & AND_7) | 0x80);
            }
            else  {  // This makes me sad, but I don't see how to avoid it.
               // If we have a negative number where the most significant byte
               // _could be_ (depending on compiler implementation of signed
               // right shift) incorrectly zero-filled, then we must do this
               // extra work. This only happens if the most significant
               // converted byte comes from fewer than 7 bits, which only
               // happens if bytes == max_bytes and the number of bits in an
               // cu_TYPE is not divisible by 7.
               //
                size_type   fill = 1;
                int         extra_bits =
                    7 - (sizeof (cu_TYPE) * 8 % 7);

                for (size_type i = 0; i < 6; i++)  {
                    fill = (fill << 1);
                    if (--extra_bits > 0)
                        fill &= 1;
                }
                buffer [idx++] =
                    static_cast<uchar>(((x >> shift) | fill) & AND_7);
                shift -= 7;

                for (; shift > 0; shift -= 7)
                    buffer [idx++] = static_cast<uchar>((x >> shift) & AND_7);
                buffer [idx++] = static_cast<uchar>((x & AND_7) | 0x80);
            }

            return (bytes);
        }

        template<class cu_TYPE>
        static inline size_type
        decode_integer (const uchar *buffer, cu_TYPE &x) throw ()  {

            x = (buffer [0] & 0x40) != 0 ? ~0 : 0;

            size_type   idx = 0;
            bool        had_stop = false;
            const   size_type       max_bytes =
                bytes_required (sizeof (cu_TYPE), false);

            while (! had_stop)  {
                if ((buffer [idx] & 0x80) != 0)
                    had_stop = true;

                x = (x << 7) | (buffer [idx] & AND_7);
                idx += 1;
                if (idx > max_bytes)
                    break;
            }
            return (idx);
        }

       // This is the slowest method (along with decode_float), so I'm
       // focusing on their performance over the integer methods.
       // They may still allow performance improvement, but it's not
       // worth the time investment right now.
       //
        template<class cu_TYPE>
        static inline size_type
        encode_real (uchar *buffer, cu_TYPE val) throw ()  {

            if (! finite (val))  {
                buffer [0] = 0xFF;
                return (1);
            }

            int     exponent = 0;
            cu_TYPE tmp_value = 0;

            if (val != 0)  {
                tmp_value = val;

                cu_TYPE floor = static_cast<long long int>(val);

                if (floor != tmp_value)  {
                    exponent = -1;

                    cu_TYPE mult = static_cast<cu_TYPE>(1.0);

                    while (floor != tmp_value)  {
                       // NOTE: don't multiply the original value by
                       // a power of 10.0 (must be double!) each time, or else
                       // much efficiency in the compression is lost
                       // due to the typically long representation of
                       // decimals in binary.
                       // This way is faster than pow(10.0, exponent + 1) and
                       // works exactly the same way.
                       //
                        tmp_value = val * mult;
                        floor = static_cast<long long int>(tmp_value);
                        exponent += 1;
                        mult *= static_cast<cu_TYPE>(10.0);
                    }
                    exponent = -exponent;
                }
            }

            long long int   mantissa = static_cast<long long int>(tmp_value);

            while (mantissa != 0 && mantissa % 10 == 0)  {
                mantissa /= 10;
                exponent += 1;
            }

            // if (actual_bytes_required_int (exponent) > 1)  {
            //     DMScu_FixedSizeString<1023> err;

            //     err.printf ("DMScu_FASTProtocolUtilities::encode_real(): "
            //                 "Too many bytes required for exponent for "
            //                 "number %lf", static_cast<double>(val));
            //     throw Exception (err.c_str ());
            // }

            buffer [0] = static_cast<uchar>(exponent & AND_7);

            return (1 + encode_integer (buffer + 1, mantissa));
        }

       // Allow the limiting of the number of significant digits after
       // the decimal.
       //
        template<class cu_TYPE>
        static inline size_type
        encode_real (uchar *buffer, cu_TYPE val, size_type precision)
            throw ()  {

            if (! finite (val))  {
                buffer [0] = 0xFF;
                return (1);
            }

            int     exponent = 0;
            cu_TYPE tmp_value = 0;

            if (val != 0)  {
                tmp_value = val;

                cu_TYPE floor = static_cast<long long int>(val);

                if (floor != tmp_value)  {
                    exponent = -1;

                    cu_TYPE mult = static_cast<cu_TYPE>(1.0);

                    while (floor != tmp_value && exponent < precision)  {
                       // NOTE: don't multiply the original value by
                       // a power of 10.0 (must be double!) each time, or else
                       // much efficiency in the compression is lost
                       // due to the typically long representation of
                       // decimals in binary.
                       // This way is faster than pow(10.0, exponent + 1) and
                       // works exactly the same way.
                       //
                        tmp_value = val * mult;
                        floor = static_cast<long long int> (tmp_value);
                        exponent += 1;
                        mult *= static_cast<cu_TYPE>(10.0);
                    }
                    exponent = -exponent;
                }
            }

            long long int   mantissa = static_cast<long long int>(tmp_value);

            while (mantissa != 0 && mantissa % 10 == 0)  {
                mantissa /= 10;
                exponent += 1;
            }

            // if (actual_bytes_required_int (exponent) > 1)  {
            //     DMScu_FixedSizeString<1023> err;

            //     err.printf ("DMScu_FASTProtocolUtilities::encode_real(): "
            //                 "Too many bytes required for exponent for "
            //                 "number %lf", static_cast<double>(val));
            //     throw Exception (err.c_str ());
            // }

            buffer [0] = static_cast<uchar>(exponent & AND_7);

            return (1 + encode_integer (buffer + 1, mantissa));
        }

       // Please see comment above regarding encode_float.
       //
        template<class cu_TYPE>
        static inline size_type
        decode_real (const uchar *buffer, cu_TYPE &x) throw ()  {

            static  const   cu_TYPE nan = ::sqrt (-1);

            unsigned char   c = buffer [0];

            if (c == 0xFF)  {
                x = nan;
                return (1);
            }

           // if the first byte has stop bit then no mantissa
           //
            if ((c & 0x80) != 0)  {
                x = 0;
                return (1);
            }

            int exponent = 0;

           // check sign bit
           //
            if ((c & 0x40) != 0)
                exponent = ~AND_7;
            exponent |= (c & AND_7);

           // the rest of the bytes make up the mantissa
           //
            long long int   mantissa  = 0;
            size_type       ret_val =
                decode_integer (buffer + 1, mantissa) + 1;

            if (exponent == 0)
                x = mantissa;
            else  {
               // Compute multiplier iteratively instead of using pow
               // for small exponents
               //
                cu_TYPE multiplier = static_cast<cu_TYPE>(1.0);

                if (::llabs (exponent) >= 9)
                    multiplier = ::pow (10.0, ::llabs (exponent));

                if (exponent > 0)  {
                    if (exponent < 9)
                        for (int i = 0; i < exponent; i++)
                            multiplier *= static_cast<cu_TYPE>(10.0);
                    x = mantissa * multiplier;
                }
                else  {
                    if (exponent > -9)
                        for (int i = 0; i < -exponent; i++)
                            multiplier *= static_cast<cu_TYPE>(10.0);
                    x = mantissa / multiplier;
                }
            }

            return (ret_val);
        }
};

// ----------------------------------------------------------------------------

// Local Variables:
// mode:C++
// tab-width:4
// c-basic-offset:4
// End:
