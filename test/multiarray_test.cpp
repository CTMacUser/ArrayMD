//  Boost Multi-dimensional Array Adaptor unit test program file  ------------//

//  Copyright 2013 Daryle Walker.
//  Distributed under the Boost Software License, Version 1.0.  (See the
//  accompanying file LICENSE_1_0.txt or a copy at
//  <http://www.boost.org/LICENSE_1_0.txt>.)

//  See <http://www.boost.org/libs/container/> for the library's home page.

#include <boost/test/unit_test.hpp>
#include <boost/mpl/list.hpp>

#include "boost/container/multiarray.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <deque>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>


// Common definitions  -------------------------------------------------------//

namespace {

// Sample testing types for elements
typedef boost::mpl::list<int, long, unsigned char>  test_types;

}

// Flag un-printable types


// Unit tests for basic functionality  ---------------------------------------//

BOOST_AUTO_TEST_SUITE( test_multiarray_basics )

BOOST_AUTO_TEST_CASE_TEMPLATE( test_static_attributes, T, test_types )
{
    using boost::container::multiarray;
    using std::is_same;
    using std::size_t;
    using std::vector;
    using std::deque;

    typedef multiarray<T, 0>  sample1_type;

    BOOST_REQUIRE( (is_same<T, typename sample1_type::value_type>::value) );
    BOOST_REQUIRE( (is_same<T &, typename sample1_type::reference>::value) );
    BOOST_REQUIRE( (is_same<T const &, typename
     sample1_type::const_reference>::value) );
    BOOST_REQUIRE( (is_same<size_t, typename sample1_type::size_type>::value) );
    BOOST_REQUIRE( (is_same<vector<T>, typename
     sample1_type::container_type>::value) );
    BOOST_REQUIRE_EQUAL( sample1_type::dimensionality, 0u );

    typedef multiarray<T, 2>  sample2_type;

    BOOST_REQUIRE( (is_same<T, typename sample2_type::value_type>::value) );
    BOOST_REQUIRE( (is_same<T &, typename sample2_type::reference>::value) );
    BOOST_REQUIRE( (is_same<T const &, typename
     sample2_type::const_reference>::value) );
    BOOST_REQUIRE( (is_same<size_t, typename sample2_type::size_type>::value) );
    BOOST_REQUIRE( (is_same<vector<T>, typename
     sample2_type::container_type>::value) );
    BOOST_REQUIRE_EQUAL( sample2_type::dimensionality, 2u );

    typedef multiarray<T, 5, deque<T>>  sample3_type;

    BOOST_REQUIRE( (is_same<T, typename sample3_type::value_type>::value) );
    BOOST_REQUIRE( (is_same<T &, typename sample3_type::reference>::value) );
    BOOST_REQUIRE( (is_same<T const &, typename
     sample3_type::const_reference>::value) );
    BOOST_REQUIRE( (is_same<size_t, typename sample3_type::size_type>::value) );
    BOOST_REQUIRE( (is_same<deque<T>, typename
     sample3_type::container_type>::value) );
    BOOST_REQUIRE_EQUAL( sample3_type::dimensionality, 5u );

#if 0
    // Element type for multiarray and array don't match!
    typedef multiarray<unsigned, 3, std::array<double, 27>>  sample4_type;

    BOOST_REQUIRE((is_same<unsigned,typename sample4_type::value_type>::value));
#endif
}

BOOST_AUTO_TEST_CASE_TEMPLATE( test_indexing, T, test_types )
{
    using boost::container::multiarray;
    using std::vector;
    using std::is_same;
    using std::array;
    using std::size_t;
    using std::deque;
    using std::out_of_range;
    using std::length_error;
    using std::begin;
    using std::end;
    using std::invalid_argument;

    // Check single, non-indexed element case, container taken by move
    multiarray<T, 0>  a{ vector<T>{T( 2 )} };
    auto const &      aa = a;
    auto const        a_extents = aa.extents(), a_priorities = a.priorities();

    BOOST_CHECK_EQUAL( a.required_size(), 1u );
    BOOST_CHECK_EQUAL( a.size(), 1u );
    BOOST_CHECK( !aa.empty() );

    BOOST_CHECK_EQUAL( aa(), (T)2 );
    a() = T( 3 );
    BOOST_CHECK_EQUAL( aa[{}], (T)3 );
    a[ {} ] = T( 5 );
    BOOST_CHECK_EQUAL( aa({}), (T)5 );
    a( {} ) = T( 7 );
    BOOST_CHECK_EQUAL( aa.at(), (T)7 );
    a.at() = T( 11 );
    BOOST_CHECK_EQUAL( aa.at({}), (T)11 );
    a.at( {} ) = T( 13 );

    BOOST_REQUIRE( (is_same<decltype( a_extents ), const array<size_t,
     0>>::value) );
    BOOST_REQUIRE( (is_same<decltype( a_priorities ), const array<size_t,
     0>>::value) );
    // a_extents and a_priorities have no state to check.

    // Single index, non-vector inner container, taken by copy
    deque<T> const              bd{ a(), T(17), T(19), T(23), T(29), T(31) };
    multiarray<T, 1, deque<T>>  b{ bd };
    auto const &                bb = b;
    auto               b_extents = bb.extents(), b_priorities = bb.priorities();
    auto const                  b_expected_extents{ 6u };
    auto const                  b_expected_priorites{ 0u };

    BOOST_CHECK_EQUAL( b.required_size(), 6u );
    BOOST_CHECK_EQUAL( b.size(), 6u );
    BOOST_CHECK( !bb.empty() );

    BOOST_CHECK_EQUAL( bb(0), aa() );
    b( 1 ) = T( 4 );
    BOOST_CHECK_EQUAL( bb[{ 1u }], (T)4 );
    b[ {2u} ] = T( 6 );
    BOOST_CHECK_EQUAL( bb({ 2u }), (T)6 );
    b( {3u} ) = T( 8 );
    BOOST_CHECK_EQUAL( bb.at(3u), (T)8 );
    b.at( 4u ) = T( 9 );
    BOOST_CHECK_EQUAL( bb.at({ 4u }), (T)9 );
    b.at( {5u} ) = T( 10 );
    BOOST_CHECK_THROW( bb.at(7u), out_of_range );
    BOOST_CHECK_THROW( b.at({ 8u }), out_of_range );
    BOOST_CHECK_THROW( bb.at({}), length_error );
    BOOST_CHECK_THROW( b.at({ 9u, 10u }), length_error );

    BOOST_CHECK_EQUAL_COLLECTIONS( begin(b_extents), end(b_extents),
     begin(b_expected_extents), end(b_expected_extents) );
    BOOST_CHECK_EQUAL_COLLECTIONS( begin(b_priorities), end(b_priorities),
     begin(b_expected_priorites), end(b_expected_priorites) );

    // Reshape
    auto const  b_expected_extents2{ 5u }, b_expected_extents3{ 7u };

    BOOST_CHECK_THROW( b.extents(0u), out_of_range );
    b.extents( 5 );
    b_extents = b.extents();
    BOOST_CHECK( !std::equal(b_extents.begin(), b_extents.end(),
     b_expected_extents.begin()) );
    BOOST_CHECK_EQUAL_COLLECTIONS( begin(b_extents), end(b_extents),
     begin(b_expected_extents2), end(b_expected_extents2) );
    BOOST_CHECK_EQUAL( b.required_size(), 5u );
    BOOST_CHECK_EQUAL( b.size(), 6u );
    BOOST_CHECK( !bb.empty() );
    BOOST_CHECK_THROW( b.at(5u), out_of_range );

    BOOST_CHECK_THROW( b.priorities(1u), out_of_range );
    BOOST_CHECK_THROW( b.priorities({ 2u }), out_of_range );
    b.priorities( {0u} );
    b_priorities = b.priorities();
    BOOST_CHECK_EQUAL_COLLECTIONS( begin(b_priorities), end(b_priorities),
     begin(b_expected_priorites), end(b_expected_priorites) );

    BOOST_CHECK_THROW( b.extents({ 0u }), out_of_range );
    b.extents( {7u} );
    b_extents = b.extents();
    BOOST_CHECK( !std::equal(b_extents.begin(), b_extents.end(),
     b_expected_extents2.begin()) );
    BOOST_CHECK_EQUAL_COLLECTIONS( begin(b_extents), end(b_extents),
     begin(b_expected_extents3), end(b_expected_extents3) );
    BOOST_CHECK_EQUAL( b.required_size(), 7u );
    BOOST_CHECK_EQUAL( b.size(), 6u );
    BOOST_CHECK( !bb.empty() );
    BOOST_CHECK_EQUAL( b.at({ 5u }), (T)10 );
#if 0
    BOOST_CHECK_EQUAL( b.at(6u), T{} );  // supported, but not actually there
#endif

    // Multiple-index, statically-sized inner container, default provided
    multiarray<T, 2, array<T, 18>>  c;
    auto const &                    cc = c;
    auto               c_extents = cc.extents(), c_priorities = cc.priorities();
    auto const                      c_expected_extents{ 18u, 1u };
    auto const                      c_expected_priorities{ 0u, 1u };

    BOOST_CHECK_EQUAL( c.required_size(), 18u );
    BOOST_CHECK_EQUAL( c.size(), 18u );
    BOOST_CHECK( !cc.empty() );
    BOOST_CHECK_EQUAL_COLLECTIONS( begin(c_extents), end(c_extents),
     begin(c_expected_extents), end(c_expected_extents) );
    BOOST_CHECK_EQUAL_COLLECTIONS( begin(c_priorities), end(c_priorities),
     begin(c_expected_priorities), end(c_expected_priorities) );

    c( 0, 0 ) = T( 37 );
    c( {1, 0} ) = T( 41 );
    c[ {2, 0} ] = T( 43 );
    c.at( 3, 0 ) = T( 47 );
    c.at( {4, 0} ) = T( 53 );
    BOOST_CHECK_THROW( c.at(4, 1), out_of_range );
    BOOST_CHECK_THROW( cc.at({ 20, 0 }), out_of_range );
    BOOST_CHECK_THROW( c.at({ 5 }), length_error );
    BOOST_CHECK_EQUAL( cc(0, 0), (T)37 );
    BOOST_CHECK_EQUAL( cc({ 1, 0 }), (T)41 );
    BOOST_CHECK_EQUAL( (cc[{ 2, 0 }]), (T)43 );
    BOOST_CHECK_EQUAL( cc.at(3, 0), (T)47 );
    BOOST_CHECK_EQUAL( cc.at({ 4, 0 }), (T)53 );
#if 0
    BOOST_CHECK_EQUAL( cc(5, 0), T{} );  // uninitialized data
#endif
    // Another resize
    auto const  c_expected_extents2{ 4u, 6u };
    auto const  c_expected_priorities2{ 1u, 0u };
    auto const  too_large = std::numeric_limits<typename
     decltype(c)::size_type>::max() / 2u;

    BOOST_CHECK_THROW( c.extents(too_large, too_large), std::overflow_error );
    BOOST_CHECK_THROW( c.extents(0u, 4u), out_of_range );
    BOOST_CHECK_THROW( c.extents({ {5u} }), out_of_range );  // actually {5, 0}
    c.extents( 4u, 6u );
    c_extents = cc.extents();
    BOOST_CHECK_EQUAL( c.required_size(), 24u );
    BOOST_CHECK_EQUAL( c.size(), 18u );
    BOOST_CHECK( !cc.empty() );
    BOOST_CHECK_EQUAL_COLLECTIONS( begin(c_extents), end(c_extents),
     begin(c_expected_extents2), end(c_expected_extents2) );
    BOOST_CHECK_EQUAL( c(0, 0), (T)37 );
    BOOST_CHECK_EQUAL( c(0, 1), (T)41 );
    BOOST_CHECK_EQUAL( c(0, 2), (T)43 );
    BOOST_CHECK_EQUAL( c(0, 3), (T)47 );
    BOOST_CHECK_EQUAL( c(0, 4), (T)53 );
    c[ {0, 5} ] = T( 59 );
    BOOST_CHECK_EQUAL( cc({ 0, 5 }), (T)59 );
    BOOST_CHECK_THROW( c.at(0, 6) = T(61), out_of_range );
    BOOST_CHECK_THROW( cc.at({}), length_error );
    c.at( {1, 0} ) = T( 61 );
    BOOST_CHECK_EQUAL( cc({ 1, 0 }), (T)61 );

    BOOST_CHECK_THROW( c.priorities({ {2u} }), out_of_range );  //actually {2,0}
    BOOST_CHECK_THROW( c.priorities(1u, 4u), out_of_range );
    BOOST_CHECK_THROW( c.priorities(1u, 1u), invalid_argument );
    BOOST_CHECK_THROW( c.priorities({ {0u, 0u} }), invalid_argument );
    c.priorities( 1u, 0u );
    c_priorities = cc.priorities();
    BOOST_CHECK_EQUAL_COLLECTIONS( begin(c_priorities), end(c_priorities),
     begin(c_expected_priorities2), end(c_expected_priorities2) );
    BOOST_CHECK_EQUAL( c(0, 0), (T)37 );
    BOOST_CHECK_EQUAL( c(1, 0), (T)41 );
    BOOST_CHECK_EQUAL( c(2, 0), (T)43 );
    BOOST_CHECK_EQUAL( c(3, 0), (T)47 );
    BOOST_CHECK_EQUAL( c(0, 1), (T)53 );
    BOOST_CHECK_EQUAL( c(1, 1), (T)59 );
    BOOST_CHECK_EQUAL( c(2, 1), (T)61 );
    c[ {3, 1} ] = T( 67 );
    BOOST_CHECK_EQUAL( cc.at(3, 1), (T)67 );

    c.use_row_major_order();
    c_priorities = cc.priorities();
    BOOST_CHECK_EQUAL_COLLECTIONS( begin(c_priorities), end(c_priorities),
     begin(c_expected_priorities), end(c_expected_priorities) );
    c.use_column_major_order();
    c_priorities = cc.priorities();
    BOOST_CHECK_EQUAL_COLLECTIONS( begin(c_priorities), end(c_priorities),
     begin(c_expected_priorities2), end(c_expected_priorities2) );

    // Handle extents and priorities together
    auto const  c_expected_extents3{ 9u, 2u };

    BOOST_CHECK_THROW( c.extents_and_priorities({ {9u, 0u} }, { {0u, 1u} }),
     out_of_range );
    BOOST_CHECK_THROW( c.extents_and_priorities({ {9u, 2u} }, { {0u, 4u} }),
     out_of_range );
    BOOST_CHECK_THROW( c.extents_and_priorities({ {9u, 2u} }, { {1u, 1u} }),
     invalid_argument );
    c.extents_and_priorities( {{ 9u, 2u }}, {{ 1u, 0u }} );
    c_extents = cc.extents();
    BOOST_CHECK_EQUAL( c.required_size(), 18u );
    BOOST_CHECK_EQUAL( c.size(), 18u );
    BOOST_CHECK( !cc.empty() );
    BOOST_CHECK_EQUAL_COLLECTIONS( begin(c_extents), end(c_extents),
     begin(c_expected_extents3), end(c_expected_extents3) );
    c_priorities = cc.priorities();
    BOOST_CHECK_EQUAL_COLLECTIONS( begin(c_priorities), end(c_priorities),
     begin(c_expected_priorities2), end(c_expected_priorities2) );

    BOOST_CHECK_EQUAL( c(0, 0), (T)37 );
    BOOST_CHECK_EQUAL( c(1, 0), (T)41 );
    BOOST_CHECK_EQUAL( c(2, 0), (T)43 );
    BOOST_CHECK_EQUAL( c(3, 0), (T)47 );
    BOOST_CHECK_EQUAL( c(4, 0), (T)53 );
    BOOST_CHECK_EQUAL( c(5, 0), (T)59 );
    BOOST_CHECK_EQUAL( c(6, 0), (T)61 );
    BOOST_CHECK_EQUAL( cc.at(7, 0), (T)67 );
    c[ {8, 0} ] = T( 71 );
    BOOST_CHECK_EQUAL( cc.at(8, 0), (T)71 );
    c( 0, 1 ) = T( 73 );
    BOOST_CHECK_EQUAL( cc(0, 1), (T)73 );
}

BOOST_AUTO_TEST_SUITE_END()  // test_multiarray_basics


// Unit tests for iteration  -------------------------------------------------//

BOOST_AUTO_TEST_SUITE( test_multiarray_iteration )

BOOST_AUTO_TEST_CASE( test_apply )
{
    using boost::container::multiarray;
    using std::fill;
    using std::size_t;
    using std::begin;
    using std::end;

    // Derived class with direct container access
    class my_multiarray
        : public multiarray<int, 2, std::array<int, 6>>
    {
        using base_type = multiarray<int, 2, std::array<int, 6>>;

    public:
        using base_type::c;
    };

    // Set up sample data
    my_multiarray  sample_rm;
    auto const &   sr = sample_rm;

    std::iota( begin(sample_rm.c), end(sample_rm.c), 0 );
    sample_rm.extents( 2u, 3u );

    // Make a transposed container
    my_multiarray  sample_tp;

    fill( begin(sample_tp.c), end(sample_tp.c), -1 );
    sample_tp.extents( 3u, 2u );
    sr.apply( [&](int x, size_t i0, size_t i1){sample_tp( i1, i0 ) = x;} );

    BOOST_CHECK_EQUAL( sr(0u, 0u), sample_tp(0u, 0u) );
    BOOST_CHECK_EQUAL( sr(0u, 1u), sample_tp(1u, 0u) );
    BOOST_CHECK_EQUAL( sr(0u, 2u), sample_tp(2u, 0u) );
    BOOST_CHECK_EQUAL( sr(1u, 0u), sample_tp(0u, 1u) );
    BOOST_CHECK_EQUAL( sr(1u, 1u), sample_tp(1u, 1u) );
    BOOST_CHECK_EQUAL( sr(1u, 2u), sample_tp(2u, 1u) );

    // Make a column-major container
    my_multiarray  sample_cm;

    fill( begin(sample_cm.c), end(sample_cm.c), -2 );
    sample_cm.extents_and_priorities( {{ 2u, 3u }}, {{ 1u, 0u }} );
    sample_rm.capply( [&](int x,size_t i0,size_t i1){sample_cm( i0,i1 ) = x;} );

    BOOST_CHECK_EQUAL( sr(0u, 0u), sample_cm(0u, 0u) );
    BOOST_CHECK_EQUAL( sr(0u, 1u), sample_cm(0u, 1u) );
    BOOST_CHECK_EQUAL( sr(0u, 2u), sample_cm(0u, 2u) );
    BOOST_CHECK_EQUAL( sr(1u, 0u), sample_cm(1u, 0u) );
    BOOST_CHECK_EQUAL( sr(1u, 1u), sample_cm(1u, 1u) );
    BOOST_CHECK_EQUAL( sr(1u, 2u), sample_cm(1u, 2u) );

    // Check the coolness between transposed row-major and column-major!
    BOOST_CHECK_EQUAL_COLLECTIONS( begin(sample_tp.c), end(sample_tp.c),
     begin(sample_cm.c), end(sample_cm.c) );

    // Write to the iterated elements
    sample_tp.apply( [&sample_cm](int &x, size_t i0, size_t i1){
        x -= sample_cm[ {i1, i0} ];
     } );
    BOOST_CHECK_EQUAL( std::count(begin(sample_tp.c), end(sample_tp.c), 0), 6 );

    sample_cm.apply( [](int &x, size_t, size_t){x *= -1;} );
    BOOST_CHECK_EQUAL( sample_cm(0u, 0u),  0 );  // NOT in in-memory order!
    BOOST_CHECK_EQUAL( sample_cm(0u, 1u), -1 );
    BOOST_CHECK_EQUAL( sample_cm(0u, 2u), -2 );
    BOOST_CHECK_EQUAL( sample_cm(1u, 0u), -3 );
    BOOST_CHECK_EQUAL( sample_cm(1u, 1u), -4 );
    BOOST_CHECK_EQUAL( sample_cm(1u, 2u), -5 );
}

BOOST_AUTO_TEST_SUITE_END()  // test_multiarray_iteration


// Unit tests for other operations  ------------------------------------------//

BOOST_AUTO_TEST_SUITE( test_multiarray_operations )

BOOST_AUTO_TEST_CASE( test_fill )
{
    using boost::container::multiarray;
    using std::count;

    multiarray<int, 2, std::array<int, 20>>     sample{};
    auto const        first_element = &sample[ {0, 0} ];

    // Exact fill
    sample.extents( 4u, 5u );
    sample.fill( +1 );
    BOOST_CHECK_EQUAL( count(first_element, first_element + 20, +1), 20 );

    // Under fill
    sample.extents( 6u, 3u );
    sample.fill( -2 );
    BOOST_CHECK_EQUAL( count(first_element, first_element + 18, -2), 18 );
    BOOST_CHECK_EQUAL( count(first_element, first_element + 20, -2), 18 );

    // Over fill
    // (Can't reference past 20 elements for range check)
    sample.extents( 3u, 7u );
    sample.fill( +5 );
    BOOST_CHECK_EQUAL( count(first_element, first_element + 20, +5), 20 );
}

BOOST_AUTO_TEST_CASE_TEMPLATE( test_swap, T, test_types )
{
    using boost::container::multiarray;
    using std::vector;
    using std::swap;

    // Differing set-ups
    multiarray<T, 2, vector<T>>  sample1( vector<T>{T(1), T(2), T(3), T(4),
     T(5), T(6), T(7), T(8), T(9), T(10)} ), sample2( vector<T>{T(101), T(102),
     T(103), T(104), T(105), T(106), T(107), T(108), T(109), T(110), T(111),
     T(112)} );
    auto const                  &ss1 = sample1, &ss2 = sample2;

    sample1.extents_and_priorities( {{2u, 5u}}, {{0u, 1u}} );
    BOOST_CHECK_EQUAL( ss1(0u, 0u), (T)1 );
    BOOST_CHECK_EQUAL( ss1(0u, 1u), (T)2 );
    BOOST_CHECK_EQUAL( ss1(0u, 2u), (T)3 );
    BOOST_CHECK_EQUAL( ss1(0u, 3u), (T)4 );
    BOOST_CHECK_EQUAL( ss1(0u, 4u), (T)5 );
    BOOST_CHECK_EQUAL( ss1(1u, 0u), (T)6 );
    BOOST_CHECK_EQUAL( ss1(1u, 1u), (T)7 );
    BOOST_CHECK_EQUAL( ss1(1u, 2u), (T)8 );
    BOOST_CHECK_EQUAL( ss1(1u, 3u), (T)9 );
    BOOST_CHECK_EQUAL( ss1(1u, 4u), (T)10 );
    sample2.extents_and_priorities( {{4u, 3u}}, {{1u, 0u}} );
    BOOST_CHECK_EQUAL( ss2(0u, 0u), (T)101 );
    BOOST_CHECK_EQUAL( ss2(1u, 0u), (T)102 );
    BOOST_CHECK_EQUAL( ss2(2u, 0u), (T)103 );
    BOOST_CHECK_EQUAL( ss2(3u, 0u), (T)104 );
    BOOST_CHECK_EQUAL( ss2(0u, 1u), (T)105 );
    BOOST_CHECK_EQUAL( ss2(1u, 1u), (T)106 );
    BOOST_CHECK_EQUAL( ss2(2u, 1u), (T)107 );
    BOOST_CHECK_EQUAL( ss2(3u, 1u), (T)108 );
    BOOST_CHECK_EQUAL( ss2(0u, 2u), (T)109 );
    BOOST_CHECK_EQUAL( ss2(1u, 2u), (T)110 );
    BOOST_CHECK_EQUAL( ss2(2u, 2u), (T)111 );
    BOOST_CHECK_EQUAL( ss2(3u, 2u), (T)112 );

    // Now swap
    auto const  new_expected_extents1 = ss2.extents(), new_expected_extents2 =
     ss1.extents(), new_expected_priorities1 = ss2.priorities(),
     new_expected_priorities2 = ss1.priorities();

    swap( sample1, sample2 );

    BOOST_CHECK( sample1.extents() == new_expected_extents1 );
    BOOST_CHECK( sample2.extents() == new_expected_extents2 );
    BOOST_CHECK( sample1.priorities() == new_expected_priorities1 );
    BOOST_CHECK( sample2.priorities() == new_expected_priorities2 );

    BOOST_CHECK_EQUAL( ss1(0u, 0u), (T)101 );
    BOOST_CHECK_EQUAL( ss1(1u, 0u), (T)102 );
    BOOST_CHECK_EQUAL( ss1(2u, 0u), (T)103 );
    BOOST_CHECK_EQUAL( ss1(3u, 0u), (T)104 );
    BOOST_CHECK_EQUAL( ss1(0u, 1u), (T)105 );
    BOOST_CHECK_EQUAL( ss1(1u, 1u), (T)106 );
    BOOST_CHECK_EQUAL( ss1(2u, 1u), (T)107 );
    BOOST_CHECK_EQUAL( ss1(3u, 1u), (T)108 );
    BOOST_CHECK_EQUAL( ss1(0u, 2u), (T)109 );
    BOOST_CHECK_EQUAL( ss1(1u, 2u), (T)110 );
    BOOST_CHECK_EQUAL( ss1(2u, 2u), (T)111 );
    BOOST_CHECK_EQUAL( ss1(3u, 2u), (T)112 );

    BOOST_CHECK_EQUAL( ss2(0u, 0u), (T)1 );
    BOOST_CHECK_EQUAL( ss2(0u, 1u), (T)2 );
    BOOST_CHECK_EQUAL( ss2(0u, 2u), (T)3 );
    BOOST_CHECK_EQUAL( ss2(0u, 3u), (T)4 );
    BOOST_CHECK_EQUAL( ss2(0u, 4u), (T)5 );
    BOOST_CHECK_EQUAL( ss2(1u, 0u), (T)6 );
    BOOST_CHECK_EQUAL( ss2(1u, 1u), (T)7 );
    BOOST_CHECK_EQUAL( ss2(1u, 2u), (T)8 );
    BOOST_CHECK_EQUAL( ss2(1u, 3u), (T)9 );
    BOOST_CHECK_EQUAL( ss2(1u, 4u), (T)10 );

    // Swap back
    swap( sample1, sample2 );

    BOOST_CHECK( sample1.extents() == new_expected_extents2 );
    BOOST_CHECK( sample2.extents() == new_expected_extents1 );
    BOOST_CHECK( sample1.priorities() == new_expected_priorities2 );
    BOOST_CHECK( sample2.priorities() == new_expected_priorities1 );

    BOOST_CHECK_EQUAL( ss1(0u, 0u), (T)1 );
    BOOST_CHECK_EQUAL( ss1(0u, 1u), (T)2 );
    BOOST_CHECK_EQUAL( ss1(0u, 2u), (T)3 );
    BOOST_CHECK_EQUAL( ss1(0u, 3u), (T)4 );
    BOOST_CHECK_EQUAL( ss1(0u, 4u), (T)5 );
    BOOST_CHECK_EQUAL( ss1(1u, 0u), (T)6 );
    BOOST_CHECK_EQUAL( ss1(1u, 1u), (T)7 );
    BOOST_CHECK_EQUAL( ss1(1u, 2u), (T)8 );
    BOOST_CHECK_EQUAL( ss1(1u, 3u), (T)9 );
    BOOST_CHECK_EQUAL( ss1(1u, 4u), (T)10 );

    BOOST_CHECK_EQUAL( ss2(0u, 0u), (T)101 );
    BOOST_CHECK_EQUAL( ss2(1u, 0u), (T)102 );
    BOOST_CHECK_EQUAL( ss2(2u, 0u), (T)103 );
    BOOST_CHECK_EQUAL( ss2(3u, 0u), (T)104 );
    BOOST_CHECK_EQUAL( ss2(0u, 1u), (T)105 );
    BOOST_CHECK_EQUAL( ss2(1u, 1u), (T)106 );
    BOOST_CHECK_EQUAL( ss2(2u, 1u), (T)107 );
    BOOST_CHECK_EQUAL( ss2(3u, 1u), (T)108 );
    BOOST_CHECK_EQUAL( ss2(0u, 2u), (T)109 );
    BOOST_CHECK_EQUAL( ss2(1u, 2u), (T)110 );
    BOOST_CHECK_EQUAL( ss2(2u, 2u), (T)111 );
    BOOST_CHECK_EQUAL( ss2(3u, 2u), (T)112 );
}

BOOST_AUTO_TEST_CASE( test_subclassing )
{
    using boost::container::multiarray;

    // Now we can mess with the container!
    class my_multiarray
        : public multiarray<int, 2>
    {
    public:
        container_type &        get_container() noexcept        { return c; }
        container_type const &  get_container() const noexcept  { return c; }
    } sample{};
    auto const &  ss = sample;

    // Check initial stats
    BOOST_CHECK_EQUAL( ss.required_size(), 1u );
    BOOST_CHECK_EQUAL( ss.size(), 0u );
    BOOST_CHECK( ss.empty() );

    // Now update
    sample.get_container().push_back( +1 );
    sample.get_container().push_back( -2 );
    sample.get_container().push_back( +3 );
    sample.get_container().push_back( -4 );

    BOOST_CHECK_EQUAL( ss.get_container().size(), 4u );
    BOOST_CHECK_EQUAL( ss.required_size(), 1u );
    BOOST_CHECK_EQUAL( ss.size(), 4u );
    BOOST_CHECK( !ss.empty() );

    // Now access those new elements
    sample.extents( 2u, 2u );

    BOOST_CHECK_EQUAL( ss.required_size(), 4u );
    BOOST_CHECK_EQUAL( ss.size(), 4u );
    BOOST_CHECK( !ss.empty() );
    BOOST_CHECK_EQUAL( ss(0u, 0u), +1 );
    BOOST_CHECK_EQUAL( ss(0u, 1u), -2 );
    BOOST_CHECK_EQUAL( ss(1u, 0u), +3 );
    BOOST_CHECK_EQUAL( ss(1u, 1u), -4 );

    // Write to the elements
    sample.fill( +5 );

    BOOST_CHECK_EQUAL( ss(0u, 0u), +5 );
    BOOST_CHECK_EQUAL( ss(0u, 1u), +5 );
    BOOST_CHECK_EQUAL( ss(1u, 0u), +5 );
    BOOST_CHECK_EQUAL( ss(1u, 1u), +5 );
    BOOST_CHECK_EQUAL( std::count(sample.get_container().begin(),
     sample.get_container().end(),+5), static_cast<std::ptrdiff_t>(ss.size()) );
}

BOOST_AUTO_TEST_SUITE_END()  // test_multiarray_operations
