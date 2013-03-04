///////////////////////////////////////////////////////////////////////////
//
// NAME
//  WarpImage.cpp -- warp an image either through a global parametric
//      transform or a local (per-pixel) transform
//
// SEE ALSO
//  WarpImage.h         longer description of functions
//
// DESIGN
//  For cubic interpolation, we have a choice of several (variants of)
//  interpolation functions.  In general, we can use any family
//  of 4-tap filters.  In particular, we will use piecewise-cubic
//  functions that are interpolating and constant and linear preserving.
//
//  We furthermore make the restriction that the interpolant is C1
//  (not all interpolants do this).  This leaves us with only one
//  degree of freedom:  the slope at (x=1), which we call "a".
//
//  For the implementation, we form a LUT for the cubic interpolation
//  function, and initialize it at the beginning of each image warp
//  (in case "a" has changed).
//
// Copyright ?Richard Szeliski, 2001.  See Copyright.h for more details
//
///////////////////////////////////////////////////////////////////////////

#include "Image.h"
#include "Transform.h"
#include "WarpImage.h"
#include <math.h>
#include <vector>


//
//  Resample a complete image, given source pixel addresses
//

template <class T>
void WarpLocal(CImageOf<T> src, CImageOf<T>& dst,
               CFloatImage uv, bool relativeCoords,
               EWarpInterpolationMode interp, float cubicA)
{
    // Check that dst is of the right shape
    CShape sh(uv.Shape().width, uv.Shape().height, src.Shape().nBands);
    dst.ReAllocate(sh);

    // Allocate a row buffer for coordinates
    int n = sh.width;
    std::vector<float> rowBuf;
    rowBuf.resize(n*2);

    // Precompute the cubic interpolant
    if (interp == eWarpInterpCubic)
        InitializeCubicLUT(cubicA);

    // Process each row
    for (int y = 0; y < sh.height; y++)
    {
        float *uvP  = &uv .Pixel(0, y, 0);
        float *xyP  = (relativeCoords) ? &rowBuf[0] : uvP;
        T *dstP     = &dst.Pixel(0, y, 0);

        // Convert to absolute coordinates if necessary
        if (relativeCoords)
        {
            for (int x = 0; x < n; x++)
            {
                xyP[2*x+0] = x + uvP[2*x+0];
                xyP[2*x+1] = y + uvP[2*x+1];
            }
        }

        // Resample the line
        WarpLine(src, dstP, xyP, n, sh.nBands, interp, src.MinVal(), src.MaxVal());
    }
}

template void WarpLocal(CImageOf<float> src, CImageOf<float>& dst,
                        CFloatImage uv, bool relativeCoords,
                        EWarpInterpolationMode interp, float cubicA);

template void WarpLocal(CImageOf<uchar> src, CImageOf<uchar>& dst,
                        CFloatImage uv, bool relativeCoords,
                        EWarpInterpolationMode interp, float cubicA);

// Instantiate the code

void WarpInstantiate(void)
{
    CByteImage i1;
    CFloatImage uv1;
    WarpLocal(i1, i1, uv1, false, eWarpInterpLinear, 1.0f);
    CTransform3x3 M;
    WarpGlobal(i1, i1, M, eWarpInterpLinear, 1.0f);
}
