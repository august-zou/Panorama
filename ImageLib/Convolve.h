///////////////////////////////////////////////////////////////////////////
//
// NAME
//  Convolve.h -- separable and non-separable linear convolution
//
// SPECIFICATION
//  void Convolve(CImageOf<T> src, CImageOf<T>& dst,
//                CFloatImage kernel);
//
//  void ConvolveSeparable(CImageOf<T> src, CImageOf<T>& dst,
//                         CFloatImage xKernel, CFloatImage yKernel,
//                         int subsample);
//
// PARAMETERS
//  src                 source image
//  dst                 destination image
//  kernel              2-D convolution kernel
//  xKernel, yKernel    1-D convolution kernels (1-row images)
//  subsample           subsampling/decimation factor (1 = none, 2 = half, ...)
//
// DESCRIPTION
//  Perform a 2D or separable 1D convolution.  The convolution kernels
//  are supplied as 2D floating point images  (1 row images for the separable
//  kernels).  The position of the "0" pixel in the kernel is determined
//  by the kernel.origin[] parameters, which specify the offset (coordinate,
//  usually negative) of the first (top-left) pixel in the kernel.
//
// SEE ALSO
//  Convolve.cpp        implementation
//  Image.h             image class definition
//
// Copyright © Richard Szeliski, 2001.  See Copyright.h for more details
//
///////////////////////////////////////////////////////////////////////////

template <class T>
void Convolve(CImageOf<T> src, CImageOf<T>& dst,
              CFloatImage kernel);

template <class T>
void ConvolveSeparable(CImageOf<T> src, CImageOf<T>& dst,
                       CFloatImage xKernel, CFloatImage yKernel,
                       int subsample);

extern CFloatImage ConvolveKernel_121;
extern CFloatImage ConvolveKernel_14641;
extern CFloatImage ConvolveKernel_7x7;
extern CFloatImage ConvolveKernel_8tapLowPass;
extern CFloatImage ConvolveKernel_SobelX;
extern CFloatImage ConvolveKernel_SobelY;
