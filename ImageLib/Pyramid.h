///////////////////////////////////////////////////////////////////////////
//
// NAME
//  Pyramid.h -- dynamic image pyramid
//
// DESCRIPTION
//  The templated CPyramidOf<T> class holds a sequence of image of
//  decreasing size.
//
//  A pyramid can either be constructed from an image, or images can
//  be added to pyramid levels at a later time.
//
//  The user can access images in the pyramid at any level.
//  If the image does not already exist, it is constructed by
//  decimating the image at the next finer (lower) level
//  (recursively, if necessary).
//
//  Coarser levels can also be interpolated to finer (lower) levels,
//  but this requires and explicit DownLevel() invocation.
//
// SEE ALSO
//  Pyramid.cpp         implementation
//  Image.h             image class definition
//
// Copyright © Richard Szeliski, 2001.  See Copyright.h for more details
//
///////////////////////////////////////////////////////////////////////////

struct CPyramidAttributes
{
    CFloatImage decimateKernel;     // decimation kernel
    CFloatImage interpolateKernel;  // interpolation kernel
};

template <class T>
class CPyramidOf : public CPyramidAttributes
{
public:
    CPyramidOf();
    CPyramidOf(CImageOf<T> image);              // create from an image

    void SetLevel(int l, CImageOf<T> image);    // re-assign new image to level
    void InvalidateAbove(int l);                // invalidate coarser (higher) levels
    void UpLevel(int l, int n_levels);          // decimate to coarser levels
    void DownLevel(int l, int n_levels);        // interpolate finer levels
    CImageOf<T>& operator[](int level);         // return image at level l

private:
    std::vector<CImageOf<T> > m_image;          // image at level l
};

// Commonly used types (supported in current implementation):

typedef CPyramidOf<uchar> CBytePyramid;
typedef CPyramidOf<int>   CIntPyramid;
typedef CPyramidOf<float> CFloatPyramid;
