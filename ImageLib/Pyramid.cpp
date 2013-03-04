///////////////////////////////////////////////////////////////////////////
//
// NAME
//  Pyramid.cpp -- dynamic image pyramid
//
// SEE ALSO
//  Pyramid.h           longer description
//
// Copyright ?Richard Szeliski, 2001.  See Copyright.h for more details
//
///////////////////////////////////////////////////////////////////////////

#include "Image.h"
#include <vector>
#include "Pyramid.h"
#include "Convolve.h"

template <class T>
CPyramidOf<T>::CPyramidOf()
{
    decimateKernel    = ConvolveKernel_14641;
    interpolateKernel = ConvolveKernel_14641;
}


template <class T>
CPyramidOf<T>::CPyramidOf(CImageOf<T> image)
{
    decimateKernel    = ConvolveKernel_14641;
    interpolateKernel = ConvolveKernel_14641;
    m_image.push_back(image);
}

template <class T>
void CPyramidOf<T>::SetLevel(int l, CImageOf<T> image)
{
    // Re-assign new image to level
    if (m_image.size() <= (unsigned int)l)
        m_image.resize(l+1);
    m_image[l] = image;
    InvalidateAbove(l);
}

template <class T>
void CPyramidOf<T>::InvalidateAbove(int l)
{
    // Invalidate coarser (higher) levels
    m_image.resize(l+1);    // just get rid of the higher levels
}

template <class T>
void CPyramidOf<T>::UpLevel(int l, int n_levels)
{
    // Decimate to coarser levels
    if (n_levels <= 0)
        return;

    if (m_image.size() <= (unsigned int)l+1)
        m_image.resize(l+2);
    CImageOf<T>& src = m_image[l];
    CImageOf<T>& dst = m_image[l+1];
    ConvolveSeparable(src, dst, decimateKernel, decimateKernel, 2);

    if (n_levels > 1)
        UpLevel(l+1, n_levels-1);
}

template <class T>
void CPyramidOf<T>::DownLevel(int l, int n_levels)
{
    // Interpolate finer levels
    throw CError("CPyramidOf<T>:DownLevel is not yet implemented");
}

template <class T>
CImageOf<T>& CPyramidOf<T>::operator[](int l)
{
    // Return image at level l
    if (m_image.size() <= (unsigned int)l)
        m_image.resize(l+1);
    CImageOf<T>& img = m_image[l];
    if (l > 0 && img.Shape().nBands == 0)        // un-initialized
    {
        (*this)[l-1];   // force construction of lower-level image
        UpLevel(l-1, 1);
    }
    return img;
}

template <class T>
void InstantiatePyramid(CPyramidOf<T> p)
{
    CImageOf<T> img;
    CPyramidOf<T> q(img);
    p.SetLevel(0, img);
    p[2];
    p.DownLevel(0, 1);
}


void InstantiatePyramids()
{
    InstantiatePyramid(CBytePyramid());
    InstantiatePyramid(CIntPyramid());
    InstantiatePyramid(CFloatPyramid());
}


#if 0   // doesn't seem to do anything:

//  Explicit template instantiation
template<> class CPyramidOf<uchar> { };
template<> class CPyramidOf<int>   { };
template<> class CPyramidOf<float> { };
#endif