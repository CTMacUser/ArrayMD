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

	\throws  Whatever each individual indexing operation may throw.

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


}  // namespace boost


#endif // BOOST_UTILITY_SLICE_HPP
