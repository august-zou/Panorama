///////////////////////////////////////////////////////////////////////////
//
// NAME
//  Convert.h -- convert between image types, copy images, select bands
//
// DESCRIPTION
//  See Convert.h for a full description
//
// SEE ALSO
//  Convert.h           full description
//
// Copyright ?Richard Szeliski, 2001.  See Copyright.h for more details
//
///////////////////////////////////////////////////////////////////////////

#include "Image.h"
#include "Convert.h"

template <class T1, class T2>
void ScaleAndOffsetLine(T1* src, T2* dst, int n,
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

template <class T1, class T2>
void ScaleAndOffset(CImageOf<T1>& src, CImageOf<T2>& dst, float scale, float offset)
{
    // Convert between images of same shape but diffent types,
    //  and optionally scale and/or offset the pixel values
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
        ScaleAndOffsetLine(&src.Pixel(0, y, 0), &dst.Pixel(0, y, 0),
                           sShape.width*sShape.nBands, scale, offset, minVal, maxVal);
    }
}

template <class T>
CImageOf<T> ConvertToRGBA(CImageOf<T> src)
{
    // Check if already RGBA
    CShape sShape = src.Shape();
    if (sShape.nBands == 4 && src.alphaChannel == 3)
        return src;

    // Make sure the source is a gray image
    if (sShape.nBands != 1)
        throw CError("ConvertToRGBA: can only convert from 1-band (gray) image");

    // Allocate the new image
    CShape dShape(sShape.width, sShape.height, 4);
    CImageOf<T> dst(dShape);

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
    return dst;
}

template <class T>
CImageOf<T> ConvertToGray(CImageOf<T> src)
{
    // Check if already gray
    CShape sShape = src.Shape();
    if (sShape.nBands == 1)
        return src;

#if 0
    // Make sure the source is a color image
    if (sShape.nBands != 4 || src.alphaChannel != 3)
        throw CError("ConvertToGray: can only convert from 4-band (RGBA) image");
#else
    // Make sure the source is a color image
    if (sShape.nBands != 3)
        throw CError("ConvertToGray: can only convert from 3-band (RGB) image");
#endif

    // Allocate the new image
    CShape dShape(sShape.width, sShape.height, 1);
    CImageOf<T> dst(dShape);

    // Process each row
    T minVal = dst.MinVal();
    T maxVal = dst.MaxVal();
    for (int y = 0; y < sShape.height; y++)
    {
        T* srcP = &src.Pixel(0, y, 0);
        T* dstP = &dst.Pixel(0, y, 0);
        for (int x = 0; x < sShape.width; x++, srcP += 3/*4*/, dstP++)
        {
            RGBA<T>& p = *(RGBA<T> *) srcP;
            float Y = 0.212671f * p.R + 0.715160f * p.G + 0.072169f * p.B;
            *dstP = (T) __min(maxVal, __max(minVal, Y));
        }
    }
    return dst;
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
// Force instantiation for the types we care about (uchar, int, float)
//

template <class T1>
void CopyPixelsInstantiate(CImageOf<T1> s1)
{
    CByteImage  b2;
    CIntImage   i2;
    CFloatImage f2;
    CopyPixels(s1, b2);
    CopyPixels(s1, i2);
    CopyPixels(s1, f2);
}

template <class T>
void InstantiateConvert(CImageOf<T> src)
{
    CopyPixelsInstantiate(src);
    CImageOf<T> r1 = ConvertToRGBA(src);
    CImageOf<T> r2 = ConvertToGray(src);
    BandSelect(r1, r2, 0, 0);
}

void InstantiateAllConverts(void)
{
    InstantiateConvert(CByteImage());
    InstantiateConvert(CIntImage());
    InstantiateConvert(CFloatImage());
}