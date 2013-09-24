//  Boost Multi-dimensional Array unit test program file  --------------------//

//  Copyright 2013 Daryle Walker.
//  Distributed under the Boost Software License, Version 1.0.  (See the
//  accompanying file LICENSE_1_0.txt or a copy at
//  <http://www.boost.org/LICENSE_1_0.txt>.)

//  See <http://www.boost.org/libs/container/> for the library's home page.

#define BOOST_TEST_MAIN  "Multi-Dimensional Array Unit Tests"
#include <boost/test/unit_test.hpp>
#include <boost/mpl/list.hpp>

#include "boost/container/array_md.hpp"

#include <algorithm>
#include <cctype>
#include <complex>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <map>
#include <stdexcept>
#include <string>
#include <type_traits>


// Common definitions  -------------------------------------------------------//

namespace {

// Sample testing types for elements
typedef boost::mpl::list<int, long, unsigned char>  test_types;

// Mutate element and report on index count
struct counting_negator
{
    std::size_t  last_argument_count;

    template < typename T, typename ...Args >
    void  operator ()( T &t, Args &&...a )
    {
        t = -t;
        last_argument_count = sizeof...( a );
    }
};

// Reverse a character's case (upper vs. lower)
void  reverse_case( char &c )
{ c = std::islower(c) ? std::toupper(c) : std::tolower(c); }

// Do it for a string
void  reverse_case( char *s )
{
    if ( !s )
        return;
    while ( *s )
        reverse_case( *s++ );
}

}

// Flag un-printable types
BOOST_TEST_DONT_PRINT_LOG_VALUE( std::reverse_iterator<int *> );
BOOST_TEST_DONT_PRINT_LOG_VALUE( std::reverse_iterator<int const *> );
BOOST_TEST_DONT_PRINT_LOG_VALUE( std::reverse_iterator<long *> );
BOOST_TEST_DONT_PRINT_LOG_VALUE( std::reverse_iterator<long const *> );
BOOST_TEST_DONT_PRINT_LOG_VALUE( std::reverse_iterator<unsigned char *> );
BOOST_TEST_DONT_PRINT_LOG_VALUE( std::reverse_iterator<unsigned char const *> );


// Unit tests for basic functionality  ---------------------------------------//

BOOST_AUTO_TEST_SUITE( test_array_md_basics )

BOOST_AUTO_TEST_CASE_TEMPLATE( test_singular_element_static, T, test_types )
{
    using std::is_same;
    using std::reverse_iterator;

    typedef boost::container::array_md<T>  sample_type;

    BOOST_REQUIRE( (is_same<T, typename sample_type::value_type>::value) );
    BOOST_REQUIRE( (is_same<T, typename sample_type::data_type>::value) );
    BOOST_REQUIRE( (is_same<std::size_t, typename
     sample_type::size_type>::value) );
    BOOST_REQUIRE( (is_same<std::ptrdiff_t, typename
     sample_type::difference_type>::value) );

    BOOST_REQUIRE_EQUAL( sample_type::dimensionality, 0u );
    BOOST_REQUIRE_EQUAL( sample_type::static_size, 1u );

    BOOST_REQUIRE( (is_same<T *, typename sample_type::pointer>::value) );
    BOOST_REQUIRE( (is_same<T const *, typename
     sample_type::const_pointer>::value) );
    BOOST_REQUIRE( (is_same<T &, typename sample_type::reference>::value) );
    BOOST_REQUIRE( (is_same<T const &, typename
     sample_type::const_reference>::value) );
    BOOST_REQUIRE( (is_same<T *, typename sample_type::iterator>::value) );
    BOOST_REQUIRE( (is_same<T const *, typename
     sample_type::const_iterator>::value) );
    BOOST_REQUIRE( (std::is_convertible<typename sample_type::iterator, typename
     sample_type::const_iterator>::value) );
    BOOST_REQUIRE( (is_same<reverse_iterator<T *>, typename
     sample_type::reverse_iterator>::value) );
    BOOST_REQUIRE( (is_same<reverse_iterator<T const *>, typename
     sample_type::const_reverse_iterator>::value) );

    BOOST_REQUIRE( sizeof( sample_type ) >= sizeof( T ) );
    BOOST_REQUIRE_EQUAL( sizeof(typename sample_type::data_type), sizeof(T) );
}

BOOST_AUTO_TEST_CASE_TEMPLATE( test_singular_element_dynamic, T, test_types )
{
    using boost::container::array_md;
    using std::length_error;

    typedef array_md<T>  sample_type;

    // Check basic access
    sample_type          t1{ 5 };
    sample_type const &  t2 = t1;

    BOOST_CHECK_EQUAL( t1(), (T)5 );
    BOOST_CHECK_EQUAL( (T)5, t2() );

    t1() = static_cast<T>( 10 );
    BOOST_CHECK_EQUAL( t2(), (T)10 );

    // Check direct access
    t1.data_block = static_cast<T>( 6 );
    BOOST_CHECK_EQUAL( t2.data_block, (T)6 );

    // Check raw access
    BOOST_CHECK_EQUAL( t1.data(), &t1.data_block );
    BOOST_CHECK_EQUAL( (T)6, *t1.data() );
    *t1.data() = static_cast<T>( 19 );
    BOOST_CHECK_EQUAL( static_cast<T>(19), *t2.data() );
    BOOST_CHECK_EQUAL( &t2.data_block, t2.data() );

    BOOST_CHECK_EQUAL( t1.size(), 1u );
    BOOST_CHECK_EQUAL( 1u, t2.size() );

    BOOST_CHECK_EQUAL( t1.max_size(), 1u );
    BOOST_CHECK_EQUAL( 1u, t2.max_size() );
    BOOST_CHECK( not t1.empty() );
    BOOST_CHECK( not t2.empty() );

    // Check "at"
    t1.at() = static_cast<T>( 100 );
    BOOST_CHECK_EQUAL( t2.at(), (T)100 );
    BOOST_CHECK_NO_THROW( t1.at() );
    BOOST_CHECK_NO_THROW( t2.at() );

    // Check initialization-list indices
    t1( {} ) = static_cast<T>( 39 );
    BOOST_CHECK_EQUAL( t2({}), (T)39 );
    t1.at( {} ) = static_cast<T>( 43 );
    BOOST_CHECK_EQUAL( t2.at({}), (T)43 );
    BOOST_CHECK_NO_THROW( t1.at({}) );
    BOOST_CHECK_NO_THROW( t2.at({}) );
    BOOST_CHECK_THROW( t1.at({ 1 }), length_error );
    BOOST_CHECK_THROW( t2.at({ 0, 3 }), length_error );

    // Check with array-type as element type
    typedef array_md<T[2]>  sample2_type;

    sample2_type          t3{ {7, 0} };
    sample2_type const &  t4 = t3;

    BOOST_CHECK_EQUAL( t3()[0], (T)7 );
    BOOST_CHECK_EQUAL( t4()[1], (T)0 );

    t3()[ 1 ] = static_cast<T>( 15 );
    t3.data_block[ 0 ] = T{};
    BOOST_CHECK_EQUAL( T(), t4()[0] );
    BOOST_CHECK_EQUAL( (T)15, t4.data_block[1] );

    BOOST_CHECK_EQUAL( static_cast<T>(15), (*t3.data())[1] );
    ( *t3.data() )[ 1 ] = static_cast<T>( 101 );
    BOOST_CHECK_EQUAL( (*t4.data())[1], (T)101 );

    BOOST_CHECK_EQUAL( t3.size(), 1u );
    BOOST_CHECK_EQUAL( 1u, t4.size() );

    BOOST_CHECK_EQUAL( t3.max_size(), 1u );
    BOOST_CHECK_EQUAL( 1u, t4.max_size() );
    BOOST_CHECK( not t3.empty() );
    BOOST_CHECK( not t4.empty() );

    t3.at()[ 0 ] = !T{};
    BOOST_CHECK_EQUAL( (T)1, t4.at()[0] );
    BOOST_CHECK_NO_THROW( t3.at()[1] );
    BOOST_CHECK_NO_THROW( t4.at()[1] );

    t3( {} )[ 1 ] = T{};
    BOOST_CHECK_EQUAL( T(0), t4({})[1] );
    t3.at( {} )[ 0 ] = T(71);
    BOOST_CHECK_EQUAL( (T)71, t4.at({})[0] );
    BOOST_CHECK_NO_THROW( t3.at({}) );
    BOOST_CHECK_NO_THROW( t4.at({}) );
    BOOST_CHECK_THROW( t3.at({ 1, 0 }), length_error );
    BOOST_CHECK_THROW( t4.at({ 7 }), length_error );
}

BOOST_AUTO_TEST_CASE_TEMPLATE( test_compound_static, T, test_types )
{
    using boost::container::array_md;
    using std::is_same;
    using std::size_t;
    using std::ptrdiff_t;
    using std::is_convertible;
    using std::reverse_iterator;

    typedef array_md<T, 7, 3>  sample_type;

    BOOST_REQUIRE( (is_same<T, typename sample_type::value_type>::value) );
    BOOST_REQUIRE( !(is_same<T, typename sample_type::data_type>::value) );
    BOOST_REQUIRE( (is_same<T[3], typename
     sample_type::direct_element_type>::value) );
    BOOST_REQUIRE( (is_same<T[7][3], typename sample_type::data_type>::value) );
    BOOST_REQUIRE( (is_same<size_t, typename sample_type::size_type>::value) );
    BOOST_REQUIRE( (is_same<ptrdiff_t, typename
     sample_type::difference_type>::value) );

    BOOST_REQUIRE_EQUAL( sample_type::dimensionality, 2u );
    BOOST_REQUIRE_EQUAL( sample_type::static_sizes[0], 7u );
    BOOST_REQUIRE_EQUAL( sample_type::static_sizes[1], 3u );
    BOOST_REQUIRE_EQUAL( sample_type::static_size, 21u );

    BOOST_REQUIRE( (is_same<T *, typename sample_type::pointer>::value) );
    BOOST_REQUIRE( (is_same<T const *, typename
     sample_type::const_pointer>::value) );
    BOOST_REQUIRE( (is_same<T &, typename sample_type::reference>::value) );
    BOOST_REQUIRE( (is_same<T const &, typename
     sample_type::const_reference>::value) );
    BOOST_REQUIRE( (is_same<T *, typename sample_type::iterator>::value) );
    BOOST_REQUIRE( (is_same<T const *, typename
     sample_type::const_iterator>::value) );
    BOOST_REQUIRE( (is_convertible<typename sample_type::iterator, typename
     sample_type::const_iterator>::value) );
    BOOST_REQUIRE( (is_same<reverse_iterator<T *>, typename
     sample_type::reverse_iterator>::value) );
    BOOST_REQUIRE( (is_same<reverse_iterator<T const *>, typename
     sample_type::const_reverse_iterator>::value) );

    BOOST_REQUIRE( sizeof(sample_type) >= sample_type::static_size *
     sizeof(T) );
    BOOST_REQUIRE_EQUAL( sizeof(typename sample_type::data_type),
     sizeof(T[ 7 ][ 3 ]) );

    typedef array_md<T, 5>  sample2_type;

    BOOST_REQUIRE( (is_same<T, typename sample2_type::value_type>::value) );
    BOOST_REQUIRE( !(is_same<T, typename sample2_type::data_type>::value) );
    BOOST_REQUIRE( (is_same<T, typename
     sample2_type::direct_element_type>::value) );
    BOOST_REQUIRE( (is_same<T[5], typename sample2_type::data_type>::value) );
    BOOST_REQUIRE( (is_same<size_t, typename sample2_type::size_type>::value) );
    BOOST_REQUIRE( (is_same<ptrdiff_t, typename
     sample2_type::difference_type>::value) );

    BOOST_REQUIRE_EQUAL( sample2_type::dimensionality, 1u );
    BOOST_REQUIRE_EQUAL( sample2_type::static_sizes[0], 5u );
    BOOST_REQUIRE_EQUAL( sample2_type::static_size, 5u );

    BOOST_REQUIRE( (is_same<T *, typename sample2_type::pointer>::value) );
    BOOST_REQUIRE( (is_same<T const *, typename
     sample2_type::const_pointer>::value) );
    BOOST_REQUIRE( (is_same<T *, typename sample2_type::iterator>::value) );
    BOOST_REQUIRE( (is_same<T const *, typename
     sample2_type::const_iterator>::value) );
    BOOST_REQUIRE( (is_convertible<typename sample_type::iterator, typename
     sample2_type::const_iterator>::value) );
    BOOST_REQUIRE( (is_same<reverse_iterator<T *>, typename
     sample2_type::reverse_iterator>::value) );
    BOOST_REQUIRE( (is_same<reverse_iterator<T const *>, typename
     sample2_type::const_reverse_iterator>::value) );

    BOOST_REQUIRE( sizeof(sample2_type) >= sample2_type::static_size *
     sizeof(T) );
    BOOST_REQUIRE_EQUAL( sizeof(typename sample2_type::data_type),
     sizeof(T[ 5 ]) );
}

BOOST_AUTO_TEST_CASE( test_compound_dynamic )
{
    using boost::container::array_md;
    using std::strcmp;
    using std::out_of_range;
    using std::length_error;

    // Bracket-based indexing
    typedef array_md<int, 2>  sample_type;

    sample_type          t1{ {10, 11} };
    sample_type const &  t2 = t1;

    BOOST_CHECK_EQUAL( t1[0], 10 );
    BOOST_CHECK_EQUAL( 11, t1[1] );

    t1[ 0 ] = 4;
    t1[ 1 ]++;
    BOOST_CHECK_EQUAL( 4, t2[0] );
    BOOST_CHECK_EQUAL( t2[1], 12 );

    // now with an array-based element type
    typedef array_md<char[6], 2, 2>  sample2_type;

    sample2_type          t3{ {{"Hello", "World"}, {"Video", "Watch"}} };
    sample2_type const &  t4 = t3;

    BOOST_CHECK( strcmp(t3[ 0 ][ 1 ], "World") == 0 );
    BOOST_CHECK( strcmp("Video", t4[ 1 ][ 0 ]) == 0 );
    BOOST_CHECK_EQUAL( t3[0][0][1], 'e' );
    BOOST_CHECK_EQUAL( t4[1][1][2], 't' );

    t3[ 1 ][ 1 ][ 0 ] = 'B';
    BOOST_CHECK( 0 == strcmp(t4[ 1 ][ 1 ], "Batch") );

    // Check direct access
    BOOST_CHECK_EQUAL( t1.data_block[0], 4 );
    BOOST_CHECK_EQUAL( 12, t1.data_block[1] );
    t1.data_block[ 0 ] = 2;
    t1.data_block[ 1 ]++;
    BOOST_CHECK_EQUAL( 2, t2.data_block[0] );
    BOOST_CHECK_EQUAL( t2.data_block[1], 13 );

    BOOST_CHECK( strcmp(t3.data_block[ 0 ][ 0 ], "Hello") == 0 );
    t3.data_block[ 0 ][ 0 ][ 0 ] = 'M';
    BOOST_CHECK( strcmp(t4.data_block[ 0 ][ 0 ], "Mello") == 0 );

    // Check raw access
    BOOST_CHECK_EQUAL( *t1.data(), 2 );
    BOOST_CHECK_EQUAL( 13, *(t1.data() + 1) );
    *t1.data() = -23;
    ( *(t1.data() + 1) )++;
    BOOST_CHECK_EQUAL( -23, *t2.data() );
    BOOST_CHECK_EQUAL( *(t2.data() + 1), 14 );

    BOOST_CHECK( strcmp(*t3.data(), "Mello") == 0 );
    ( *t3.data() )[ 3 ] = 'k';
    BOOST_CHECK( strcmp(*t4.data(), "Melko") == 0 );

    BOOST_CHECK_EQUAL( 2u, t1.size() );
    BOOST_CHECK_EQUAL( t2.size(), 2u );
    BOOST_CHECK_EQUAL( 4u, t3.size() );
    BOOST_CHECK_EQUAL( t4.size(), 4u );

    BOOST_CHECK_EQUAL( 2u, t1.max_size() );
    BOOST_CHECK_EQUAL( t2.max_size(), 2u );
    BOOST_CHECK_EQUAL( 4u, t3.max_size() );
    BOOST_CHECK_EQUAL( t4.max_size(), 4u );
    BOOST_CHECK( not t1.empty() );
    BOOST_CHECK( not t2.empty() );
    BOOST_CHECK( not t3.empty() );
    BOOST_CHECK( not t4.empty() );

    // Try out the "operator ()" interface
    BOOST_CHECK_EQUAL( t1()[0], -23 );
    BOOST_CHECK_EQUAL( 14, t1()[1] );
    t1()[ 0 ] = 7;
    t1()[ 1 ]++;
    BOOST_CHECK_EQUAL( 7, t2()[0] );
    BOOST_CHECK_EQUAL( t2()[1], 15 );

    BOOST_CHECK_EQUAL( t1(0), 7 );
    BOOST_CHECK_EQUAL( 15, t1(1) );
    t1( 0 ) = 3;
    t1( 1 )++;
    BOOST_CHECK_EQUAL( 3, t2(0) );
    BOOST_CHECK_EQUAL( t2(1), 16 );

    // with more dimensions
    BOOST_CHECK( strcmp(t3()[ 0 ][ 1 ], "World") == 0 );
    BOOST_CHECK( strcmp(t3( 0 )[ 1 ], "World") == 0 );
    BOOST_CHECK( strcmp(t3( 0, 1 ), "World") == 0 );

    BOOST_CHECK( strcmp("Video", t4()[ 1 ][ 0 ]) == 0 );
    BOOST_CHECK( strcmp("Video", t4( 1 )[ 0 ]) == 0 );
    BOOST_CHECK( strcmp("Video", t4( 1, 0 )) == 0 );

    BOOST_CHECK_EQUAL( t3()[0][0][1], 'e' );
    BOOST_CHECK_EQUAL( t3(0)[0][1], 'e' );
    BOOST_CHECK_EQUAL( t3(0, 0)[1], 'e' );

    BOOST_CHECK_EQUAL( t4()[1][1][2], 't' );
    BOOST_CHECK_EQUAL( t4(1)[1][2], 't' );
    BOOST_CHECK_EQUAL( t4(1, 1)[2], 't' );

    t3()[ 1 ][ 1 ][ 1 ] = 'o';
    BOOST_CHECK( strcmp(t4()[ 1 ][ 1 ], "Botch") == 0 );
    t3( 1 )[ 1 ][ 1 ] = 'a';
    BOOST_CHECK( strcmp(t4( 1 )[ 1 ], "Batch") == 0 );
    t3( 1, 1 )[ 0 ] = 'C';
    BOOST_CHECK( strcmp(t4( 1, 1 ), "Catch") == 0 );

    // Try out "at"
    BOOST_CHECK_EQUAL( t1.at()[0], 3 );
    BOOST_CHECK_EQUAL( 16, t1.at()[1] );
    t1.at()[ 0 ] = 5;
    t1.at()[ 1 ]++;
    BOOST_CHECK_EQUAL( 5, t2.at()[0] );
    BOOST_CHECK_EQUAL( t2.at()[1], 17 );

    BOOST_CHECK_EQUAL( t1.at(0), 5 );
    BOOST_CHECK_EQUAL( 17, t1.at(1) );
    t1.at( 0 ) = 9;
    t1.at( 1 )++;
    BOOST_CHECK_EQUAL( 9, t2.at(0) );
    BOOST_CHECK_EQUAL( t2.at(1), 18 );

    BOOST_CHECK_NO_THROW( t1.at() );
    BOOST_CHECK_NO_THROW( t2.at() );
    BOOST_CHECK_THROW( t1.at(2), out_of_range );
    BOOST_CHECK_THROW( t2.at(7), out_of_range );
    BOOST_CHECK_THROW( t1.at(-8), out_of_range );

    BOOST_CHECK( strcmp(t3.at()[ 0 ][ 1 ], "World") == 0 );
    BOOST_CHECK( strcmp(t3.at( 0 )[ 1 ], "World") == 0 );
    BOOST_CHECK( strcmp(t3.at( 0, 1 ), "World") == 0 );

    BOOST_CHECK( strcmp("Video", t4.at()[ 1 ][ 0 ]) == 0 );
    BOOST_CHECK( strcmp("Video", t4.at( 1 )[ 0 ]) == 0 );
    BOOST_CHECK( strcmp("Video", t4.at( 1, 0 )) == 0 );

    BOOST_CHECK_EQUAL( t3.at()[0][0][1], 'e' );
    BOOST_CHECK_EQUAL( t3.at(0)[0][1], 'e' );
    BOOST_CHECK_EQUAL( t3.at(0, 0)[1], 'e' );

    BOOST_CHECK_EQUAL( t4.at()[1][1][2], 't' );
    BOOST_CHECK_EQUAL( t4.at(1)[1][2], 't' );
    BOOST_CHECK_EQUAL( t4.at(1, 1)[2], 't' );

    t3.at()[ 1 ][ 1 ][ 0 ] = 'L';
    BOOST_CHECK( strcmp(t4.at()[ 1 ][ 1 ], "Latch") == 0 );
    t3.at( 1 )[ 1 ][ 1 ] = 'u';
    BOOST_CHECK( strcmp(t4.at( 1 )[ 1 ], "Lutch") == 0 );
    t3.at( 1, 1 )[ 2 ] = 'n';
    BOOST_CHECK( strcmp(t4.at( 1, 1 ), "Lunch") == 0 );

    BOOST_CHECK_NO_THROW( t3.at() );
    BOOST_CHECK_NO_THROW( t4.at() );
    BOOST_CHECK_THROW( t3.at(2), out_of_range );
    BOOST_CHECK_THROW( t4.at(-6), out_of_range );
    BOOST_CHECK_THROW( t3.at(1, -9L), out_of_range );
    BOOST_CHECK_THROW( t4.at(0, 0xAAu), out_of_range );

#if 0
    // The implementation for the recursive-case class template can technically
    // do these, but it's blocked since the versions from the base-case class
    // template can't do it.
    BOOST_CHECK_EQUAL( t3(0, 0, 1), 'e' );
    BOOST_CHECK_EQUAL( t4(1, 1, 2), 'n' );
    t3( 1, 1, 0 ) = 'H';     // !strcmp( t4(1, 1), "Hunch" )
    t3.at( 1, 1, 0 ) = 'M';  // !strcmp( t4.at(1, 1), "Munch" )
    BOOST_CHECK_EQUAL( t4.at(1, 1, 4), 'h' );
#endif

    // Try out initialization-list indices
    BOOST_CHECK_EQUAL( t1[{ 0 }], 9 );
    BOOST_CHECK_EQUAL( 18, t1[{ 1 }] );
    t1[ {0} ] = -1;
    t1[ {1} ]++;
    BOOST_CHECK_EQUAL( -1, t2[{ 0 }] );
    BOOST_CHECK_EQUAL( t2[{ 1 }], 19 );

    t3[ {1, 1} ][ 0 ] = 'P';
    BOOST_CHECK_EQUAL( 0, strcmp(t4[ {1, 1} ], "Punch") );

    BOOST_CHECK_EQUAL( t1({ 0 }), -1 );
    BOOST_CHECK_EQUAL( 19, t1({ 1 }) );
    t1( {0} ) = 21;
    t1( {1} )++;
    BOOST_CHECK_EQUAL( 21, t2({ 0 }) );
    BOOST_CHECK_EQUAL( t2({ 1 }), 20 );

    t3( {1, 1} )[ 1 ] = 'i';
    BOOST_CHECK_EQUAL( 0, strcmp(t4( {1, 1} ), "Pinch") );

    BOOST_CHECK_EQUAL( t1.at({ 0 }), 21 );
    BOOST_CHECK_EQUAL( 20, t1.at({ 1 }) );
    t1.at( {0} ) = 29;
    t1.at( {1} )++;
    BOOST_CHECK_EQUAL( 29, t2.at({ 0 }) );
    BOOST_CHECK_EQUAL( t2.at({ 1 }), 21 );

    t3.at( {1, 1} )[ 2 ] = 't';
    BOOST_CHECK_EQUAL( 0, strcmp(t4.at( {1, 1} ), "Pitch") );

    BOOST_CHECK_NO_THROW( t1.at({ 0 }) );
    BOOST_CHECK_NO_THROW( t2.at({ 1 }) );
    BOOST_CHECK_THROW( t1.at({ 2 }), out_of_range );
    BOOST_CHECK_THROW( t2.at({ 7 }), out_of_range );
    BOOST_CHECK_THROW( t1.at({ 8 }), out_of_range );
    BOOST_CHECK_THROW( t1.at({}), length_error );
    BOOST_CHECK_THROW( t2.at({}), length_error );
    BOOST_CHECK_THROW( t1.at({1, 2}), length_error );
    BOOST_CHECK_THROW( t2.at({9, 10, 211}), length_error );

    BOOST_CHECK_NO_THROW( t3.at({ 0, 0 }) );
    BOOST_CHECK_NO_THROW( t4.at({ 1, 1 }) );
    BOOST_CHECK_THROW( t3.at({}), length_error );
    BOOST_CHECK_THROW( t4.at({}), length_error );
    BOOST_CHECK_THROW( t3.at({ 2 }), length_error );
    BOOST_CHECK_THROW( t4.at({ 1 }), length_error );
    BOOST_CHECK_THROW( t3.at({ 204, 1L }), out_of_range );
    BOOST_CHECK_THROW( t4.at({ 0, 0xAAu }), out_of_range );
    BOOST_CHECK_THROW( t3.at({ 1, 2, 3 }), length_error );
    BOOST_CHECK_THROW( t4.at({ 9, 8, 7 }), length_error );
}

BOOST_AUTO_TEST_CASE_TEMPLATE( test_zero_size, T, test_types )
{
    using boost::container::array_md;
    using std::is_same;

    array_md<T, 0>        e1;
    auto const &          e2 = e1;
    array_md<T, 0, 2>     e3;
    auto const &          e4 = e3;
    array_md<T, 0, 5, 7>  e5;
    auto const &          e6 = e5;
#if 0
    array_md<T, 3, 0>     e7;
    auto const &          e8 = e7;  // Only first extent can be 0.
#endif

    BOOST_CHECK( (is_same<typename decltype(e1)::data_type, T[]>::value) );
    BOOST_CHECK_EQUAL( e1.size(), 0u );
    BOOST_CHECK( e1.empty() );
    BOOST_CHECK_EQUAL( e1.begin(), e1.end() );
    BOOST_CHECK_EQUAL( e2.size(), 0u );
    BOOST_CHECK( e2.empty() );
    BOOST_CHECK_EQUAL( e2.begin(), e2.end() );

    BOOST_CHECK( (is_same<typename decltype(e3)::data_type, T[][2]>::value) );
    BOOST_CHECK_EQUAL( e3.size(), 0u );
    BOOST_CHECK( e3.empty() );
    BOOST_CHECK_EQUAL( e3.begin(), e3.end() );
    BOOST_CHECK_EQUAL( e4.size(), 0u );
    BOOST_CHECK( e4.empty() );
    BOOST_CHECK_EQUAL( e4.begin(), e4.end() );

    BOOST_CHECK( (is_same<typename decltype(e5)::data_type,T[][5][7]>::value) );
    BOOST_CHECK_EQUAL( e5.size(), 0u );
    BOOST_CHECK( e5.empty() );
    BOOST_CHECK_EQUAL( e5.begin(), e5.end() );
    BOOST_CHECK_EQUAL( e6.size(), 0u );
    BOOST_CHECK( e6.empty() );
    BOOST_CHECK_EQUAL( e6.begin(), e6.end() );
}

BOOST_AUTO_TEST_SUITE_END()  // test_array_md_basics


// Unit tests for iteration  -------------------------------------------------//

BOOST_AUTO_TEST_SUITE( test_array_md_iteration )

BOOST_AUTO_TEST_CASE_TEMPLATE( test_range_for, T, test_types )
{
    using boost::container::array_md;
    using std::begin;
    using std::end;

    // Singular array
    array_md<T>        t1;
    array_md<T> const  t2{ 2 };
    T                  total = T{};

    for ( auto &x1 : t1 )
        x1 = static_cast<T>( 1 );
    BOOST_CHECK_EQUAL( t1(), (T)1 );
    BOOST_CHECK_EQUAL( t1.end() - t1.begin(), 1 );

    BOOST_CHECK_EQUAL( t2(), (T)2 );
    for ( auto const &x2 : t2 )
        total += x2;
    BOOST_CHECK_EQUAL( static_cast<T>(2), total );
    BOOST_CHECK_EQUAL( t2.end() - t2.begin(), 1 );

    // Compound array
    array_md<T, 2, 3>          t3{ {{ 2, 3, 5 }, { 7, 11, 13 }} };
    array_md<T, 2, 3> const &  t4 = t3;
    T const                    results1[] = { 3, 4, 6, 8, 12, 14 };

    for ( auto &x3 : t3 )
        ++x3;
    BOOST_CHECK_EQUAL_COLLECTIONS( begin(t3), end(t3), begin(results1),
     end(results1) );
    BOOST_CHECK_EQUAL( t3(0, 0), (T)3 );
    BOOST_CHECK_EQUAL( t3(0, 1), (T)4 );
    BOOST_CHECK_EQUAL( t3(0, 2), (T)6 );
    BOOST_CHECK_EQUAL( t3(1, 0), (T)8 );
    BOOST_CHECK_EQUAL( t3(1, 1), (T)12 );
    BOOST_CHECK_EQUAL( t3(1, 2), (T)14 );
    BOOST_CHECK_EQUAL( t3.end() - t3.begin(), 6 );

    total = T{};
    for ( auto const &x4 : t4 )
        total += x4;
    BOOST_CHECK_EQUAL( static_cast<T>(47), total );
    BOOST_CHECK_EQUAL( t4.end() - t4.begin(), 6 );
}

BOOST_AUTO_TEST_CASE_TEMPLATE( test_const_iteration, T, test_types )
{
    using boost::container::array_md;
    using std::is_same;

    // If this happened, then "T *" and "T const *" would be the same.
    BOOST_REQUIRE( not std::is_const<T>::value );

    // Singular array
    array_md<T>          t1{ 23 };
    array_md<T> const &  t2 = t1;

    BOOST_CHECK( not (is_same<decltype( t1.begin() ), decltype( t1.cbegin()
     )>::value) );
    BOOST_CHECK( (is_same<decltype( t2.begin() ), decltype( t2.cbegin()
     )>::value) );
    BOOST_CHECK( (is_same<decltype( t1.cbegin() ), decltype( t2.cbegin()
     )>::value) );

    BOOST_CHECK( not (is_same<decltype( t1.end() ), decltype( t1.cend()
     )>::value) );
    BOOST_CHECK((is_same<decltype( t2.end() ), decltype( t2.cend() )>::value));
    BOOST_CHECK((is_same<decltype( t1.cend() ), decltype( t2.cend() )>::value));

    BOOST_CHECK_EQUAL( t1.begin(), t1.cbegin() );
    BOOST_CHECK_EQUAL( t2.begin(), t2.cbegin() );
    BOOST_CHECK_EQUAL( t1.end(), t1.cend() );
    BOOST_CHECK_EQUAL( t2.end(), t2.cend() );

    BOOST_CHECK_EQUAL( *t1.begin(), (T)23 );
    BOOST_CHECK_EQUAL( *t2.begin(), (T)23 );
    BOOST_CHECK_EQUAL( *t1.cbegin(), (T)23 );
    BOOST_CHECK_EQUAL( *t2.cbegin(), (T)23 );

    BOOST_CHECK_EQUAL( t1.cend() - t1.cbegin(), 1 );
    BOOST_CHECK_EQUAL( t2.cend() - t2.cbegin(), 1 );

    // Compound array
    array_md<T, 2, 3>          t3{ {{ 2, 3, 5 }, { 7, 11, 13 }} };
    array_md<T, 2, 3> const &  t4 = t3;

    BOOST_CHECK( not (is_same<decltype( t3.begin() ), decltype( t3.cbegin()
     )>::value) );
    BOOST_CHECK( (is_same<decltype( t4.begin() ), decltype( t4.cbegin()
     )>::value) );
    BOOST_CHECK( (is_same<decltype( t3.cbegin() ), decltype( t4.cbegin()
     )>::value) );

    BOOST_CHECK( not (is_same<decltype( t3.end() ), decltype( t3.cend()
     )>::value) );
    BOOST_CHECK((is_same<decltype( t4.end() ), decltype( t4.cend() )>::value));
    BOOST_CHECK((is_same<decltype( t3.cend() ), decltype( t4.cend() )>::value));

    BOOST_CHECK_EQUAL( t3.begin(), t3.cbegin() );
    BOOST_CHECK_EQUAL( t4.begin(), t4.cbegin() );
    BOOST_CHECK_EQUAL( t3.end(), t3.cend() );
    BOOST_CHECK_EQUAL( t4.end(), t4.cend() );

    BOOST_CHECK_EQUAL( *t3.begin(), (T)2 );
    BOOST_CHECK_EQUAL( *t4.begin(), (T)2 );
    BOOST_CHECK_EQUAL( *t3.cbegin(), (T)2 );
    BOOST_CHECK_EQUAL( *t4.cbegin(), (T)2 );

    BOOST_CHECK_EQUAL( t3.cend() - t3.cbegin(), 6 );
    BOOST_CHECK_EQUAL( t4.cend() - t4.cbegin(), 6 );
}

BOOST_AUTO_TEST_CASE_TEMPLATE( test_reverse_iteration, T, test_types )
{
    using boost::container::array_md;
    using std::is_same;
    using std::begin;
    using std::end;

    // If this happened, then "T *" and "T const *" would be the same.
    BOOST_REQUIRE( not std::is_const<T>::value );

    // Singular array
    array_md<T>          t1{ 23 };
    array_md<T> const &  t2 = t1;

    BOOST_CHECK( not (is_same<decltype( t1.rbegin() ), decltype( t1.crbegin()
     )>::value) );
    BOOST_CHECK( (is_same<decltype( t2.rbegin() ), decltype( t2.crbegin()
     )>::value) );
    BOOST_CHECK( (is_same<decltype( t1.crbegin() ), decltype( t2.crbegin()
     )>::value) );

    BOOST_CHECK( not (is_same<decltype( t1.rend() ), decltype( t1.crend()
     )>::value) );
    BOOST_CHECK((is_same<decltype( t2.rend() ), decltype( t2.crend()
     )>::value));
    BOOST_CHECK((is_same<decltype( t1.crend() ), decltype( t2.crend()
     )>::value));

    BOOST_CHECK_EQUAL( t1.rbegin(), t1.crbegin() );
    BOOST_CHECK_EQUAL( t2.rbegin(), t2.crbegin() );
    BOOST_CHECK_EQUAL( t1.rend(), t1.crend() );
    BOOST_CHECK_EQUAL( t2.rend(), t2.crend() );

    BOOST_CHECK_EQUAL( *t1.rbegin(), (T)23 );
    BOOST_CHECK_EQUAL( *t2.rbegin(), (T)23 );
    BOOST_CHECK_EQUAL( *t1.crbegin(), (T)23 );
    BOOST_CHECK_EQUAL( *t2.crbegin(), (T)23 );

    BOOST_CHECK_EQUAL( t1.rend() - t1.rbegin(), 1 );
    BOOST_CHECK_EQUAL( t2.rend() - t2.rbegin(), 1 );
    BOOST_CHECK_EQUAL( t1.crend() - t1.crbegin(), 1 );
    BOOST_CHECK_EQUAL( t2.crend() - t2.crbegin(), 1 );

    *t1.rbegin() = static_cast<T>( 29 );
    BOOST_CHECK_EQUAL( *t1.rbegin(), (T)29 );
    BOOST_CHECK_EQUAL( *t2.rbegin(), (T)29 );
    BOOST_CHECK_EQUAL( *t1.crbegin(), (T)29 );
    BOOST_CHECK_EQUAL( *t2.crbegin(), (T)29 );

    // Compound array
    array_md<T, 2, 3>          t3{ {{ 2, 3, 5 }, { 7, 11, 13 }} };
    array_md<T, 2, 3> const &  t4 = t3;
    T const                    results1[]{ 13, 11, 7, 5, 3, 2 }, results2[]{ 13,
     24, 31, 36, 39, 41 };

    BOOST_CHECK( not (is_same<decltype( t3.rbegin() ), decltype( t3.crbegin()
     )>::value) );
    BOOST_CHECK( (is_same<decltype( t4.rbegin() ), decltype( t4.crbegin()
     )>::value) );
    BOOST_CHECK( (is_same<decltype( t3.crbegin() ), decltype( t4.crbegin()
     )>::value) );

    BOOST_CHECK( not (is_same<decltype( t3.rend() ), decltype( t3.crend()
     )>::value) );
    BOOST_CHECK((is_same<decltype( t4.rend() ), decltype( t4.crend() )>::value));
    BOOST_CHECK((is_same<decltype( t3.crend() ), decltype( t4.crend() )>::value));

    BOOST_CHECK_EQUAL( t3.rbegin(), t3.crbegin() );
    BOOST_CHECK_EQUAL( t4.rbegin(), t4.crbegin() );
    BOOST_CHECK_EQUAL( t3.rend(), t3.crend() );
    BOOST_CHECK_EQUAL( t4.rend(), t4.crend() );

    BOOST_CHECK_EQUAL( *t3.rbegin(), (T)13 );
    BOOST_CHECK_EQUAL( *t4.rbegin(), (T)13 );
    BOOST_CHECK_EQUAL( *t3.crbegin(), (T)13 );
    BOOST_CHECK_EQUAL( *t4.crbegin(), (T)13 );

    BOOST_CHECK_EQUAL( t3.crend() - t3.crbegin(), 6 );
    BOOST_CHECK_EQUAL( t4.crend() - t4.crbegin(), 6 );

    BOOST_CHECK_EQUAL_COLLECTIONS( t3.rbegin(), t3.rend(), begin(results1),
     end(results1) );
    BOOST_CHECK_EQUAL_COLLECTIONS( t4.rbegin(), t4.rend(), begin(results1),
     end(results1) );
    BOOST_CHECK_EQUAL_COLLECTIONS( t3.crbegin(), t3.crend(), begin(results1),
     end(results1) );

    for ( auto  i = t3.rbegin() + 1 ; t3.rend() != i ; ++i )
        *i += *( i - 1 );
    BOOST_CHECK_EQUAL_COLLECTIONS( t4.rbegin(), t4.rend(), begin(results2),
     end(results2) );
    BOOST_CHECK_EQUAL_COLLECTIONS( t3.crbegin(), t3.crend(), begin(results2),
     end(results2) );
}

BOOST_AUTO_TEST_CASE( test_apply )
{
    using boost::container::array_md;
    using std::size_t;
    using std::strlen;
    using std::strcmp;

    // Sample function objects
    counting_negator  negator;  // works for any index count
    bool              flag;
    auto              flagger = [ &flag ]( int x ){ flag = !(x % 2); };

   // Singular array
    array_md<int>            t1{ 4 };
    array_md<char[6]> const  t2{ "Help" };
    size_t                   length;

    t1.apply( negator );
    BOOST_CHECK_EQUAL( t1(), -4 );
    BOOST_CHECK_EQUAL( negator.last_argument_count, 0u );

    t1.apply( flagger );
    BOOST_CHECK( flag );
    t1() = 9;
    t1.apply( flagger );
    BOOST_CHECK( not flag );
    t2.apply( [&length](char const ( &x )[ 6 ]){length = strlen( x );} );
    BOOST_CHECK_EQUAL( length, 4u );

#if 0
    t1.capply( negator );  // mutating function on const reference
#endif
    t1() = -10;
    t1.capply( flagger );
    BOOST_CHECK( flag );
    t2.capply( [&length](char const *s){length = strlen( s );} );
    BOOST_CHECK_EQUAL( length, 4u );

    // Compound array
    array_md<int, 3>             t3{ {1, 4, 9} };
    array_md<int, 2, 3>          t4{ {{ 2, 3, 5 }, { 7, 11, 13 }} };
    array_md<int, 2, 3> const &  t5 = t4;
    std::complex<double>         s{};

    t3.apply( negator );
    BOOST_CHECK_EQUAL( t3(0), -1 );
    BOOST_CHECK_EQUAL( t3(1), -4 );
    BOOST_CHECK_EQUAL( t3(2), -9 );
    BOOST_CHECK_EQUAL( negator.last_argument_count, 1u );

    length = 0u;
    t3.capply( [&length](int x, unsigned){length += x < 0 && !( x % 2 );} );
    BOOST_CHECK_EQUAL( length, 1u );
#if 0
    t3.capply( negator );  // mutating function on const reference
#endif

    t4.apply( [](int &x, size_t i0, size_t i1){x *= ( (i0 + i1) % 2u ) ? -1 :
     +1;} );
    BOOST_CHECK_EQUAL( t4(0, 0), +2 );
    BOOST_CHECK_EQUAL( t4(0, 1), -3 );
    BOOST_CHECK_EQUAL( t4(0, 2), +5 );
    BOOST_CHECK_EQUAL( t4(1, 0), -7 );
    BOOST_CHECK_EQUAL( t4(1, 1), +11 );
    BOOST_CHECK_EQUAL( t4(1, 2), -13 );

    t4.apply( negator );
    BOOST_CHECK_EQUAL( t4(0, 0), -2 );
    BOOST_CHECK_EQUAL( t4(0, 1), +3 );
    BOOST_CHECK_EQUAL( t4(0, 2), -5 );
    BOOST_CHECK_EQUAL( t4(1, 0), +7 );
    BOOST_CHECK_EQUAL( t4(1, 1), -11 );
    BOOST_CHECK_EQUAL( t4(1, 2), +13 );
    BOOST_CHECK_EQUAL( negator.last_argument_count, 2u );

    length = 0u;
    t4.capply( [&length](int x, unsigned, unsigned){length += x<0 && x%2;} );
    BOOST_CHECK_EQUAL( length, 2u );

    t5.apply( [&s](int x, size_t i0, size_t i1){
        if ( (i0 + i1) % 2u )
            s.real( s.real() + static_cast<double>(x) );
        else
            s.imag( s.imag() + static_cast<double>(x) );
     } );
    BOOST_CHECK_CLOSE( s.real(), +23.0, 0.1 );
    BOOST_CHECK_CLOSE( s.imag(), -18.0, 0.1 );

    length = 0u;
    t5.capply( [&length](int x, unsigned, unsigned){length += x<0 && !(x%2);} );
    BOOST_CHECK_EQUAL( length, 1u );

    // Fun with array-based elements
    array_md<char[6], 3> const  t6{ {"duck", "duck", "goose"} };
    array_md<char[6], 2, 2>     t7{ {{"Hello", "World"}, {"Video", "Watch"}} };

    length = 0u;
    t6.apply( [&length](char const *x, size_t){length += !std::strcmp( x,
     "duck" );} );
    BOOST_CHECK_EQUAL( length, 2u );

    t7.apply( [](char *x, size_t i0, size_t i1){if ( i0 != i1 ) reverse_case(
     x );} );
    BOOST_CHECK_EQUAL( strcmp(t7( 0, 0 ), "Hello"), 0 );
    BOOST_CHECK_EQUAL( strcmp(t7( 0, 1 ), "wORLD"), 0 );
    BOOST_CHECK_EQUAL( strcmp(t7( 1, 0 ), "vIDEO"), 0 );
    BOOST_CHECK_EQUAL( strcmp(t7( 1, 1 ), "Watch"), 0 );

    length = 0u;
    t7.capply( [&length](char const ( &x )[ 6 ], size_t, size_t){for ( char xx :
     x ) length += !!std::islower( xx );} );
    BOOST_CHECK_EQUAL( length, 10u );
}

BOOST_AUTO_TEST_SUITE_END()  // test_array_md_iteration


// Unit tests for other operations  ------------------------------------------//

BOOST_AUTO_TEST_SUITE( test_array_md_operations )

BOOST_AUTO_TEST_CASE( test_front_back )
{
    using boost::container::array_md;

    // Singular array
    array_md<int>        t1{ 2 }, t2{ 3 };
    array_md<int> const  &t3 = t1, &t4 = t2;

    BOOST_CHECK_EQUAL( t1.back(), 2 );
    BOOST_CHECK_EQUAL( t2.front(), 3 );

    t1.front() = 5;
    t2.back() = 7;
    BOOST_CHECK_EQUAL( t3.back(), 5 );
    BOOST_CHECK_EQUAL( t4.front(), 7 );

    BOOST_CHECK_EQUAL( &t3.front(), t3.data() );
    BOOST_CHECK_EQUAL( &t4.back(), t4.data() );
    BOOST_CHECK_EQUAL( &t1.front(), &t1.back() );
    BOOST_CHECK_EQUAL( &t2.front(), &t2.back() );

    // Compound array
    array_md<int, 2, 3>        t5{ {{ 2, 3, 5 }, { 7, 11, 13 }} },
     t6{ {{ 1, 4, 9 }, { 16, 25, 36 }} };
    array_md<int, 2, 3> const  &t7 = t5, &t8 = t6;

    BOOST_CHECK_EQUAL( t5.front(), 2 );
    BOOST_CHECK_EQUAL( t5.back(), 13 );
    BOOST_CHECK_EQUAL( t6.front(), 1 );
    BOOST_CHECK_EQUAL( t6.back(), 36 );

    t5.front() = -1002;
    t5.back() = -1013;
    t6.front() = -1001;
    t6.back() = -1036;
    BOOST_CHECK_EQUAL( t7.front(), -1002 );
    BOOST_CHECK_EQUAL( t7.back(), -1013 );
    BOOST_CHECK_EQUAL( t8.front(), -1001 );
    BOOST_CHECK_EQUAL( t8.back(), -1036 );

    BOOST_CHECK_EQUAL( &t5.front(), &*t5.begin() );
    BOOST_CHECK_EQUAL( &t5.back(), &*t5.rbegin() );
    BOOST_CHECK_NE( &t6.front(), &t6.back() );
    BOOST_CHECK_EQUAL( &t7.front(), t7.data() );
    BOOST_CHECK_EQUAL( &t8.back(), t8.data() + t8.size() - 1 );
}

BOOST_AUTO_TEST_CASE( test_fill )
{
    using boost::container::array_md;
    using std::strcmp;

    // Scalar element type
    array_md<int>        t1{};
    array_md<int, 2, 3>  t2{ {{ 2, 3, 5 }, { 7, 11, 13 }} };

    BOOST_CHECK_EQUAL( t1(), 0 );
    t1.fill( 4 );
    BOOST_CHECK_EQUAL( t1(), 4 );

    BOOST_CHECK_NE( t2(0, 0), -23 );
    BOOST_CHECK_NE( t2(0, 1), -23 );
    BOOST_CHECK_NE( t2(0, 2), -23 );
    BOOST_CHECK_NE( t2(1, 0), -23 );
    BOOST_CHECK_NE( t2(1, 1), -23 );
    BOOST_CHECK_NE( t2(1, 2), -23 );
    t2.fill( -23 );
    BOOST_CHECK_EQUAL( t2(0, 0), -23 );
    BOOST_CHECK_EQUAL( t2(0, 1), -23 );
    BOOST_CHECK_EQUAL( t2(0, 2), -23 );
    BOOST_CHECK_EQUAL( t2(1, 0), -23 );
    BOOST_CHECK_EQUAL( t2(1, 1), -23 );
    BOOST_CHECK_EQUAL( t2(1, 2), -23 );

    // Array element type
    array_md<char[6]>        t3{ "North" };
    array_md<char[6], 2, 2>  t4{ {{ "Hello", "World" }, { "Video", "Watch" }} };
    char const               r[] = "South";  // avoid array-to-pointer decay

    BOOST_REQUIRE_EQUAL( sizeof(r), sizeof(t3()) );
    BOOST_CHECK_NE( strcmp(t3(), r), 0 );
    t3.fill( r );
    BOOST_CHECK_EQUAL( strcmp(t3(), r), 0 );

    BOOST_CHECK_NE( strcmp(t4( 0, 0 ), r), 0 );
    BOOST_CHECK_NE( strcmp(t4( 0, 1 ), r), 0 );
    BOOST_CHECK_NE( strcmp(t4( 1, 0 ), r), 0 );
    BOOST_CHECK_NE( strcmp(t4( 1, 1 ), r), 0 );
    t4.fill( r );
    BOOST_CHECK_EQUAL( strcmp(t4( 0, 0 ), r), 0 );
    BOOST_CHECK_EQUAL( strcmp(t4( 0, 1 ), r), 0 );
    BOOST_CHECK_EQUAL( strcmp(t4( 1, 0 ), r), 0 );
    BOOST_CHECK_EQUAL( strcmp(t4( 1, 1 ), r), 0 );
}

BOOST_AUTO_TEST_CASE( test_equality )
{
    using boost::container::array_md;

    // Singular array
    array_md<int>  t1{ 3 }, t2{ 4 };
    array_md<long>  t3{ 5L }, t4{ 3L };

    BOOST_CHECK( t1 == t1 );
    BOOST_CHECK( t1 != t2 );
    BOOST_CHECK( t3 != t4 );
    BOOST_CHECK( t1 == t4 );
    BOOST_CHECK( t2 != t3 );

    // Compound array
    array_md<int, 2, 3>   t5{ {{ 2, 3, 5 }, { 7, 11, 13 }} }, t6 = t5;
    array_md<long, 2, 3>  t7{ {{ 2L, 3L, 5L }, { 7L, 11L, 13L }} };

    BOOST_CHECK( t5 == t6 );
    BOOST_CHECK( t6 == t7 );
    t6( 0, 2 )++;
    BOOST_CHECK( t5 != t6 );
    BOOST_CHECK( t6 != t7 );
}

BOOST_AUTO_TEST_CASE( test_less_than )
{
    using boost::container::array_md;

    // Singular array
    array_md<int>  t1{ 3 }, t2{ 4 };
    array_md<long>  t3{ 5L }, t4{ 3L };

    BOOST_CHECK( not(t1 < t1) );
    BOOST_CHECK( not(t1 > t1) );
    BOOST_CHECK( t1 <= t1 );
    BOOST_CHECK( t1 >= t1 );
    BOOST_CHECK( t1 < t2 );
    BOOST_CHECK( not(t1 > t2) );
    BOOST_CHECK( t1 <= t2 );
    BOOST_CHECK( not(t1 >= t2) );
    BOOST_CHECK( t1 < t3 );
    BOOST_CHECK( not(t1 > t3) );
    BOOST_CHECK( t1 <= t3 );
    BOOST_CHECK( not(t1 >= t3) );
    BOOST_CHECK( not(t1 < t4) );
    BOOST_CHECK( not(t1 > t4) );
    BOOST_CHECK( t1 <= t4 );
    BOOST_CHECK( t1 >= t4 );

    BOOST_CHECK( t2 > t1 );
    BOOST_CHECK( t3 > t1 );
    BOOST_CHECK( t2 >= t1 );
    BOOST_CHECK( t3 >= t1 );

    // Compound array
    array_md<int, 2, 3>   t5{ {{ 2, 3, 5 }, { 7, 11, 13 }} }, t6 = t5;
    array_md<long, 2, 3>  t7{ {{ 2L, 3L, 5L }, { 7L, 11L, 13L }} };

    BOOST_CHECK( not(t5 < t6) );
    BOOST_CHECK( not(t5 > t6) );
    BOOST_CHECK( not(t6 < t7) );
    BOOST_CHECK( not(t6 > t7) );
    BOOST_CHECK( t5 <= t6 );
    BOOST_CHECK( t5 >= t6 );
    BOOST_CHECK( t6 <= t7 );
    BOOST_CHECK( t6 >= t7 );
    t6( 0, 2 )++;
    BOOST_CHECK( t5 < t6 );
    BOOST_CHECK( t5 <= t6 );
    BOOST_CHECK( not(t6 < t7) );
    BOOST_CHECK( t6 >= t7 );
    BOOST_CHECK( t6 > t5 );
    BOOST_CHECK( t6 > t7 );
    BOOST_CHECK( not(t5 > t7) );
    BOOST_CHECK( t5 <= t7 );
}

BOOST_AUTO_TEST_CASE_TEMPLATE( test_swap, T, test_types )
{
    using boost::container::array_md;

    // Singular array
    array_md<T> const  t1{ 23 }, t2{ 101 };
    array_md<T>        t3 = t1, t4 = t2;

    BOOST_CHECK( t1 == t3 );
    BOOST_CHECK( t2 == t4 );
    BOOST_CHECK( t1 != t4 );
    BOOST_CHECK( t2 != t3 );

    swap( t3, t4 );
    BOOST_CHECK( t3 == t2 );
    BOOST_CHECK( t4 == t1 );
    BOOST_CHECK( t4 != t2 );
    BOOST_CHECK( t3 != t1 );

    // Compound array
    array_md<T, 2, 3> const  t5{ {{ 2, 3, 5 }, { 7, 11, 13 }} },
     t6{ {{ 1, 4, 9 }, { 16, 25, 36 }} };
    array_md<T, 2, 3>        t7 = t5, t8 = t6;

    BOOST_CHECK( t5 == t7 );
    BOOST_CHECK( t6 == t8 );
    BOOST_CHECK( t5 != t8 );
    BOOST_CHECK( t6 != t7 );

    swap( t7, t8 );
    BOOST_CHECK( t7 == t6 );
    BOOST_CHECK( t8 == t5 );
    BOOST_CHECK( t8 != t6 );
    BOOST_CHECK( t7 != t5 );
}

BOOST_AUTO_TEST_CASE( test_get )
{
    using boost::container::array_md;
    using std::is_same;
    using std::tuple_size;
    using std::tuple_element;
    using boost::container::get;
    using std::strcmp;
    using std::strcpy;

    // Singular array, scalar element
    typedef array_md<int>  sample1_type;

    BOOST_REQUIRE_EQUAL( tuple_size<sample1_type>::value, 1u );
    BOOST_REQUIRE( (is_same<typename tuple_element<0, sample1_type>::type,
     int>::value) );

    sample1_type          t1{ 2 };
    sample1_type const &  t2 = t1;

    BOOST_CHECK_EQUAL( get<0>(t1), 2 );
    get<0>( t1 ) = 3;
    BOOST_CHECK_EQUAL( get<0>(t2), 3 );
    BOOST_CHECK_EQUAL( get<0>(sample1_type{ 5 }), 5 );

    // Singular array, array element
    typedef array_md<char[6]>  sample2_type;

    BOOST_REQUIRE_EQUAL( tuple_size<sample2_type>::value, 1u );
    BOOST_REQUIRE( (is_same<typename tuple_element<0, sample2_type>::type,
     char[6]>::value) );

    sample2_type          t3{ "Hello" };
    sample2_type const &  t4 = t3;

    BOOST_CHECK_EQUAL( strcmp(get<0>( t3 ), "Hello"), 0 );
    strcpy( get<0>(t3), "Mello" );
    BOOST_CHECK_EQUAL( strcmp(get<0>( t4 ), "Mello"), 0 );
    BOOST_CHECK_NE( strcmp(get<0>( sample2_type{"World"} ), "Video"), 0 );

    // Compound array, scalar element
    typedef array_md<long, 2>  sample3_type;

    BOOST_REQUIRE_EQUAL( tuple_size<sample3_type>::value, 2u );
    BOOST_REQUIRE( (is_same<typename tuple_element<0, sample3_type>::type,
     long>::value) );
    BOOST_REQUIRE( (is_same<typename tuple_element<1, sample3_type>::type,
     long>::value) );

    sample3_type          t5{ {3L, 5L} };
    sample3_type const &  t6 = t5;

    BOOST_CHECK_EQUAL( get<0>(t5), 3L );
    get<0>( t5 ) = 4L;
    BOOST_CHECK_EQUAL( get<0>(t6), 4L );
    BOOST_CHECK_EQUAL( get<1>(t6), 5L );
    BOOST_CHECK_EQUAL( get<1>(sample3_type{ {6L, 7L} }), 7L );

    // Compound array, array element
    typedef array_md<char[6], 3>  sample4_type;

    BOOST_REQUIRE_EQUAL( tuple_size<sample4_type>::value, 3u );
    BOOST_REQUIRE( (is_same<typename tuple_element<0, sample4_type>::type,
     char[6]>::value) );
    BOOST_REQUIRE( (is_same<typename tuple_element<1, sample4_type>::type,
     char[6]>::value) );
    BOOST_REQUIRE( (is_same<typename tuple_element<2, sample4_type>::type,
     char[6]>::value) );

    sample4_type          t7{ {"Hello", "there", "World"} };
    sample4_type const &  t8 = t7;

    BOOST_CHECK_EQUAL( strcmp(get<1>( t7 ), "there"), 0 );
    strcpy( get<1>(t7), "where" );
    BOOST_CHECK_EQUAL( strcmp(get<0>( t8 ), "Hello"), 0 );
    BOOST_CHECK_EQUAL( strcmp(get<1>( t8 ), "where"), 0 );
    BOOST_CHECK_EQUAL( strcmp(get<2>( t8 ), "World"), 0 );
    BOOST_CHECK_NE( strcmp(get<0>( sample4_type{{ "North", "South",
     "Watch" }} ), "Video"), 0 );

    // More compound arrays, scalar element
    typedef array_md<short, 2, 3>  sample5_type;

    BOOST_REQUIRE_EQUAL( tuple_size<sample5_type>::value, 6u );
    BOOST_REQUIRE( (is_same<typename tuple_element<0, sample5_type>::type,
     short>::value) );
    BOOST_REQUIRE( (is_same<typename tuple_element<1, sample5_type>::type,
     short>::value) );
    BOOST_REQUIRE( (is_same<typename tuple_element<2, sample5_type>::type,
     short>::value) );
    BOOST_REQUIRE( (is_same<typename tuple_element<3, sample5_type>::type,
     short>::value) );
    BOOST_REQUIRE( (is_same<typename tuple_element<4, sample5_type>::type,
     short>::value) );
    BOOST_REQUIRE( (is_same<typename tuple_element<5, sample5_type>::type,
     short>::value) );

    sample5_type          t9{ {{ 2, 3, 5 }, { 7, 11, 13 }} };
    sample5_type const &  t10 = t9;

    BOOST_CHECK_EQUAL( get<0>(t9), 2 );
    get<0>( t9 ) = -4;
    BOOST_CHECK_EQUAL( get<0>(t10), -4 );
    BOOST_CHECK_EQUAL( get<1>(t10), 3 );
    BOOST_CHECK_EQUAL( get<2>(t10), 5 );
    BOOST_CHECK_EQUAL( get<3>(t10), 7 );
    BOOST_CHECK_EQUAL( get<4>(t10), 11 );
    BOOST_CHECK_EQUAL( get<5>(t10), 13 );
    BOOST_CHECK_EQUAL( get<4>(sample5_type{ {{1, 4, 9}, {16, 25, 36}} }), 25 );

    // More compound arrays, array element
    typedef array_md<char[6], 3, 2>  sample6_type;

    BOOST_REQUIRE_EQUAL( tuple_size<sample6_type>::value, 6u );
    BOOST_REQUIRE( (is_same<typename tuple_element<0, sample6_type>::type,
     char[6]>::value) );
    BOOST_REQUIRE( (is_same<typename tuple_element<1, sample6_type>::type,
     char[6]>::value) );
    BOOST_REQUIRE( (is_same<typename tuple_element<2, sample6_type>::type,
     char[6]>::value) );
    BOOST_REQUIRE( (is_same<typename tuple_element<3, sample6_type>::type,
     char[6]>::value) );
    BOOST_REQUIRE( (is_same<typename tuple_element<4, sample6_type>::type,
     char[6]>::value) );
    BOOST_REQUIRE( (is_same<typename tuple_element<5, sample6_type>::type,
     char[6]>::value) );

    sample6_type          t11{ {{ "Four", "score" }, { "and", "seven" },
     { "years", "ago" }} };
    sample6_type const &  t12 = t11;

    BOOST_CHECK_EQUAL( strcmp(get<3>( t11 ), "seven"), 0 );
    strcpy( get<3>(t11), "lever" );
    BOOST_CHECK_EQUAL( strcmp(get<0>( t12 ), "Four"), 0 );
    BOOST_CHECK_EQUAL( strcmp(get<1>( t12 ), "score"), 0 );
    BOOST_CHECK_EQUAL( strcmp(get<2>( t12 ), "and"), 0 );
    BOOST_CHECK_EQUAL( strcmp(get<3>( t12 ), "lever"), 0 );
    BOOST_CHECK_EQUAL( strcmp(get<4>( t12 ), "years"), 0 );
    BOOST_CHECK_EQUAL( strcmp(get<5>( t12 ), "ago"), 0 );
    BOOST_CHECK_NE( strcmp(get<2>( sample6_type{{{"array", "lambd"}, {"struc",
     "class"}, {"short", "nullp"}}} ), "Video"), 0 );
}

BOOST_AUTO_TEST_CASE( test_conversion )
{
    using boost::container::array_md;

    // Singular arrays
    array_md<unsigned char> const  t1{ 13u };
    array_md<char[6]> const        t2{ "Hello" };
    auto const                     t3 = static_cast<array_md<signed char>>(t1);
    auto const                     t4 = static_cast<array_md<std::string>>(t2);

    BOOST_CHECK_EQUAL( t3.front(), 13 );
    BOOST_CHECK_EQUAL( t4.front(), "Hello" );

    // Compound arrays
    using std::string;

    array_md<int, 3> const         t5{ {2, -3, 5} };
    array_md<char[6], 2, 2> const  t6{ {{"Hello","World"}, {"Video","Watch"}} };
    array_md<long, 2, 3, 5> const  t7{ {{{-2, +3, -5, +7, -11}, {+13, -17, +19,
     -23, +29}, {-31, +37, -41, +43, -47}}, {{+53, -57, +59, -61, +67}, {-71,
     +73, -79, +83, -87}, {+89, -93, +97, -101, +103}}} };
    auto const                     t8 = static_cast<array_md<double, 3>>( t5 );
    auto const                   t9 = static_cast<array_md<string, 2, 2>>( t6 );
    auto const                t10 = static_cast<array_md<float, 2, 3, 5>>( t7 );

    BOOST_CHECK_CLOSE( t8[0], +2.0, 0.1 );
    BOOST_CHECK_CLOSE( t8[1], -3.0, 0.1 );
    BOOST_CHECK_CLOSE( t8[2], +5.0, 0.1 );
    BOOST_CHECK_EQUAL( t9[0][0], "Hello" );
    BOOST_CHECK_EQUAL( t9[0][1], "World" );
    BOOST_CHECK_EQUAL( t9[1][0], "Video" );
    BOOST_CHECK_EQUAL( t9[1][1], "Watch" );
    BOOST_CHECK_CLOSE( t10[0][0][0], -2.0f, 0.1 );
    BOOST_CHECK_CLOSE( t10[0][0][1], +3.0f, 0.1 );
    BOOST_CHECK_CLOSE( t10[0][1][0], +13.0f, 0.1 );
    BOOST_CHECK_CLOSE( t10[1][0][0], +53.0f, 0.1 );
}

BOOST_AUTO_TEST_CASE( test_creation )
{
    using boost::container::make_array;
    using boost::container::array_md;
    using std::is_same;
    using boost::container::make_auto_array;
    using boost::container::reshape_array;

    // Non-auto
#if 0
    auto  s0 = make_array<int>( 4, 5 );  // too many initializers
#endif
    auto  s1 = make_array<int>( 4 );
    auto  s2 = make_array<long>();
    auto  s3 = make_array<float, 3u>( -1.0, 2u, 3.5L );
    auto  s4 = make_array<double, 2, 5>( -5L, 4.3f, 3ULL, -2.0, 100, 1.1L );

    BOOST_CHECK( (is_same<decltype( s1 ), array_md<int>>::value) );
    BOOST_CHECK( (is_same<decltype( s2 ), array_md<long>>::value) );
    BOOST_CHECK( (is_same<decltype( s3 ), array_md<float, 3u>>::value) );
    BOOST_CHECK( (is_same<decltype( s4 ), array_md<double, 2u, 5u>>::value) );
    BOOST_CHECK_EQUAL( s1(), 4 );
    BOOST_CHECK_EQUAL( s2(), 0L );
    BOOST_CHECK_CLOSE( s3[0], -1.0f, 0.1 );
    BOOST_CHECK_CLOSE( s3[1], +2.0f, 0.1 );
    BOOST_CHECK_CLOSE( s3[2], +3.5f, 0.1 );
    BOOST_CHECK_CLOSE( s4[0][0],  -5.0, 0.1 );
    BOOST_CHECK_CLOSE( s4[0][1],  +4.3, 0.1 );
    BOOST_CHECK_CLOSE( s4[0][2],  +3.0, 0.1 );
    BOOST_CHECK_CLOSE( s4[0][3],  -2.0, 0.1 );
    BOOST_CHECK_CLOSE( s4[0][4], 100.0, 0.1 );
    BOOST_CHECK_CLOSE( s4[1][0],  +1.1, 0.1 );
    BOOST_CHECK_CLOSE( s4[1][1],   0.0, 0.1 );
    BOOST_CHECK_CLOSE( s4[1][2],   0.0, 0.1 );
    BOOST_CHECK_CLOSE( s4[1][3],   0.0, 0.1 );
    BOOST_CHECK_CLOSE( s4[1][4],   0.0, 0.1 );

    // Auto-typed, all same type
    auto  t1 = make_auto_array( 0, -1, +2 );
    auto  t2 = make_auto_array( 1UL, 4UL, 0UL, 57UL );

    BOOST_CHECK( (is_same<decltype( t1 ), array_md<int, 3u>>::value) );
    BOOST_CHECK( (is_same<decltype( t2 ), array_md<unsigned long,4u>>::value) );

    // Auto-typed, mixed types
    auto  t3 = make_auto_array( -6, +3u );
    auto  t4 = make_auto_array( -7, +4u, -99LL, +100UL );
    auto  t5 = make_auto_array( 1.0f, -3.5 );
    auto  t6 = make_auto_array( 2.1, -709.0f, +1024.2048L );
    auto  t7 = make_auto_array( -80.0L, 79ULL );

    BOOST_CHECK( (is_same<decltype( t3 ), array_md<unsigned, 2u>>::value) );
    BOOST_CHECK( (is_same<decltype( t4 ), array_md<long long, 4u>>::value) );
    BOOST_CHECK_EQUAL( t4[0],   -7LL );
    BOOST_CHECK_EQUAL( t4[1],   +4LL );
    BOOST_CHECK_EQUAL( t4[2],  -99LL );
    BOOST_CHECK_EQUAL( t4[3], +100LL );
    BOOST_CHECK( (is_same<decltype( t5 ), array_md<double, 2u>>::value) );
    BOOST_CHECK_CLOSE( t5[0],  1.0, 0.1 );
    BOOST_CHECK_CLOSE( t5[1], -3.5, 0.1 );
    BOOST_CHECK( (is_same<decltype( t6 ), array_md<long double, 3u>>::value) );
    BOOST_CHECK_CLOSE( t6[0],     2.1L,    0.1 );
    BOOST_CHECK_CLOSE( t6[1],  -709.0L,    0.1 );
    BOOST_CHECK_CLOSE( t6[2], +1024.2048L, 0.1 );
    BOOST_CHECK( (is_same<decltype( t7 ), array_md<long double, 2u>>::value) );
    BOOST_CHECK_CLOSE( t7[0], -80.0L, 0.1 );
    BOOST_CHECK_CLOSE( t7[1], +79.0L, 0.1 );

    // Reshape, different element types, same shape
    auto  u1 = reshape_array<double>( make_array<unsigned>(7u) );
    auto  u2 = reshape_array<double, 3>( make_array<int, 3>(-3, 0, +2) );
    auto  u3 = reshape_array<long, 2, 2>( make_array<int,2,2>(1, -4, 9, -16) );

    BOOST_CHECK( (is_same<decltype( u1 ), array_md<double>>::value) );
    BOOST_CHECK_CLOSE( u1(), +7.0, 0.1 );
    BOOST_CHECK( (is_same<decltype( u2 ), array_md<double, 3>>::value) );
    BOOST_CHECK_CLOSE( u2[0], -3.0, 0.1 );
    BOOST_CHECK_CLOSE( u2[1],  0.0, 0.1 );
    BOOST_CHECK_CLOSE( u2[2], +2.0, 0.1 );
    BOOST_CHECK( (is_same<decltype( u3 ), array_md<long, 2, 2>>::value) );
    BOOST_CHECK_EQUAL( u3[0][0], 1L );
    BOOST_CHECK_EQUAL( u3[0][1], -4L );
    BOOST_CHECK_EQUAL( u3[1][0], 9L );
    BOOST_CHECK_EQUAL( u3[1][1], -16L );

    // Reshape, same element type, different shapes, same size
    auto  u5 = reshape_array<double, 1>( u1 );
    auto  u6 = reshape_array<double, 1, 1>( u1 );
    auto  u7 = reshape_array<long, 4>( u3 );

    BOOST_CHECK( (is_same<decltype( u5 ), array_md<double, 1>>::value) );
    BOOST_CHECK_CLOSE( u5[0], u1(), 0.1 );
    BOOST_CHECK( (is_same<decltype( u6 ), array_md<double, 1, 1>>::value) );
    BOOST_CHECK_CLOSE( u6[0][0], u1(), 0.1 );
    BOOST_CHECK( (is_same<decltype( u7 ), array_md<long, 4>>::value) );
    BOOST_CHECK_EQUAL( u7[0], u3[0][0] );
    BOOST_CHECK_EQUAL( u7[1], u3[0][1] );
    BOOST_CHECK_EQUAL( u7[2], u3[1][0] );
    BOOST_CHECK_EQUAL( u7[3], u3[1][1] );

    // Reshape, same element type, different shapes, different size
    auto   u8 = reshape_array<double, 2, 3, 5>( u1 );
    auto   u9 = reshape_array<long, 3>( u3 );
    auto  u10 = reshape_array<long, 2, 2, 2>( u3 );

    BOOST_CHECK( (is_same<decltype( u8 ), array_md<double, 2, 3, 5>>::value) );
    BOOST_CHECK_CLOSE( u8[0][0][0], u1(), 0.1 );
    BOOST_CHECK( std::all_of(u8.begin() + 1, u8.end(), []( double x ){ return x
     == 0.0; }) );
    BOOST_CHECK( (is_same<decltype( u9 ), array_md<long, 3>>::value) );
    BOOST_CHECK_EQUAL( u9[0], u3[0][0] );
    BOOST_CHECK_EQUAL( u9[1], u3[0][1] );
    BOOST_CHECK_EQUAL( u9[2], u3[1][0] );
    BOOST_CHECK( (is_same<decltype( u10 ), array_md<long, 2, 2, 2>>::value) );
    BOOST_CHECK_EQUAL( u10[0][0][0], u3[0][0] );
    BOOST_CHECK_EQUAL( u10[0][0][1], u3[0][1] );
    BOOST_CHECK_EQUAL( u10[0][1][0], u3[1][0] );
    BOOST_CHECK_EQUAL( u10[0][1][1], u3[1][1] );
    BOOST_CHECK_EQUAL( u10[1][0][0], 0L );
    BOOST_CHECK_EQUAL( u10[1][0][1], 0L );
    BOOST_CHECK_EQUAL( u10[1][1][0], 0L );
    BOOST_CHECK_EQUAL( u10[1][1][1], 0L );

    // Reshape, different element type, different shapes, different size
    auto u11 = reshape_array<int>( u2 );
    auto u12 = reshape_array<long, 2>( u2 );
    auto u13 = reshape_array<long, 4>( u2 );
    auto u14 = reshape_array<float, 2, 2>( u2 );

    BOOST_CHECK( (is_same<decltype( u11 ), array_md<int>>::value) );
    BOOST_CHECK_EQUAL( u11(), -3 );
    BOOST_CHECK( (is_same<decltype( u12 ), array_md<long, 2>>::value) );
    BOOST_CHECK_EQUAL( u12[0], -3L );
    BOOST_CHECK_EQUAL( u12[1],  0L );
    BOOST_CHECK( (is_same<decltype( u13 ), array_md<long, 4>>::value) );
    BOOST_CHECK_EQUAL( u13[0], -3L );
    BOOST_CHECK_EQUAL( u13[1],  0L );
    BOOST_CHECK_EQUAL( u13[2], +2L );
    BOOST_CHECK_EQUAL( u13[3],  0L );
    BOOST_CHECK( (is_same<decltype( u14 ), array_md<float, 2, 2>>::value) );
    BOOST_CHECK_CLOSE( u14[0][0], -3.0f, 0.1 );
    BOOST_CHECK_CLOSE( u14[0][1],  0.0f, 0.1 );
    BOOST_CHECK_CLOSE( u14[1][0], +2.0f, 0.1 );
    BOOST_CHECK_CLOSE( u14[1][1],  0.0f, 0.1 );
}

BOOST_AUTO_TEST_SUITE_END()  // test_array_md_operations
