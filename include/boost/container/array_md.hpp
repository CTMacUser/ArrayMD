//  Boost Multi-dimensional Array header file  -------------------------------//

//  Copyright 2013 Daryle Walker.
//  Distributed under the Boost Software License, Version 1.0.  (See the
//  accompanying file LICENSE_1_0.txt or a copy at
//  <http://www.boost.org/LICENSE_1_0.txt>.)

//  See <http://www.boost.org/libs/container/> for the library's home page.

/** \file
    \brief  A container class template that's a multi-dimensional adapatation of
      std::array.

    \author  Daryle Walker

    \version  0.1

    \copyright  Boost Software License, version 1.0

    Contains the declarations (and definitions) of class templates modeling a
    multi-dimensional array that integrates extent definitions and indexing as
    comma-separated lists, instead of bracket-isolated descriptors.  This should
    make multi-dimensionality compatible with C++11's variadic stuff.

    \warning  This library requires C++2011 features.
 */

#ifndef BOOST_CONTAINER_ARRAY_MD_HPP
#define BOOST_CONTAINER_ARRAY_MD_HPP

#include <cstddef>
#include <stdexcept>

#include "boost/type_traits/indexing.hpp"
#include "boost/utility/slice.hpp"


namespace boost
{
//! Name-space for Boost.Container and other container types
namespace container
{


//  Forward declarations  ----------------------------------------------------//

/** \brief  A container encapsulating multi-dimensional constant-size arrays.

(Adapted from <http://en.cppreference.com/w/cpp/container/array>.)
As `std::array` puts built-in C-level arrays in a framework compatible with
other standard containers, this class template does the same with nested
built-in C-level arrays.  The extents are taken as value-based varadic
template parameters, making multi-dimension stuff compatible.

This class is designed as an aggregate, so aggregate-initialization has to
be used.  Objects support default-, copy-, and move-construction if the
element type does.  The same applies to copy- and move-assignment and
destruction.

Since a multi-dimensional array is not a linear structure, most linear-based
access is not provided.  The exception is `begin`/`end`, to support the
range-`for` statement.

    \pre  If any extents are given, they all must be greater than zero.

    \tparam ElementType  The type of the elements.
    \tparam Extents      The size of each dimension of the multi-level array.
                         Given in the same order as nested C-level arrays, e.g.
                         `array_md<int, 6, 5>` maps to `int[6][5]`.  If no
                         extents are given, then a single element is stored.
 */
template < typename ElementType, std::size_t ...Extents >
struct array_md;


//  Multi-dimensional array class template specialization declarations  ------//

/** \brief  Base-case specialization of `array_md`.

The base case stores a single (non-array) element.

    \tparam T  The type of the elements.
 */
template < typename T >
struct array_md<T>
{
    // Core types
    //! The element type.  Gives access to the template parameter.
    typedef T           value_type;
    //! The type of #data_block; equal to #value_type in the base case.
    typedef value_type   data_type;
    //! The type for size-based meta-data.
    typedef std::size_t  size_type;

    // Data-block types
    //! The type for pointing to an element.
    typedef value_type *              pointer;
    //! The type for pointing to an element, immutable access.
    typedef value_type const *  const_pointer;

    // Sizing parameters
    //! The number of array extents supplied as dimensions.
    static constexpr  size_type  dimensionality = 0u;
    //! The total number of elements (of #value_type).
    static constexpr  size_type  static_size = 1u;

    // Capacity
    /** \brief  Returns element count.

    Since this type has a fixed number of elements, this function's
    implementation can be trivial and `constexpr`.

        \returns  The number of stored elements (of #value_type).
     */
    constexpr
    auto  size() const noexcept -> size_type  { return static_size; }

    // Element access
    /** \brief  Returns underlying access.

    `[ data(), data() + size() )` is a valid range of #value_type covering all
    the stored elements.

        \returns  The address of the first element (of #value_type).
     */
    auto  data()       noexcept -> pointer        { return &data_block; }
    //! \overload
    auto  data() const noexcept -> const_pointer  { return &data_block; }

    /** \brief  Whole-object access to the element data.

    Since this version of `array_md` doesn't carry an internal array, only
    access to the whole internal data object is available.  If the element
    type itself supports indexing (i.e. built-in array, bulit-in pointer,
    or class-type with `operator []`), further indexing has to be supplied
    to the result of this member function.

        \throws  Nothing.

        \returns  A reference to the sole element.
     */
    auto  operator ()() noexcept -> data_type &
    { return data_block; }
    //! \overload
    constexpr
    auto  operator ()() const noexcept -> data_type const &
    { return data_block; }

    /** \brief  Whole-object access to the element data, bounds-checked.

    Works like #operator()(), except for added checking for the indices to be
    within their bounds.  But since this version doesn't take indices, there
    isn't any actual checking.

        \see  #operator()()

        \throws  Nothing.

        \returns  A reference to the sole element.
     */
    auto  at()       noexcept -> data_type &        { return data_block; }
    //! \overload
    constexpr
    auto  at() const noexcept -> data_type const &  { return data_block; }

    // Member data
    //! The element, public to support aggregate initialization.
    data_type  data_block;
};

/** \brief  Recursive-case specialization of `array_md`.

The recursive case directly stores a C-level array of the previous case's
data type.

    \pre  All extents must be positive.

    \tparam T  The type of the elements.
    \tparam M  The first (outer-most) extent.
    \tparam N  The remaining extents.  May be empty.
 */
template < typename T, std::size_t M, std::size_t ...N >
struct array_md<T, M, N...>
{
    // Core types
    //! The element type.  Gives access to the first template parameter.
    typedef T                                               value_type;
    //! The type for the direct elements of #data_block.  Equal to #value_type
    //! only when #dimensionality is 1.
    typedef typename array_md<T, N...>::data_type  direct_element_type;
    //! The type of #data_block; not equal to #value_type in recursive cases.
    typedef direct_element_type                              data_type[ M ];
    //! The type for size-based meta-data and access indices.
    typedef std::size_t                                      size_type;

    // Data-block types
    //! The type for pointing to an element.
    typedef value_type *              pointer;
    //! The type for pointing to an element, immutable access.
    typedef value_type const *  const_pointer;

    // Sizing parameters
    //! The number of extents supplied as template parameters.
    static constexpr  size_type  dimensionality = 1u + sizeof...( N );
    //! The array extents, in an array.
    static constexpr  size_type  static_sizes[] = { M, N... };
    //! The total number of elements (of #value_type).
    static constexpr  size_type  static_size = sizeof( data_type ) / sizeof(
     value_type );

    // Capacity
    /** \brief  Returns element count.

    Since this type has a fixed number of elements, this function's
    implementation can be trivial and `constexpr`.

        \returns  The number of stored elements (of #value_type).
     */
    constexpr
    auto  size() const noexcept -> size_type  { return static_size; }

    // Element (or sub-array) access
    /** \brief  Returns underlying access.

    `[ data(), data() + size() )` is a valid range of #value_type covering all
    the stored elements.  Using this method flattens the dimensions away to
    linear access.

        \returns  The address of the first element (of #value_type).
     */
    auto  data()       noexcept -> pointer
    { return static_cast<pointer>(static_cast<void *>( &data_block )); }
    //! \overload
    auto  data() const noexcept -> const_pointer
    {
        return static_cast<const_pointer>(
         static_cast<void const *>(&data_block) );
    }

    /** \brief  Access to element data, with depth of exactly one.

    Provides access to a slice of the element data, with one index.  If
    #dimensionality is 1, then an element is directly returned.  Otherwise,
    an array slice of the immediately-lower level is returned.  For example, an
    `array_md<int, 6, 5, 4>` will return an `int(&)[5][4]` from this method.

    If a slice is returned, then `operator []` or other means can be used to
    continue indexing.  The same applies if the element type supports
    indexing itself (i.e. it's a bulit-in array, built-in data pointer, or a
    class type with `operator []` support).

    The supplied index value is **not** bounds-checked.

        \pre  *i* \< #static_sizes[ 0 ]

        \param i  The index of the selected element (array).

        \throws  Nothing.

        \returns  A reference to the given element (array).
     */
    auto  operator []( size_type i ) noexcept -> direct_element_type &
    { return data_block[i]; }
    //! \overload
    constexpr
    auto  operator []( size_type i ) const noexcept ->direct_element_type const&
    { return data_block[i]; }

    /** \brief  Access to element data, arbitrary depth.

    Provides access to some part of the internal array object, based on how
    many indices are given.

    Index Count (*x*)           | Returns
    ----------------------------| ------------------------
    0                           | the whole element array
    0 \< *x* \< #dimensionality | an array slice
    #dimensionality             | an element
    \> #dimensionality          | error

    Just remove the *x* left-most (i.e. outer-most) extents from the
    internal array type.  If the return type supports indexing (i.e. the
    whole array, a slice, or an element type that's a bulit-in array, a
    built-in data pointer, or a class-type with `operator []` support), then
    indexing can continue with a separate call.

    Supplied index values are **not** bounds-checked.  Each index has to be,
    or implicitly convertible to, a built-in integer type.

        \pre  0 \<= `sizeof...(i)` \<= #dimensionality.
        \pre  For an *i* as the *K* function argument (zero-based),
              0 \<= *i* \< #static_sizes[ *K* ].

        \param i  The indices for the selected element or slice.  May be empty.

        \throws  Whatever  from converting an index to a built-in integer or
                 enumeration type.  Otherwise, nothing.

        \returns  A reference to the requested element (array).
     */
    template < typename ...Indices >
    auto  operator ()( Indices &&...i )
      noexcept( boost::indexing_noexcept<data_type &, Indices...>::value )
      -> typename boost::indexing_result<data_type &, Indices...>::type
    {
        static_assert( sizeof...(i) <= dimensionality, "Too many indices" );
        return boost::slice(data_block, static_cast<Indices &&>( i )...);
    }
    //! \overload
    template < typename ...Indices >
    constexpr
    auto  operator ()( Indices &&...i ) const
      noexcept( boost::indexing_noexcept<data_type const &, Indices...>::value )
      -> typename boost::indexing_result<data_type const &, Indices...>::type
    {
        static_assert( sizeof...(i) <= dimensionality, "Too many indices" );
        return boost::slice(data_block, static_cast<Indices &&>( i )...);
    }

    /** \brief  Access to element data, arbitrary depth, bounds-checked.

    Works like #operator()(), except for added checking for the indices to be
    within their bounds.

        \pre  0 \<= `sizeof...(i)` \<= #dimensionality.

        \param i  The indices for the selected element or slice.  May be empty.

        \see  #operator()()

        \throws  `std::out_of_range`  if an index is out-of-bounds.
        \throws  Whatever  from converting an index to a built-in integer or
                 enumeration type.

        \returns  A reference to the requested element (array).
     */
    template < typename ...Indices >
    auto  at( Indices &&...i )
      -> typename boost::indexing_result<data_type &, Indices...>::type
    {
        static_assert( sizeof...(i) <= dimensionality, "Too many indices" );

        return boost::checked_slice(std::out_of_range{ "Index out of bounds" },
         data_block, static_cast<Indices &&>( i )...);
    }
    //! \overload
    template < typename ...Indices >
    constexpr
    auto  at( Indices &&...i ) const
      -> typename boost::indexing_result<const data_type &, Indices...>::type
    {
        static_assert( sizeof...(i) <= dimensionality, "Too many indices" );

        return boost::checked_slice(std::out_of_range{ "Index out of bounds" },
         data_block, static_cast<Indices &&>( i )...);
    }

    // Member data
    //! The element(s), public to support aggregate initialization.
    data_type  data_block;
};


//  Class-static data member definitions  ------------------------------------//

/** Since no array extents are given, there are no dimensions.
 */
template < typename T >
constexpr
typename array_md<T>::size_type  array_md<T>::dimensionality;

/** Total size is formed by the product of the extents, so the base case has to
    start from the mulitplicative identity (i.e. 1).
 */
template < typename T >
constexpr
typename array_md<T>::size_type  array_md<T>::static_size;

/** Should be the same as `std::rank<data_type>::value -
    std::rank<value_type>::value`.
 */
template < typename T, std::size_t M, std::size_t ...N >
constexpr
typename array_md<T, M, N...>::size_type  array_md<T, M, N...>::dimensionality;

/** The extents are given in the same order as the template parameters, after
    the first one (of course).
 */
template < typename T, std::size_t M, std::size_t ...N >
constexpr
typename array_md<T, M, N...>::size_type  array_md<T, M, N...>::static_sizes[ 1u
 + sizeof...(N) ];

/** Total size is formed by the product of the extents in the recursive case.
 */
template < typename T, std::size_t M, std::size_t ...N >
constexpr
typename array_md<T, M, N...>::size_type  array_md<T, M, N...>::static_size;


}  // namespace container
}  // namespace boost


#endif // BOOST_CONTAINER_ARRAY_MD_HPP
