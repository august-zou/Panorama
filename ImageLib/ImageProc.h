#pragma once

#include "Image.h"

//
// Type and band conversion routines
//

template <class T1, class T2>
void TypeConvert(CImageOf<T1>& src, CImageOf<T2>& dst,
                 float scale = 1.0f, float offset = 0.0f);

//
// Helper functions for point and neighborhood processing
//

void PointProcess1(CImage& img1, void* dataPtr,
                   bool (*fn)(int n, CImage **iptrs, void* dataPtr,
                              void* p1, int b1));

void PointProcess2(CImage& img1, CImage& img2, void* dataPtr,
                   bool (*fn)(int n, CImage **iptrs, void* dataPtr,
                              void* p1, int b1,
                              void* p2, int b2));

void NeighborhoodProcess(CImage& src, CImage& dst, void* dataPtr,
                   int halfWidth, int halfHeight,
                   bool reverseRaster,
                   bool (*fn)(int n, CImage **iptrs, void* dataPtr,
                              void* p1, int b1,
                              void* p2, int b2));

void NeighborhoodProcessSeparable(CImage& src, CImage& dst, void* dataPtr,
                   int halfWidth, int halfHeight,
                   bool reverseRaster,
                   bool (*f1)(int n, CImage **iptrs, void* dataPtr,
                              void* p1, int b1,
                              void* p2, int b2),
                   bool (*f2)(int n, CImage **iptrs, void* dataPtr,
                              void* p1, int b1,
                              void* p2, int b2));

//
// Miscellaneous utility routines
//

CImage Rotate90(CImage img, int nTimesCCW);
