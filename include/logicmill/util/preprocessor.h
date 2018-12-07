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

/* 
 * File:   preprocessor.h
 * Author: David Curtis
 *
 * Created on July 5, 2017, 11:21 AM
 */

#ifndef LOGICMILL_UTILS_PREPROCESSOR_H
#define LOGICMILL_UTILS_PREPROCESSOR_H

#include <logicmill/util/macro_switches.h>

#define LGCML_PP_ARG16_(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, ...) _15
#define LGCML_PP_HAS_COMMA_(...) LGCML_PP_ARG16_(__VA_ARGS__, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0)
#define LGCML_PP_TRIGGER_PARENTHESIS_(...) ,

#define LGCML_PP_ISEMPTY(...)                                                    \
LGCML_PP_ISEMPTY_(                                                               \
          /* test if there is just one argument, eventually an empty    \
             one */                                                     \
          LGCML_PP_HAS_COMMA_(__VA_ARGS__),                                       \
          /* test if LGCML_PP_TRIGGER_PARENTHESIS_ together with the argument   \
             adds a comma */                                            \
          LGCML_PP_HAS_COMMA_(LGCML_PP_TRIGGER_PARENTHESIS_ __VA_ARGS__),                 \
          /* test if the argument together with a parenthesis           \
             adds a comma */                                            \
          LGCML_PP_HAS_COMMA_(__VA_ARGS__ (/*empty*/)),                           \
          /* test if placing it between LGCML_PP_TRIGGER_PARENTHESIS_ and the   \
             parenthesis adds a comma */                                \
          LGCML_PP_HAS_COMMA_(LGCML_PP_TRIGGER_PARENTHESIS_ __VA_ARGS__ (/*empty*/))      \
          )
          
#define LGCML_PP_PASTE5_(_0, _1, _2, _3, _4) _0 ## _1 ## _2 ## _3 ## _4
#define LGCML_PP_ISEMPTY_(_0, _1, _2, _3) LGCML_PP_HAS_COMMA_(LGCML_PP_PASTE5_(LGCML_PP_IS_EMPTY_CASE_, _0, _1, _2, _3))
#define LGCML_PP_IS_EMPTY_CASE_0001 ,

#define LGCML_PP_ECHO(...) __VA_ARGS__

#define LGCML_PP_WRAP_ECHO(...) (__VA_ARGS__)

#define LGCML_PP_DO_NOTHING(...)

#if (LGCML_USE_SHORT_MACRO_NAMES)
    #define ECHO LGCML_PP_ECHO
	#define WRAP_ECHO LGCML_PP_WRAP_ECHO
    #define DO_NOTHING LGCML_PP_DO_NOTHING
	#define IS_EMPTY LGCML_PP_ISEMPTY
#endif


#endif /* LOGICMILL_UTILS_PREPROCESSOR_H */

