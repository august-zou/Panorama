///////////////////////////////////////////////////////////////////////////
//
// NAME
//  Image.inl -- a simple reference-counted image structure
//
// DESCRIPTION
//  The incline function definitions to go with Image.h
//
// SEE ALSO
//  Image.h             more complete description
//
// Copyright © Richard Szeliski, 2001.  See Copyright.h for more details
//
///////////////////////////////////////////////////////////////////////////

//
// struct CShape: shape of image (width x height x nbands)
//

inline bool CShape::InBounds(int x, int y)
{
    // Is given pixel address valid?
    return (0 <= x && x < width &&
            0 <= y && y < height);
}

inline bool CShape::InBounds(int x, int y, int b)
{
    // Is given pixel address valid?
    return (0 <= x && x < width &&
            0 <= y && y < height &&
            0 <= b && b < nBands);
}

//
// class CImage : generic (weakly typed) image
//

inline void* CImage::PixelAddress(int x, int y, int band)
{
    // This could also go into the implementation file (CImage.cpp):
    return (void *) &m_memStart[y * m_rowSize + x * m_pixSize + band * m_bandSize];
}

//  Strongly typed image;
//    these are defined inline so user-defined image types can be supported:

template <class T>
inline CImageOf<T>::CImageOf(void) :
CImage(CShape(), typeid(T), sizeof(T)) {}

template <class T>
inline CImageOf<T>::CImageOf(CShape s) :
CImage(s, typeid(T), sizeof(T)) {}

template <class T>
inline CImageOf<T>::CImageOf(int width, int height, int nBands) :
CImage(CShape(width, height, nBands), typeid(T), sizeof(T)) {}

template <class T>
inline void CImageOf<T>::ReAllocate(CShape s, bool evenIfShapeDiffers)
{
    CImage::ReAllocate(s, typeid(T), sizeof(T), evenIfShapeDiffers);
}

template <class T>
inline void CImageOf<T>::ReAllocate(CShape s, T *memory,
                                    bool deleteWhenDone, int rowSize)
{
    CImage::ReAllocate(s, typeid(T), sizeof(T), memory, deleteWhenDone, rowSize);
}
    
template <class T>
inline T& CImageOf<T>::Pixel(int x, int y, int band)
{
    return *(T *) PixelAddress(x, y, band);
}

template <class T>
inline CImageOf<T> CImageOf<T>::SubImage(int x, int y, int width, int height)
{
    // sub-image sharing memory
    CImageOf<T> retval = *this;
    retval.SetSubImage(x, y, width, height);
    return retval;
}

template <class T>
inline double CImageOf<T>::PixelLerp(double x, double y, int band)
{
    if (x < 0 || y < 0 || x > Shape().width || y > Shape().height)
        printf("Lerp called out of bounds: (%0.3f, %0.3f) ==> (%d, %d)\n", 
	       x, y, Shape().width, Shape().height);

    int xf = (int) x;
    int yf = (int) y;

    double t = x - xf;
    double u = y - yf;

    T p1 = Pixel(xf, yf, band);
    T p2 = Pixel(xf+1, yf, band);
    T p3 = Pixel(xf, yf+1, band);
    T p4 = Pixel(xf+1, yf+1, band);

    return (1.0 - u) * ((1.0 - t) * p1 + t * p2) + 
                  u  * ((1.0 - t) * p3 + t * p4);
}
