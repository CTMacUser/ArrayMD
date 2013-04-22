//  Boost Type-Traits array-indexing result header file  ---------------------//

//  Copyright 2013 Daryle Walker.
//  Distributed under the Boost Software License, Version 1.0.  (See the
//  accompanying file LICENSE_1_0.txt or a copy at
//  <http://www.boost.org/LICENSE_1_0.txt>.)

//  See <http://www.boost.org/libs/type_traits/> for the library's home page.

/** \file
    \brief  Class templates providing information about the results of
      array-indexing, including chains of array-indexing calls.

    \author  Daryle Walker

    \version  0.1

    \copyright  Boost Software License, version 1.0

    Contains the declarations (and definitions) of class templates providing the
    results of calling a chain of array-indexing operations (i.e. `operator []`)
    together.  The information can the result type and/or whether exceptions
    could be thrown.

    \warning  This library requires C++2011 features.

	\todo  Add unit tests.
 */

#ifndef BOOST_TYPE_TRAITS_INDEXING_HPP
#define BOOST_TYPE_TRAITS_INDEXING_HPP

#include <type_traits>
#include <utility>


namespace boost
{


//  Forward declarations  ----------------------------------------------------//

/** \brief  Type-traits for handling chains of array-indexing calls.

	\details  Given a chain of `operator []` calls, find the result type and
	          whether that expression can throw.

	\tparam Base     The type of the first (and non-bracketed) object/value.
	\tparam Indices  The types of the various indexing objects/values, each one
	                 bracket-enclosed.  May be empty.
 */
template < typename Base, typename ...Indices >
class indexing_result;


//  Array-indexing traits class template specialization declarations  --------//

/** \brief  Base-case specialization of `indexing_result`.

	\details  The base case handles when there are no indices.

	\tparam T  The type of the base object.
 */
template < typename T >
class indexing_result<T>
{
public:
	//! With no indices, the base type is the result type.
	typedef T  type;

	//! Indicates if the indexing operations can raise exceptions.
	static constexpr  bool  can_throw = false;
};

/** \brief  Recursive-case specialization of `indexing_result`.

	\details  The recursive case reduces its list from the beginning by
	          combining the base object and first index with `operator []`.  So
			  the results are copied from the next step.

	\tparam T  The type of the base object.
	\tparam U  The type of the first index.
	\tparam V  The remaining indices.  May be empty.
 */
template < typename T, typename U, typename ...V >
class indexing_result<T, U, V...>
{
	// Compact the base and first index types into the new base type.
	typedef decltype( std::declval<T>()[std::declval<U>()] )  direct_type;
	typedef indexing_result< direct_type, V... >                next_type;

	// Check if the first indexing call can throw.
	static constexpr
	bool  direct_throw = not noexcept( std::declval<T>()[std::declval<U>()] );

public:
	//! The result type comes from the end of the chain.
	typedef typename next_type::type  type;

	//! Indicates if any of the indexing operations can raise exceptions.
	static constexpr  bool  can_throw = direct_throw || next_type::can_throw;
};


//  Class-static data member definitions  ------------------------------------//

/** Since no indexing actually happens, there can't be any exceptions.
 */
template < typename T >
constexpr
bool  indexing_result<T>::can_throw;

// Private member
template < typename T, typename U, typename ...V >
constexpr
bool  indexing_result<T, U, V...>::direct_throw;

/** Exceptions from indexing can come from the current indexing operations, or
    from later ones.  The evaluation uses Boolean short-circuiting.
 */
template < typename T, typename U, typename ...V >
constexpr
bool  indexing_result<T, U, V...>::can_throw;


//  Auxillary array-indexing traits class template definition  ---------------//

/** \brief  Throwing status for array-indexing chains.

	\details  Given a chain of `operator []` calls, find if that expression can
	          throw.  This result is already given from `indexing_result`, but
			  this class template puts the result in a format compatible with
			  other Boolean type traits.

	\tparam Base     The type of the first (and non-bracketed) object/value.
	\tparam Indices  The types of the various indexing objects/values, each one
	                 bracket-enclosed.  May be empty.
 */
template < typename Base, typename ...Indices >
struct indexing_noexcept
	: std::integral_constant<
	    bool,
		not indexing_result<Base, Indices...>::can_throw
	  >
{ };


}  // namespace boost


#endif // BOOST_TYPE_TRAITS_INDEXING_HPP
