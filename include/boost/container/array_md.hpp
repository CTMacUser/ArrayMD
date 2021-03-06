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
    make multi-dimensionality compatible with variadic manipulations.

    \warning  This library requires C++2011 features.
 */

#ifndef BOOST_CONTAINER_ARRAY_MD_HPP
#define BOOST_CONTAINER_ARRAY_MD_HPP

#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <stdexcept>
#include <tuple>
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
destruction.  This class should be trivially-copyable, trivial, standard-layout,
and/or POD if the element type is.

Ideally, a multi-dimensional array is not a linear structure and would avoid
providing linear-based access.  However, support for range-`for` and many
standard and standard-inspired algorithms require linear access, so it's
provided.  In fact, this class provides the same external interface as
`std::array`, especially when a single
extent is given.  It meets the requirements for Container (except empty on
default/value-initialization), Reversible Container, the optional container
operations, none of the Sequence container requirements (since they are either
special constructor calls or size-changing operations), all the optional
Sequence operations that don't involve size-changing (but `operator []` and `at`
act differently when the number of extents isn't exactly one), the Allocator
pointer type-aliases, and the (out-of-class) `tuple` interface.

    \pre  If at least one extent is given, the first one must be at least zero.
          Any extents after the first must be greater than zero.

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

    //! Apply function to nested arrays of given depth
    template < typename Function, typename T, std::size_t N, typename ...Args >
    void  array_apply( std::integral_constant<std::size_t, 0u>, Function &&f,
     T (&t)[N], Args &&...args )
    { std::forward<Function>(f)(t, std::forward<Args>( args )...); }
    //! \overload
    template < typename Function, typename T, typename ...Args >
    void  array_apply( std::integral_constant<std::size_t, 0u>, Function &&f,
      T &&t, Args &&...args )
    {
        using std::forward;

        forward<Function>( f )( forward<T>(t), forward<Args>(args)... );
    }
    //! \overload
    template < std::size_t L, typename Function, typename T, std::size_t N,
     typename ...Args >
    void  array_apply( std::integral_constant<std::size_t, L>, Function &&f,
     T (&t)[N], Args &&...args )
    {
        using std::size_t;
        using std::forward;

        for ( size_t  i = 0u ; i < N ; ++i )
            array_apply( std::integral_constant<size_t, L - 1u>{},
             forward<Function>(f), t[i], forward<Args>(args)..., i );
    }

    //! Strip a specified number of array extents (instead of either one or all)
    template < typename T, std::size_t Amount > struct remove_some_extents;
    //! Base-case specialization, no change to input type.
    template < typename T > struct remove_some_extents<T, 0u>
    { typedef T type; };
    //! The general (recursive) case
    template < typename T, std::size_t Amount >
    struct remove_some_extents
    {
        typedef typename remove_some_extents<typename
         std::remove_extent<T>::type, Amount - 1u>::type type;
    };

    //! Add the specified array extents
    template < typename T, std::size_t ...N > struct append_extents;
    //! Base-case specialization, no change to input type.
    template < typename T > struct append_extents<T>  { typedef T type; };
    //! Recursive-case specialization, zero-sized extent becomes unknown bound.
    template < typename T, std::size_t ...M > struct append_extents<T, 0u, M...>
    { typedef typename append_extents<T, M...>::type type[]; };
    //! The general (recursive) case, add new extent at top level.
    template < typename T, std::size_t N, std::size_t ...M >
    struct append_extents<T, N, M...>
    { typedef typename append_extents<T, M...>::type type[N]; };

    //! `address_first_element` when input is a non-array type
    template < typename T >
    constexpr
    auto  address_first_element_impl( T &&t, std::integral_constant<std::size_t,
     0u> ) noexcept -> typename std::remove_reference<T>::type *
    { return &t; }  // should use std::addressof once it's constexpr
    //! `address_first_element` when array nesting is removed
    template < typename T, std::size_t N >
    constexpr
    auto  address_first_element_impl( T (&t)[N],
     std::integral_constant<std::size_t, 1u> ) noexcept -> T *
    { return t; }
    //! `address_first_element` when there's too many array nestings
    template < typename T, std::size_t K >
    constexpr
    auto  address_first_element_impl( T &&t, std::integral_constant<std::size_t,
     K> ) noexcept -> typename remove_some_extents<typename
     std::remove_reference<T>::type, K>::type *
    {
        return address_first_element_impl( static_cast<T&&>(t)[0],
         std::integral_constant<std::size_t, K - 1u>{} );
    }

    //! Get the address of the first element
    template < std::size_t Extents = 1u, typename T >
    constexpr
    auto  address_first_element( T &&t ) noexcept ->
     typename remove_some_extents<typename std::remove_reference<T>::type,
     Extents>::type *
    {
        return address_first_element_impl( static_cast<T&&>(t),
         std::integral_constant<std::size_t, Extents *
         std::is_array<typename std::remove_reference<T>::type>::value>{} );
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
    bool  empty() const noexcept  { return !size(); }

    // Element access
    /** \brief  Returns underlying access.

    `[ data(), data() + size() )` is a valid range of #value_type covering all
    the stored elements.

        \returns  The address of the first element (of #value_type).
     */
    auto  data()       noexcept -> pointer
    { return detail::address_first_element<dimensionality>(data_block); }
    //! \overload
    constexpr
    auto  data() const noexcept -> const_pointer
    { return detail::address_first_element<dimensionality>(data_block); }

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
    void  fill( const_reference v )
     noexcept( noexcept(detail::deep_assign( (reference)v, v )) )
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

    /** \brief  Calls function on element.

    Calls the given function, with the sole element as its argument.  It
    includes the index coordinates, which are none.  The element will be passed
    with the same `const` state as its owning `array_md` object.

        \param f  The function, function-pointer, function-object, or lambda
                  that will execute the code.  It has to take a single argument
                  compatible with #value_type (or (immutable) reference of).

        \post  Unspecified, since *f* is allowed to alter the sole element (when
               taking a mutable reference) and/or itself during the call.
     */
    template < typename Function >
    void  apply( Function &&f )  { std::forward<Function>(f)(data_block); }
    //! \overload
    template < typename Function >
    void  apply( Function &&f ) const
    { std::forward<Function>(f)(data_block); }
    /** \brief  Calls function on element, immutable access

    Provides a way for a mutable-mode object to get immutable-mode element
    access (via looping) without `const_cast` convolutions with #apply.

        \param f  The function, function-pointer, function-object, or lambda
                  that will execute the code.  It has to take a single argument
                  compatible with #value_type (or `const` reference of).

        \post  Unspecified, since *f* may alter itself during the call.
     */
    template < typename Function >
    void  capply( Function &&f ) const
    { std::forward<Function>(f)(data_block); }

    /** \brief  Conversion, cross-type same-shape

    Creates a copy of this object, with a different type for the value.

        \pre  `static_cast<U>( std::declval<value_type>() )` is well-formed.

        \throws  Whatever  the element-level conversion throws.

        \returns  An object containing a conversion of the sole element.
     */
    template < typename U >
    explicit constexpr  operator array_md<U>() const
    { return {static_cast<U>( data_block )}; }

    // Member data
    //! The element, public to support aggregate initialization.
    data_type  data_block;
};

/** \brief  Recursive-case specialization of `array_md`.

The recursive case directly stores a C-level array of the previous case's
data type.

When the first extent is zero, the first extent of the corresponding C-level
array is of unknown bound.  This is an incomplete type, so the member data is
instead implemented as an unspecified POD type.

    \pre  Any extents after the first must be positive.

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
    typedef typename detail::append_extents<T, N...>::type  direct_element_type;
    //! The type of #data_block; not equal to #value_type in recursive cases.
    typedef typename detail::append_extents<T,M,N...>::type  data_type;
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
    static constexpr  size_type  static_size = std::extent<data_type>::value *
     ( sizeof(direct_element_type) / sizeof(value_type) );

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
    bool  empty() const noexcept  { return !size(); }

    // Element (or sub-array) access
    /** \brief  Returns underlying access.

    `[ data(), data() + size() )` is a valid range of #value_type covering all
    the stored elements.  Using this method flattens the dimensions away to
    linear access.

        \returns  The address of the first element (of #value_type).
     */
    auto  data()       noexcept -> pointer
    { return secret_data(std::integral_constant<bool, static_size>{}); }
    //! \overload
    constexpr
    auto  data() const noexcept -> const_pointer
    { return secret_data(std::integral_constant<bool, static_size>{}); }

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
    void  fill( const_reference v )
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
        // (eventually) call the ADL swap of the inner non-array type.
        std::swap( data_block, other.data_block );
    }

    /** \brief  Calls function on all elements, with indices.

    Calls the given function, with the each element as its argument.  It
    includes the index coordinates as trailing arguments.  Each element will be
    passed with the same mutablility state as its container.

    The order the elements will be called should be the same as forward
    iteration.  However, the called function should be path-conservative and not
    depend on the visiting order, using the passed indices instead.

        \param f  The function, function-pointer, function-object, or lambda
                  that will execute the code.  It has to take #dimensionality +
                  1 arguments.  The first argument must be compatible with
                  #value_type (or (immutable) reference of); subsequent
                  arguments have to be compatible with `std::size_t`.

        \post  Unspecified, since *f* is allowed to alter the elements (when
               taking a mutable reference) and/or itself during the calls.
     */
    template < typename Function >
    void  apply( Function &&f )
    {
        detail::array_apply( std::integral_constant<std::size_t,
         dimensionality>{}, std::forward<Function>(f), data_block );
    }
    //! \overload
    template < typename Function >
    void  apply( Function &&f ) const
    {
        detail::array_apply( std::integral_constant<std::size_t,
         dimensionality>{}, std::forward<Function>(f), data_block );
    }
    /** \brief  Calls function on all elements, with indices, immutable access

    Provides a way for a mutable-mode object to get immutable-mode element
    access (via looping) without `const_cast` convolutions with #apply.

        \param f  The function, function-pointer, function-object, or lambda
                  that will execute the code.  It has to take #dimensionality +
                  1 arguments.  The first argument must be compatible with
                  #value_type (or `const` reference of); subsequent arguments
                  have to be compatible with `std::size_t`.

        \post  Unspecified, since *f* may alter itself during the calls.
     */
    template < typename Function >
    void  capply( Function &&f ) const  { apply(std::forward<Function>( f )); }

    //! Conversion, cross-type same-shape
    template < typename U >
    explicit constexpr  operator array_md<U, M, N...>() const;

    // Member data
    //! The element(s), public to support aggregate initialization.
    typename std::conditional<static_size, data_type, typename
     std::aligned_storage<sizeof(value_type), alignof(value_type)>::type>::type
     data_block;

private:
    // Secret implmentation
          pointer  secret_data( std::true_type )
    { return detail::address_first_element<dimensionality>(data_block); }
    constexpr
    const_pointer  secret_data( std::true_type ) const
    { return detail::address_first_element<dimensionality>(data_block); }
          pointer  secret_data( std::false_type )
    { return reinterpret_cast<pointer>(&data_block); }
    constexpr
    const_pointer  secret_data( std::false_type ) const
    { return reinterpret_cast<const_pointer>(&data_block); }
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


//  More implementation details  ---------------------------------------------//

//! \cond
namespace detail
{
    //! Helper class for building externally-nested array_md instantiations.
    template < typename T, std::size_t ...N >
    struct nested_array_helper;
    //! Zero-dimension arrays can't be nested.
    template < typename T >
    struct nested_array_helper<T>
    {
        typedef boost::container::array_md<T>  type;
    };
    //! One-dimensional arrays are trivially nested.
    template < typename T, std::size_t N >
    struct nested_array_helper<T, N>
    {
        typedef boost::container::array_md<T, N>  type;
    };
    //! In general, nest the class types instead of the array types.
    template < typename T, std::size_t N0, std::size_t N1, std::size_t ...N >
    struct nested_array_helper<T, N0, N1, N...>
    {
        typedef boost::container::array_md<typename nested_array_helper<T, N1,
         N...>::type, N0>  type;
    };

}  // namespace detail
//! \endcond


//  Externally multi-dimensional array alias template definition  ------------//

/** \brief  Method to specify that a multi-dimensional constant-size array will
            nest at the class level.

The `array_md` class template applies multiple dimensions via nesting array
types by default.  Another philosophy is to nest the class types; an `array_md`
instantiation as the element type of another.  Such instantiations are linear
arrays outside the special alias.

Multi-level access is done by a chain of `operator []()` calls, or other
single-element options.  As a favor to usability, a one-dimensional use of this
template stores the element type directly, instead of within zero-dimension
array objects.

    \tparam ElementType  The type of the elements.
    \tparam Extents      The size of each dimension of the multi-level array.
                         Given in the same order as nested C-level arrays, e.g.
                         `array_md<int, 6, 5>` maps to a type whose objects,
                         like `x`, have a maximum indexing of `x[5][4]`.  If no
                         extents are given, then a single element is stored.

 */
template < typename ElementType, std::size_t ...Extents >
using nested_array_md = typename detail::nested_array_helper<ElementType,
 Extents...>::type;


//  More implementation details, part deux  ----------------------------------//

//! \cond
namespace detail
{
    //! Base construct for simulating C++14's integer sequences
    //! (The next few types and function taken from StackOverflow.)
    template < std::size_t ... > struct seq { typedef seq type; };
    //! Make larger sequences, prototype
    template < typename S1, typename S2 > struct concat;
    //! Make a larger sequence, actual work
    template < std::size_t ...I1, std::size_t ...I2 >
    struct concat< seq<I1...>, seq<I2...> >
        : seq< I1..., (sizeof...( I1 ) + I2)... >
    { };

    //! Make an in-order sequence, prototype
    template <std::size_t N> struct gen_seq;
    //! In-order integer sequence, degenerate case
    template < > struct gen_seq< 0u > : seq< > {};
    //! In-order integer sequence, base case
    template < > struct gen_seq< 1u > : seq< 0 > {};
    //! In-order integer sequence, recursive case
    template < std::size_t N > struct gen_seq
        : concat< typename gen_seq<N/2u>::type, typename gen_seq<N - N/2u>::type
          >::type
    { };

    //! Create objects with the integer sequence encoded
    template < std::size_t N >
    constexpr
    typename gen_seq<N>::type  make_int_seq() noexcept  { return {}; }

    //! Convert arrays with the help of a integer sequence
    template < typename U, typename T, std::size_t ...N, std::size_t ...I >
    constexpr
    boost::container::array_md<U, N...>
    convert_array( boost::container::array_md<T, N...> const &t, seq<I...> )
    {
        // The expression deliberately uses the sloppy array initialization that
        // forgoes internal braces.  That makes it work with any array rank, but
        // flags warnings in compilers that care about proper form.
        return boost::container::array_md<U, N...>{ {static_cast<U>( *(t.data()
         + I) )...} };
    }

}  // namespace detail
//! \endcond


//  Multi-dimensional array class template, member operator definition  ------//

/** Creates a copy of this object, with a different type for the value.

    \pre  `static_cast<U>( std::declval<value_type>() )` is well-formed.

    \throws  Whatever  the element-level conversion throws.

    \returns  An object containing conversions of each element.
 */
template < typename T, std::size_t M, std::size_t ...N >
template < typename U >
inline constexpr
array_md<T, M, N...>::operator array_md<U, M, N...>() const
{ return detail::convert_array<U>(*this, detail::make_int_seq<static_size>()); }


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

    \see  #operator==(array_md<T,N...>const&,array_md<U,N...>const&)

    \retval true   If any element in *l* doesn't equal its corresponding element
                   in *r*.
    \retval false  Otherwise.
 */
template < typename T, typename U, std::size_t ...N >
inline
bool  operator !=( array_md<T, N...> const &l, array_md<U, N...> const &r )
{ return !(l == r); }

/** \brief  Less-than comparison for `array_md`.

Compares two `array_md` objects, with the same size (same number of dimensions
and the corresponding dimensions have to be equal), for the first object being
ordered less than the second.  Note that it usually won't work with built-in
arrays as the element types, since they don't support neither `operator ==` nor
`operator <`.

Comparison is elementary when the dimension count (`sizeof...(N)`) is zero or
one.  For higher dimensions, it's the lexicographical compare of the top-level
array-components, which switch to the lexicographical compare of the 2nd-level
array-components of the differing 1st-level components, etc., which translates
to a lexicographical compare to the elements in iteration order, since elements
are stored in row-major order.

    \pre  `std::declval<T>() == std::declval<U>()` is well-formed.
    \pre  `std::declval<T>() < std::declval<U>()` is well-formed.

    \param l  The left-side argument.
    \param r  The right-side argument.

    \see  #operator==(array_md<T,N...>const&,array_md<U,N...>const&)

    \retval true   If the element sequences for *l* and *r* differ in at least
                   one spot, and the *l*'s element at that spot is less than the
                   corresponding element from *r*.
    \retval false  Otherwise.
 */
template < typename T, typename U, std::size_t ...N >
inline
bool  operator <( array_md<T, N...> const &l, array_md<U, N...> const &r )
{
    auto const  s = std::mismatch( l.begin(), l.end(), r.begin() );

    return ( s.first != l.end() ) && ( *s.first < *s.second );
}

/** \brief  Greater-than comparison for `array_md`.

Compares two `array_md` objects, with the same size (same number of dimensions
and the corresponding dimensions have to be equal), for the first object being
ordered greater than the second.  Note that it usually won't work with built-in
arrays as the element types, since they don't support neither `operator ==` nor
`operator <`.

    \pre  `std::declval<U>() == std::declval<T>()` is well-formed.
    \pre  `std::declval<U>() < std::declval<T>()` is well-formed.

    \param l  The left-side argument.
    \param r  The right-side argument.

    \see  #operator<(array_md<T,N...>const&,array_md<U,N...>const&)

    \retval true   If the element sequences for *l* and *r* differ in at least
                   one spot, and the *l*'s element at that spot is greater than
                   the corresponding element from *r*.
    \retval false  Otherwise.
 */
template < typename T, typename U, std::size_t ...N >
inline
bool  operator >( array_md<T, N...> const &l, array_md<U, N...> const &r )
{ return operator <( r, l ); }

/** \brief  Greater-than-or-equal-to comparison for `array_md`.

Compares two `array_md` objects, with the same size (same number of dimensions
and the corresponding dimensions have to be equal), for the first object being
ordered greater than or equal to the second (i.e. not less than).  Note that it
usually won't work with built-in arrays as the element types, since they don't
support neither `operator ==` nor `operator <`.

    \pre  `std::declval<T>() == std::declval<U>()` is well-formed.
    \pre  `std::declval<T>() < std::declval<U>()` is well-formed.

    \param l  The left-side argument.
    \param r  The right-side argument.

    \see  #operator<(array_md<T,N...>const&,array_md<U,N...>const&)

    \retval true   If the element sequences for *l* and *r* differ in at least
                   one spot, and the *l*'s element at that spot is greater than
                   or equal to the corresponding element from *r*.
    \retval false  Otherwise.
 */
template < typename T, typename U, std::size_t ...N >
inline
bool  operator >=( array_md<T, N...> const &l, array_md<U, N...> const &r )
{ return not operator <( l, r ); }

/** \brief  Less-than-or-equal-to comparison for `array_md`.

Compares two `array_md` objects, with the same size (same number of dimensions
and the corresponding dimensions have to be equal), for the first object being
ordered less than or equal to the second (i.e. not greater than).  Note that it
usually won't work with built-in arrays as the element types, since they don't
support neither `operator ==` nor `operator <`.

    \pre  `std::declval<U>() == std::declval<T>()` is well-formed.
    \pre  `std::declval<U>() < std::declval<T>()` is well-formed.

    \param l  The left-side argument.
    \param r  The right-side argument.

    \see  #operator<(array_md<T,N...>const&,array_md<U,N...>const&)

    \retval true   If the element sequences for *l* and *r* differ in at least
                   one spot, and the *l*'s element at that spot is less than or
                   equal to the corresponding element from *r*.
    \retval false  Otherwise.
 */
template < typename T, typename U, std::size_t ...N >
inline
bool  operator <=( array_md<T, N...> const &l, array_md<U, N...> const &r )
{ return not operator >( l, r ); }


//  More implementation details, part trois  ---------------------------------//

//! \cond
namespace detail
{
    //! Create array with list of initializers, non-braced
    template < typename T, std::size_t ...N, typename ...Args >
    constexpr
    boost::container::array_md<T, N...>
    make_array_impl( std::false_type, Args &&...args )
    {
        // This should be called only when sizeof...(N) == 0.  In that case,
        // having sizeof...(args) > 1 should lead to an error.
        return boost::container::array_md< T, N... >{ static_cast<T>(
         static_cast<Args &&>( args ))... };
    }
    //! Create array with list of initializers, braced
    template < typename T, std::size_t ...N, typename ...Args >
    constexpr
    boost::container::array_md<T, N...>
    make_array_impl( std::true_type, Args &&...args )
    {
        // This should be called only when sizeof...(N) > 0.  In those cases,
        // having sizeof...(N) > 1 when sizeof...(args) > 0 will use the sloppy
        // array initialization syntax, which will flag warnings in compilers
        // that care about proper form.
        return boost::container::array_md< T, N... >{ {static_cast<T>(
         static_cast<Args &&>( args ))...} };
    }

    //! Peel extents from a built-in array type to an array container class.
    template < typename ArrayType, std::size_t PeeledExtents, std::size_t
     ...ExtraExtents >
    struct array_peeler;
    //! Base case: no further peeling; make an `array_md` from what we got.
    template < typename T, std::size_t ...E >
    struct array_peeler<T, 0, E...>
    { typedef boost::container::array_md<T, E...> type; };
    //! Recursive case: shift outermost extent from array to list.
    template < typename T, std::size_t P, std::size_t ...E >
    struct array_peeler
    {
        typedef typename array_peeler<
            typename std::remove_extent<T>::type,
            P - 1u,
            E..., std::extent<T>::value
        >::type  type;
    };

    //! Create a nested `array_md` from a built-in array.
    template < typename T >
    void  make_nested_helper( boost::container::array_md<T> &out, T const &s )
    { deep_assign(out.data_block, s); }
    //! \overload
    template < typename T, std::size_t N >
    void  make_nested_helper( boost::container::array_md<T, N> &out, T const
     (&s)[N] )
    { deep_assign(out.data_block, s); }
    //! \overload
    template < typename T, typename U, std::size_t N, std::size_t ...M >
    void  make_nested_helper( boost::container::array_md<T, N, M...> &out,
     U const (&s)[N] )
    { for (std::size_t i = 0u ; i < N ; ++i) make_nested_helper(out[i], s[i]); }
    //! Fill a built-in array from a nested `array_md`.
    template < typename T >
    void  unmake_nested_helper( boost::container::array_md<T> const &s, T &out )
    { deep_assign(out, s.data_block); }
    //! \overload
    template < typename T, std::size_t N >
    void  unmake_nested_helper( boost::container::array_md<T, N> const &s, T
     (&out)[N] )
    { deep_assign(out, s.data_block); }
    //! \overload
    template < typename T, typename U, std::size_t N, std::size_t ...M >
    void  unmake_nested_helper( boost::container::array_md<T, N, M...> const &s,
     U (&out)[N] )
    { for(std::size_t i = 0u ; i < N ; ++i) unmake_nested_helper(s[i],out[i]); }

    //! Helper class for unbuilding externally-nested `array_md` instantiations.
    template < class ArrayType, std::size_t ...ExtraExtents >
    struct nested_array_unhelper;
    //! Base case: no inner `array_md` type to justify stripping an outer.
    template < typename T, std::size_t ...D, std::size_t ...L >
    struct nested_array_unhelper< boost::container::array_md<T, L...>, D... >
    { typedef boost::container::array_md<T, D..., L...> type; };
    //! Recursive case: strip outer extents to list.
    template <typename U, std::size_t ...E, std::size_t ...N, std::size_t ...M>
    struct nested_array_unhelper<
     boost::container::array_md<boost::container::array_md<U, M...>, N...>,
     E... >
    {
        typedef typename nested_array_unhelper<
            boost::container::array_md<U, M...>,
            E..., N...
        >::type  type;
    };

}  // namespace detail
//! \endcond


//  Multi-dimensional array class template, creation functions  --------------//

/** \brief  Create a typed and shaped array using a list of values.

Makes an new `array_md` object with the given element type and extent list, and
uses the given arguments as the initializers.

    \pre  `sizeof...(args) <= Product(N...)`.
    \pre  `static_cast<T>( std::declval<A>() )` is well-formed, where `A` is any
          of the function argument types.

    \tparam T  The type of the elements.
    \tparam N  The size of each dimension of the array.  May be empty.

    \param args  The objects to initialize each array element.  May be empty.

    \throws Whatever  converting any argument to `T` may throw.
    \throws What      copy/move-initialization of `T` may throw.

    \returns  an array object *x* such that
              - For `0 <= k < Min( x.size(), sizeof...(args) )`, `*( x.begin() +
                k )` is equivalent to `static_cast<T>( args[k] )`.
              - For `sizeof...(args) <= k < x.size()`, `*( x.begin() + k )` is
                equivalent to `T{}`.
 */
template < typename T, std::size_t ...N, typename ...Args >
inline constexpr
array_md<T, N...>  make_array( Args &&...args )
{
    return detail::make_array_impl<T, N...>( std::integral_constant<bool,
     sizeof...(N)>{}, static_cast<Args &&>(args)... );
}

/** \brief  Create a fitted array from a list of values.

Makes an new `array_md` object, and uses the given arguments as the
initializers.  The element type and number of elements are automatically
computed from the arguments.  (A zero-sized array cannot be created.)

    \pre  `sizeof...(args) > 0`.
    \pre  `typename std::common_type<decltype(args)...>::type` is well-formed.
    \pre  `static_cast<C>( std::declval<A>() )` is well-formed, where `C` is the
          common type and `A` is any of the function argument types.

    \param args  The objects to initialize each array element.

    \throws Whatever  converting any argument to the element type may throw.

    \returns  an array with a copy of the function arguments.
 */
template < typename ...Args >
inline constexpr
array_md<typename std::common_type<Args...>::type, sizeof...(Args)>
make_auto_array( Args &&...args )
{
    return make_array< typename std::common_type<Args...>::type, sizeof...(Args)
     >( static_cast<Args &&>(args)... );
}

/** \brief  Copy an array's data to another array of arbitrary type and shape.

Makes an new `array_md` object with the given element type and extent list, and
copy another array's elements into it.

    \pre  `static_cast<T>( std::declval<typename decltype(source)::value_type>()
          )` is well-formed.
    \pre  `T` is Default-Constructible.

    \tparam T  The type of the elements.
    \tparam N  The size of each dimension of the array.  May be empty.

    \param source  The objects to copy into each array element.

    \throws Whatever  converting any argument to `T` may throw.
    \throws What      copy-assignment of `T` may throw.

    \returns  an array object *x* such that
              - For `0 <= k < Min( x.size(), source.size() )`, `*( x.begin() +
                k )` is equivalent to `static_cast<T>( *(source.begin() + k) )`.
              - For `source.size() <= k < x.size()`, `*( x.begin() + k )` is
                equivalent to `T{}`.
 */
template < typename T, std::size_t ...N, typename U, std::size_t ...M >
//constexpr  // in C++14?
array_md<T, N...>  remake_array( array_md<U, M...> const &source )
{
    array_md<T, N...>  result{};

    std::transform( source.begin(), source.begin() + std::min(source.size(),
     result.size()), result.begin(), [](U const&u){return static_cast<T>(u);} );
    return result;
}

/** \brief  Copy a built-in array to an `array_md`.

Makes a new `array_md` object from the elements of the given array, each of the
given number of extents moved from the built-in array's extents to the
`array_md` trailing extent list.  The count of outer extents has to be specified
by the user; for example, should an built-in array of built-in strings, which
are built-in arrays of `char`, be converted at the string or character level?

    \pre  `Peelings <= std::rank<T>::value`.
    \pre  `T` is Default-Constructible and Copy-Assignable.

    \tparam Peelings  The number of extents to transfer from the built-in array
                      type to the `array_md` instantiation.  (Note that the
                      class's internal data type will always equal the built-in
                      array type.)  Transfers are taken from the outer-most
                      extents (i.e. the largest element block type).  Defaults
                      to 1 for ease of the common case.
    \tparam T         The type of the built-in (multidimensional) array.  Should
                      be deduced by the compiler.

    \param source  The elements to copy.

    \returns  an array object *x* such that `x[n0]..[nLast] ==
              source[n0]..[nLast]` for each valid index-tuple for elements of
              the inner non-array type.
 */
template < std::size_t Peelings = 1, typename T >
typename detail::array_peeler<T, Peelings>::type  to_array( T const &source )
{
    static_assert( std::rank<T>::value >= Peelings, "Too few dimensions" );

    decltype( to_array<Peelings>(source) )  result;

    detail::deep_assign( result.data_block, source );
    return result;
}

/** \brief  Convert from array- to class-level data nesting for `array_md`.

    \param source  The elements to copy.

    \returns  an array object *x* such that `x[n0]..[nLast] ==
              source[n0]..[nLast]` for each valid index-tuple for elements of
              the inner non-array type.
 */
template < typename T, std::size_t ...N >
nested_array_md<T, N...>  make_nested( array_md<T, N...> const &source )
{
    decltype( make_nested(source) )  result;

    detail::make_nested_helper( result, source.data_block );
    return result;
}

/** \brief  Convert from class- to array-level data nesting for `array_md`.

    \pre  `decltype(source)` has to be a type that could match a
          `nested_array_md` instantiation.  (In other words, all of the
          `array_md` instantiations have to be linear.)

    \param source  The elements to copy.

    \returns  an array object *x* such that `x[n0]..[nLast] ==
              source[n0]..[nLast]` for each valid index-tuple for elements of
              the inner non-array type.
 */
template < typename T, std::size_t ...N >
auto  unmake_nested( array_md<T, N...> const &source ) -> typename
 detail::nested_array_unhelper< array_md<T, N...> >::type
{
    decltype( unmake_nested(source) )  result;

    detail::unmake_nested_helper( source, result.data_block );
    return result;
}


//  Multi-dimensional array class template, other operations  ----------------//

/** \brief  Swap routine for `array_md`.

Exchanges the state of two `array_md` objects.  The objects have to have the
same element type and number of dimensions, and the corresponding dimensions
have to be equal.

    \pre  There is a swapping routine, called `swap`, either in namespace `std`
          for built-ins, or found via ADL for other types.

    \param a  The first object to have its state exchanged.
    \param b  The second object to have its state exchanged.

    \see  #array_md<T,N...>::swap(array_md<T,N...>&)

    \throws Whatever  the element-level swap does.

    \post  `a` is equivalent to the old state of `b`, while `b` is equivalent to
           the old state of `a`.
 */
template < typename T, std::size_t ...N >
inline
void  swap( array_md<T, N...> &a, array_md<T, N...> &b )
 noexcept( noexcept(a.swap( b )) )
{ a.swap(b); }

/** \brief  Non-member array element access

Extracts the *I*th element from the array.  Even when the `dimensionality` is
greater than one, the index treats the array like it's flat, so the element
type is always `value_type`, and not some intermediate sub-array type.

    \pre  0 \<= *I* \< array_md<T, N...>::static_size.

    \tparam I  The index of the desired element.  The index is the offset of the
               element from begin(*a*).

    \param a  The array to be extracted from.

    \returns  A reference to the desired element.
 */
template < std::size_t I, typename T, std::size_t ...N >
inline constexpr
auto  get( array_md<T, N...> const &a ) noexcept -> T const &
{
    static_assert( I < array_md<T, N...>::static_size, "Index too large" );

    return a.data()[ I ];
}

//! \overload
template < std::size_t I, typename T, std::size_t ...N >
inline
auto  get( array_md<T, N...> &a ) noexcept -> T &
{ return const_cast<T &>( get<I>(const_cast<array_md<T,N...> const &>( a )) ); }

//! \overload
template < std::size_t I, typename T, std::size_t ...N >
inline
auto  get( array_md<T, N...> &&a ) noexcept -> T &&
{ return std::forward<T>( get<I>(a) ); }


}  // namespace container
}  // namespace boost


//  Specializations from the STD namespace  ----------------------------------//

//! The standard namespace
namespace std
{
    //! Provides number of direct elements in an `array_md` object.  For this
    //! type, all of the elements are of type `value_type`.
    template < typename T, size_t ...N >
    class tuple_size< boost::container::array_md<T, N...> >
        : public integral_constant< size_t, boost::container::array_md<T,
           N...>::static_size >
    { };

    //! Provides the type of each element in an `array_md` object.
    template < size_t I, typename T, size_t ...N >
    class tuple_element< I, boost::container::array_md<T, N...> >
    {
        static_assert( I < boost::container::array_md<T, N...>::static_size,
         "Index too large" );

        //! The target type, index-independent for an array.
        typedef T  type;
    };

}  // namespace std


#endif // BOOST_CONTAINER_ARRAY_MD_HPP
