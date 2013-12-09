/*  This file is part of the Vc library. {{{

    Copyright (C) 2010-2013 Matthias Kretz <kretz@kde.org>

    Vc is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as
    published by the Free Software Foundation, either version 3 of
    the License, or (at your option) any later version.

    Vc is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with Vc.  If not, see <http://www.gnu.org/licenses/>.

}}}*/

#include "../common/x86_prefetches.h"
#include "limits.h"
#include "../common/bitscanintrinsics.h"
#include "../common/set.h"
#include "macros.h"

Vc_NAMESPACE_BEGIN(Vc_IMPL_NAMESPACE)

// constants {{{1
template<typename T, int Size> Vc_ALWAYS_INLINE Vc_CONST const T *_IndexesFromZero() {
    if (Size == 4) {
        return reinterpret_cast<const T *>(_IndexesFromZero4);
    } else if (Size == 8) {
        return reinterpret_cast<const T *>(_IndexesFromZero8);
    } else if (Size == 16) {
        return reinterpret_cast<const T *>(_IndexesFromZero16);
    }
    return 0;
}

template<typename T> Vc_INTRINSIC Vector<T>::Vector(VectorSpecialInitializerZero::ZEnum)
    : d(VectorHelper<VectorType>::zero())
{
}

template<typename T> Vc_INTRINSIC Vector<T>::Vector(VectorSpecialInitializerOne::OEnum)
    : d(VectorHelper<T>::one())
{
}

template<typename T> Vc_INTRINSIC Vector<T>::Vector(VectorSpecialInitializerIndexesFromZero::IEnum)
    : d(VectorHelper<VectorType>::template load<AlignedTag>(_IndexesFromZero<EntryType, Size>()))
{
}

template<> Vc_INTRINSIC float_v::Vector(VectorSpecialInitializerIndexesFromZero::IEnum)
    : d(StaticCastHelper<int, float>::cast(int_v::IndexesFromZero().data()))
{
}

template<> Vc_INTRINSIC double_v::Vector(VectorSpecialInitializerIndexesFromZero::IEnum)
    : d(StaticCastHelper<int, double>::cast(int_v::IndexesFromZero().data()))
{
}

template<typename T> Vc_INTRINSIC Vc_CONST Vector<T> Vector<T>::Zero()
{
    return VectorHelper<VectorType>::zero();
}

template<typename T> Vc_INTRINSIC Vc_CONST Vector<T> Vector<T>::One()
{
    return VectorHelper<T>::one();
}

template<typename T> Vc_INTRINSIC Vc_CONST Vector<T> Vector<T>::IndexesFromZero()
{
    return Vector<T>(VectorSpecialInitializerIndexesFromZero::IndexesFromZero);
}

// load member functions {{{1
template<typename T> template<typename Flags> Vc_INTRINSIC void Vector<T>::load(const EntryType *mem, Flags flags)
{
    handleLoadPrefetches(mem, flags);
    d.v() = HV::template load<Flags>(mem);
}

// LoadHelper {{{2
template<typename DstT, typename SrcT, typename Flags> struct LoadHelper;

// float {{{2
template<typename Flags> struct LoadHelper<float, double, Flags> {
    static Vc_ALWAYS_INLINE Vc_PURE __m128 load(const double *mem, Flags)
    {
        return _mm_movelh_ps(_mm_cvtpd_ps(VectorHelper<__m128d>::load<Flags>(&mem[0])),
                             _mm_cvtpd_ps(VectorHelper<__m128d>::load<Flags>(&mem[2])));
    }
};
template<typename Flags> struct LoadHelper<float, unsigned int, Flags> {
    static Vc_ALWAYS_INLINE Vc_PURE __m128 load(const unsigned int *mem, Flags)
    {
        return StaticCastHelper<unsigned int, float>::cast(VectorHelper<__m128i>::load<Flags>(mem));
    }
};
template<typename Flags> struct LoadHelper<float, int, Flags> {
    static Vc_ALWAYS_INLINE Vc_PURE __m128 load(const int *mem, Flags)
    {
        return StaticCastHelper<int, float>::cast(VectorHelper<__m128i>::load<Flags>(mem));
    }
};
template<typename Flags> struct LoadHelper<float, unsigned short, Flags> {
    static Vc_ALWAYS_INLINE Vc_PURE __m128 load(const unsigned short *mem, Flags f)
    {
        return _mm_cvtepi32_ps(LoadHelper<int, unsigned short, Flags>::load(mem, f));
    }
};
template<typename Flags> struct LoadHelper<float, short, Flags> {
    static Vc_ALWAYS_INLINE Vc_PURE __m128 load(const short *mem, Flags f)
    {
        return _mm_cvtepi32_ps(LoadHelper<int, short, Flags>::load(mem, f));
    }
};
template<typename Flags> struct LoadHelper<float, unsigned char, Flags> {
    static Vc_ALWAYS_INLINE Vc_PURE __m128 load(const unsigned char *mem, Flags f)
    {
        return _mm_cvtepi32_ps(LoadHelper<int, unsigned char, Flags>::load(mem, f));
    }
};
template<typename Flags> struct LoadHelper<float, signed char, Flags> {
    static Vc_ALWAYS_INLINE Vc_PURE __m128 load(const signed char *mem, Flags f)
    {
        return _mm_cvtepi32_ps(LoadHelper<int, signed char, Flags>::load(mem, f));
    }
};


// int {{{2
template<typename Flags> struct LoadHelper<int, unsigned int, Flags> {
    static Vc_ALWAYS_INLINE Vc_PURE __m128i load(const unsigned int *mem, Flags)
    {
        return VectorHelper<__m128i>::load<Flags>(mem);
    }
};
// no difference between streaming and alignment, because the
// 32/64 bit loads are not available as streaming loads, and can always be unaligned
template<typename Flags> struct LoadHelper<int, unsigned short, Flags> {
    static Vc_ALWAYS_INLINE Vc_PURE __m128i load(const unsigned short *mem, Flags)
    {
        return _mm_cvtepu16_epi32( _mm_loadl_epi64(reinterpret_cast<const __m128i *>(mem)));
    }
};
template<typename Flags> struct LoadHelper<int, short, Flags> {
    static Vc_ALWAYS_INLINE Vc_PURE __m128i load(const short *mem, Flags)
    {
        return _mm_cvtepi16_epi32(_mm_loadl_epi64(reinterpret_cast<const __m128i *>(mem)));
    }
};
template<typename Flags> struct LoadHelper<int, unsigned char, Flags> {
    static Vc_ALWAYS_INLINE Vc_PURE __m128i load(const unsigned char *mem, Flags)
    {
        return _mm_cvtepu8_epi32(_mm_cvtsi32_si128(*reinterpret_cast<const int *>(mem)));
    }
};
template<typename Flags> struct LoadHelper<int, signed char, Flags> {
    static Vc_ALWAYS_INLINE Vc_PURE __m128i load(const signed char *mem, Flags)
    {
        return _mm_cvtepi8_epi32(_mm_cvtsi32_si128(*reinterpret_cast<const int *>(mem)));
    }
};

// unsigned int {{{2
template<typename Flags> struct LoadHelper<unsigned int, unsigned short, Flags> {
    static Vc_ALWAYS_INLINE Vc_PURE __m128i load(const unsigned short *mem, Flags)
    {
        return _mm_cvtepu16_epi32(_mm_loadl_epi64(reinterpret_cast<const __m128i *>(mem)));
    }
};
template<typename Flags> struct LoadHelper<unsigned int, unsigned char, Flags> {
    static Vc_ALWAYS_INLINE Vc_PURE __m128i load(const unsigned char *mem, Flags)
    {
        return _mm_cvtepu8_epi32(_mm_cvtsi32_si128(*reinterpret_cast<const int *>(mem)));
    }
};

// short {{{2
template<typename Flags> struct LoadHelper<short, unsigned short, Flags> {
    static Vc_ALWAYS_INLINE Vc_PURE __m128i load(const unsigned short *mem, Flags)
    {
        return VectorHelper<__m128i>::load<Flags>(mem);
    }
};
template<typename Flags> struct LoadHelper<short, unsigned char, Flags> {
    static Vc_ALWAYS_INLINE Vc_PURE __m128i load(const unsigned char *mem, Flags)
    {
        return _mm_cvtepu8_epi16(_mm_loadl_epi64(reinterpret_cast<const __m128i *>(mem)));
    }
};
template<typename Flags> struct LoadHelper<short, signed char, Flags> {
    static Vc_ALWAYS_INLINE Vc_PURE __m128i load(const signed char *mem, Flags)
    {
        return _mm_cvtepi8_epi16(_mm_loadl_epi64(reinterpret_cast<const __m128i *>(mem)));
    }
};

// unsigned short {{{2
template<typename Flags> struct LoadHelper<unsigned short, unsigned char, Flags> {
    static Vc_ALWAYS_INLINE Vc_PURE __m128i load(const unsigned char *mem, Flags)
    {
        return _mm_cvtepu8_epi16(_mm_loadl_epi64(reinterpret_cast<const __m128i *>(mem)));
    }
};

// general load, implemented via LoadHelper {{{2
template <typename DstT>
template <typename SrcT,
          typename Flags,
          typename std::enable_if<std::is_arithmetic<SrcT>::value, int>::type = 0>
Vc_INTRINSIC void Vector<DstT>::load(const SrcT *mem, Flags flags)
{
    handleLoadPrefetches(mem, flags);
    d.v() = LoadHelper<DstT, SrcT, Flags>::load(mem, flags);
}

///////////////////////////////////////////////////////////////////////////////////////////
// expand/combine {{{1
template<typename T> Vc_INTRINSIC Vector<T>::Vector(const Vector<typename CtorTypeHelper<T>::Type> *a)
    : d(VectorHelper<T>::concat(a[0].data(), a[1].data()))
{
}

template<typename T> inline void Vector<T>::expand(Vector<typename ExpandTypeHelper<T>::Type> *x) const
{
    *x = *this;
}

template<> inline void float_v::expand(double_v *x) const
{
    x[0].data() = _mm_cvtps_pd(data());
    x[1].data() = _mm_cvtps_pd(_mm_movehl_ps(data(), data()));
}
template<> inline void short_v::expand(int_v *x) const
{
    x[0].data() = HT::expand0(data());
    x[1].data() = HT::expand1(data());
}
template<> inline void ushort_v::expand(uint_v *x) const
{
    x[0].data() = HT::expand0(data());
    x[1].data() = HT::expand1(data());
}

///////////////////////////////////////////////////////////////////////////////////////////
// zeroing {{{1
template<typename T> Vc_INTRINSIC void Vector<T>::setZero()
{
    data() = VectorHelper<VectorType>::zero();
}

template<typename T> Vc_INTRINSIC void Vector<T>::setZero(const Mask &k)
{
    data() = VectorHelper<VectorType>::andnot_(mm128_reinterpret_cast<VectorType>(k.data()), data());
}

template<> Vc_INTRINSIC void Vector<double>::setQnan()
{
    data() = _mm_setallone_pd();
}
template<> Vc_INTRINSIC void Vector<double>::setQnan(Mask::Argument k)
{
    data() = _mm_or_pd(data(), k.dataD());
}
template<> Vc_INTRINSIC void Vector<float>::setQnan()
{
    data() = _mm_setallone_ps();
}
template<> Vc_INTRINSIC void Vector<float>::setQnan(Mask::Argument k)
{
    data() = _mm_or_ps(data(), k.data());
}

///////////////////////////////////////////////////////////////////////////////////////////
// stores {{{1
template <typename T>
template <typename U,
          typename Flags,
          typename std::enable_if<std::is_arithmetic<U>::value, int>::type = 0>
Vc_INTRINSIC void Vector<T>::store(U *mem, Flags flags) const
{
    handleStorePrefetches(mem, flags);
    HV::template store<Flags>(mem, data());
}

template <typename T>
template <typename U,
          typename Flags,
          typename std::enable_if<std::is_arithmetic<U>::value, int>::type = 0>
Vc_INTRINSIC void Vector<T>::store(U *mem, Mask mask, Flags flags) const
{
    handleStorePrefetches(mem, flags);
    HV::template store<Flags>(mem, data(), sse_cast<VectorType>(mask.data()));
}

///////////////////////////////////////////////////////////////////////////////////////////
// division {{{1
template<typename T> Vc_INTRINSIC Vector<T> &WriteMaskedVector<T>::operator/=(const Vector<T> &x)
{
    return operator=(*vec / x);
}
template<> Vc_INTRINSIC int_v &WriteMaskedVector<int>::operator/=(const int_v &x)
{
    Vc_foreach_bit (int i, mask) {
        vec->d.m(i) /= x.d.m(i);
    }
    return *vec;
}
template<> Vc_INTRINSIC uint_v &WriteMaskedVector<unsigned int>::operator/=(const uint_v &x)
{
    Vc_foreach_bit (int i, mask) {
        vec->d.m(i) /= x.d.m(i);
    }
    return *vec;
}
template<> Vc_INTRINSIC short_v &WriteMaskedVector<short>::operator/=(const short_v &x)
{
    Vc_foreach_bit (int i, mask) {
        vec->d.m(i) /= x.d.m(i);
    }
    return *vec;
}
template<> Vc_INTRINSIC ushort_v &WriteMaskedVector<unsigned short>::operator/=(const ushort_v &x)
{
    Vc_foreach_bit (int i, mask) {
        vec->d.m(i) /= x.d.m(i);
    }
    return *vec;
}

template<typename T> inline Vector<T> &Vector<T>::operator/=(EntryType x)
{
    if (VectorTraits<T>::HasVectorDivision) {
        return operator/=(Vector<T>(x));
    }
    for_all_vector_entries(i,
            d.m(i) /= x;
            );
    return *this;
}

template<typename T> inline Vector<T> &Vector<T>::operator/=(VC_ALIGNED_PARAMETER(Vector<T>) x)
{
    for_all_vector_entries(i,
            d.m(i) /= x.d.m(i);
            );
    return *this;
}

template<typename T> inline Vc_PURE Vector<T> Vector<T>::operator/(VC_ALIGNED_PARAMETER(Vector<T>) x) const
{
    Vector<T> r;
    for_all_vector_entries(i,
            r.d.m(i) = d.m(i) / x.d.m(i);
            );
    return r;
}

template<> inline Vector<short> &Vector<short>::operator/=(VC_ALIGNED_PARAMETER(Vector<short>) x)
{
    __m128 lo = _mm_cvtepi32_ps(VectorHelper<short>::expand0(d.v()));
    __m128 hi = _mm_cvtepi32_ps(VectorHelper<short>::expand1(d.v()));
    lo = _mm_div_ps(lo, _mm_cvtepi32_ps(VectorHelper<short>::expand0(x.d.v())));
    hi = _mm_div_ps(hi, _mm_cvtepi32_ps(VectorHelper<short>::expand1(x.d.v())));
    d.v() = HT::concat(_mm_cvttps_epi32(lo), _mm_cvttps_epi32(hi));
    return *this;
}

template<> inline Vc_PURE Vector<short> Vector<short>::operator/(VC_ALIGNED_PARAMETER(Vector<short>) x) const
{
    __m128 lo = _mm_cvtepi32_ps(VectorHelper<short>::expand0(d.v()));
    __m128 hi = _mm_cvtepi32_ps(VectorHelper<short>::expand1(d.v()));
    lo = _mm_div_ps(lo, _mm_cvtepi32_ps(VectorHelper<short>::expand0(x.d.v())));
    hi = _mm_div_ps(hi, _mm_cvtepi32_ps(VectorHelper<short>::expand1(x.d.v())));
    return HT::concat(_mm_cvttps_epi32(lo), _mm_cvttps_epi32(hi));
}

template<> inline Vector<unsigned short> &Vector<unsigned short>::operator/=(VC_ALIGNED_PARAMETER(Vector<unsigned short>) x)
{
    __m128 lo = _mm_cvtepi32_ps(VectorHelper<unsigned short>::expand0(d.v()));
    __m128 hi = _mm_cvtepi32_ps(VectorHelper<unsigned short>::expand1(d.v()));
    lo = _mm_div_ps(lo, _mm_cvtepi32_ps(VectorHelper<unsigned short>::expand0(x.d.v())));
    hi = _mm_div_ps(hi, _mm_cvtepi32_ps(VectorHelper<unsigned short>::expand1(x.d.v())));
    d.v() = HT::concat(_mm_cvttps_epi32(lo), _mm_cvttps_epi32(hi));
    return *this;
}

template<> Vc_ALWAYS_INLINE Vc_PURE Vector<unsigned short> Vector<unsigned short>::operator/(VC_ALIGNED_PARAMETER(Vector<unsigned short>) x) const
{
    __m128 lo = _mm_cvtepi32_ps(VectorHelper<unsigned short>::expand0(d.v()));
    __m128 hi = _mm_cvtepi32_ps(VectorHelper<unsigned short>::expand1(d.v()));
    lo = _mm_div_ps(lo, _mm_cvtepi32_ps(VectorHelper<unsigned short>::expand0(x.d.v())));
    hi = _mm_div_ps(hi, _mm_cvtepi32_ps(VectorHelper<unsigned short>::expand1(x.d.v())));
    return HT::concat(_mm_cvttps_epi32(lo), _mm_cvttps_epi32(hi));
}

template<> Vc_ALWAYS_INLINE Vector<float> &Vector<float>::operator/=(VC_ALIGNED_PARAMETER(Vector<float>) x)
{
    d.v() = _mm_div_ps(d.v(), x.d.v());
    return *this;
}

template<> Vc_ALWAYS_INLINE Vc_PURE Vector<float> Vector<float>::operator/(VC_ALIGNED_PARAMETER(Vector<float>) x) const
{
    return _mm_div_ps(d.v(), x.d.v());
}

template<> Vc_ALWAYS_INLINE Vector<double> &Vector<double>::operator/=(VC_ALIGNED_PARAMETER(Vector<double>) x)
{
    d.v() = _mm_div_pd(d.v(), x.d.v());
    return *this;
}

template<> Vc_ALWAYS_INLINE Vc_PURE Vector<double> Vector<double>::operator/(VC_ALIGNED_PARAMETER(Vector<double>) x) const
{
    return _mm_div_pd(d.v(), x.d.v());
}

///////////////////////////////////////////////////////////////////////////////////////////
// operator- {{{1
template<> Vc_ALWAYS_INLINE Vector<double> Vc_PURE Vc_FLATTEN Vector<double>::operator-() const
{
    return _mm_xor_pd(d.v(), _mm_setsignmask_pd());
}
template<> Vc_ALWAYS_INLINE Vector<float> Vc_PURE Vc_FLATTEN Vector<float>::operator-() const
{
    return _mm_xor_ps(d.v(), _mm_setsignmask_ps());
}
template<> Vc_ALWAYS_INLINE Vector<int> Vc_PURE Vc_FLATTEN Vector<int>::operator-() const
{
#ifdef VC_IMPL_SSSE3
    return _mm_sign_epi32(d.v(), _mm_setallone_si128());
#else
    return _mm_add_epi32(_mm_xor_si128(d.v(), _mm_setallone_si128()), _mm_setone_epi32());
#endif
}
template<> Vc_ALWAYS_INLINE Vector<int> Vc_PURE Vc_FLATTEN Vector<unsigned int>::operator-() const
{
#ifdef VC_IMPL_SSSE3
    return _mm_sign_epi32(d.v(), _mm_setallone_si128());
#else
    return _mm_add_epi32(_mm_xor_si128(d.v(), _mm_setallone_si128()), _mm_setone_epi32());
#endif
}
template<> Vc_ALWAYS_INLINE Vector<short> Vc_PURE Vc_FLATTEN Vector<short>::operator-() const
{
#ifdef VC_IMPL_SSSE3
    return _mm_sign_epi16(d.v(), _mm_setallone_si128());
#else
    return _mm_mullo_epi16(d.v(), _mm_setallone_si128());
#endif
}
template<> Vc_ALWAYS_INLINE Vector<short> Vc_PURE Vc_FLATTEN Vector<unsigned short>::operator-() const
{
#ifdef VC_IMPL_SSSE3
    return _mm_sign_epi16(d.v(), _mm_setallone_si128());
#else
    return _mm_mullo_epi16(d.v(), _mm_setallone_si128());
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////
// integer ops {{{1
#define OP_IMPL(T, symbol, fun) \
template<> Vc_ALWAYS_INLINE Vector<T> &Vector<T>::operator symbol##=(const Vector<T> &x) \
{ \
    d.v() = VectorHelper<T>::fun(d.v(), x.d.v()); \
    return *this; \
} \
template<> Vc_ALWAYS_INLINE Vc_PURE Vector<T>  Vector<T>::operator symbol(const Vector<T> &x) const \
{ \
    return VectorHelper<T>::fun(d.v(), x.d.v()); \
}
OP_IMPL(int, &, and_)
OP_IMPL(int, |, or_)
OP_IMPL(int, ^, xor_)
OP_IMPL(unsigned int, &, and_)
OP_IMPL(unsigned int, |, or_)
OP_IMPL(unsigned int, ^, xor_)
OP_IMPL(short, &, and_)
OP_IMPL(short, |, or_)
OP_IMPL(short, ^, xor_)
OP_IMPL(unsigned short, &, and_)
OP_IMPL(unsigned short, |, or_)
OP_IMPL(unsigned short, ^, xor_)
OP_IMPL(float, &, and_)
OP_IMPL(float, |, or_)
OP_IMPL(float, ^, xor_)
OP_IMPL(double, &, and_)
OP_IMPL(double, |, or_)
OP_IMPL(double, ^, xor_)
#undef OP_IMPL

#ifdef VC_IMPL_XOP
static Vc_INTRINSIC Vc_CONST __m128i shiftLeft (const    int_v &value, const    int_v &count) { return _mm_sha_epi32(value.data(), count.data()); }
static Vc_INTRINSIC Vc_CONST __m128i shiftLeft (const   uint_v &value, const   uint_v &count) { return _mm_shl_epi32(value.data(), count.data()); }
static Vc_INTRINSIC Vc_CONST __m128i shiftLeft (const  short_v &value, const  short_v &count) { return _mm_sha_epi16(value.data(), count.data()); }
static Vc_INTRINSIC Vc_CONST __m128i shiftLeft (const ushort_v &value, const ushort_v &count) { return _mm_shl_epi16(value.data(), count.data()); }
static Vc_INTRINSIC Vc_CONST __m128i shiftRight(const    int_v &value, const    int_v &count) { return shiftLeft(value,          -count ); }
static Vc_INTRINSIC Vc_CONST __m128i shiftRight(const   uint_v &value, const   uint_v &count) { return shiftLeft(value,   uint_v(-count)); }
static Vc_INTRINSIC Vc_CONST __m128i shiftRight(const  short_v &value, const  short_v &count) { return shiftLeft(value,          -count ); }
static Vc_INTRINSIC Vc_CONST __m128i shiftRight(const ushort_v &value, const ushort_v &count) { return shiftLeft(value, ushort_v(-count)); }

#define _VC_OP(T, symbol, impl) \
template<> Vc_INTRINSIC T &T::operator symbol##=(T::AsArg shift) \
{ \
    d.v() = impl(*this, shift); \
    return *this; \
} \
template<> Vc_INTRINSIC Vc_PURE T  T::operator symbol   (T::AsArg shift) const \
{ \
    return impl(*this, shift); \
}
VC_APPLY_2(VC_LIST_INT_VECTOR_TYPES, _VC_OP, <<, shiftLeft)
VC_APPLY_2(VC_LIST_INT_VECTOR_TYPES, _VC_OP, >>, shiftRight)
#undef _VC_OP
#else
#if defined(VC_GCC) && VC_GCC == 0x40600 && defined(VC_IMPL_XOP)
#define VC_WORKAROUND __attribute__((optimize("no-tree-vectorize"),weak))
#else
#define VC_WORKAROUND Vc_INTRINSIC
#endif

#define OP_IMPL(T, symbol) \
template<> VC_WORKAROUND Vector<T> &Vector<T>::operator symbol##=(Vector<T>::AsArg x) \
{ \
    for_all_vector_entries(i, \
            d.m(i) symbol##= x.d.m(i); \
            ); \
    return *this; \
} \
template<> inline Vc_PURE Vector<T>  Vector<T>::operator symbol(Vector<T>::AsArg x) const \
{ \
    Vector<T> r; \
    for_all_vector_entries(i, \
            r.d.m(i) = d.m(i) symbol x.d.m(i); \
            ); \
    return r; \
}
OP_IMPL(int, <<)
OP_IMPL(int, >>)
OP_IMPL(unsigned int, <<)
OP_IMPL(unsigned int, >>)
OP_IMPL(short, <<)
OP_IMPL(short, >>)
OP_IMPL(unsigned short, <<)
OP_IMPL(unsigned short, >>)
#undef OP_IMPL
#undef VC_WORKAROUND
#endif

template<typename T> Vc_ALWAYS_INLINE Vector<T> &Vector<T>::operator>>=(int shift) {
    d.v() = VectorHelper<T>::shiftRight(d.v(), shift);
    return *this;
}
template<typename T> Vc_ALWAYS_INLINE Vc_PURE Vector<T> Vector<T>::operator>>(int shift) const {
    return VectorHelper<T>::shiftRight(d.v(), shift);
}
template<typename T> Vc_ALWAYS_INLINE Vector<T> &Vector<T>::operator<<=(int shift) {
    d.v() = VectorHelper<T>::shiftLeft(d.v(), shift);
    return *this;
}
template<typename T> Vc_ALWAYS_INLINE Vc_PURE Vector<T> Vector<T>::operator<<(int shift) const {
    return VectorHelper<T>::shiftLeft(d.v(), shift);
}

///////////////////////////////////////////////////////////////////////////////////////////
// swizzles {{{1
template<typename T> Vc_INTRINSIC Vc_PURE const Vector<T> &Vector<T>::abcd() const { return *this; }
template<typename T> Vc_INTRINSIC Vc_PURE const Vector<T>  Vector<T>::cdab() const { return Mem::permute<X2, X3, X0, X1>(data()); }
template<typename T> Vc_INTRINSIC Vc_PURE const Vector<T>  Vector<T>::badc() const { return Mem::permute<X1, X0, X3, X2>(data()); }
template<typename T> Vc_INTRINSIC Vc_PURE const Vector<T>  Vector<T>::aaaa() const { return Mem::permute<X0, X0, X0, X0>(data()); }
template<typename T> Vc_INTRINSIC Vc_PURE const Vector<T>  Vector<T>::bbbb() const { return Mem::permute<X1, X1, X1, X1>(data()); }
template<typename T> Vc_INTRINSIC Vc_PURE const Vector<T>  Vector<T>::cccc() const { return Mem::permute<X2, X2, X2, X2>(data()); }
template<typename T> Vc_INTRINSIC Vc_PURE const Vector<T>  Vector<T>::dddd() const { return Mem::permute<X3, X3, X3, X3>(data()); }
template<typename T> Vc_INTRINSIC Vc_PURE const Vector<T>  Vector<T>::bcad() const { return Mem::permute<X1, X2, X0, X3>(data()); }
template<typename T> Vc_INTRINSIC Vc_PURE const Vector<T>  Vector<T>::bcda() const { return Mem::permute<X1, X2, X3, X0>(data()); }
template<typename T> Vc_INTRINSIC Vc_PURE const Vector<T>  Vector<T>::dabc() const { return Mem::permute<X3, X0, X1, X2>(data()); }
template<typename T> Vc_INTRINSIC Vc_PURE const Vector<T>  Vector<T>::acbd() const { return Mem::permute<X0, X2, X1, X3>(data()); }
template<typename T> Vc_INTRINSIC Vc_PURE const Vector<T>  Vector<T>::dbca() const { return Mem::permute<X3, X1, X2, X0>(data()); }
template<typename T> Vc_INTRINSIC Vc_PURE const Vector<T>  Vector<T>::dcba() const { return Mem::permute<X3, X2, X1, X0>(data()); }

#define VC_SWIZZLES_16BIT_IMPL(T) \
template<> Vc_INTRINSIC Vc_PURE const Vector<T> Vector<T>::cdab() const { return Mem::permute<X2, X3, X0, X1, X6, X7, X4, X5>(data()); } \
template<> Vc_INTRINSIC Vc_PURE const Vector<T> Vector<T>::badc() const { return Mem::permute<X1, X0, X3, X2, X5, X4, X7, X6>(data()); } \
template<> Vc_INTRINSIC Vc_PURE const Vector<T> Vector<T>::aaaa() const { return Mem::permute<X0, X0, X0, X0, X4, X4, X4, X4>(data()); } \
template<> Vc_INTRINSIC Vc_PURE const Vector<T> Vector<T>::bbbb() const { return Mem::permute<X1, X1, X1, X1, X5, X5, X5, X5>(data()); } \
template<> Vc_INTRINSIC Vc_PURE const Vector<T> Vector<T>::cccc() const { return Mem::permute<X2, X2, X2, X2, X6, X6, X6, X6>(data()); } \
template<> Vc_INTRINSIC Vc_PURE const Vector<T> Vector<T>::dddd() const { return Mem::permute<X3, X3, X3, X3, X7, X7, X7, X7>(data()); } \
template<> Vc_INTRINSIC Vc_PURE const Vector<T> Vector<T>::bcad() const { return Mem::permute<X1, X2, X0, X3, X5, X6, X4, X7>(data()); } \
template<> Vc_INTRINSIC Vc_PURE const Vector<T> Vector<T>::bcda() const { return Mem::permute<X1, X2, X3, X0, X5, X6, X7, X4>(data()); } \
template<> Vc_INTRINSIC Vc_PURE const Vector<T> Vector<T>::dabc() const { return Mem::permute<X3, X0, X1, X2, X7, X4, X5, X6>(data()); } \
template<> Vc_INTRINSIC Vc_PURE const Vector<T> Vector<T>::acbd() const { return Mem::permute<X0, X2, X1, X3, X4, X6, X5, X7>(data()); } \
template<> Vc_INTRINSIC Vc_PURE const Vector<T> Vector<T>::dbca() const { return Mem::permute<X3, X1, X2, X0, X7, X5, X6, X4>(data()); } \
template<> Vc_INTRINSIC Vc_PURE const Vector<T> Vector<T>::dcba() const { return Mem::permute<X3, X2, X1, X0, X7, X6, X5, X4>(data()); }
VC_SWIZZLES_16BIT_IMPL(short)
VC_SWIZZLES_16BIT_IMPL(unsigned short)
#undef VC_SWIZZLES_16BIT_IMPL

// operators {{{1
#include "../common/operators.h"
// isNegative {{{1
template<> Vc_INTRINSIC Vc_PURE float_m float_v::isNegative() const
{
    return sse_cast<__m128>(_mm_srai_epi32(sse_cast<__m128i>(_mm_and_ps(_mm_setsignmask_ps(), d.v())), 31));
}
template<> Vc_INTRINSIC Vc_PURE double_m double_v::isNegative() const
{
    return Mem::permute<X1, X1, X3, X3>(sse_cast<__m128>(
                _mm_srai_epi32(sse_cast<__m128i>(_mm_and_pd(_mm_setsignmask_pd(), d.v())), 31)
                ));
}
// gathers {{{1
template<typename T> template<typename IndexT> Vc_ALWAYS_INLINE Vector<T>::Vector(const EntryType *mem, const IndexT *indexes)
{
    gather(mem, indexes);
}
template<typename T> template<typename IndexT> Vc_ALWAYS_INLINE Vector<T>::Vector(const EntryType *mem, VC_ALIGNED_PARAMETER(Vector<IndexT>) indexes)
{
    gather(mem, indexes);
}

template<typename T> template<typename IndexT> Vc_ALWAYS_INLINE Vector<T>::Vector(const EntryType *mem, const IndexT *indexes, MaskArg mask)
    : d(HT::zero())
{
    gather(mem, indexes, mask);
}

template<typename T> template<typename IndexT> Vc_ALWAYS_INLINE Vector<T>::Vector(const EntryType *mem, VC_ALIGNED_PARAMETER(Vector<IndexT>) indexes, MaskArg mask)
    : d(HT::zero())
{
    gather(mem, indexes, mask);
}

template<typename T> template<typename S1, typename IT> Vc_ALWAYS_INLINE Vector<T>::Vector(const S1 *array, const EntryType S1::* member1, VC_ALIGNED_PARAMETER(IT) indexes)
{
    gather(array, member1, indexes);
}
template<typename T> template<typename S1, typename IT> Vc_ALWAYS_INLINE Vector<T>::Vector(const S1 *array, const EntryType S1::* member1, VC_ALIGNED_PARAMETER(IT) indexes, MaskArg mask)
    : d(HT::zero())
{
    gather(array, member1, indexes, mask);
}
template<typename T> template<typename S1, typename S2, typename IT> Vc_ALWAYS_INLINE Vector<T>::Vector(const S1 *array, const S2 S1::* member1, const EntryType S2::* member2, VC_ALIGNED_PARAMETER(IT) indexes)
{
    gather(array, member1, member2, indexes);
}
template<typename T> template<typename S1, typename S2, typename IT> Vc_ALWAYS_INLINE Vector<T>::Vector(const S1 *array, const S2 S1::* member1, const EntryType S2::* member2, VC_ALIGNED_PARAMETER(IT) indexes, MaskArg mask)
    : d(HT::zero())
{
    gather(array, member1, member2, indexes, mask);
}
template<typename T> template<typename S1, typename IT1, typename IT2> Vc_ALWAYS_INLINE Vector<T>::Vector(const S1 *array, const EntryType *const S1::* ptrMember1, VC_ALIGNED_PARAMETER(IT1) outerIndexes, VC_ALIGNED_PARAMETER(IT2) innerIndexes)
{
    gather(array, ptrMember1, outerIndexes, innerIndexes);
}
template<typename T> template<typename S1, typename IT1, typename IT2> Vc_ALWAYS_INLINE Vector<T>::Vector(const S1 *array, const EntryType *const S1::* ptrMember1, VC_ALIGNED_PARAMETER(IT1) outerIndexes, VC_ALIGNED_PARAMETER(IT2) innerIndexes, MaskArg mask)
    : d(HT::zero())
{
    gather(array, ptrMember1, outerIndexes, innerIndexes, mask);
}

template<typename T, size_t Size> struct IndexSizeChecker { static void check() {} };
template<typename T, size_t Size> struct IndexSizeChecker<Vector<T>, Size>
{
    static void check() {
        static_assert(Vector<T>::Size >= Size, "IndexVector_must_have_greater_or_equal_number_of_entries");
    }
};
template<> template<typename Index> Vc_ALWAYS_INLINE void Vc_FLATTEN Vector<double>::gather(const EntryType *mem, VC_ALIGNED_PARAMETER(Index) indexes)
{
    IndexSizeChecker<Index, Size>::check();
    d.v() = _mm_setr_pd(mem[indexes[0]], mem[indexes[1]]);
}
template<> template<typename Index> Vc_ALWAYS_INLINE void Vc_FLATTEN Vector<float>::gather(const EntryType *mem, VC_ALIGNED_PARAMETER(Index) indexes)
{
    IndexSizeChecker<Index, Size>::check();
    d.v() = _mm_setr_ps(mem[indexes[0]], mem[indexes[1]], mem[indexes[2]], mem[indexes[3]]);
}
template<> template<typename Index> Vc_ALWAYS_INLINE void Vc_FLATTEN Vector<int>::gather(const EntryType *mem, VC_ALIGNED_PARAMETER(Index) indexes)
{
    IndexSizeChecker<Index, Size>::check();
    d.v() = _mm_setr_epi32(mem[indexes[0]], mem[indexes[1]], mem[indexes[2]], mem[indexes[3]]);
}
template<> template<typename Index> Vc_ALWAYS_INLINE void Vc_FLATTEN Vector<unsigned int>::gather(const EntryType *mem, VC_ALIGNED_PARAMETER(Index) indexes)
{
    IndexSizeChecker<Index, Size>::check();
    d.v() = _mm_setr_epi32(mem[indexes[0]], mem[indexes[1]], mem[indexes[2]], mem[indexes[3]]);
}
template<> template<typename Index> Vc_ALWAYS_INLINE void Vc_FLATTEN Vector<short>::gather(const EntryType *mem, VC_ALIGNED_PARAMETER(Index) indexes)
{
    IndexSizeChecker<Index, Size>::check();
    d.v() = set(mem[indexes[0]], mem[indexes[1]], mem[indexes[2]], mem[indexes[3]],
            mem[indexes[4]], mem[indexes[5]], mem[indexes[6]], mem[indexes[7]]);
}
template<> template<typename Index> Vc_ALWAYS_INLINE void Vc_FLATTEN Vector<unsigned short>::gather(const EntryType *mem, VC_ALIGNED_PARAMETER(Index) indexes)
{
    IndexSizeChecker<Index, Size>::check();
    d.v() = set(mem[indexes[0]], mem[indexes[1]], mem[indexes[2]], mem[indexes[3]],
                mem[indexes[4]], mem[indexes[5]], mem[indexes[6]], mem[indexes[7]]);
}

#ifdef VC_USE_SET_GATHERS
template<typename T> template<typename IT> Vc_ALWAYS_INLINE void Vector<T>::gather(const EntryType *mem, VC_ALIGNED_PARAMETER(Vector<IT>) indexes, MaskArg mask)
{
    IndexSizeChecker<Vector<IT>, Size>::check();
    Vector<IT> indexesTmp = indexes;
    indexesTmp.setZero(!static_cast<typename Vector<IT>::Mask>(mask));
    (*this)(mask) = Vector<T>(mem, indexesTmp);
}
#endif

#ifdef VC_USE_BSF_GATHERS
#define VC_MASKED_GATHER                        \
    int bits = mask.toInt();                    \
    while (bits) {                              \
        const int i = _bit_scan_forward(bits);  \
        bits &= ~(1 << i); /* btr? */           \
        d.m(i) = ith_value(i);                  \
    }
#elif defined(VC_USE_POPCNT_BSF_GATHERS)
#define VC_MASKED_GATHER                        \
    unsigned int bits = mask.toInt();           \
    unsigned int low, high = 0;                 \
    switch (mask.count()) {             \
    case 8:                                     \
        high = _bit_scan_reverse(bits);         \
        d.m(high) = ith_value(high);            \
        high = (1 << high);                     \
    case 7:                                     \
        low = _bit_scan_forward(bits);          \
        bits ^= high | (1 << low);              \
        d.m(low) = ith_value(low);              \
    case 6:                                     \
        high = _bit_scan_reverse(bits);         \
        d.m(high) = ith_value(high);            \
        high = (1 << high);                     \
    case 5:                                     \
        low = _bit_scan_forward(bits);          \
        bits ^= high | (1 << low);              \
        d.m(low) = ith_value(low);              \
    case 4:                                     \
        high = _bit_scan_reverse(bits);         \
        d.m(high) = ith_value(high);            \
        high = (1 << high);                     \
    case 3:                                     \
        low = _bit_scan_forward(bits);          \
        bits ^= high | (1 << low);              \
        d.m(low) = ith_value(low);              \
    case 2:                                     \
        high = _bit_scan_reverse(bits);         \
        d.m(high) = ith_value(high);            \
    case 1:                                     \
        low = _bit_scan_forward(bits);          \
        d.m(low) = ith_value(low);              \
    case 0:                                     \
        break;                                  \
    }
#else
#define VC_USE_SPECIALIZED_GATHER 1
#define VC_MASKED_GATHER                        \
    if (VC_IS_UNLIKELY(mask.isEmpty())) {       \
        return;                                 \
    }                                           \
    for_all_vector_entries(i,                   \
            if (mask[i]) d.m(i) = ith_value(i); \
            );
#endif

namespace
{
    template<typename V> static Vc_INTRINSIC_L Vc_PURE_L V setHelper(const typename V::EntryType *mem, const size_t ii[]) Vc_INTRINSIC_R Vc_PURE_R;
    template<> Vc_INTRINSIC Vc_PURE float_v setHelper(const float_v::EntryType *mem, const size_t ii[])
    {
        return _mm_setr_ps(mem[ii[0]], mem[ii[1]], mem[ii[2]], mem[ii[3]]);
    }
    template<> Vc_INTRINSIC Vc_PURE double_v setHelper(const double_v::EntryType *mem, const size_t ii[])
    {
        return _mm_setr_pd(mem[ii[0]], mem[ii[1]]);
    }
    template<> Vc_INTRINSIC Vc_PURE int_v setHelper(const int_v::EntryType *mem, const size_t ii[])
    {
        return _mm_setr_epi32(mem[ii[0]], mem[ii[1]], mem[ii[2]], mem[ii[3]]);
    }
    template<> Vc_INTRINSIC Vc_PURE uint_v setHelper(const uint_v::EntryType *mem, const size_t ii[])
    {
        return _mm_setr_epi32(mem[ii[0]], mem[ii[1]], mem[ii[2]], mem[ii[3]]);
    }
    template<> Vc_INTRINSIC Vc_PURE short_v setHelper(const short_v::EntryType *mem, const size_t ii[])
    {
        return set(mem[ii[0]], mem[ii[1]], mem[ii[2]], mem[ii[3]], mem[ii[4]], mem[ii[5]], mem[ii[6]], mem[ii[7]]);
    }
    template<> Vc_INTRINSIC Vc_PURE ushort_v setHelper(const ushort_v::EntryType *mem, const size_t ii[])
    {
        return set(mem[ii[0]], mem[ii[1]], mem[ii[2]], mem[ii[3]], mem[ii[4]], mem[ii[5]], mem[ii[6]], mem[ii[7]]);
    }

    template<size_t S> struct DetermineUInt { typedef unsigned int Type; };
    template<> struct DetermineUInt<2> { typedef unsigned short Type; };
    template<> struct DetermineUInt<1> { typedef unsigned char Type; };
    template<> struct DetermineUInt<8> { typedef unsigned long long Type; };
} // anonymous namespace

template<typename T> template<typename Index>
Vc_INTRINSIC void Vector<T>::gather(const EntryType *mem, VC_ALIGNED_PARAMETER(Index) indexes, MaskArg mask)
{
    IndexSizeChecker<Index, Size>::check();
#ifdef VC_USE_SPECIALIZED_GATHER
#undef VC_USE_SPECIALIZED_GATHER
    if (VC_IS_UNLIKELY(mask.isEmpty())) {
        return;
    }
    size_t ii[Size];
    for_all_vector_entries(i,
            ii[i] = mask[i] ? static_cast<typename DetermineUInt<sizeof(indexes[i])>::Type>(indexes[i]) : 0;
            );
    (*this)(mask) = setHelper<Vector<T> >(mem, ii);
#else
#define ith_value(_i_) (mem[static_cast<typename DetermineUInt<sizeof(indexes[0])>::Type>(indexes[_i_])])
    VC_MASKED_GATHER
#undef ith_value
#endif
}

template<> template<typename S1, typename IT>
Vc_ALWAYS_INLINE void Vc_FLATTEN Vector<double>::gather(const S1 *array, const EntryType S1::* member1, VC_ALIGNED_PARAMETER(IT) indexes)
{
    IndexSizeChecker<IT, Size>::check();
    d.v() = _mm_setr_pd(array[indexes[0]].*(member1), array[indexes[1]].*(member1));
}
template<> template<typename S1, typename IT>
Vc_ALWAYS_INLINE void Vc_FLATTEN Vector<float>::gather(const S1 *array, const EntryType S1::* member1, VC_ALIGNED_PARAMETER(IT) indexes)
{
    IndexSizeChecker<IT, Size>::check();
    d.v() = _mm_setr_ps(array[indexes[0]].*(member1), array[indexes[1]].*(member1), array[indexes[2]].*(member1),
            array[indexes[3]].*(member1));
}
template<> template<typename S1, typename IT>
Vc_ALWAYS_INLINE void Vc_FLATTEN Vector<int>::gather(const S1 *array, const EntryType S1::* member1, VC_ALIGNED_PARAMETER(IT) indexes)
{
    IndexSizeChecker<IT, Size>::check();
    d.v() = _mm_setr_epi32(array[indexes[0]].*(member1), array[indexes[1]].*(member1), array[indexes[2]].*(member1),
            array[indexes[3]].*(member1));
}
template<> template<typename S1, typename IT>
Vc_ALWAYS_INLINE void Vc_FLATTEN Vector<unsigned int>::gather(const S1 *array, const EntryType S1::* member1, VC_ALIGNED_PARAMETER(IT) indexes)
{
    IndexSizeChecker<IT, Size>::check();
    d.v() = _mm_setr_epi32(array[indexes[0]].*(member1), array[indexes[1]].*(member1), array[indexes[2]].*(member1),
            array[indexes[3]].*(member1));
}
template<> template<typename S1, typename IT>
Vc_ALWAYS_INLINE void Vc_FLATTEN Vector<short>::gather(const S1 *array, const EntryType S1::* member1, VC_ALIGNED_PARAMETER(IT) indexes)
{
    IndexSizeChecker<IT, Size>::check();
    d.v() = set(array[indexes[0]].*(member1), array[indexes[1]].*(member1), array[indexes[2]].*(member1),
            array[indexes[3]].*(member1), array[indexes[4]].*(member1), array[indexes[5]].*(member1),
            array[indexes[6]].*(member1), array[indexes[7]].*(member1));
}
template<> template<typename S1, typename IT>
Vc_ALWAYS_INLINE void Vc_FLATTEN Vector<unsigned short>::gather(const S1 *array, const EntryType S1::* member1, VC_ALIGNED_PARAMETER(IT) indexes)
{
    IndexSizeChecker<IT, Size>::check();
    d.v() = set(array[indexes[0]].*(member1), array[indexes[1]].*(member1), array[indexes[2]].*(member1),
            array[indexes[3]].*(member1), array[indexes[4]].*(member1), array[indexes[5]].*(member1),
            array[indexes[6]].*(member1), array[indexes[7]].*(member1));
}
template<typename T> template<typename S1, typename IT>
Vc_ALWAYS_INLINE void Vc_FLATTEN Vector<T>::gather(const S1 *array, const EntryType S1::* member1, VC_ALIGNED_PARAMETER(IT) indexes, MaskArg mask)
{
    IndexSizeChecker<IT, Size>::check();
#define ith_value(_i_) (array[indexes[_i_]].*(member1))
    VC_MASKED_GATHER
#undef ith_value
}
template<> template<typename S1, typename S2, typename IT>
Vc_ALWAYS_INLINE void Vc_FLATTEN Vector<double>::gather(const S1 *array, const S2 S1::* member1, const EntryType S2::* member2, VC_ALIGNED_PARAMETER(IT) indexes)
{
    IndexSizeChecker<IT, Size>::check();
    d.v() = _mm_setr_pd(array[indexes[0]].*(member1).*(member2), array[indexes[1]].*(member1).*(member2));
}
template<> template<typename S1, typename S2, typename IT>
Vc_ALWAYS_INLINE void Vc_FLATTEN Vector<float>::gather(const S1 *array, const S2 S1::* member1, const EntryType S2::* member2, VC_ALIGNED_PARAMETER(IT) indexes)
{
    IndexSizeChecker<IT, Size>::check();
    d.v() = _mm_setr_ps(array[indexes[0]].*(member1).*(member2), array[indexes[1]].*(member1).*(member2), array[indexes[2]].*(member1).*(member2),
            array[indexes[3]].*(member1).*(member2));
}
template<> template<typename S1, typename S2, typename IT>
Vc_ALWAYS_INLINE void Vc_FLATTEN Vector<int>::gather(const S1 *array, const S2 S1::* member1, const EntryType S2::* member2, VC_ALIGNED_PARAMETER(IT) indexes)
{
    IndexSizeChecker<IT, Size>::check();
    d.v() = _mm_setr_epi32(array[indexes[0]].*(member1).*(member2), array[indexes[1]].*(member1).*(member2),
            array[indexes[2]].*(member1).*(member2), array[indexes[3]].*(member1).*(member2));
}
template<> template<typename S1, typename S2, typename IT>
Vc_ALWAYS_INLINE void Vc_FLATTEN Vector<unsigned int>::gather(const S1 *array, const S2 S1::* member1, const EntryType S2::* member2, VC_ALIGNED_PARAMETER(IT) indexes)
{
    IndexSizeChecker<IT, Size>::check();
    d.v() = _mm_setr_epi32(array[indexes[0]].*(member1).*(member2), array[indexes[1]].*(member1).*(member2),
            array[indexes[2]].*(member1).*(member2), array[indexes[3]].*(member1).*(member2));
}
template<> template<typename S1, typename S2, typename IT>
Vc_ALWAYS_INLINE void Vc_FLATTEN Vector<short>::gather(const S1 *array, const S2 S1::* member1, const EntryType S2::* member2, VC_ALIGNED_PARAMETER(IT) indexes)
{
    IndexSizeChecker<IT, Size>::check();
    d.v() = set(array[indexes[0]].*(member1).*(member2), array[indexes[1]].*(member1).*(member2), array[indexes[2]].*(member1).*(member2),
            array[indexes[3]].*(member1).*(member2), array[indexes[4]].*(member1).*(member2), array[indexes[5]].*(member1).*(member2),
            array[indexes[6]].*(member1).*(member2), array[indexes[7]].*(member1).*(member2));
}
template<> template<typename S1, typename S2, typename IT>
Vc_ALWAYS_INLINE void Vc_FLATTEN Vector<unsigned short>::gather(const S1 *array, const S2 S1::* member1, const EntryType S2::* member2, VC_ALIGNED_PARAMETER(IT) indexes)
{
    IndexSizeChecker<IT, Size>::check();
    d.v() = set(array[indexes[0]].*(member1).*(member2), array[indexes[1]].*(member1).*(member2), array[indexes[2]].*(member1).*(member2),
            array[indexes[3]].*(member1).*(member2), array[indexes[4]].*(member1).*(member2), array[indexes[5]].*(member1).*(member2),
            array[indexes[6]].*(member1).*(member2), array[indexes[7]].*(member1).*(member2));
}
template<typename T> template<typename S1, typename S2, typename IT>
Vc_ALWAYS_INLINE void Vc_FLATTEN Vector<T>::gather(const S1 *array, const S2 S1::* member1, const EntryType S2::* member2, VC_ALIGNED_PARAMETER(IT) indexes, MaskArg mask)
{
    IndexSizeChecker<IT, Size>::check();
#define ith_value(_i_) (array[indexes[_i_]].*(member1).*(member2))
    VC_MASKED_GATHER
#undef ith_value
}
template<> template<typename S1, typename IT1, typename IT2>
Vc_ALWAYS_INLINE void Vc_FLATTEN Vector<double>::gather(const S1 *array, const EntryType *const S1::* ptrMember1, VC_ALIGNED_PARAMETER(IT1) outerIndexes, VC_ALIGNED_PARAMETER(IT2) innerIndexes)
{
    IndexSizeChecker<IT1, Size>::check();
    IndexSizeChecker<IT2, Size>::check();
    d.v() = _mm_setr_pd((array[outerIndexes[0]].*(ptrMember1))[innerIndexes[0]],
            (array[outerIndexes[1]].*(ptrMember1))[innerIndexes[1]]);
}
template<> template<typename S1, typename IT1, typename IT2>
Vc_ALWAYS_INLINE void Vc_FLATTEN Vector<float>::gather(const S1 *array, const EntryType *const S1::* ptrMember1, VC_ALIGNED_PARAMETER(IT1) outerIndexes, VC_ALIGNED_PARAMETER(IT2) innerIndexes)
{
    IndexSizeChecker<IT1, Size>::check();
    IndexSizeChecker<IT2, Size>::check();
    d.v() = _mm_setr_ps((array[outerIndexes[0]].*(ptrMember1))[innerIndexes[0]], (array[outerIndexes[1]].*(ptrMember1))[innerIndexes[1]],
            (array[outerIndexes[2]].*(ptrMember1))[innerIndexes[2]], (array[outerIndexes[3]].*(ptrMember1))[innerIndexes[3]]);
}
template<> template<typename S1, typename IT1, typename IT2>
Vc_ALWAYS_INLINE void Vc_FLATTEN Vector<int>::gather(const S1 *array, const EntryType *const S1::* ptrMember1, VC_ALIGNED_PARAMETER(IT1) outerIndexes, VC_ALIGNED_PARAMETER(IT2) innerIndexes)
{
    IndexSizeChecker<IT1, Size>::check();
    IndexSizeChecker<IT2, Size>::check();
    d.v() = _mm_setr_epi32((array[outerIndexes[0]].*(ptrMember1))[innerIndexes[0]], (array[outerIndexes[1]].*(ptrMember1))[innerIndexes[1]],
            (array[outerIndexes[2]].*(ptrMember1))[innerIndexes[2]], (array[outerIndexes[3]].*(ptrMember1))[innerIndexes[3]]);
}
template<> template<typename S1, typename IT1, typename IT2>
Vc_ALWAYS_INLINE void Vc_FLATTEN Vector<unsigned int>::gather(const S1 *array, const EntryType *const S1::* ptrMember1, VC_ALIGNED_PARAMETER(IT1) outerIndexes, VC_ALIGNED_PARAMETER(IT2) innerIndexes)
{
    IndexSizeChecker<IT1, Size>::check();
    IndexSizeChecker<IT2, Size>::check();
    d.v() = _mm_setr_epi32((array[outerIndexes[0]].*(ptrMember1))[innerIndexes[0]], (array[outerIndexes[1]].*(ptrMember1))[innerIndexes[1]],
            (array[outerIndexes[2]].*(ptrMember1))[innerIndexes[2]], (array[outerIndexes[3]].*(ptrMember1))[innerIndexes[3]]);
}
template<> template<typename S1, typename IT1, typename IT2>
Vc_ALWAYS_INLINE void Vc_FLATTEN Vector<short>::gather(const S1 *array, const EntryType *const S1::* ptrMember1, VC_ALIGNED_PARAMETER(IT1) outerIndexes, VC_ALIGNED_PARAMETER(IT2) innerIndexes)
{
    IndexSizeChecker<IT1, Size>::check();
    IndexSizeChecker<IT2, Size>::check();
    d.v() = set((array[outerIndexes[0]].*(ptrMember1))[innerIndexes[0]], (array[outerIndexes[1]].*(ptrMember1))[innerIndexes[1]],
            (array[outerIndexes[2]].*(ptrMember1))[innerIndexes[2]], (array[outerIndexes[3]].*(ptrMember1))[innerIndexes[3]],
            (array[outerIndexes[4]].*(ptrMember1))[innerIndexes[4]], (array[outerIndexes[5]].*(ptrMember1))[innerIndexes[5]],
            (array[outerIndexes[6]].*(ptrMember1))[innerIndexes[6]], (array[outerIndexes[7]].*(ptrMember1))[innerIndexes[7]]);
}
template<> template<typename S1, typename IT1, typename IT2>
Vc_ALWAYS_INLINE void Vc_FLATTEN Vector<unsigned short>::gather(const S1 *array, const EntryType *const S1::* ptrMember1, VC_ALIGNED_PARAMETER(IT1) outerIndexes, VC_ALIGNED_PARAMETER(IT2) innerIndexes)
{
    IndexSizeChecker<IT1, Size>::check();
    IndexSizeChecker<IT2, Size>::check();
    d.v() = set((array[outerIndexes[0]].*(ptrMember1))[innerIndexes[0]], (array[outerIndexes[1]].*(ptrMember1))[innerIndexes[1]],
            (array[outerIndexes[2]].*(ptrMember1))[innerIndexes[2]], (array[outerIndexes[3]].*(ptrMember1))[innerIndexes[3]],
            (array[outerIndexes[4]].*(ptrMember1))[innerIndexes[4]], (array[outerIndexes[5]].*(ptrMember1))[innerIndexes[5]],
            (array[outerIndexes[6]].*(ptrMember1))[innerIndexes[6]], (array[outerIndexes[7]].*(ptrMember1))[innerIndexes[7]]);
}
template<typename T> template<typename S1, typename IT1, typename IT2>
Vc_ALWAYS_INLINE void Vc_FLATTEN Vector<T>::gather(const S1 *array, const EntryType *const S1::* ptrMember1, VC_ALIGNED_PARAMETER(IT1) outerIndexes, VC_ALIGNED_PARAMETER(IT2) innerIndexes, MaskArg mask)
{
    IndexSizeChecker<IT1, Size>::check();
    IndexSizeChecker<IT2, Size>::check();
#define ith_value(_i_) (array[outerIndexes[_i_]].*(ptrMember1))[innerIndexes[_i_]]
    VC_MASKED_GATHER
#undef ith_value
}
// scatters {{{1
#undef VC_MASKED_GATHER
#ifdef VC_USE_BSF_SCATTERS
#define VC_MASKED_SCATTER                       \
    int bits = mask.toInt();                    \
    while (bits) {                              \
        const int i = _bit_scan_forward(bits);  \
        bits ^= (1 << i); /* btr? */            \
        ith_value(i) = d.m(i);                  \
    }
#elif defined(VC_USE_POPCNT_BSF_SCATTERS)
#define VC_MASKED_SCATTER                       \
    unsigned int bits = mask.toInt();           \
    unsigned int low, high = 0;                 \
    switch (mask.count()) {             \
    case 8:                                     \
        high = _bit_scan_reverse(bits);         \
        ith_value(high) = d.m(high);            \
        high = (1 << high);                     \
    case 7:                                     \
        low = _bit_scan_forward(bits);          \
        bits ^= high | (1 << low);              \
        ith_value(low) = d.m(low);              \
    case 6:                                     \
        high = _bit_scan_reverse(bits);         \
        ith_value(high) = d.m(high);            \
        high = (1 << high);                     \
    case 5:                                     \
        low = _bit_scan_forward(bits);          \
        bits ^= high | (1 << low);              \
        ith_value(low) = d.m(low);              \
    case 4:                                     \
        high = _bit_scan_reverse(bits);         \
        ith_value(high) = d.m(high);            \
        high = (1 << high);                     \
    case 3:                                     \
        low = _bit_scan_forward(bits);          \
        bits ^= high | (1 << low);              \
        ith_value(low) = d.m(low);              \
    case 2:                                     \
        high = _bit_scan_reverse(bits);         \
        ith_value(high) = d.m(high);            \
    case 1:                                     \
        low = _bit_scan_forward(bits);          \
        ith_value(low) = d.m(low);              \
    case 0:                                     \
        break;                                  \
    }
#else
#define VC_MASKED_SCATTER                       \
    if (mask.isEmpty()) {                       \
        return;                                 \
    }                                           \
    for_all_vector_entries(i,                   \
            if (mask[i]) ith_value(i) = d.m(i); \
            );
#endif

template<typename T> template<typename Index> Vc_ALWAYS_INLINE void Vc_FLATTEN Vector<T>::scatter(EntryType *mem, VC_ALIGNED_PARAMETER(Index) indexes) const
{
    for_all_vector_entries(i,
            mem[indexes[i]] = d.m(i);
            );
}
template<typename T> template<typename Index> Vc_ALWAYS_INLINE void Vc_FLATTEN Vector<T>::scatter(EntryType *mem, VC_ALIGNED_PARAMETER(Index) indexes, MaskArg mask) const
{
#define ith_value(_i_) mem[indexes[_i_]]
    VC_MASKED_SCATTER
#undef ith_value
}
template<typename T> template<typename S1, typename IT> Vc_ALWAYS_INLINE void Vc_FLATTEN Vector<T>::scatter(S1 *array, EntryType S1::* member1, VC_ALIGNED_PARAMETER(IT) indexes) const
{
    for_all_vector_entries(i,
            array[indexes[i]].*(member1) = d.m(i);
            );
}
template<typename T> template<typename S1, typename IT> Vc_ALWAYS_INLINE void Vc_FLATTEN Vector<T>::scatter(S1 *array, EntryType S1::* member1, VC_ALIGNED_PARAMETER(IT) indexes, MaskArg mask) const
{
#define ith_value(_i_) array[indexes[_i_]].*(member1)
    VC_MASKED_SCATTER
#undef ith_value
}
template<typename T> template<typename S1, typename S2, typename IT> Vc_ALWAYS_INLINE void Vc_FLATTEN Vector<T>::scatter(S1 *array, S2 S1::* member1, EntryType S2::* member2, VC_ALIGNED_PARAMETER(IT) indexes) const
{
    for_all_vector_entries(i,
            array[indexes[i]].*(member1).*(member2) = d.m(i);
            );
}
template<typename T> template<typename S1, typename S2, typename IT> Vc_ALWAYS_INLINE void Vc_FLATTEN Vector<T>::scatter(S1 *array, S2 S1::* member1, EntryType S2::* member2, VC_ALIGNED_PARAMETER(IT) indexes, MaskArg mask) const
{
#define ith_value(_i_) array[indexes[_i_]].*(member1).*(member2)
    VC_MASKED_SCATTER
#undef ith_value
}
template<typename T> template<typename S1, typename IT1, typename IT2> Vc_ALWAYS_INLINE void Vc_FLATTEN Vector<T>::scatter(S1 *array, EntryType *S1::* ptrMember1, VC_ALIGNED_PARAMETER(IT1) outerIndexes, VC_ALIGNED_PARAMETER(IT2) innerIndexes) const
{
    for_all_vector_entries(i,
            (array[innerIndexes[i]].*(ptrMember1))[outerIndexes[i]] = d.m(i);
            );
}
template<typename T> template<typename S1, typename IT1, typename IT2> Vc_ALWAYS_INLINE void Vc_FLATTEN Vector<T>::scatter(S1 *array, EntryType *S1::* ptrMember1, VC_ALIGNED_PARAMETER(IT1) outerIndexes, VC_ALIGNED_PARAMETER(IT2) innerIndexes, MaskArg mask) const
{
#define ith_value(_i_) (array[outerIndexes[_i_]].*(ptrMember1))[innerIndexes[_i_]]
    VC_MASKED_SCATTER
#undef ith_value
}

///////////////////////////////////////////////////////////////////////////////////////////
// operator[] {{{1
template<typename T> Vc_INTRINSIC typename Vector<T>::EntryType Vc_PURE Vector<T>::operator[](size_t index) const
{
    return d.m(index);
}
#ifdef VC_GCC
template<> Vc_INTRINSIC double Vc_PURE Vector<double>::operator[](size_t index) const
{
    if (__builtin_constant_p(index)) {
        return extract_double_imm(d.v(), index);
    }
    return d.m(index);
}
template<> Vc_INTRINSIC float Vc_PURE Vector<float>::operator[](size_t index) const
{
    return extract_float(d.v(), index);
}
template<> Vc_INTRINSIC int Vc_PURE Vector<int>::operator[](size_t index) const
{
    if (__builtin_constant_p(index)) {
#if VC_GCC >= 0x40601 || !defined(VC_USE_VEX_CODING) // GCC < 4.6.1 incorrectly uses vmovq instead of movq for the following
#ifdef __x86_64__
        if (index == 0) return _mm_cvtsi128_si64(d.v()) & 0xFFFFFFFFull;
        if (index == 1) return _mm_cvtsi128_si64(d.v()) >> 32;
#else
        if (index == 0) return _mm_cvtsi128_si32(d.v());
#endif
#endif
        return _mm_extract_epi32(d.v(), index);
    }
    return d.m(index);
}
template<> Vc_INTRINSIC unsigned int Vc_PURE Vector<unsigned int>::operator[](size_t index) const
{
    if (__builtin_constant_p(index)) {
#if VC_GCC >= 0x40601 || !defined(VC_USE_VEX_CODING) // GCC < 4.6.1 incorrectly uses vmovq instead of movq for the following
#ifdef __x86_64__
        if (index == 0) return _mm_cvtsi128_si64(d.v()) & 0xFFFFFFFFull;
        if (index == 1) return _mm_cvtsi128_si64(d.v()) >> 32;
#else
        if (index == 0) return _mm_cvtsi128_si32(d.v());
#endif
#endif
        return _mm_extract_epi32(d.v(), index);
    }
    return d.m(index);
}
template<> Vc_INTRINSIC short Vc_PURE Vector<short>::operator[](size_t index) const
{
    if (__builtin_constant_p(index)) {
        return _mm_extract_epi16(d.v(), index);
    }
    return d.m(index);
}
template<> Vc_INTRINSIC unsigned short Vc_PURE Vector<unsigned short>::operator[](size_t index) const
{
    if (__builtin_constant_p(index)) {
        return _mm_extract_epi16(d.v(), index);
    }
    return d.m(index);
}
#endif // GCC
///////////////////////////////////////////////////////////////////////////////////////////
// horizontal ops {{{1
template<typename T> Vc_ALWAYS_INLINE Vector<T> Vector<T>::partialSum() const
{
    //   a    b    c    d    e    f    g    h
    // +      a    b    c    d    e    f    g    -> a ab bc  cd   de    ef     fg      gh
    // +           a    ab   bc   cd   de   ef   -> a ab abc abcd bcde  cdef   defg    efgh
    // +                     a    ab   abc  abcd -> a ab abc abcd abcde abcdef abcdefg abcdefgh
    Vector<T> tmp = *this;
    if (Size >  1) tmp += tmp.shifted(-1);
    if (Size >  2) tmp += tmp.shifted(-2);
    if (Size >  4) tmp += tmp.shifted(-4);
    if (Size >  8) tmp += tmp.shifted(-8);
    if (Size > 16) tmp += tmp.shifted(-16);
    return tmp;
}
#ifndef VC_IMPL_SSE4_1
// without SSE4.1 integer multiplication is slow and we rather multiply the scalars
template<> Vc_INTRINSIC Vc_PURE int Vector<int>::product() const
{
    return (d.m(0) * d.m(1)) * (d.m(2) * d.m(3));
}
template<> Vc_INTRINSIC Vc_PURE unsigned int Vector<unsigned int>::product() const
{
    return (d.m(0) * d.m(1)) * (d.m(2) * d.m(3));
}
#endif
template<typename T> Vc_ALWAYS_INLINE Vc_PURE typename Vector<T>::EntryType Vector<T>::min(MaskArg m) const
{
    Vector<T> tmp = std::numeric_limits<Vector<T> >::max();
    tmp(m) = *this;
    return tmp.min();
}
template<typename T> Vc_ALWAYS_INLINE Vc_PURE typename Vector<T>::EntryType Vector<T>::max(MaskArg m) const
{
    Vector<T> tmp = std::numeric_limits<Vector<T> >::min();
    tmp(m) = *this;
    return tmp.max();
}
template<typename T> Vc_ALWAYS_INLINE Vc_PURE typename Vector<T>::EntryType Vector<T>::product(MaskArg m) const
{
    Vector<T> tmp(VectorSpecialInitializerOne::One);
    tmp(m) = *this;
    return tmp.product();
}
template<typename T> Vc_ALWAYS_INLINE Vc_PURE typename Vector<T>::EntryType Vector<T>::sum(MaskArg m) const
{
    Vector<T> tmp(VectorSpecialInitializerZero::Zero);
    tmp(m) = *this;
    return tmp.sum();
}

///////////////////////////////////////////////////////////////////////////////////////////
// copySign {{{1
template<> Vc_INTRINSIC Vc_PURE Vector<float> Vector<float>::copySign(Vector<float>::AsArg reference) const
{
    return _mm_or_ps(
            _mm_and_ps(reference.d.v(), _mm_setsignmask_ps()),
            _mm_and_ps(d.v(), _mm_setabsmask_ps())
            );
}
template<> Vc_INTRINSIC Vc_PURE Vector<double> Vector<double>::copySign(Vector<double>::AsArg reference) const
{
    return _mm_or_pd(
            _mm_and_pd(reference.d.v(), _mm_setsignmask_pd()),
            _mm_and_pd(d.v(), _mm_setabsmask_pd())
            );
}//}}}1
// exponent {{{1
template<> Vc_INTRINSIC Vc_PURE Vector<float> Vector<float>::exponent() const
{
    VC_ASSERT((*this >= 0.f).isFull());
    return Internal::exponent(d.v());
}
template<> Vc_INTRINSIC Vc_PURE Vector<double> Vector<double>::exponent() const
{
    VC_ASSERT((*this >= 0.).isFull());
    return Internal::exponent(d.v());
}
// }}}1
// Random {{{1
static void _doRandomStep(Vector<unsigned int> &state0,
        Vector<unsigned int> &state1)
{
    state0.load(&Common::RandomState[0]);
    state1.load(&Common::RandomState[uint_v::Size]);
    (state1 * 0xdeece66du + 11).store(&Common::RandomState[uint_v::Size]);
    uint_v(_mm_xor_si128((state0 * 0xdeece66du + 11).data(), _mm_srli_epi32(state1.data(), 16))).store(&Common::RandomState[0]);
}

template<typename T> Vc_ALWAYS_INLINE Vector<T> Vector<T>::Random()
{
    Vector<unsigned int> state0, state1;
    _doRandomStep(state0, state1);
    return state0.reinterpretCast<Vector<T> >();
}

template<> Vc_ALWAYS_INLINE Vector<float> Vector<float>::Random()
{
    Vector<unsigned int> state0, state1;
    _doRandomStep(state0, state1);
    return _mm_sub_ps(_mm_or_ps(_mm_castsi128_ps(_mm_srli_epi32(state0.data(), 2)), HT::one()), HT::one());
}

template<> Vc_ALWAYS_INLINE Vector<double> Vector<double>::Random()
{
    typedef unsigned long long uint64 Vc_MAY_ALIAS;
    uint64 state0 = *reinterpret_cast<const uint64 *>(&Common::RandomState[8]);
    uint64 state1 = *reinterpret_cast<const uint64 *>(&Common::RandomState[10]);
    const __m128i state = _mm_load_si128(reinterpret_cast<const __m128i *>(&Common::RandomState[8]));
    *reinterpret_cast<uint64 *>(&Common::RandomState[ 8]) = (state0 * 0x5deece66dull + 11);
    *reinterpret_cast<uint64 *>(&Common::RandomState[10]) = (state1 * 0x5deece66dull + 11);
    return (Vector<double>(_mm_castsi128_pd(_mm_srli_epi64(state, 12))) | One()) - One();
}
// shifted / rotated {{{1
template<typename T> Vc_INTRINSIC Vc_PURE Vector<T> Vector<T>::shifted(int amount) const
{
    switch (amount) {
    case  0: return *this;
    case  1: return mm128_reinterpret_cast<VectorType>(_mm_srli_si128(mm128_reinterpret_cast<__m128i>(d.v()), 1 * sizeof(EntryType)));
    case  2: return mm128_reinterpret_cast<VectorType>(_mm_srli_si128(mm128_reinterpret_cast<__m128i>(d.v()), 2 * sizeof(EntryType)));
    case  3: return mm128_reinterpret_cast<VectorType>(_mm_srli_si128(mm128_reinterpret_cast<__m128i>(d.v()), 3 * sizeof(EntryType)));
    case  4: return mm128_reinterpret_cast<VectorType>(_mm_srli_si128(mm128_reinterpret_cast<__m128i>(d.v()), 4 * sizeof(EntryType)));
    case  5: return mm128_reinterpret_cast<VectorType>(_mm_srli_si128(mm128_reinterpret_cast<__m128i>(d.v()), 5 * sizeof(EntryType)));
    case  6: return mm128_reinterpret_cast<VectorType>(_mm_srli_si128(mm128_reinterpret_cast<__m128i>(d.v()), 6 * sizeof(EntryType)));
    case  7: return mm128_reinterpret_cast<VectorType>(_mm_srli_si128(mm128_reinterpret_cast<__m128i>(d.v()), 7 * sizeof(EntryType)));
    case  8: return mm128_reinterpret_cast<VectorType>(_mm_srli_si128(mm128_reinterpret_cast<__m128i>(d.v()), 8 * sizeof(EntryType)));
    case -1: return mm128_reinterpret_cast<VectorType>(_mm_slli_si128(mm128_reinterpret_cast<__m128i>(d.v()), 1 * sizeof(EntryType)));
    case -2: return mm128_reinterpret_cast<VectorType>(_mm_slli_si128(mm128_reinterpret_cast<__m128i>(d.v()), 2 * sizeof(EntryType)));
    case -3: return mm128_reinterpret_cast<VectorType>(_mm_slli_si128(mm128_reinterpret_cast<__m128i>(d.v()), 3 * sizeof(EntryType)));
    case -4: return mm128_reinterpret_cast<VectorType>(_mm_slli_si128(mm128_reinterpret_cast<__m128i>(d.v()), 4 * sizeof(EntryType)));
    case -5: return mm128_reinterpret_cast<VectorType>(_mm_slli_si128(mm128_reinterpret_cast<__m128i>(d.v()), 5 * sizeof(EntryType)));
    case -6: return mm128_reinterpret_cast<VectorType>(_mm_slli_si128(mm128_reinterpret_cast<__m128i>(d.v()), 6 * sizeof(EntryType)));
    case -7: return mm128_reinterpret_cast<VectorType>(_mm_slli_si128(mm128_reinterpret_cast<__m128i>(d.v()), 7 * sizeof(EntryType)));
    case -8: return mm128_reinterpret_cast<VectorType>(_mm_slli_si128(mm128_reinterpret_cast<__m128i>(d.v()), 8 * sizeof(EntryType)));
    }
    return Zero();
}
template<typename T> Vc_INTRINSIC Vector<T> Vector<T>::shifted(int amount, Vector shiftIn) const
{
    return shifted(amount) | (amount > 0 ?
                              shiftIn.shifted(amount - Size) :
                              shiftIn.shifted(Size + amount));
}
template<typename T> Vc_INTRINSIC Vc_PURE Vector<T> Vector<T>::rotated(int amount) const
{
    const __m128i v = mm128_reinterpret_cast<__m128i>(d.v());
    switch (static_cast<unsigned int>(amount) % Size) {
    case  0: return *this;
    case  1: return mm128_reinterpret_cast<VectorType>(_mm_alignr_epi8(v, v, 1 * sizeof(EntryType)));
    case  2: return mm128_reinterpret_cast<VectorType>(_mm_alignr_epi8(v, v, 2 * sizeof(EntryType)));
    case  3: return mm128_reinterpret_cast<VectorType>(_mm_alignr_epi8(v, v, 3 * sizeof(EntryType)));
             // warning "Immediate parameter to intrinsic call too large" disabled in VcMacros.cmake.
             // ICC fails to see that the modulo operation (Size == sizeof(VectorType) / sizeof(EntryType))
             // disables the following four calls unless sizeof(EntryType) == 2.
    case  4: return mm128_reinterpret_cast<VectorType>(_mm_alignr_epi8(v, v, 4 * sizeof(EntryType)));
    case  5: return mm128_reinterpret_cast<VectorType>(_mm_alignr_epi8(v, v, 5 * sizeof(EntryType)));
    case  6: return mm128_reinterpret_cast<VectorType>(_mm_alignr_epi8(v, v, 6 * sizeof(EntryType)));
    case  7: return mm128_reinterpret_cast<VectorType>(_mm_alignr_epi8(v, v, 7 * sizeof(EntryType)));
    }
    return Zero();
}
// sorted specializations {{{1
template<> inline Vc_PURE uint_v uint_v::sorted() const
{
    __m128i x = data();
    __m128i y = _mm_shuffle_epi32(x, _MM_SHUFFLE(2, 3, 0, 1));
    __m128i l = _mm_min_epu32(x, y);
    __m128i h = _mm_max_epu32(x, y);
    x = _mm_unpacklo_epi32(l, h);
    y = _mm_unpackhi_epi32(h, l);

    // sort quads
    l = _mm_min_epu32(x, y);
    h = _mm_max_epu32(x, y);
    x = _mm_unpacklo_epi32(l, h);
    y = _mm_unpackhi_epi64(x, x);

    l = _mm_min_epu32(x, y);
    h = _mm_max_epu32(x, y);
    return _mm_unpacklo_epi32(l, h);
}
template<> inline Vc_PURE ushort_v ushort_v::sorted() const
{
    __m128i lo, hi, y, x = data();
    // sort pairs
    y = Mem::permute<X1, X0, X3, X2, X5, X4, X7, X6>(x);
    lo = _mm_min_epu16(x, y);
    hi = _mm_max_epu16(x, y);
    x = _mm_blend_epi16(lo, hi, 0xaa);

    // merge left and right quads
    y = Mem::permute<X3, X2, X1, X0, X7, X6, X5, X4>(x);
    lo = _mm_min_epu16(x, y);
    hi = _mm_max_epu16(x, y);
    x = _mm_blend_epi16(lo, hi, 0xcc);
    y = _mm_srli_si128(x, 2);
    lo = _mm_min_epu16(x, y);
    hi = _mm_max_epu16(x, y);
    x = _mm_blend_epi16(lo, _mm_slli_si128(hi, 2), 0xaa);

    // merge quads into octs
    y = _mm_shuffle_epi32(x, _MM_SHUFFLE(1, 0, 3, 2));
    y = _mm_shufflelo_epi16(y, _MM_SHUFFLE(0, 1, 2, 3));
    lo = _mm_min_epu16(x, y);
    hi = _mm_max_epu16(x, y);

    x = _mm_unpacklo_epi16(lo, hi);
    y = _mm_srli_si128(x, 8);
    lo = _mm_min_epu16(x, y);
    hi = _mm_max_epu16(x, y);

    x = _mm_unpacklo_epi16(lo, hi);
    y = _mm_srli_si128(x, 8);
    lo = _mm_min_epu16(x, y);
    hi = _mm_max_epu16(x, y);

    return _mm_unpacklo_epi16(lo, hi);
}
// }}}1
Vc_IMPL_NAMESPACE_END

#include "undomacros.h"

// vim: foldmethod=marker
