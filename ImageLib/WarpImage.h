///////////////////////////////////////////////////////////////////////////
//
// NAME
//  WarpImage.h -- warp an image either through a global parametric
//      transform or a local (per-pixel) transform
//
// SPECIFICATION
//  void WarpLocal(CImageOf<T> src, CImageOf<T>& dst,
//                 CFloatImage uv, bool relativeCoords,
//                 WarpInterpolationMode interp);
//
//  void WarpGlobal(CImageOf<T> src, CImageOf<T>& dst,
//                  CTransform3x3 M,
//                  WarpInterpolationMode interp);
//
// PARAMETERS
//  src                 source image
//  dst                 destination image
//  uv                  source pixel coordinates array/image
//  relativeCoords      source coordinates are relative (offsets = "flow")
//  interp              interpolation mode (nearest neighbor, bilinear, bicubic)
//  cubicA              parameter controlling cubic interpolation
//  M                   global 3x3 transformation matrix
//
// DESCRIPTION
//  WarpLocal preforms an inverse sampling of the source image into the
//  destination image.  In other words, for every pixel in dst, the pixel
//  in src at address uv is sampled (and interpolated, if required).
//
//  If any of the pixels involved in the interpolation are outside the
//  addressable region of src, the corresponding pixel in dst is set to all 0s.
//  Note that for cubic interpololation, this may result in significant loss
//  of pixels near the edges.  (Even for linear sampling with an integer
//  shift, the rightmost column will be lost.)
//
//  WarpGlobal performs a similar resampling, except that the transformation
//  is specified by a simple matrix that can be used to represent rigid,
//  affine, or perspective transforms.
//
//
// SEE ALSO
//  WarpImage.cpp       implementation
//  Image.h             image class definition
//
// Copyright © Richard Szeliski, 2001.  See Copyright.h for more details
//
///////////////////////////////////////////////////////////////////////////

#include<math.h>
#include<vector>

enum EWarpInterpolationMode
{
    eWarpInterpNearest   = 0,    // nearest neighbor
    eWarpInterpLinear    = 1,    // bi-linear interpolation
    eWarpInterpCubic     = 3     // bi-cubic interpolation
};

static const int cubicLUTsize = 256;
static float cubicInterp[cubicLUTsize][4];


static inline float ResampleCubic(float v0, float v1, float v2, float v3, float f)
{
    int fi = int(f*cubicLUTsize);
    float *c = cubicInterp[fi];
    float v = c[0]*v0 + c[1]*v1 + c[2]*v2 + c[3]*v3;
    return v;
}


//
//  Bicubic interpolation: cascade horizontal and vertical resamplings
//

template <class T>
static T ResampleBiCubic(T src[], int oH, int oV, float xf, float yf)
{
    // Resample a pixel using bilinear interpolation
    float h[4];
    for (int i = 0; i < 4; i++)
    {
        int j = (i-1)*oV;
        h[i] = ResampleCubic(src[j-oH], src[j], src[j+oH], src[j+2*oH], xf);
    }
    float  v = ResampleCubic(h[0], h[1], h[2], h[3], yf);
    return (T) v;
}

static inline float ResampleLinear(float v0, float v1, float f)
{
    return v0 + f * (v1 - v0);
}


//
//  Bilinear interpolation: cascade horizontal and vertical resamplings
//

template <class T>
static inline T ResampleBiLinear(T src[], int oH, int oV, float xf, float yf)
{
    // Resample a pixel using bilinear interpolation
    float h1 = ResampleLinear(src[0 ], src[   oH], xf);
    float h2 = ResampleLinear(src[oV], src[oV+oH], xf);
    float  v = ResampleLinear(h1, h2, yf);
    return (T) v;
}

static void InitializeCubicLUT(float a)
{
    float zero = 0.0;      // not implemented yet
    float error = 1.0f / zero;
}

template <class T>
void WarpLocal(CImageOf<T> src, CImageOf<T>& dst,
               CFloatImage uv, bool relativeCoords,
               EWarpInterpolationMode interp, float cubicA = 1.0);




//
//  Resample a complete image, given dst->src pixel transformation
//
template <class T>
void WarpGlobal(CImageOf<T> src, CImageOf<T>& dst,
                CTransform3x3 M,
                EWarpInterpolationMode interp, float cubicA = 1.0);

//
//  Resample a complete line, given the source pixel addresses
//
template <class T>
void WarpLine(CImageOf<T> src, T* dstP, float *xyP, int n, int nBands,
              EWarpInterpolationMode interp, T minVal, T maxVal)
{
    // Determine the interpolator's "footprint"
    const int o0 = int(interp)/2;       // negative extent
    const int o1 = int(interp) - o0;    // positive extent
    const int oH = nBands;              // horizonal offset between pixels
    const int oV = &src.Pixel(0, 1, 0) -
                   &src.Pixel(0, 0, 0); // vertical  offset between pixels
    CShape sh = src.Shape();

    // Resample a single output scanline
    for (int i = 0; i < n; i++, dstP += nBands, xyP += 2)
    {
        // Round down pixel coordinates
        int x = int(floor(xyP[0]));
        int y = int(floor(xyP[1]));

        // Check if all participating pixels are in bounds
        if (! (sh.InBounds(x-o0, y-o0) && sh.InBounds(x+o1, y+o1)))
        {
            for (int j = 0; j < nBands; j++)
                dstP[j] = 0;
            continue;
        }
        T* srcP = &src.Pixel(x, y, 0);

        // Nearest-neighbor: just copy pixels
        if (interp == eWarpInterpNearest)
        {
            for (int j = 0; j < nBands; j++)
                dstP[j] = srcP[j];
            continue;
        }

        float xf = xyP[0] - x;
        float yf = xyP[1] - y;

        // Bilinear and bi-cubic
        if (interp == eWarpInterpLinear)
        {
            for (int j = 0; j < nBands; j++)
                dstP[j] = __max(minVal, __min(maxVal,
                    ResampleBiLinear(&srcP[j], oH, oV, xf, yf)));
        }
        if (interp == eWarpInterpCubic)
        {
            for (int j = 0; j < nBands; j++)
                dstP[j] = __max(minVal, __min(maxVal,
                    ResampleBiCubic(&srcP[j], oH, oV, xf, yf)));
        }
    }
}


template <class T>
void WarpGlobal(CImageOf<T> src, CImageOf<T>& dst,
                CTransform3x3 M,
                EWarpInterpolationMode interp, float cubicA)

{
    // Not implemented yet, since haven't decided on semantics of M yet...

    // Check that dst is of a valid shape
    if (dst.Shape().width == 0)
        dst.ReAllocate(src.Shape());
    CShape sh = dst.Shape();

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
        float *xyP  = &rowBuf[0];
        T *dstP     = &dst.Pixel(0, y, 0);

        // Compute pixel coordinates
        float X0 = (float) (M[0][1]*y + M[0][2]);
        float dX = (float) M[0][0];
        float Y0 = (float) (M[1][1]*y + M[1][2]);
        float dY = (float) M[1][0];
        float Z0 = (float) (M[2][1]*y + M[2][2]);
        float dZ = (float) M[2][0];
        bool affine = (dZ == 0.0);
        float Zi = 1.0f / Z0;           // TODO:  doesn't guard against divide by 0
        if (affine)
        {
            X0 *= Zi, dX *= Zi, Y0 *= Zi, dY *= Zi;
        }
        for (int x = 0; x < n; x++)
        {
            xyP[2*x+0] = X0 * Zi;
            xyP[2*x+1] = Y0 * Zi;
            X0 += dX;
            Y0 += dY;
            if (! affine)
            {
                Z0 += dZ;
                Zi = 1.0f / Z0;
            }
        }

        // Resample the line
        WarpLine(src, dstP, xyP, n, sh.nBands, interp, src.MinVal(), src.MaxVal());
    }
}


