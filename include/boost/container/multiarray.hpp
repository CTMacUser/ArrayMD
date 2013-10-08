//  Boost Multi-dimensional Array Adapter header file  -----------------------//

//  Copyright 2013 Daryle Walker.
//  Distributed under the Boost Software License, Version 1.0.  (See the
//  accompanying file LICENSE_1_0.txt or a copy at
//  <http://www.boost.org/LICENSE_1_0.txt>.)

//  See <http://www.boost.org/libs/container/> for the library's home page.

/** \file
    \brief  A class template that is a container adapter class that grants
      multiple-value indexing.

    \author  Daryle Walker

    \version  0.1

    \copyright  Boost Software License, version 1.0

    Contains the declarations (and definitions) of class templates modeling a
    container adapter.  The adaptation grants a sequence container (with random-
    access iterators) a way to access elements with a given number of indexes.

    \warning  This library requires C++2011 features.
 */

#ifndef BOOST_CONTAINER_MULTIARRAY_HPP
#define BOOST_CONTAINER_MULTIARRAY_HPP

#include <algorithm>
#include <array>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

// Put Boost #includes here.


namespace boost
{
namespace container
{


//  Implementation details  --------------------------------------------------//

//! \cond
namespace detail
{
    //! Detect if a type's swap (found via ADL for non-built-ins) throws.
    template < typename T, typename U = T >
    inline constexpr
    bool  is_swap_nothrow_too() noexcept
    {
        using std::swap;

        return noexcept( swap(std::declval<T &>(), std::declval<U &>()) );
    }

    // Yet another compile-time integer sequence
    template < std::size_t ...N >
    struct indices_too  { using next = indices_too<N..., sizeof...(N)>; };
    template < std::size_t N >
    struct make_indices_too
    { using type = typename make_indices_too<N - 1u>::type::next; };
    template < > struct make_indices_too<0u> { using type = indices_too<>; };

    // Implementation for following function
    template < typename Function, typename T, class Tuple, std::size_t ...I >
    void  apply_x_and_exploded_tuple_impl( Function &&f, T &&x, Tuple &&t,
     indices_too<I...> )
    {
        using std::forward;
        using std::tuple_element;
        using std::get;

        forward<Function>( f )( forward<T>(x), forward<typename tuple_element<I,
         typename std::remove_reference<Tuple>::type>::type>(get<I>( t ))... );
    }
    //! Call the given function with the given argument followed by the elements
    //! of the composite object (individually).
    template < typename Function, typename T, class Tuple >
    void  apply_x_and_exploded_tuple( Function &&f, T &&x, Tuple &&t )
    {
        using std::forward;

        apply_x_and_exploded_tuple_impl( forward<Function>(f), forward<T>(x),
         forward<Tuple>(t), typename make_indices_too<std::tuple_size<typename
         std::remove_reference<Tuple>::type>::value>::type{} );
    }

}  // namespace detail
//! \endcond


//  Multi-dimensional array adapter base class template definitions  ---------//

namespace detail
{

/** \brief  Base class for `multiarray` to store the container and reference its
            elements.

This type is meant to be used as an implementation detail for #multiarray, but
is public since it's easier to document its members here, instead of its derived
class.

    \pre  `Element` can be used as a container element type.
    \pre  `Element` is the element type for `Container`.
    \pre  `Container` should be a sequence-container that supports random-access
          iterators.  (But it can be any Standard-esque container that supports
          at least forward iterators.)  It has to have at least the `begin`,
          `size`, `empty`, and `swap` member functions (plus needed support
          types and type-aliases), with their expected Standard semantics.

    \tparam Element    The type of the elements.
    \tparam Container  The internal container for the elements.  If not given,
                       defaults to `std::vector<Element>`.

 */
template < typename Element, class Container = std::vector<Element> >
class multiarray_storage_base
{
    static_assert( std::is_same<Element, typename Container::value_type>::value,
     "Container doesn't hold right kind of element" );

public:
    // Template parameters
    //! The element type.  Gives access to its template parameter.
    using     value_type = Element;
    //! The container type.  Gives access to its template parameter.
    using container_type = Container;

    // Other types
    //! The type for referring to an element.
    using       reference = typename container_type::reference;
    //! The type for referring to an element, immutable access.
    using const_reference = typename container_type::const_reference;
    //! The type for size-based meta-data.
    using  size_type      = typename container_type::size_type;

    // Container status
    //! \returns  `size() == 0`; i.e. if there are no elements.
    bool      empty() const  { return c.empty(); }
    //! \returns  The number of stored elements (of #value_type).
    size_type  size() const  { return c.size(); }

    // Element access
    /** \returns  A reference to the selected element.
        \pre      The length or values of `i` may be subject to bounds set by a
                  derived type's override of the private pure-virtual function
                  #get_offset.  Said bounds may be static or dynamic.
        \param i  The list of indexes needed to locate the element.
        \details  Converts `i` to an index within #c with #get_offset and
                  returns a reference to the element with that offset from
                  `begin`.
     */
          reference  operator ()( std::initializer_list<size_type> i )
    {
        return const_cast<reference>( const_cast<multiarray_storage_base const
         *>(this)->operator ()(i) );
    }
    //! \overload
    const_reference  operator ()( std::initializer_list<size_type> i ) const
    {
        auto  ci = c.begin();

        std::advance( ci, get_offset(i.begin(), i.end(), false) );
        return *ci;
    }
    /** \overload
        \pre  Each entry of `args` has to implicitly convert to `size_type`.
        \param args  The individual indexes.
        \returns  `operator ()( {args...} )`.
        \see  #operator()(std::initializer_list<size_type>)
     */
    template < typename ...Args >       reference  operator()( Args &&...args )
    {
        return const_cast<reference>( const_cast<multiarray_storage_base const
         *>(this)->operator ()(std::forward<Args>( args )...) );
    }
    //! \overload
    template < typename ...Args > const_reference  operator()( Args &&...args )
     const
    {
        constexpr auto   al = sizeof...( Args );
        size_type const  indexes[ al + !al ] = {
         static_cast<size_type>(std::forward<Args>( args ))... };
        auto             ci = c.begin();

        std::advance( ci, get_offset(std::begin( indexes ), std::end( indexes )
         - !al, false) );
        return *ci;
    }

    /** \brief  Checked element access.
        \details  Converts `i` to an index within #c with #get_offset and
                  returns a reference to the element with that offset from
                  `begin`.
        \param i  The list of indexes needed to locate the element.
        \throws Whatever  the derived class's override of #get_offset does when
                  `i` has the wrong number of entries or at least one is not in
                  the correct range.
        \returns  A reference to the selected element.
     */
          reference  at( std::initializer_list<size_type> i )
    {
        return const_cast<reference>( const_cast<multiarray_storage_base const
         *>(this)->at(i) );
    }
    //! \overload
    const_reference  at( std::initializer_list<size_type> i ) const
    {
        auto  ci = c.begin();

        std::advance( ci, get_offset(i.begin(), i.end(), true) );
        return *ci;
    }
    /** \overload
        \pre  Each entry of `args` has to implicitly convert to `size_type`.
        \param args  The individual indexes.
        \returns  `at()( {args...} )`.
        \see  #at(std::initializer_list<size_type>)
     */
    template < typename ...Args >        reference  at( Args &&...args )
    {
        return const_cast<reference>( const_cast<multiarray_storage_base const
         *>(this)->at(std::forward<Args>( args )...) );
    }
    //! \overload
    template < typename ...Args >  const_reference  at( Args &&...args ) const
    {
        constexpr auto   al = sizeof...( Args );
        size_type const  indexes[ al + !al ] = {
         static_cast<size_type>(std::forward<Args>( args ))... };
        auto             ci = c.begin();

        std::advance( ci, get_offset(std::begin( indexes ), std::end( indexes )
         - !al, true) );
        return *ci;
    }

    //! \returns  `operator ()( i )`.
    //! \see  #operator()(std::initializer_list<size_type>)
          reference  operator []( std::initializer_list<size_type> i )
    { return operator ()(i); }
    //! \overload
    const_reference  operator []( std::initializer_list<size_type> i ) const
    { return operator ()(i); }

    // Assignments
    /** \brief  Exchange state with the given object.
        \param other  The object to exchange state with.
        \throws Whatever  #c (and/or `other.c`) throws when swapping.
        \post  #c is equivalent to the pre-swap state of `other.c`.
        \post  `other.c` is equivalent to the pre-swap state of #c.
     */
    void  swap( multiarray_storage_base &other )
     noexcept( is_swap_nothrow_too<container_type>() )
    { using std::swap; swap(c, other.c); }

protected:
    /** \brief    The container for the element data.
        \details  It is not private so derived classes, implementation- or user-
                  level, can mess with it.  But it's not public to indicate
                  direct access isn't needed by default, just like the Standard
                  container adapter class templates.
     */
    container_type  c;

    // Lifetime management
    // (Use automatically-defined copy-ctr, move-ctr, and destructor)
    /** \brief  Default constructor
        \post   #c is default-initialized.
     */
              multiarray_storage_base()  {}
    /** \brief     Initialize with a container copy
        \param cc  The container to copy from.
        \post  #c is equivalent to `cc`.
     */
    explicit  multiarray_storage_base( container_type const &cc )  : c{ cc }  {}
    /** \brief  Initialize with a container move
        \param cc  The container to move from.
        \post  #c is equivalent to the pre-move value of `cc`.  The state of
               `cc` is unspecified.
     */
    explicit  multiarray_storage_base( container_type &&cc )
      : c{ std::move(cc) }
    {}

private:
    /** \brief    Convert a list of external indexes to its corresponding single
                  internal index.
        \details  The original version of this method is pure-virtual since the
                  base class doesn't have data to provide the external to
                  internal mapping, or when bounds are violated.  The method is
                  private since it's only to be called by #operator() and
                  similar member functions.
        \pre  `index_end` is reachable from `index_begin` with a nonnegative
              finite number of forward iterations.
        \param index_begin  The beginning of the range of external index values.
        \param index_end    The end of the range of external index values.
        \param throw_on_bad_input  If the external index range has the wrong
                            length, or at least one of the entries has an
                            improper value, then an exception shall be thrown
                            when this parameter is `true`.
        \throws Whatever    the derived class wants.  Mandatory when the input
                            is bad.
        \returns  The index within the internal container that the external
                  indexes map to.  May be outside the *current* bounds of #c.
     */
    virtual  size_type  get_offset( size_type const *index_begin, size_type
     const *index_end, bool throw_on_bad_input ) const = 0;
};

/** \brief  Base class for `multiarray` to track the index bounds and priority
            order, and to translate the index offsets to 1-D.

This type is meant to be used as an implementation detail for #multiarray, but
is public since it's easier to document its members here, instead of its derived
class.

    \pre  `SizeType` should be a built-in unsigned integer type.
    \pre  `Rank >= 0`.

    \tparam SizeType  The type of `Rank`.
    \tparam Rank      The number of index coordinates to access an element.

 */
template < typename SizeType, SizeType Rank >
class multiarray_indexed_base
{
    static_assert( std::is_unsigned<SizeType>::value, "Improper sizing type" );

public:
    // Template parameters
    //! The type for size-based meta-data.
    using size_type = SizeType;

    //! The number of extents.  Gives access to its template parameter.
    static constexpr  size_type  dimensionality = Rank;

    // Other types
    //! The type for giving and receiving extent or priority lists.
    using stats_type = std::array<size_type, dimensionality>;

    // Indexing status
    //! \returns  The number of elements needed to support all valid index-tuple
    //!           combinations.  (Equals the product of all index extents.)
    size_type  required_size() const
    {
        auto const  major_index = stats[ priorities_i ][ 0 ];

        return stats[ extents_i ][ major_index ] * stats[ strides_i ][
         major_index ];
    }

    //! \returns  The current extent for each index.
    stats_type  extents() const
    {
        stats_type  result;

        std::copy_n( std::begin(stats[ extents_i ]), dimensionality,
         result.begin() );
        return result;
    }
    /** \brief    Sets the extent for each index.
        \details  Changes the number of valid indexes for each dimension.  For a
                  given value *k* at `e[n]`, valid index values for the nth
                  parameter of addressing an element can range from `0` to `k -
                  1`.  The size of `e` is such that `n` ranges from `0` to
                  `dimensionality - 1`.  Also updates internal caches.
        \pre  `e.size() == dimensionality`.
        \pre  There is no element in `e` equal to 0.
        \pre  The product of the elements of `e` cannot exceed the maximum value
              supported by #size_type.
        \param e  The array of new extents.
        \throws std::out_of_range    when any element of `e` is zero.
        \throws std::overflow_error  when the product of `e`'s elements exceeds
                                     the limit of `size_type`.
        \post  `extents() == e`.
     */
    void        extents( stats_type const &e )
    { update_extents(e.begin(), e.end()); recalculate_strides(); }
    /** \overload
        \details  Does `extents( {{e0, e...}} )`.
        \pre  There are exactly #dimensionality arguments given.
        \pre  Each entry of `e` has to implicitly convert to `size_type`.
        \param e0  The first extent.
        \param e   The remaining extents.
        \post  `extents() == {{ e0, e... }}`.
        \see   #extents(stats_type const&)
     */
    template <
        typename ...Args,
        typename         = typename std::enable_if<1 + sizeof...(Args) ==
         dimensionality>::type
    >
    void        extents( size_type e0, Args &&...e )
    { extents(stats_type{ {e0, std::forward<Args>(e)...} }); }

    //! \returns  The current index priority list.
    stats_type  priorities() const
    {
        stats_type  result;

        std::copy_n( std::begin(stats[ priorities_i ]), dimensionality,
         result.begin() );
        return result;
    }
    /** \brief    Sets the priority for each index.
        \details  Changes the priority each index gets when computing the
                  singular offset.  The top place, a.k.a. most-major, has the
                  largest stride.  The last place index, a.k.a. least-major, has
                  the least influence with a stride of 1.  (But elements that
                  differ only in the least-major index, with a difference of 1,
                  are adjacent in memory.)  Also updates internal caches.
        \pre  `p.size() == dimensionality`.
        \pre  The elements of `p` are the values `0` through `dimensionality -
              1`, each appearing exactly once (in any order).
        \param p  The array of new priorities.  `p[0]` has the value of the
                  most-major index, down to the least-major index being stored
                  in `p[dimensionality - 1]`.
        \throws std::out_of_range      when any element of `p` matches or
                                       exceeds #dimensionality.
        \throws std::invalid_argument  when all of `p`'s elements are in range,
                                       but `p` isn't exactly one of each legal
                                       value.  (There are missing and repeated
                                       values.)
        \post  `priorities() == p`.
     */
    void        priorities( stats_type const &p )
    { update_priorities(p.begin(), p.end()); recalculate_strides(); }
    /** \overload
        \details  Does `priorities( {{p0, p...}} )`.
        \pre  There are exactly #dimensionality arguments given.
        \pre  Each entry of `p` has to implicitly convert to `size_type`.
        \param p0  The index of the most major extent.
        \param p   The remaining extents, in declining priority level.
        \post  `priorities() == {{ p0, p... }}`.
        \see   #priorities(stats_type const&)
     */
    template <
        typename ...Args,
        typename         = typename std::enable_if<1 + sizeof...(Args) ==
         dimensionality>::type
    >
    void        priorities( size_type p0, Args &&...p )
    { priorities(stats_type{ {p0, std::forward<Args>(p)...} }); }

    /** \brief    Sets indexing to use row-major order.
        \details  Calls #priorities(stats_type const&) with a value that sets
                  row-major order.  That order has the first index as the most-
                  major, down to the last index being least-major.
        \post     `priorities() == {{ 0 .. (dimensionality - 1) }}`.
     */
    void  use_row_major_order()
    {
        stats_type  row_major_order;

        std::iota(row_major_order.begin(), row_major_order.end(), size_type(0));
        priorities( row_major_order );
    }
    /** \brief    Sets indexing to use column-major order.
        \details  Calls #priorities(stats_type const&) with a value that sets
                  column-major order.  That order has the last index as the
                  most-major, up to the first index being least-major.
        \post     `priorities() == {{ (dimensionality - 1) .. 0 }}`.
     */
    void  use_column_major_order()
    {
        stats_type  column_major_order;
        size_type   i = 0u;

        for ( auto &x : column_major_order )
            x = dimensionality - 1u - i++;
        priorities( column_major_order );
    }

    /** \brief    Set the extents and priorities at the same time.
        \details  Calls `extents(e)` and `priorities(p)` in an unspecified
                  order.  The difference between this function and just calling
                  #extents(stats_type const&) and #priorities(stats_type const&)
                  consecutively is that the internal caches will be updated only
                  once, instead of twice (with the first time being thrown out).
        \pre  `e` and `p` have the same restrictions as in the single-attribute
              mutator member functions.
        \param e  The new extents for the indexes.
        \param p  The new index stride priorities.
        \post  `extents() == e && priorities() == p`.
     */
    void  extents_and_priorities( stats_type const &e, stats_type const &p )
    {
        // Save old extent list in case the priority list throws
        auto const  save = extents();

        update_extents( e.begin(), e.end() );
        try {
            update_priorities( p.begin(), p.end() );
        } catch ( ... ) {
            // Shouldn't throw because it's known-good.
            update_extents( save.begin(), save.end() );
            throw;
        }
        recalculate_strides();  // can't throw
    }

    // Assignments
    /** \brief  Exchange state with the given object.
        \param other  The object to exchange state with.
        \throws Whatever  #size_type objects may throw when swapping.
        \post  #extents() and #priorities() are equivalent to `other.extents()`
               and `other.priorities()` of pre-swap `other`.
        \post  `other.extents()` and `other.priorities()` are equivalent to
               #extents() and #priorities() of pre-swap `this`.
     */
    void  swap( multiarray_indexed_base &other )
     noexcept( is_swap_nothrow_too<size_type>() )
    { std::swap(stats, other.stats); }

protected:
    // Lifetime management
    // (Use automatically-defined copy-ctr, move-ctr, and destructor)
    /** \brief    Default constructor
        \details  Puts the index manager in a sane state with one element.
        \post  #extents() returns a value with no values that are not 1.  (This
               is either a singular element or an array of one element total.)
        \post  #priorities() returns a value indicating row-major order.
     */
    multiarray_indexed_base()
    {
        using std::begin;
        using std::end;

        // Singular element
        std::fill( begin(stats[ extents_i ]), end(stats[ extents_i ]),
         size_type(1) );

        // Row-major order
        std::iota( begin(stats[ priorities_i ]), end(stats[ priorities_i ]),
         size_type(0) );

        // Singular element means all ones for strides.
        std::fill( begin(stats[ strides_i ]), end(stats[ strides_i ]),
         size_type(1) );
    }

    // Multi-D to 1-D implementation
    /** \brief  Sanity-checks a list of indexes to (possibly) dereference.
        \pre  `ie` must be reachable from `ib` with a nonnegative finite number
              of forward iterations.
        \param ib  The beginning of the range of indexes to check.
        \param ie  The past-the-end of the range of indexes to check.
        \throws std::length_error  if the number of indexes is wrong.
        \throws std::out_of_range  if at least one index is not less than its
                                   corresponding extent.
     */
    void  throw_for_bad_indexes( size_type const *ib,size_type const *ie ) const
    {
        using  diff_type = decltype( std::distance(ib, ie) );

        if ( std::distance(ib, ie) != static_cast<diff_type>(dimensionality) )
            throw std::length_error{ "Wrong number of indexes" };

        if ( std::inner_product(ib, ie, std::begin( stats[extents_i] ), false,
         std::logical_or<bool>{}, std::greater_equal<size_type>{}) )
            throw std::out_of_range{ "Index too large" };
    }
    /** \returns  The singular internal offset mapped to the given external
                  index tuple.
        \pre  `ie` must be reachable from `ib` with a nonnegative finite number
              of forward iterations.
        \param ib  The beginning of the range of indexes to convert.
        \param ie  The past-the-end of the range of indexes to convert.
     */
    size_type  indexes_to_offset(size_type const *ib, size_type const *ie) const
    {
        return std::inner_product( ib, ie, std::begin(stats[ strides_i ]),
         size_type(0) );
    }

    // Given a pack of indexes, go to the next one (in memory)
    //! \returns  Starting value of index tuple iteration, no non-zeros.
    stats_type  first_index_pack() const  { return stats_type{}; }
    /** \brief    Iterate an index-tuple to its next value.
        \details  Changes `indexes` to the index coordinates of the next element
                  (adjacent) in memory.  The index with the least-major priority
                  is incremented, and any needed carries are propagated to next
                  more major index.  Incrementing the maximum index tuple wraps
                  around to all-zeros.
        \pre  `indexes` has to be valid, i.e. it wouldn't trigger
              #throw_for_bad_indexes.
        \param[in,out] indexes  The tuple to indexes to increment.
        \returns  `true` if the post-increment value equals #first_index_pack().
     */
    bool      advance_index_pack( stats_type &indexes ) const
    {
        // Start off with the least-major index being incremented.
        bool  turnover = true;

        // Go from least-major to most-major extent (use priorities)
        for ( auto  i = dimensionality ; i-- ; )
        {
            auto const  index = stats[ priorities_i ][ i ];

            indexes[ index ] += turnover;   // increment when TRUE
            turnover          = indexes[ index ] >= stats[ extents_i ][ index ];
            indexes[ index ] *= !turnover;  // reset when FALSE
        }

        // This returns TRUE only on an all-index turnover (back to all zeros).
        return turnover;

        // TODO: should this function change the return type to "void"?  That
        // way, I don't need the loop to go all the way; I can use "turnover" as
        // a flag to stop the loop and end early.
    }

private:
    // Cache implementation
    void  update_extents( size_type const *eb, size_type const *ee )
    {
        // Zero extents are illegal.
        if ( std::find(eb, ee, size_type( 0 )) != ee )
            throw std::out_of_range{ "Zero-sized extent" };

        // Check if the extents' combined product is too big.
        size_type  limit = 1u;
        auto const   max = std::numeric_limits<size_type>::max();

        for ( auto  i = eb ; i != ee ; ++i )
        {
            if ( *i > max / limit )
                throw std::overflow_error{ "Total element count too large" };
            else
                limit *= *i;
        }

        // No more problems, do the transfer.
        std::copy( eb, ee, std::begin(stats[ extents_i ]) );
    }
    void  update_priorities( size_type const *pb, size_type const *pe )
    {
        // Input requires: Range: 0 <= x < Rank
        if ( std::find_if(pb, pe, []( size_type x ){ return x >= dimensionality;
         }) != pe )
            throw std::out_of_range{ "Illegal priority value" };

        // Input requires: Scramble of 0 .. (Rank - 1), no missing or repeats.
        stats_type  row_major_order;

        std::iota(row_major_order.begin(), row_major_order.end(), size_type(0));
        if ( !std::is_permutation(pb, pe, row_major_order.begin()) )
            throw std::invalid_argument{ "Improper priority list" };

        // No more problems, do the transfer.
        std::copy( pb, pe, std::begin(stats[ priorities_i ]) );
    }
    void  recalculate_strides()
    {
        auto  product = std::accumulate( std::begin(stats[ extents_i ]),
         std::end(stats[ extents_i ]), size_type(1),
         std::multiplies<size_type>{} );

        for ( size_type  i = 0u ; i < dimensionality ; ++i )
        {
            auto const  which_index = stats[ priorities_i ][ i ];

            stats[ strides_i ][ which_index ] = ( product /=
             stats[extents_i][which_index] );
        }
    }

    // There are two direct pieces of data, the extent for each index and the
    // priority for each index (most to least major).  The third piece of data,
    // the stride for each index, is synthesized from the other two.  All of
    // them have to be the same size, so it's easier to use a giant array.
    size_type  stats[ 3 ][ dimensionality + !dimensionality ];

    enum { extents_i, priorities_i, strides_i };
};

//! The length of extent and priority packs is based on this value.
template < typename SizeType, SizeType Rank >
constexpr
typename multiarray_indexed_base<SizeType, Rank>::size_type
multiarray_indexed_base<SizeType, Rank>::dimensionality;

}  // namespace detail


//  Multi-dimensional array adapter class template definition  ---------------//

/** \brief  A container adapter to view a multi-dimensional array.

Like the Standard container adapter class templates (`queue`, `priority_queue`,
and `stack`), this class template uses an established container type to store
its elements instead of handling them directly.  With element management out of
the way, the adapter can deal with its specialized interface.  For this class
template, the special interface is a multi-dimensional container; one that uses
multiple index values to refer to a particular element.

Unlike the Standard adapters, with special interfaces for element access and
insert & removal, this class template only provides a special interface for
access.  The internal container must have the right number of elements as it is
introduced through the constructor, or this class template must be sub-classed
so the derived class has provisions for sizing the internal container
appropriately.

    \pre  `Element` can be used as a container element type.
    \pre  `Element` is the element type for `Container`.
    \pre  `Container` should be a sequence-container that supports random-access
          iterators.  (But it can be any Standard-esque container that supports
          at least forward iterators.)  It has to have at least the `begin`,
          `size`, `empty`, and `swap` member functions (plus needed support
          types and type-aliases), with their expected Standard semantics.

    \tparam Element    The type of the elements.
    \tparam Rank       The number of index coordinates to access an element.
                       May be zero.
    \tparam Container  The internal container for the elements.  If not given,
                       defaults to `std::vector<Element>`.

 */
template <
    typename Element, std::size_t Rank, class Container = std::vector<Element>
>
class multiarray
    : private detail::multiarray_storage_base<Element, Container>
    , private detail::multiarray_indexed_base<typename Container::size_type,
      Rank>
{
    // Base types
    using sbase_type = detail::multiarray_storage_base<Element, Container>;
    using ibase_type = detail::multiarray_indexed_base<typename
     Container::size_type, Rank>;

public:
    // Template parameters
    using sbase_type::value_type;
    using ibase_type::dimensionality;
    //! The container type (Container).  Gives access to its template parameter.
    typedef typename sbase_type::container_type  container_type;

    // Other types
    using typename ibase_type::stats_type;
    using typename sbase_type::reference;
    using typename sbase_type::const_reference;
    //! The type for size-based meta-data (`Container::size_type`).
    typedef typename ibase_type::size_type  size_type;

    // Lifetime management
    // (Use automatically-defined copy-ctr, move-ctr, and destructor)
    /** \brief  Default constructor
        \post  #c is default-initialized.
        \post  `extents() == {{ cc.empty() ? 1 : cc.size(), 1, ..., 1 }}`.
        \post  `priorities() == {{ 0, ..., (dimensionality - 1) }}`.
     */
              multiarray()  : sbase_type(), ibase_type()  { resize_to_fit(); }
    /** \brief  Initialize with a copy of the given container
        \param cc  The container to copy from.
        \post  #c is equivalent to `cc`.
        \post  `extents() == {{ c.empty() ? 1 : c.size(), 1, ..., 1 }}`.
        \post  `priorities() == {{ 0, ..., (dimensionality - 1) }}`.
     */
    explicit  multiarray( container_type const &cc )
      : sbase_type( cc ), ibase_type()
    { resize_to_fit(); }
    /** \brief  Initialize from moving in the given container
        \param cc  The container to move from.
        \post  #c is equivalent to the pre-move state of `cc`.  The state of
               `cc` is unspecified.
        \post  `extents() == {{ cc.empty() ? 1 : cc.size(), 1, ..., 1 }}`.
        \post  `priorities() == {{ 0, ..., (dimensionality - 1) }}`.
     */
    explicit  multiarray( container_type &&cc )
      : sbase_type( std::move(cc) ), ibase_type()
    { resize_to_fit(); }

    // Status
    using ibase_type::required_size;
    using sbase_type::empty;
    using sbase_type::size;

    using ibase_type::extents;
    using ibase_type::priorities;
    using ibase_type::extents_and_priorities;

    using ibase_type::use_row_major_order;
    using ibase_type::use_column_major_order;

    // Access
    using sbase_type::operator ();
    using sbase_type::at;
    using sbase_type::operator [];

    // Assignments
    /** \brief    Fill elements with specified value.
        \details  Assigns the given value to all the elements.  If the number of
                  stored elements differs from the amount needed, iteration will
                  stop at the shorter length.
        \pre      #value_type has to be Assignable.
        \param v  The value of the assignment source.
        \throws Whatever  assignment for #value_type throws.
        \post     Each element is equivalent to *v*.
     */
    void  fill( const_reference v )
    { std::fill_n(std::begin( c ), std::min( required_size(), size() ), v); }

    /** \brief  Swaps states with another object.

    The swapping should use the element- or container-types' `swap`, found in
    the `std` namespace or via ADL.

        \param other  The object to trade state with.

        \throws  Whatever  the element-, `size_type`-, or container-level swap
                           throws.
        \post  `*this` is equivalent to the old state of *other*, while that
               object is equivalent to the old state of `*this`.
     */
    void  swap( multiarray &other )
     noexcept( detail::is_swap_nothrow_too<container_type>() &&
     detail::is_swap_nothrow_too<size_type>() )
    { sbase_type::swap(other); ibase_type::swap(other); }

    /** \brief    Calls function on all elements, with indices.
        \details  Loops through all the extant elements, calling the given
                  function.  Each call includes the index coordinates to the
                  current element as trailing arguments.  Each element will be
                  passed with the same mutability state as its container.  The
                  iteration order should follow in-memory layout (to reduce
                  cache faults), but the called function should be path-
                  conservative and not count on visitation order.
        \param f  The function, function-pointer, function-object, or lambda
                  that will execute the code.  It has to take #dimensionality +
                  1 arguments.  The first argument must be compatible with
                  #value_type (or (immutable) reference of); subsequent
                  arguments have to be compatible with #size_type.
        \post     Unspecified, since *f* is allowed to alter the elements (when
                  taking a mutable reference) and/or itself during the calls.
     */
    template < typename Function >
    void  apply( Function &&f )
    {
        // This is the same as the "const" version, but I can't reuse it using
        // "const_cast" games since the inner calls would still be "const."

        auto  limit = std::min( required_size(), size() );
        auto  current = std::begin( c );
        auto  indexes = this->first_index_pack();

        while ( limit-- )
        {
            detail::apply_x_and_exploded_tuple( f, *current++, indexes );
            this->advance_index_pack( indexes );
        }
    }
    //! \overload
    template < typename Function >
    void  apply( Function &&f ) const
    {
        auto  limit = std::min( required_size(), size() );
        auto  current = std::begin( c );
        auto  indexes = this->first_index_pack();

        while ( limit-- )
        {
            detail::apply_x_and_exploded_tuple( f, *current++, indexes );
            this->advance_index_pack( indexes );
        }
    }
    /** \brief    Calls function on all elements, with indices, immutable access
        \details  Provides a way for a mutable-mode object to get immutable-mode
                  element access (via looping) without `const_cast` convolutions
                  with #apply.
        \param f  The function, function-pointer, function-object, or lambda
                  that will execute the code.  It has to take #dimensionality +
                  1 arguments.  The first argument must be compatible with
                  #value_type (or `const` reference of); subsequent arguments
                  have to be compatible with #size_type.
        \post     Unspecified, since *f* may alter itself during the calls.
     */
    template < typename Function >
    void  capply( Function &&f ) const
    { apply(std::forward<Function>( f )); }

protected:
    using sbase_type::c;

private:
    // Set the virtual-array size to match the container's size.
    void  resize_to_fit()
    {
        size_type const  new_size = dimensionality && !c.empty() ? c.size() : 1;
        std::array<size_type, dimensionality>  e;

        std::fill( e.begin(), e.end(), size_type(1) );
        if ( !e.empty() )
            e[ 0 ] = new_size;
        extents( e );
    }

    // Override to connect the two bases together.
    size_type  get_offset( size_type const *index_begin, size_type const
     *index_end, bool throw_on_bad_input ) const final override
    {
        if ( throw_on_bad_input )
            ibase_type::throw_for_bad_indexes( index_begin, index_end );

        return ibase_type::indexes_to_offset( index_begin, index_end );
    }
};

 /** \brief  Swap routine for `multiarray`.

Exchanges the state of two `multiarray` objects.  The objects have to have the
same element type, number of dimensions, and container type.

    \pre  There is a swapping routine, called `swap`, either in namespace `std`
          for built-ins, or found via ADL for other types.

    \param a  The first object to have its state exchanged.
    \param b  The second object to have its state exchanged.

    \see  #multiarray<Element,Rank,Container>::swap(multiarray&)

    \throws Whatever  the element-, index-, and the container-level swaps do.

    \post  `a` is equivalent to the old state of `b`, while `b` is equivalent to
           the old state of `a`.
 */
template < typename T, std::size_t Rank, class Cont >
void  swap( multiarray<T, Rank, Cont> &a, multiarray<T, Rank, Cont> &b )
 noexcept( noexcept(a.swap( b )) )
{ a.swap(b); }

}  // namespace container
}  // namespace boost


//  Specializations from the STD namespace  ----------------------------------//

#if 0
namespace std
{
    //! A `multiarray` instantiation can use the C++ (2011) Standard allocator
    //! interface if the internal container type can.
    template < typename T, size_t Rank, class Container, class Alloc >
    struct uses_allocator<boost::container::multiarray<T,Rank,Container>, Alloc>
        : uses_allocator< Container, Alloc >::type
    { };

}  // namespace std
#endif


#endif // BOOST_CONTAINER_MULTIARRAY_HPP
