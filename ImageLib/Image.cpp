///////////////////////////////////////////////////////////////////////////
//
// NAME
//  Image.cpp -- a simple reference-counted image structure
//
// SEE ALSO
//  Image.h             definition and explanation of these classes
//
// Copyright © Richard Szeliski, 2001.  See Copyright.h for more details
//
///////////////////////////////////////////////////////////////////////////

#include "Image.h"

//
// struct CShape: shape of image (width x height x nbands)
//

bool CShape::operator==(const CShape& ref)
{
    // Are two shapes the same?
    return (width  == ref.width &&
            height == ref.height &&
            nBands == ref.nBands);
}

bool CShape::SameIgnoringNBands(const CShape& ref)
{
    // Are two shapes the same ignoring the number of bands?
    return (width  == ref.width &&
            height == ref.height);
}

bool CShape::operator!=(const CShape& ref)
{
    // Are two shapes not the same?
    return ! ((*this) == ref);
}


//
// class CImage : generic (weakly typed) image
//

void CImage::SetDefaults()
{
    // Set internal state to default values
    m_pTI = 0;              // pointer to type_info class
    m_bandSize = 0;         // size of each band in bytes
    m_pixSize = 0;          // stride between pixels in bytes
    m_rowSize = 0;          // stride between rows in bytes
    m_memStart = 0;         // start of addressable memory

    // Set default attribute values
    alphaChannel = 3;       // which channel contains alpha (for compositing)
    origin[0] = 0;          // x and y coordinate origin (for some operations)
    origin[1] = 0;          // x and y coordinate origin (for some operations)
    borderMode = eBorderReplicate;   // border behavior for neighborhood operations...
}

CImage::CImage()
{
    // Default constructor
    SetDefaults();
}

CImage::CImage(CShape s, const type_info& ti, int cS)
{
    SetDefaults();
    ReAllocate(s, ti, cS, 0, true, 0);
}

void CImage::ReAllocate(CShape s, const type_info& ti, int bandSize,
                        bool evenIfShapeDiffers)
{
    if (! evenIfShapeDiffers && s == m_shape && ti == *m_pTI && bandSize == m_bandSize)
        return;
    ReAllocate(s, ti, bandSize, 0, true, 0);
}

void CImage::ReAllocate(CShape s, const type_info& ti, int bandSize,
                        void *memory, bool deleteWhenDone, int rowSize)
{
    // Set up the type_id, shape, and size info
    m_shape     = s;                        // image shape (dimensions)
    m_pTI       = &ti;                      // pointer to type_info class
    m_bandSize  = bandSize;                 // size of each band in bytes
    m_pixSize   = m_bandSize * s.nBands;    // stride between pixels in bytes

    // Do the real allocation work
    m_rowSize   = (rowSize) ? m_pixSize*rowSize :     // stride between rows in bytes
                  (m_pixSize * s.width + 7) & -8;     // round up to 8 (quadwords)
    int nBytes  = m_rowSize * s.height;
    if (memory == 0 && nBytes > 0)          // allocate if necessary
    {
        memory = new double[(nBytes + 7)/ 8];
        if (memory == 0)
            throw CError("CImage::Reallocate: could not allocate %d bytes", nBytes);
    }
    m_memStart = (char *) memory;           // start of addressable memory
    m_memory.ReAllocate(nBytes, memory, deleteWhenDone);
}

void CImage::DeAllocate()
{
    // Release the memory & set to default values
    ReAllocate(CShape(), *(const type_info *) 0, 0, 0, false, 0);
    SetDefaults();
}


void CImage::SetSubImage(int xO, int yO, int width, int height)
{
    // NOTE:  the subimage is with respect to the rectangle specified
    //  by the origin and current shape
    int x = xO - origin[0];
    int y = yO - origin[1];

    // Adjust the start of memory pointer
    m_memStart = (char *) PixelAddress(x, y, 0);

    // Compute area of intersection and adjust the shape and origin
    int x1 = __min(m_shape.width,  x+width);    // end column
    int y1 = __min(m_shape.height, y+height);   // end row
    x = __max(0, __min(x, m_shape.width));      // clip to original shape
    y = __max(0, __min(y, m_shape.height));     // clip to original shape
    m_shape.width  = x1 - x;                    // actual width
    m_shape.height = y1 - y;                    // actual height
    origin[0] += x1;                            // adjust the origin
    origin[1] += y1;                            // adjust the origin
}

void CImage::ClearPixels(void)
{
    // Set all the pixels to 0
    for (int y = 0; y < m_shape.height; y++)
        memset(PixelAddress(0, y, 0), 0, m_pixSize * m_shape.width);
}

//
// class CImageOf<T>: strongly typed image
//

#include <float.h>
#ifndef FLT_MAX
#define FLT_MAX         3.402823466e+38F        /* max value */
#define FLT_MIN         1.175494351e-38F        /* min positive value */
#endif

template<> uchar CImageOf<uchar>::MinVal(void)     { return 0; }
template<> uchar CImageOf<uchar>::MaxVal(void)     { return 255; }
template<> int   CImageOf<int  >::MinVal(void)     { return -1 ^ (1 << 31); }
template<> int   CImageOf<int  >::MaxVal(void)     { return  0 ^ (1 << 31); }
template<> float CImageOf<float>::MinVal(void)     { return -FLT_MAX; }
template<> float CImageOf<float>::MaxVal(void)     { return FLT_MAX; }
