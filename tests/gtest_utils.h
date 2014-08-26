#ifndef _GTEST_UTILS
#define _GTEST_UTILS
#include "../Renderer/Backend/Backend.h"
//bool image_match(RGBPixel* lhs, RGBPixel* rhs, uint16_t w, uint16_t h);
bool image_match(const char* lhs, const char* rhs, uint16_t w, uint16_t);

#endif  // ~_GTEST_UTILS