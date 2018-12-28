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

#ifndef LOGICMILL_UTIL_MACROS_H
#define LOGICMILL_UTIL_MACROS_H

#include <boost/predef.h>

// disable warning for missing overrides;
// see definitions for BSTRM_POLY_SERIALIZE_METHOD_() and 
// BSTRM_POLY_SERIALIZE_DECL(), in include/logicmill/bstream/macros.h

#if ( BOOST_COMP_CLANG )
	#define LGCML_UTIL_START_DISABLE_OVERRIDE_WARNING()										\
		_Pragma ( "clang diagnostic push" )													\
		_Pragma ( "clang diagnostic ignored \"-Winconsistent-missing-override\"" )			\
/**/
	#define LGCML_UTIL_END_DISABLE_OVERRIDE_WARNING()										\
		_Pragma ( "clang diagnostic pop" )													\
/**/
#elif ( BOOST_COMP_MSVC )
// may not be necessary, probably won't work if it is:
	#define LGCML_UTIL_START_DISABLE_OVERRIDE_WARNING()										\
//        __pragma( warning( push ) )														\
//        __pragma( warning( disable : ???? ) )												\
/**/
	#define LGCML_UTIL_END_DISABLE_OVERRIDE_WARNING()										\
//        __pragma( warning( pop ) )														\
/**/
#elif ( BOOST_COMP_GNUC )
// may not be necessary, probably won't work if it is:
	#define LGCML_UTIL_START_DISABLE_OVERRIDE_WARNING()										\
//        _Pragma ( "GCC diagnostic push" )													\
//        _Pragma ( "GCC diagnostic ignored \"-W????????\"" )								\
/**/
	#define LGCML_UTIL_END_DISABLE_OVERRIDE_WARNING()										\
//        _Pragma ( "GCC diagnostic pop" )													\
/**/
#endif


// disable warning for unused values;

#if ( BOOST_COMP_CLANG )
	#define LGCML_UTIL_START_DISABLE_UNUSED_VALUE_WARNING()									\
		_Pragma ( "clang diagnostic push" )													\
		_Pragma ( "clang diagnostic ignored \"-Wunused-value\"" )							\
/**/
	#define LGCML_UTIL_END_DISABLE_UNUSED_VALUE_WARNING()									\
		_Pragma ( "clang diagnostic pop" )													\
/**/
#elif ( BOOST_COMP_MSVC )
// may not be necessary, certainly won't work if it is:
	#define LGCML_UTIL_START_DISABLE_UNUSED_VALUE_WARNING()									\
//        __pragma( warning( push ) )														\
//        __pragma( warning( disable : ???? ) )												\
/**/
	#define LGCML_UTIL_END_DISABLE_UNUSED_VALUE_WARNING()									\
//        __pragma( warning( pop ) )														\
/**/
#elif ( BOOST_COMP_GNUC )
// may not be necessary, certainly won't work if it is:
	#define LGCML_UTIL_START_DISABLE_UNUSED_VALUE_WARNING()									\
//        _Pragma ( "GCC diagnostic push" )													\
//        _Pragma ( "GCC diagnostic ignored \"-W????????\"" )								\
/**/
	#define LGCML_UTIL_END_DISABLE_UNUSED_VALUE_WARNING()									\
//        _Pragma ( "GCC diagnostic pop" )													\
/**/
#endif

#endif    // LOGICMILL_UTIL_MACROS_H