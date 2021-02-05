/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

#define _RESULT(expr, exprorig, result) if (expr) { printf("  SUCCESS: %s\n", #exprorig); __pass++; } else printf("  FAILED: %s, actual: %u\n", #exprorig, result)
#define _EVAL(result, expected, op) do { __cnt++; uint32_t resval = ((uint32_t)(result)); uint32_t expval = ((uint32_t)(expected)); _RESULT(resval op expval, result op expected, resval); } while(0)
#define _HEAD(group, name) printf("======== %s::%s\n", group, name); __cnt = 0; __pass = 0
#define _FOOT(group, name) printf("======== %s::%s - %i PASSED, %i FAILED\n", group, name, __pass, (__cnt - __pass)); TotalTests += __cnt; TotalTestsPassed += __pass;
#define _CALL(group, name) do { _HEAD(#group, #name); _##group##_##name##_();  _FOOT(#group, #name); printf("\n"); } while(0)

#define TEST(group, test) static void _##group##_##test##_(void); static void _##group##_##test(void) { _CALL(group, test); } static void _##group##_##test##_(void)
#define CALL(group, test) _##group##_##test()
#define EXPECT_EQ(result, expected) _EVAL(result, expected, ==)
#define EXPECT_NE(result, expected) _EVAL(result, expected, !=)
#define EXPECT_LT(result, expected) _EVAL(result, expected, <)
#define EXPECT_LE(result, expected) _EVAL(result, expected, <=)
#define EXPECT_GT(result, expected) _EVAL(result, expected, >)
#define EXPECT_GE(result, expected) _EVAL(result, expected, >=)


#ifdef __cplusplus
extern "C" {
#endif

extern int __cnt;
extern int __pass;

extern int TotalTests ;
extern int TotalTestsPassed;

#ifdef __cplusplus
}
#endif
