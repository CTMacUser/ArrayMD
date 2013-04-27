//  Boost Utility array-indexing function header file  -----------------------//

//  Copyright 2013 Daryle Walker.
//  Distributed under the Boost Software License, Version 1.0.  (See the
//  accompanying file LICENSE_1_0.txt or a copy at
//  <http://www.boost.org/LICENSE_1_0.txt>.)

//  See <http://www.boost.org/libs/utility/> for the library's home page.

/** \file
    \brief  Function templates for array indexing.

    \author  Daryle Walker

    \version  0.1

    \copyright  Boost Software License, version 1.0

    Contains the declarations (and definitions) of function templates performing
    a chain of array-indexing calls (i.e. `operator []`) when a bunch of
    expressions are given (and an indexing chain is legal).

    \warning  This library requires C++2011 features.

	\todo  Add unit tests.
 */

#ifndef BOOST_UTILITY_SLICE_HPP
#define BOOST_UTILITY_SLICE_HPP

#include <cstddef>
#include <type_traits>

#include "boost/type_traits/indexing.hpp"


namespace boost
{


//  Array-chain-indexing function template definitions  ----------------------//

/** \brief  Apply `operator []` serially for a list of expressions, base case.

	\details  With no index expressions, return the base object.

	\param t  The base (and sole) object/value.

	\throws  Nothing.

	\returns  A reference to *t*.  Should retain l- vs. r-value-ness.
 */
template < typename T >
inline constexpr
auto  slice( T &&t ) noexcept -> T &&
{ return static_cast<T &&>(t); }

/** \brief  Apply `operator []` serially for a list of expressions.

	\details  Takes the first two arguments and determines their index result
	          (with the first expression as the base object, and the second as
			  the index).  If there's more arguments, keep going with the new
			  result as the base object and the subsequent argument as the
			  index, and so on.

	\param t  The base object/value.
	\param u  The first index object/value.
	\param v  Subsequent indexing objects/values.  May be empty.

	\throws  Whatever  each individual indexing operation may throw.

	\returns  The result from the last indexing operation.
 */
template < typename T, typename U, typename ...V >
inline constexpr
auto  slice( T &&t, U &&u, V &&...v )
  noexcept( indexing_noexcept<T, U, V...>::value )
  -> typename indexing_result<T, U, V...>::type
{
	return slice( static_cast<T &&>(t)[static_cast<U &&>( u )],
	 static_cast<V &&>(v)... );
}


//  Checked array-chain-indexing function template definitions  --------------//

/** \brief  Apply `operator []` serially for a list of expressions, with bounds
            checking, base case.

	\details  With no index expressions, return the base object (second
	          parameter).  The first parameter, a potential exception object,
			  is not used.

	\param t  The base (and sole) object/value.

	\throws  Nothing.

	\returns  A reference to *t*.  Should retain l- vs. r-value-ness.
 */
template < typename E, typename T >
inline constexpr
auto  checked_slice( E &&, T &&t ) noexcept -> T &&
{ return static_cast<T &&>(t); }

/** \brief  Apply `operator []` serially for a list of expressions, with bounds
            checking.

	\details  Takes the 2nd and 3rd arguments and determines their index result
	          (with the second expression as the base object, and the third as
			  the index).  If there are following arguments, keep going with the
			  new result as the base object and the subsequent argument as the
			  index, and so on.

			  Each index is compared to its corresponding bound, and throws a
			  user-provided exception object (the first argument) upon boundary
			  violations.  Each index has to be comparable and/or (implicitly)
			  convertible to a built-in numeric type.

			  Note that *t* has to be either (an l- or r-value reference to) a
			  built-in array type, or (from an overload) a class type with the
			  indexing operator and a usuable `size_type` type-alias.  Pointer
			  types and non-standard container types will fail.

	\param e  The exception object thrown if an index violates the array bound.
	\param t  The base object/value.
	\param u  The first index object/value.
	\param v  Subsequent indexing objects/values.  May be empty.

	\throws  *e*  if bounds-checking reports a violation
	\throws  Whatever  exceptions may occur during the actual bound-checking
	         (comparisons with and/or conversions to a built-in integer or
			 enumeration type).

	\returns  The result from the last indexing operation.
 */
template < typename E, typename T, std::size_t N, typename U, typename ...V >
inline constexpr
auto  checked_slice( E &&e, T (&t)[N], U &&u, V &&...v )
 -> typename indexing_result<T(&)[N], U, V...>::type
{
	// Smooth over comparisons
	typedef typename std::remove_reference<U>::type                 u_type;
	typedef typename std::common_type<u_type, std::size_t>::type  cmp_type;

// Compact a long expression needed twice
#define BOOST_PRIVATE_CS  checked_slice( static_cast<E &&>(e),\
 t[static_cast<U &&>( u )], static_cast<V &&>(v)... )

	// The ", BOOST_PRIVATE_CS" part changes the type of the second part of the
	// conditional to be the same as the third.  Without it, the second part
	// (i.e. the throw) would have type "void," and trigger array-to-pointer
	// conversion on the third part (which is bad).  It shouldn't get evaluated.
	// See sec. 5.16 para. 2 of the C++ 2011 standard for the dirty.
	return ( u < u_type{} ) || ( static_cast<cmp_type>(u) >=
	 static_cast<cmp_type>(N) ) ? throw e, BOOST_PRIVATE_CS : BOOST_PRIVATE_CS;

// Undo my "shameful" use of the preprocessor
#undef BOOST_PRIVATE_CS
}

//! \overload
template < typename E, typename T, std::size_t N, typename U, typename ...V >
inline constexpr
auto  checked_slice( E &&e, T (&&t)[N], U &&u, V &&...v )
 -> typename indexing_result<T(&&)[N], U, V...>::type
{
	typedef typename std::remove_reference<U>::type                 u_type;
	typedef typename std::common_type<u_type, std::size_t>::type  cmp_type;

#define BOOST_PRIVATE_CS  checked_slice( static_cast<E &&>(e),\
 static_cast<T (&&)[N]>(t)[static_cast<U &&>( u )], static_cast<V &&>(v)... )

	// See the note in the previous function.
	return ( u < u_type{} ) || ( static_cast<cmp_type>(u) >=
	 static_cast<cmp_type>(N) ) ? throw e, BOOST_PRIVATE_CS : BOOST_PRIVATE_CS;

#undef BOOST_PRIVATE_CS
}

//! \overload
template < typename E, typename T, typename U, typename ...V >
inline constexpr
auto  checked_slice( E &&e, T &&t, U &&u, V &&...v )
 -> typename indexing_result<T, U, V...>::type
{
	typedef typename std::remove_reference<T>::type  t_type;
	typedef typename std::remove_reference<U>::type  u_type;
	typedef typename std::common_type<u_type, typename t_type::size_type>::type
	  cmp_type;

#define BOOST_PRIVATE_CS  checked_slice( static_cast<E &&>(e),\
 static_cast<T &&>(t)[static_cast<U &&>( u )], static_cast<V &&>(v)... )

	// See the note in the previous function.
	return ( u < u_type{} ) || ( static_cast<cmp_type>(u) >=
	 static_cast<cmp_type>(t.size()) ) ? throw e, BOOST_PRIVATE_CS :
	 BOOST_PRIVATE_CS;

#undef BOOST_PRIVATE_CS
}


}  // namespace boost


#endif // BOOST_UTILITY_SLICE_HPP
