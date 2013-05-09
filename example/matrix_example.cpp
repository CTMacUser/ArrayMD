//  Boost Multi-dimensional Array matrix example file  -----------------------//

//  Copyright 2013 Daryle Walker.
//  Distributed under the Boost Software License, Version 1.0.  (See the
//  accompanying file LICENSE_1_0.txt or a copy at
//  <http://www.boost.org/LICENSE_1_0.txt>.)

//  See <http://www.boost.org/libs/container/> for the library's home page.

#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <iomanip>
#include <iostream>
#include <ostream>

#include <boost/assert.hpp>
#include "boost/container/array_md.hpp"


namespace
{
    // Quickie matrix type
    template < typename T, unsigned Rows, unsigned Columns >
    class matrix
    {
    public:
        // Template parameters
        typedef T  value_type;

        static constexpr
        std::size_t  row_count = Rows, column_count = Columns;

        // Lifetime management
        // (Use automatically defined copy-ctr, move-ctr, dtr)
        matrix() = default;
        matrix( std::initializer_list<T> i )
        {
            BOOST_ASSERT( i.size() <= decltype(data)::static_size );
            std::copy( i.begin(), i.end(), data.begin() );
        }

        // Operators
        // (Use automatically-defined copy- and move-assignment)
        value_type &  operator ()( unsigned r, unsigned c )
        { return data(r, c); }
        constexpr
        value_type const &  operator ()( unsigned r, unsigned c ) const
        { return data(r, c); }

        explicit
        operator bool() const
        {
            return std::any_of( data.begin(), data.end(), [](T const &x){return
             static_cast<bool>(x);} );
        }

        matrix &  operator +=( matrix const &addend )
        {
            auto  a = addend.data.begin();
            for ( auto &x : data )
                x += *a++;
            return *this;
        }
        matrix &  operator -=( matrix const &subtrahend )
        {
            auto  s = subtrahend.data.begin();
            for ( auto &x : data )
                x -= *s++;
            return *this;
        }

        template < unsigned X >
        void  add_product( matrix<T, Rows, X> const &addend_multiplicand,
         matrix<T, X, Columns> const &addend_multiplier )
        {
            data.apply( [&](T &x, unsigned rr, unsigned cc){
                for ( unsigned  i = 0u ; i < X ; ++i )
                    x += addend_multiplicand(rr, i) * addend_multiplier(i, cc);
            } );
        }
        matrix &  operator *=( matrix<T, Columns, Columns> const &multiplier )
        {
            matrix  this_copy{};

            this_copy.add_product( *this, multiplier );
            *this = this_copy;
            return *this;
        }

    private:
        boost::container::array_md<T, Rows, Columns>  data;
    };

    // Static member definitions
    template < typename T, unsigned Rows, unsigned Columns >
    constexpr std::size_t  matrix<T, Rows, Columns>::row_count;

    template < typename T, unsigned Rows, unsigned Columns >
    constexpr std::size_t  matrix<T, Rows, Columns>::column_count;

    // More operators
    template < typename T, unsigned R, unsigned C >
    matrix<T, R, C>  operator +( matrix<T, R, C> l, matrix<T, R, C> const &r )
    { return l += r; }
    template < typename T, unsigned R, unsigned C >
    matrix<T, R, C>  operator -( matrix<T, R, C> l, matrix<T, R, C> const &r )
    { return l -= r; }
    template < typename T, unsigned R, unsigned X, unsigned C >
    auto  operator *( matrix<T, R, X> const &l, matrix<T, X, C> const &r )
     -> matrix<T, R, C>
    {
        matrix<T, R, C>  result{};

        result.add_product( l, r );
        return result;
    }

    // Printing
    template < typename Ch, class Tr, typename T, unsigned Rows, unsigned
     Columns >
    std::basic_ostream<Ch, Tr> &
    operator <<( std::basic_ostream<Ch, Tr> &o, matrix<T, Rows, Columns> const &
     m )
    {
        o << '[';
        for ( unsigned i = 0u ; i < Rows ; ++i )
        {
            o << '[' << m( i, 0 );
            for ( unsigned j = 1u ; j < Columns ; ++j )
                o << ',' << ' ' << m( i, j );
            o << ']';
        }
        o << ']';
        return o;
    }

}


// Main function
int  main( int, char *[] )
{
    using std::cout;
    using std::endl;

    // Basic operations
    matrix<int, 2, 2>  a{ 1, 2, 3, 4 }, b{ 5, 6, 7, 8 };

    cout << "a: " << a << endl;
    cout << "b: " << b << endl;
    cout << "a + b: " << a + b << endl;
    cout << "a - b: " << a - b << endl;
    cout << "a * b: " << a * b << endl;

    // R-value and *= testing
    auto  m = matrix<int, 3, 2>{ 2, 3, 5, 7, 11, 13 } * matrix<int, 2, 3>{ 1, 4,
     9, 16, 25, 36 };

    cout << "m's size: " << decltype( m )::row_count << "-by-" <<
     decltype( m )::column_count << '.' << endl;
    cout << "m: " << m << endl;
    m *= matrix<int, 3, 3>{ 1, 0, 0, 0, 1, 0, 0, 0, 1 };
    cout << "m * I<3>: " << m << endl;

    // Boolean checks
    cout << std::boolalpha << "Bool(m): " << static_cast<bool>( m )
     << "; Bool(Z<2,2>): " << static_cast<bool>( a - a ) << endl;

    return 0;
}
