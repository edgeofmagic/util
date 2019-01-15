/*
 * The MIT License
 *
 * Copyright 2017 David Curtis.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef LOGICMILL_BSTREAM_MEMORY_H
#define LOGICMILL_BSTREAM_MEMORY_H

#include <memory>

namespace logicmill
{
namespace bstream
{
template<class Derived, class Base>
typename std::enable_if_t<std::is_move_constructible<Derived>::value, std::unique_ptr<Derived>>
static_unique_ptr_cast(std::unique_ptr<Base>&& p)
{
	std::unique_ptr<Derived> derp = std::make_unique<Derived>(std::move(reinterpret_cast<Derived&>(*p.get())));
	p                             = nullptr;
	return derp;
}


// template< class Derived, class Base >
// std::unique_ptr< Derived >
// static_unique_ptr_cast( std::unique_ptr< Base >&& p )
// {
//     return std::unique_ptr< Derived >{ reinterpret_cast< Derived* >( p.release() ) };
// }

/*
template< typename Derived, typename Base, typename Deleter >
inline std::unique_ptr< Derived, Deleter > 
static_unique_ptr_cast( std::unique_ptr< Base, Deleter >&& p )
{
    auto d = static_cast< Derived * >( p.release() );
    return std::unique_ptr< Derived, Deleter >( d, std::move( p.get_deleter() ) );
}

template< typename Derived, typename Base, typename Deleter >
inline std::unique_ptr< Derived, Deleter > 
dynamic_unique_ptr_cast( std::unique_ptr< Base, Deleter >&& p )
{
    if( Derived *result = dynamic_cast< Derived * >( p.get() ) ) 
	{
        p.release();
        return std::unique_ptr< Derived, Deleter >( result, std::move( p.get_deleter() ) );
    }
    return std::unique_ptr< Derived, Deleter >( nullptr, p.get_deleter() );
}
*/

}    // namespace bstream
}    // namespace logicmill

#endif    // LOGICMILL_BSTREAM_MEMORY_H