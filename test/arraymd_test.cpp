//  Boost Multi-dimensional Array unit test program file  --------------------//

//  Copyright 2013 Daryle Walker.
//  Distributed under the Boost Software License, Version 1.0.  (See the
//  accompanying file LICENSE_1_0.txt or a copy at
//  <http://www.boost.org/LICENSE_1_0.txt>.)

//  See <http://www.boost.org/libs/container/> for the library's home page.

#define BOOST_TEST_MAIN  "Multi-Dimensional Array Unit Tests"
#include <boost/test/included/unit_test.hpp>
#include <boost/test/test_case_template.hpp>
#include <boost/mpl/list.hpp>

#include "boost/container/array_md.hpp"

#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <type_traits>


// Common definitions  -------------------------------------------------------//

namespace {

typedef boost::mpl::list<int, long, unsigned char>  test_types;

}


// Unit tests for basic functionality  ---------------------------------------//

BOOST_AUTO_TEST_SUITE( test_array_md_basics )

BOOST_AUTO_TEST_CASE_TEMPLATE( test_singular_element_static, T, test_types )
{
    using std::is_same;

    typedef boost::container::array_md<T>  sample_type;

    BOOST_REQUIRE( (is_same<T, typename sample_type::value_type>::value) );
    BOOST_REQUIRE( (is_same<T, typename sample_type::data_type>::value) );
    BOOST_REQUIRE( (is_same<std::size_t, typename
     sample_type::size_type>::value) );

    BOOST_REQUIRE_EQUAL( sample_type::dimensionality, 0u );
    BOOST_REQUIRE_EQUAL( sample_type::static_size, 1u );

    BOOST_REQUIRE( (is_same<T *, typename sample_type::pointer>::value) );
    BOOST_REQUIRE( (is_same<T const *, typename
     sample_type::const_pointer>::value) );

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

    typedef array_md<T, 7, 3>  sample_type;

    BOOST_REQUIRE( (is_same<T, typename sample_type::value_type>::value) );
    BOOST_REQUIRE( !(is_same<T, typename sample_type::data_type>::value) );
    BOOST_REQUIRE( (is_same<T[3], typename
     sample_type::direct_element_type>::value) );
    BOOST_REQUIRE( (is_same<T[7][3], typename sample_type::data_type>::value) );
    BOOST_REQUIRE( (is_same<size_t, typename sample_type::size_type>::value) );

    BOOST_REQUIRE_EQUAL( sample_type::dimensionality, 2u );
    BOOST_REQUIRE_EQUAL( sample_type::static_sizes[0], 7u );
    BOOST_REQUIRE_EQUAL( sample_type::static_sizes[1], 3u );
    BOOST_REQUIRE_EQUAL( sample_type::static_size, 21u );

    BOOST_REQUIRE( (is_same<T *, typename sample_type::pointer>::value) );
    BOOST_REQUIRE( (is_same<T const *, typename
     sample_type::const_pointer>::value) );

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

    BOOST_REQUIRE_EQUAL( sample2_type::dimensionality, 1u );
    BOOST_REQUIRE_EQUAL( sample2_type::static_sizes[0], 5u );
    BOOST_REQUIRE_EQUAL( sample2_type::static_size, 5u );

    BOOST_REQUIRE( (is_same<T *, typename sample2_type::pointer>::value) );
    BOOST_REQUIRE( (is_same<T const *, typename
     sample2_type::const_pointer>::value) );

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

BOOST_AUTO_TEST_SUITE_END()  // test_array_md_basics
