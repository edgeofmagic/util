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

#ifndef LOGICMILL_BSTREAM_FWD_DECLS_H
#define LOGICMILL_BSTREAM_FWD_DECLS_H

namespace logicmill
{
namespace bstream
{

template<class T, class Enable = void>
struct value_deserializer;

template<class T, class Enable = void>
struct ref_deserializer;

template<class T, class Enable = void>
struct ptr_deserializer;

template<class T, class Enable = void>
struct shared_ptr_deserializer;

template<class T, class Enable = void>
struct unique_ptr_deserializer;

template<class T, class Enable = void>
struct serializer;

template<class Derived, class Base, class Enable = void>
struct base_serializer;

template<class T, class Enable = void>
struct ibstream_initializer;

class ibstream;

class obstream;

}    // namespace bstream
}    // namespace logicmill

#endif    // LOGICMILL_BSTREAM_FWD_DECLS_H