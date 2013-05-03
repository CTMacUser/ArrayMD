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

#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <stdexcept>
#include <type_traits>
#include <utility>

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

Since a multi-dimensional array is not a linear structure, linear-based access
should be avoided.  However, much of the traditional linear-aligned standard
container interface is provided, to support range-`for` and many standard and
standard-inspired algorithms.

    \pre  If any extents are given, they all must be greater than zero.

    \tparam ElementType  The type of the elements.
    \tparam Extents      The size of each dimension of the multi-level array.
                         Given in the same order as nested C-level arrays, e.g.
                         `array_md<int, 6, 5>` maps to `int[6][5]`.  If no
                         extents are given, then a single element is stored.
 */
template < typename ElementType, std::size_t ...Extents >
struct array_md;


//  Implementation details  --------------------------------------------------//

//! \cond
namespace detail
{
    //! Detect if a type's swap (found via ADL for non-built-ins) throws.
    template < typename T, typename U = T >
    inline constexpr
    bool  is_swap_nothrow() noexcept
    {
        using std::swap;

        return noexcept( swap(std::declval<T &>(), std::declval<U &>()) );
    }

    //! Assign second object to first
    template < typename T, typename U >
    inline
    void  deep_assign( T &t, U const &u )
     noexcept( std::is_nothrow_assignable<T &, U const &>::value )
    { t = u; }

    //! Assign second object to first, when both are arrays!
    template < typename T, typename U, std::size_t N >
    inline
    void  deep_assign( T (&t)[N], U const (&u)[N] )
     noexcept( noexcept(deep_assign( t[0], u[0] )) )
    {
        for ( std::size_t  i = 0u ; i < N ; ++i )
            deep_assign( t[i], u[i] );
    }

}  // namespace detail
//! \endcond


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
    //! The type for referring to an element.
    typedef value_type &              reference;
    //! The type for referring to an element, immutable access.
    typedef value_type const &  const_reference;

    // Other types needed for the Container interface
    //! The type for spacing between element positions.
    typedef std::ptrdiff_t  difference_type;
    //! The type for referring to an element's position.
    typedef pointer                iterator;
    //! The type for referring to an element's position, immutable access.
    typedef const_pointer    const_iterator;

    // Types needed for the ReversibleContainer interface
    //! The type for traversing element-positions backwards.
    typedef std::reverse_iterator<iterator>              reverse_iterator;
    //! The type for traversing element-positions backwards, immutable access.
    typedef std::reverse_iterator<const_iterator>  const_reverse_iterator;

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
    auto      size() const noexcept -> size_type  { return static_size; }
    /** \brief  Returns maximum possible element count.

    Since this type has a fixed number of elements, this function's
    implementation can be trivial and `constexpr`.

        \returns  The largest number of stored elements (of #value_type) that
                  this object can support.
     */
    constexpr
    auto  max_size() const noexcept -> size_type  { return size(); }
    /** \brief  Returns if there aren't any elements.

    Since this type has a fixed number of elements, this function's
    implementation can be trivial and `constexpr`.

        \returns  `size() == 0`.
     */
    constexpr
    bool  empty() const noexcept  { return false; }

    // Element access
    /** \brief  Returns underlying access.

    `[ data(), data() + size() )` is a valid range of #value_type covering all
    the stored elements.

        \returns  The address of the first element (of #value_type).
     */
    auto  data()       noexcept -> pointer        { return &data_block; }
    //! \overload
    constexpr
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
    /** \brief  Access to element data, "full" depth.

    Provides access to an element, based on the indices given as a standard
    initializer list.  Since there is no internal array, only an empty list is
    acceptable.  If the element type is a built-in array or otherwise supports
    `operator []`, then further indexing has to be supplied after this call.
    The supplied index tuple is **not** bounds-checked.

        \pre  The `size()` member function of the submitted list has to return
              #dimensionality.

        \throws  Nothing.

        \returns  A reference to the sole element.
     */
    auto  operator ()( std::initializer_list<size_type> ) noexcept -> reference
    { return data_block; }
    //! \overload
    auto  operator ()( std::initializer_list<size_type> ) const noexcept
      -> const_reference
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
    /** \brief  Access to element data, "full" depth, bounds-checked.

    Works like #operator()(std::initializer_list<size_type>), except for added
    checking for the indices to be within their bounds.  But since this version
    doesn't take indices, the only check is for the number of indices.

        \param i  The indices of the selected element.

        \see  #operator()(std::initializer_list<size_type>)

        \throws  std::length_error  if i.size() != #dimensionality.

        \returns  A reference to the sole element.
     */
    auto  at( std::initializer_list<size_type> i ) -> reference
    {
        if ( i.size() )
            throw std::length_error{ "Too many indices" };
        return data_block;
    }
    //! \overload
    auto  at( std::initializer_list<size_type> i ) const -> const_reference
    {
        if ( i.size() )
            throw std::length_error{ "Too many indices" };
        return data_block;
    }

    /** \brief  Access to first element.

    Since there's exactly one element, it's always the front and easy to find.

        \throws  Nothing.

        \returns  `*begin()`
     */
    auto  front()       noexcept ->       reference  { return data_block; }
    //! \overload
    constexpr
    auto  front() const noexcept -> const_reference  { return data_block; }
    /** \brief  Access to last element.

    Since there's exactly one element, it's always the back and easy to find.

        \throws  Nothing.

        \returns  `*rbegin()`
     */
    auto   back()       noexcept ->       reference  { return data_block; }
    //! \overload
    constexpr
    auto   back() const noexcept -> const_reference  { return data_block; }

    // Iteration (and range-for) support
    /** \brief  Forward iteration, start point

    Generates a start point for iterating over this object's element data in a
    forward direction.  Since the data is a single object, the iterator just
    points to that sole object.

        \throws  Nothing.

        \returns  An iterator pointing to the sole element.
     */
    auto  begin()       noexcept ->       iterator  { return data(); }
    //! \overload
    constexpr
    auto  begin() const noexcept -> const_iterator  { return data(); }
    /** \brief  Forward iteration, end point

    Generates an end point for iterating over this object's element data in a
    forward direction.  Since the data is a single object, the iterator points
    just past that sole object.

        \throws  Nothing.

        \returns  An iterator pointing to one past the sole element.
     */
    auto    end()       noexcept ->       iterator  { return data() + size(); }
    //! \overload
    constexpr
    auto    end() const noexcept -> const_iterator  { return data() + size(); }

    /** \brief  Forward iteration, start point, immutable access

    Provides a way for a mutable-mode object to get immutable-mode element
    access (via iterator) without `const_cast` convolutions with #begin.

        \throws  Nothing.

        \returns  An iterator pointing to the sole element.
     */
    constexpr
    auto  cbegin() const noexcept -> const_iterator  { return begin(); }
    /** \brief  Forward iteration, end point, immutable access

    Provides a way for a mutable-mode object to get immutable-mode element
    access (via iterator) without `const_cast` convolutions with #end.

        \throws  Nothing.

        \returns  An iterator pointing to one past the sole element.
     */
    constexpr
    auto    cend() const noexcept -> const_iterator  { return end(); }

    /** \brief  Reverse iteration, start point

    Generates a start point for iterating over this object's element data in a
    backward direction.  Since the data is a single object, the iterator just
    points to that sole object.

        \returns  An iterator pointing to the sole element.
     */
    auto  rbegin()       ->       reverse_iterator
    { return reverse_iterator(end()); }
    //! \overload
    auto  rbegin() const -> const_reverse_iterator
    { return const_reverse_iterator(end()); }
    /** \brief  Reverse iteration, end point

    Generates an end point for iterating over this object's element data in a
    backward direction.  Since the data is a single object, the iterator points
    just past that sole object.

        \returns  An iterator pointing to one before the sole element.
     */
    auto    rend()       ->       reverse_iterator
    { return reverse_iterator(begin()); }
    //! \overload
    auto    rend() const -> const_reverse_iterator
    { return const_reverse_iterator(begin()); }

    /** \brief  Reverse iteration, start point, immutable access

    Provides a way for a mutable-mode object to get immutable-mode element
    access (via iterator) without `const_cast` convolutions with #rbegin.

        \returns  An iterator pointing to the sole element.
     */
    auto  crbegin() const -> const_reverse_iterator
    { return const_reverse_iterator(cend()); }
    /** \brief  Reverse iteration, end point, immutable access

    Provides a way for a mutable-mode object to get immutable-mode element
    access (via iterator) without `const_cast` convolutions with #rend.

        \returns  An iterator pointing to one before the sole element.
     */
    auto    crend() const -> const_reverse_iterator
    { return const_reverse_iterator(cbegin()); }

    // Other operations
    /** \brief  Fill element with specified value.

    Assigns the given value to the sole element.

        \pre  #value_type has to be Assignable.  There is special compensation
              for built-in arrays, which are not Assignable.  The code will
              assign them inner-element-wise instead.  In that case, the inner
              non-array type has to be Assignable.

        \param v  The value of the assignment source.

        \throws  Whatever  assignment for
                 `std::remove_all_extents<value_type>::type` throws.

        \post  The sole element is equivalent to *v*.
     */
    void  fill( value_type const &v )
     noexcept( noexcept(detail::deep_assign( (value_type &)v, v )) )
    { detail::deep_assign(data_block, v); }

    /** \brief  Swaps states with another object.

    The swapping should use the element-type's `swap`, found via ADL.

        \param other  The object to trade state with.

        \throws  Whatever  the element-level swap throws.
        \post  `*this` is equivalent to the old state of *other*, while that
               object is equivalent to the old state of `*this`.
     */
    void  swap( array_md &other )
     noexcept( detail::is_swap_nothrow<value_type>() )
    {
        using std::swap;

        swap( data_block, other.data_block );
    }

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
    //! The type for referring to an element.
    typedef value_type &              reference;
    //! The type for referring to an element, immutable access.
    typedef value_type const &  const_reference;

    // Other types needed for the Container interface
    //! The type for spacing between element positions.
    typedef std::ptrdiff_t  difference_type;
    //! The type for referring to an element's position.
    typedef pointer                iterator;
    //! The type for referring to an element's position, immutable access.
    typedef const_pointer    const_iterator;

    // Types needed for the ReversibleContainer interface
    //! The type for traversing element-positions backwards.
    typedef std::reverse_iterator<iterator>              reverse_iterator;
    //! The type for traversing element-positions backwards, immutable access.
    typedef std::reverse_iterator<const_iterator>  const_reverse_iterator;

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
    auto      size() const noexcept -> size_type  { return static_size; }
    /** \brief  Returns maximum possible element count.

    Since this type has a fixed number of elements, this function's
    implementation can be trivial and `constexpr`.

        \returns  The largest number of stored elements (of #value_type) that
                  this object can support.
     */
    constexpr
    auto  max_size() const noexcept -> size_type  { return size(); }
    /** \brief  Returns if there are no elements.

    Since this type has a fixed number of elements, this function's
    implementation can be trivial and `constexpr`.

        \returns  `size() == 0`.
     */
    constexpr
    bool  empty() const noexcept  { return false; }

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
    constexpr
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

    /** \brief  Access to element data, full depth.

    Provides access to one element, with its coordinates expressed as a standard
    initializer list.

    If the element type is a built-in array or otherwise supports `operator []`,
    then further indexing has to be supplied after this call.

    The supplied index tuple is **not** bounds-checked.

        \pre  i.size() == #dimensionality
        \pre  For each valid *K*, when advancing *K* places into *i*, the value
              at that place has to be less than #static_sizes[ *K* ].

        \param i  The indices of the selected element.

        \throws  Nothing.

        \returns  A reference to the given element.
     */
    auto  operator []( std::initializer_list<size_type> i ) noexcept
      -> reference
    {
        // Keep the mutable and const versions in sync
        return const_cast<value_type &>( const_cast<array_md const
         *>(this)->operator [](i) );
    }
    //! \overload
    auto  operator []( std::initializer_list<size_type> i ) const noexcept
      -> const_reference
    {
        const_pointer          start = data();
        size_type             stride = static_size;
        size_type const *  pfraction = static_sizes;

        for ( auto const  ii : i )
        {
            stride /= *pfraction++;
            start += stride * ii;
        }
        return *start;
    }

    /** \brief  Access to element data, full depth.

    Provides access to an element of the internal array object, based on the
    indices given as a standard initializer list.

    If the element type is a built-in array or otherwise supports `operator []`,
    then further indexing has to be supplied after this call.

    The supplied index tuple is **not** bounds-checked.

        \pre  i.size() == #dimensionality
        \pre  For each valid *K*, when advancing *K* places into *i*, the value
              at that place has to be less than #static_sizes[ *K* ].

        \param i  The indices of the selected element.

        \see  #operator[](std::initializer_list<size_type>)

        \throws  Nothing.

        \returns  A reference to the requested element.
     */
    auto  operator ()( std::initializer_list<size_type> i ) noexcept
      -> reference
    { return operator []( i ); }
    //! \overload
    auto  operator ()( std::initializer_list<size_type> i ) const noexcept
      -> const_reference
    { return operator []( i ); }
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

    /** \brief  Access to element data, full depth, bounds-checked.

    Works like #operator()(std::initializer_list<size_type>), except for added
    checking for the amount of indices and if they're within their bounds.

        \param i  The indices of the selected element.

        \see  #operator()(std::initializer_list<size_type>)

        \throws  std::length_error  if i.size() != #dimensionality.
        \throws  std::out_of_range  if an index is out-of-bounds.

        \returns  A reference to the requested element.
     */
    auto  at( std::initializer_list<size_type> i ) -> reference
    {
        // Keep the mutable and const versions in sync
        return const_cast<reference>( (this->*static_cast<auto
         (array_md::*)(std::initializer_list<size_type>) const ->
         const_reference>( &array_md::at ))(i) );
    }
    //! \overload
    auto  at( std::initializer_list<size_type> i ) const -> const_reference
    {
        const_pointer          start = data();
        size_type             stride = static_size;
        size_type const *  pfraction = static_sizes;

        if ( i.size() != dimensionality )
            throw std::length_error{ "Wrong number of indices" };
        for ( auto const  ii : i )
        {
            stride /= *pfraction;
            if ( ii >= *pfraction++ )
                throw std::out_of_range{ "Index out of bounds" };
            start += stride * ii;
        }
        return *start;
    }
    /** \brief  Access to element data, arbitrary depth, bounds-checked.

    Works like #operator()(Indices&&...), except for added checking for the
    indices to be within their bounds.

        \pre  0 \<= `sizeof...(i)` \<= #dimensionality.

        \param i  The indices for the selected element or slice.  May be empty.

        \see  #operator()(Indices&&...)

        \throws  std::out_of_range  if an index is out-of-bounds.
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

    /** \brief  Access to first element.

    The first/front element is determined by iteration order, so it's at
    `(*this)[0]...[0]`.

        \throws  Nothing.

        \returns  `*begin()`
     */
    auto  front()       noexcept ->       reference  { return *begin(); }
    //! \overload
    constexpr
    auto  front() const noexcept -> const_reference  { return *begin(); }
    /** \brief  Access to last element.

    The last/back element is determined by iteration order, so it's at
    `*(this->data() + static_size - 1u)`.

        \throws  Nothing.

        \returns  `*rbegin()`
     */
    auto   back()       noexcept ->       reference  { return *(end() - 1); }
    //! \overload
    constexpr
    auto   back() const noexcept -> const_reference  { return *(end() - 1); }

    // Iteration (and range-for) support
    /** \brief  Forward iteration, start point

    Generates a start point for iterating over this object's element data in a
    forward direction.  Progression through the elements is the "row-major"
    order that C/C++ array layout rules imply.  The traversal order of
    index-coordinates is invariant for a given value of #static_sizes, so
    multiple traversals of the same object gives a stable experience.  And
    iterating over several `array_md` objects in parallel, with the same
    #static_sizes, starting point, and in-sync traversal amounts, will keep the
    visited index-coordinates in sync.

        \throws  Nothing.

        \returns  An iterator pointing to the first element.  (At address
                  `&((*this)[0]...[0])`)
     */
    auto  begin()       noexcept ->       iterator  { return data(); }
    //! \overload
    constexpr
    auto  begin() const noexcept -> const_iterator  { return data(); }
    /** \brief  Forward iteration, end point

    Generates an end point for iterating over this object's element data in a
    forward direction.

        \throws  Nothing.

        \returns  An iterator pointing to one past the last element.  (At
                  address `&((*this)[static_sizes[0]][0]...[0])`)
     */
    auto    end()       noexcept ->       iterator  { return data() + size(); }
    //! \overload
    constexpr
    auto    end() const noexcept -> const_iterator  { return data() + size(); }

    /** \brief  Forward iteration, start point, immutable access

    Provides a way for a mutable-mode object to get immutable-mode element
    access (via iterator) without `const_cast` convolutions with #begin.

        \throws  Nothing.

        \returns  An iterator pointing to the first element.
     */
    constexpr
    auto  cbegin() const noexcept -> const_iterator  { return begin(); }
    /** \brief  Forward iteration, end point, immutable access

    Provides a way for a mutable-mode object to get immutable-mode element
    access (via iterator) without `const_cast` convolutions with #end.

        \throws  Nothing.

        \returns  An iterator pointing to one past the last element.
     */
    constexpr
    auto    cend() const noexcept -> const_iterator  { return end(); }

    /** \brief  Reverse iteration, start point

    Generates a start point for iterating over this object's element data in a
    backward direction.  Progression through the elements is the reverse given
    by #begin and #end member functions.  The traversal order of
    index-coordinates is invariant for a given value of #static_sizes, so
    multiple traversals of the same object gives a stable experience.  And
    iterating over several `array_md` objects in parallel, with the same
    #static_sizes, starting point, and in-sync traversal amounts, will keep the
    visited index-coordinates in sync.

        \returns  An iterator pointing to the last element.  (At address
     `&((*this)[static_sizes[0] - 1]...[static_sizes[dimensionality - 1] - 1])`)
     */
    auto  rbegin()       ->       reverse_iterator
    { return reverse_iterator(end()); }
    //! \overload
    auto  rbegin() const -> const_reverse_iterator
    { return const_reverse_iterator(end()); }
    /** \brief  Reverse iteration, end point

    Generates an end point for iterating over this object's element data in a
    backward direction.

        \returns  An iterator pointing to one before the first element.  (At
                  address `&((*this)[0]...[0][-1])`)
     */
    auto    rend()       ->       reverse_iterator
    { return reverse_iterator(begin()); }
    //! \overload
    auto    rend() const -> const_reverse_iterator
    { return const_reverse_iterator(begin()); }

    /** \brief  Reverse iteration, start point, immutable access

    Provides a way for a mutable-mode object to get immutable-mode element
    access (via iterator) without `const_cast` convolutions with #rbegin.

        \returns  An iterator pointing to the last element.
     */
    auto  crbegin() const -> const_reverse_iterator
    { return const_reverse_iterator(cend()); }
    /** \brief  Reverse iteration, end point, immutable access

    Provides a way for a mutable-mode object to get immutable-mode element
    access (via iterator) without `const_cast` convolutions with #rend.

        \returns  An iterator pointing to one before the first element.
     */
    auto    crend() const -> const_reverse_iterator
    { return const_reverse_iterator(cbegin()); }

    // Other operations
    /** \brief  Fill elements with specified value.

    Assigns the given value to all the elements.

        \pre  #value_type has to be Assignable.  If #value_type is a bulit-in
              array-type, which are not Assignable, then its extents-stripped
              type (i.e. the inner non-array type) must be Assignable instead.

        \param v  The value of the assignment source.

        \throws  Whatever  assignment for (an extents-stripped) #value_type
                 throws.

        \post  Each element is equivalent to *v*.
     */
    void  fill( value_type const &v )
     noexcept( std::is_nothrow_copy_assignable<typename
     std::remove_all_extents<value_type>::type>::value )
    { for (value_type &x : *this) detail::deep_assign(x, v); }

    /** \brief  Swaps states with another object.

    The swapping should use the element-type's `swap`, found via ADL.

        \param other  The object to trade state with.

        \throws  Whatever  the element-level swap throws.
        \post  `*this` is equivalent to the old state of *other*, while that
               object is equivalent to the old state of `*this`.
     */
    void  swap( array_md &other )
     noexcept( detail::is_swap_nothrow<value_type>() )
    {
        // Built-in arrays get their swap from the standard namespace.  It'll
        // (eventually) call the ADL swap from the outermost contained non-array
        // type.
        std::swap( data_block, other.data_block );
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


//  Multi-dimensional array class template, operator definitions  ------------//

/** \brief  Equality comparison for `array_md`.

Compares two `array_md` objects, with the same size (same number of dimensions
and the corresponding dimensions have to be equal), for equality.  Note that it
usually won't work with built-in arrays as the element types, since they don't
support `operator ==`.

    \pre  `std::declval<T>() == std::declval<U>()` is well-formed.

    \param l  The left-side argument.
    \param r  The right-side argument.

    \retval true   If every element in *l* is equal to its corresponding element
                   in *r*.
    \retval false  Otherwise.
 */
template < typename T, typename U, std::size_t ...N >
inline
bool  operator ==( array_md<T, N...> const &l, array_md<U, N...> const &r )
{ return std::equal(l.begin(), l.end(), r.begin()); }

/** \brief  Inequality comparison for `array_md`.

Compares two `array_md` objects, with the same size (same number of dimensions
and the corresponding dimensions have to be equal), for inequality.  Note that
it usually won't work with built-in arrays as the element types, since they
don't support `operator ==`.

    \pre  `std::declval<T>() == std::declval<U>()` is well-formed.

    \param l  The left-side argument.
    \param r  The right-side argument.

    \see  #operator==(array_md<T,U,N...>const&,array_md<T,U,N...>const&)

    \retval true   If any element in *l* doesn't equal its corresponding element
                   in *r*.
    \retval false  Otherwise.
 */
template < typename T, typename U, std::size_t ...N >
inline
bool  operator !=( array_md<T, N...> const &l, array_md<U, N...> const &r )
{ return !(l == r); }


//  Multi-dimensional array class template, other operations  ----------------//

/** \brief  Swap routine for `array_md`.

Exchanges the state of two `array_md` objects.  The objects have to have the
same element type and number of dimensions, and the corresponding dimensions
have to be equal.

    \pre  There is a swapping routine, called `swap`, either in namespace `std`
          for built-ins, or found via ADL for other types.

    \param a  The first object to have its state exchanged.
    \param b  The second object to have its state exchanged.

    \see  #array_md<T>::swap(array_md<T>&)
    \see  #array_md<T,M,N...>::swap(array_md<T,M,N...>&)

    \throws Whatever  the element-level swap does.

    \post  `a` is equivalent to the old state of `b`, while `b` is equivalent to
           the old state of `a`.
 */
template < typename T, std::size_t ...N >
inline
void  swap( array_md<T, N...> &a, array_md<T, N...> &b )
 noexcept( noexcept(a.swap( b )) )
{ a.swap(b); }


}  // namespace container
}  // namespace boost


#endif // BOOST_CONTAINER_ARRAY_MD_HPP
