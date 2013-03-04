#include "ImageProc.h"

//
// Type conversion utilities
//

template <class T1, class T2>
void TypeConvertTyped(T1* src, T2* dst, int n,
                      float scale, float offset,
                      T2 minVal, T2 maxVal)
{
    // This routine does NOT round values when converting from float to int
    const bool scaleOffset = (scale != 1.0f) || (offset != 0.0f);
    const bool clip = (minVal < maxVal);
    if (scaleOffset)
        for (int i = 0; i < n; i++)
        {
            float val = src[i] * scale + offset;
            if (clip)
                val = __min(__max(val, minVal), maxVal);
            dst[i] = (T2) val;
        }
    else if (clip)
        for (int i = 0; i < n; i++)
        {
            dst[i] = (T2) __min(__max(src[i], minVal), maxVal);
        }
    else if (typeid(T1) == typeid(T2))
        memcpy(dst, src, n*sizeof(T2));
    else
        for (int i = 0; i < n; i++)
        {
            dst[i] = (T2) src[i];
        }
}

#if 0
            else
                memcpy(dst, src, n*sizeof(uchar));
#endif

template <class T1, class T2>
void TypeConvert(CImageOf<T1>& src, CImageOf<T2>& dst, float scale, float offset)
{
    // Convert between images of same shape but diffent types
    CShape sShape = src.Shape();
    CShape dShape = dst.Shape();

    // Make sure the shapes (ignoring bands) are compatible
    if (sShape != dShape)
        dst.ReAllocate(sShape);

    // Determine if clipping is required
    T2 minVal = dst.MinVal();
    T2 maxVal = dst.MaxVal();
    if (minVal <= src.MinVal() && maxVal >= src.MaxVal())
        minVal = maxVal = 0;

    // Process each row
    for (int y = 0; y < sShape.height; y++)
    {
        TypeConvertLine(&src.Pixel(0, y, 0), &dst.Pixel(0, y, 0),
                        sShape.width, scale, offset, minVal, maxVal);
    }
}

template <class T>
void GrayToRGBA(CImageOf<T>& src, CImageOf<T>& dst)
{
    // Make sure the shapes (ignoring bands) are compatible
    CShape sShape = src.Shape();
    CShape dShape = dst.Shape();
    if (sShape.nBands != 1)
        throw CError("GrayToRGBA: source image is not single-banded");
    if (! sShape.SameIgnoringNBands(dShape))
    {
        dShape = CShape(sShape.width, sShape.height, 4);
        dst.ReAllocate(dShape);
    }

    // Process each row
    int aC = dst.alphaChannel;
    for (int y = 0; y < sShape.height; y++)
    {
        T* srcP = &src.Pixel(0, y, 0);
        T* dstP = &dst.Pixel(0, y, 0);
        for (int x = 0; x < sShape.width; x++, srcP++)
            for (int b = 0; b < dShape.nBands; b++, dstP++)
                *dstP = (b == aC) ? 255 : *srcP;
    }
}

template <class T>
void RGBAToGray(CImageOf<T>& src, CImageOf<T>& dst)
{
    // Make sure the shapes (ignoring bands) are compatible
    CShape sShape = src.Shape();
    CShape dShape = dst.Shape();
    if (sShape.nBands != 4)
        throw CError("RGBAToGray: source image is not 4-banded");
    if (! sShape.SameIgnoringNBands(dShape))
        dst.ReAllocate(CShape(sShape.width, sShape.height, 4));
    if (dShape.nBands != 4)
        throw CError("RGBAToGray: source image is not 4-banded");
    if (src.alphaChannel != 3)
        throw CError("RGBAToGray: source A is not in the 4th band");

    // Process each row
    T minVal = dst.MinVal();
    T maxVal = dst.MaxVal();
    for (int y = 0; y < sShape.height; y++)
    {
        T* srcP = &src.Pixel(0, y, 0);
        T* dstP = &dst.Pixel(0, y, 0);
        for (int x = 0; x < sShape.width; x++, srcP += 4, dstP++)
        {
            RGBA<T>& p = *(RGBA<T> *) srcP;
            float Y = 0.212671 * p.R + 0.715160 * p.G + 0.072169 * p.B;
            dstP = (T) __min(maxVal, __max(minVal, Y));
        }
    }
}

template <class T>
void BandSelect(CImageOf<T>& src, CImageOf<T>& dst, int sBand, int dBand)
{
    // Convert between images of same type but different # of bands
    CShape sShape = src.Shape();
    CShape dShape = dst.Shape();

    // Make sure the shapes (ignoring bands) are compatible
    if (! sShape.SameIgnoringNBands(dShape) || dShape.nBands == 0)
    {
        dShape.width  = sShape.width;
        dShape.height = sShape.height;
        dShape.nBands = (dShape.nBands) ? dShape.nBands : 1;
        dst.ReAllocate(dShape);
    }

    // Check the bands are valid
    int sB = sShape.nBands;
    int dB = dShape.nBands;
    if (sBand < 0 || sBand >= sB)
        throw CError("BandSelect: source band %d is invalid", sBand);
    if (dBand < 0 || dBand >= dB)
        throw CError("BandSelect: destination band %d is invalid", dBand);

    // Process each row
    for (int y = 0; y < sShape.height; y++)
    {
        T* srcP = &src.Pixel(0, y, 0);
        T* dstP = &dst.Pixel(0, y, 0);
        for (int x = 0; x < sShape.width; x++, srcP += sB, dstP += dB)
            dstP[dBand] = srcP[sBand];
    }
}

//
// Helper functions for point and neighborhood processing
//

void PointProcess1(CImage& img1,
                   bool (*fn)(int n, CImage **iptrs, void* p1, int b1))
{
    // Call the line processing function on each line
    CImage *ptr[1] = {&img1};   // pointer for getting at data
    CShape s1 = img1.Shape();
    int w  = s1.width;
    int h  = s1.height;
    int b1 = s1.nBands;
    for (int y = 0; y < s1.height; y++)
    {
        fn(w, ptr, img1.PixelAddress(0, y, 0), b1);
    }
}

//
// Miscellaneous utility routines
//

template <class PixType>
static void CopyPixels(PixType* p1, int s1, PixType* p2, int s2, int w)
{
    s1 /= sizeof(PixType);
    s2 /= sizeof(PixType);
    if (s2 == 1)
    {
        for (int x = 0; x < w; x++, p1 += s1, p2 += s2)
            *p2 = *p1;
    }
    else
    {
        for (int x = 0; x < w; x++, p1 += s1, p2 += s2)
            for (int b = 0; b < s2; b++)
                p2[b] = p1[b];
    }
}

CImage Rotate90(CImage img1, int nTimesCCW)
{
    // Allocate the result image
    nTimesCCW &= 0x3;   // 0, 1, 2, or 3
    CShape s1 = img1.Shape();
    CShape s2((nTimesCCW & 1) ? s1.height : s1.width,
              (nTimesCCW & 1) ? s1.width : s1.height, s1.nBands);
    CImage img2(s2, img1.PixType(), img1.BandSize());

    // Iterate over the output pixels
    //  TODO:  blocking this into tiles could be more efficient...
    for (int y = 0; y < s2.height; y++)
    {
        // Determine the starting location and stride
        int x0, y0, xd, yd;
        switch (nTimesCCW)
        {
        case 0: x0 = 0, y0 = y, xd = 1, yd = 0; break;
        case 1: x0 = s2.height-1-y, y0 = 0, xd = 0, yd = 1; break;
        case 2: x0 = s2.width-1, y0 = s2.height-1-y, xd = -1, yd = 0; break;
        case 3: x0 = y, y0 = s2.width-1, xd = 0, yd = -1; break;
        }
        void* p1a = img1.PixelAddress(x0,    y0,    0);
        void* p1b = img1.PixelAddress(x0+xd, y0+yd, 0);
        void* p2a = img2.PixelAddress(0,     y,     0);
        void* p2b = img2.PixelAddress(1,     y,     0);
        int stride1 = (char *) p1b - (char *) p1a;
        int stride2 = (char *) p2b - (char *) p2a;

        // Copy the pixels using the largest possible unit
        if ((stride2 & -4) == stride2)
            CopyPixels((int *) p1a, stride1, (int *) p2a, stride2, s2.width);
        else if ((stride2 & -2) == stride2)
            CopyPixels((short *) p1a, stride1, (short *) p2a, stride2, s2.width);
        else
            CopyPixels((char *) p1a, stride1, (char *) p2a, stride2, s2.width);
    }

    return img2;
}
