///////////////////////////////////////////////////////////////////////////
//
// NAME
//  Convolve.cpp -- separable and non-separable linear convolution
//
// DESIGN NOTES
//  This version of the code calls out to IPL.
//
//  The separable version could be made more efficient when subsampling
//  by subsampling after each of the two stages of the separable convolution.
//
// SEE ALSO
//  Convolve.h          longer description of these routines
//
// Copyright © Richard Szeliski, 2001.  See Copyright.h for more details
//
///////////////////////////////////////////////////////////////////////////

#include "Image.h"
#include "Convolve.h"
#include "IPLInterface.h"
#include <vector>

IplConvKernel* IPLConvKernel(CFloatImage kernel, bool vertical = false)
{
    // Convert from a floating point image to an integer kernel
    const int nShiftR = 8;      // scale result by shifting to the right
    const float scale = (1 << nShiftR);
    CShape sh = kernel.Shape();
    std::vector<int> values;         // temporary storage for values
    values.resize(sh.width * sh.height);
    for (int y = 0, k = 0; y < sh.height; y++)
        for (int x = 0; x < sh.width; x++, k++)
            values[k] = int(kernel.Pixel(x, y, 0) * 256.0f + 0.5f);
    int nCols   = (vertical) ? sh.height : sh.width;
    int nRows   = (vertical) ? sh.width  : sh.height;
    int anchorX = -kernel.origin[(vertical) ? 1 : 0];
    int anchorY = -kernel.origin[(vertical) ? 0 : 1];
    return iplCreateConvKernel(nCols, nRows, anchorX, anchorY,
                               &values[0], nShiftR);
}

IplConvKernelFP* IPLConvKernelFP(CFloatImage kernel, bool vertical = false)
{
    // Convert from a floating point image to a floating point kernel
    CShape sh = kernel.Shape();
    std::vector<float> values;       // temporary storage for values
    values.resize(sh.width * sh.height);
    for (int y = 0, k = 0; y < sh.height; y++)
        for (int x = 0; x < sh.width; x++, k++)
            values[k] = kernel.Pixel(x, y, 0);
    int nCols   = (vertical) ? sh.height : sh.width;
    int nRows   = (vertical) ? sh.width  : sh.height;
    int anchorX = -kernel.origin[(vertical) ? 1 : 0];
    int anchorY = -kernel.origin[(vertical) ? 0 : 1];
    return iplCreateConvKernelFP(nCols, nRows, anchorX, anchorY, &values[0]);
}

template <class T>
void Convolve(CImageOf<T> src, CImageOf<T>& dst,
              CFloatImage kernel)
{
    // Allocate the result, if necessary
    dst.ReAllocate(src.Shape(), false);

    // Create the IPL images
    IplImage* srcImage = IPLCreateImage(src);
    IplImage* dstImage = IPLCreateImage(dst);

    // Call the convolution code
    if (typeid(T) == typeid(float))
    {
        IplConvKernelFP* iplKernel = IPLConvKernelFP(kernel);
        iplConvolve2DFP(srcImage, dstImage, &iplKernel, 1, IPL_SUM);
        iplDeleteConvKernelFP(iplKernel);
    }
    else
    {
        IplConvKernel* iplKernel = IPLConvKernel(kernel);
        iplConvolve2D(srcImage, dstImage, &iplKernel, 1, IPL_SUM);
        iplDeleteConvKernel(iplKernel);
    }
    iplDeallocate(srcImage, IPL_IMAGE_HEADER);
    iplDeallocate(dstImage, IPL_IMAGE_HEADER);
}

template <class T>
void ConvolveSeparable(CImageOf<T> src, CImageOf<T>& dst,
                       CFloatImage x_kernel, CFloatImage y_kernel,
                       int subsample)
{
    // Allocate the result, if necessary
    CShape dShape = src.Shape();
    if (subsample > 1)
    {
        dShape.width  = (dShape.width  + subsample-1) / subsample;
        dShape.height = (dShape.height + subsample-1) / subsample;
    }
    dst.ReAllocate(dShape, false);

    // Allocate the intermediate (or final) result
    CImageOf<T> tmpImg;
    if (subsample > 1)
        tmpImg.ReAllocate(src.Shape());
    CImageOf<T>& tmp = (subsample > 1) ? tmpImg : dst;

    // Create the IPL images
    IplImage* srcImage = IPLCreateImage(src);
    IplImage* dstImage = IPLCreateImage(tmp);
    srcImage->alphaChannel = 0;     // convolve the A channel also
    dstImage->alphaChannel = 0;     // convolve the A channel also

    // Call the convolution code
    if (typeid(T) == typeid(float))
    {
        IplConvKernelFP* xKernel = IPLConvKernelFP(x_kernel, false);
        IplConvKernelFP* yKernel = IPLConvKernelFP(y_kernel, true);
        iplConvolveSep2DFP(srcImage, dstImage, xKernel, yKernel);
        iplDeleteConvKernelFP(xKernel);
        iplDeleteConvKernelFP(yKernel);
    }
    else
    {
        IplConvKernel* xKernel = IPLConvKernel(x_kernel, false);
        IplConvKernel* yKernel = IPLConvKernel(y_kernel, true);
        iplConvolveSep2D(srcImage, dstImage, xKernel, yKernel);
        iplDeleteConvKernel(xKernel);
        iplDeleteConvKernel(yKernel);
    }
    iplDeallocate(srcImage, IPL_IMAGE_HEADER);
    iplDeallocate(dstImage, IPL_IMAGE_HEADER);

    // Downsample if necessary
    if (subsample > 1)
    {
        for (int y = 0; y < dShape.height; y++)
        {
            T* sPtr = &tmp.Pixel(0, y * subsample, 0);
            T* dPtr = &dst.Pixel(0, y, 0);
            int nB  = dShape.nBands;
            for (int x = 0; x < dShape.width; x++)
            {
                for (int b = 0; b < nB; b++)
                    dPtr[b] = sPtr[b];
                sPtr += subsample * nB;
                dPtr += nB;
            }
        }
    }
}

template <class T>
void InstantiateConvolutionOf(CImageOf<T> img)
{
    CFloatImage kernel;
    Convolve(img, img, kernel);
    ConvolveSeparable(img, img, kernel, kernel, 1);
}

void InstantiateConvolutions()
{
    InstantiateConvolutionOf(CByteImage());
    InstantiateConvolutionOf(CIntImage());
    InstantiateConvolutionOf(CFloatImage());
}

//
//  Default kernels
//

CFloatImage ConvolveKernel_121;
CFloatImage ConvolveKernel_14641;
CFloatImage ConvolveKernel_8tapLowPass;

struct KernelInit
{
    KernelInit();
};

KernelInit::KernelInit()
{
    static float k_11[2] = {0.5f, 0.5f};
    static float k_121[3] = {0.25f, 0.5f, 0.25f};
    static float k_14641[5] = {0.0625f, 0.25f, 0.375f, 0.25f, 0.0625f};
    static float k_8ptFP[8] = {-0.044734f, -0.059009f,  0.156544f,  0.449199f,
                                0.449199f,  0.156544f, -0.059009f, -0.044734f};
    // The following are derived as fix-point /256 fractions of the above:
    //  -12, -15, 40, 115
    static float k_8ptI [8] = {-0.04687500f, -0.05859375f,  0.15625000f,  0.44921875f,
                                0.44921875f,  0.15625000f, -0.05859375f, -0.04687500f};

    ConvolveKernel_121.ReAllocate(CShape(3, 1, 1), k_121, false, 3);
    ConvolveKernel_121.origin[0] = -1;
    ConvolveKernel_14641.ReAllocate(CShape(5, 1, 1), k_14641, false, 5);
    ConvolveKernel_14641.origin[0] = -2;
    ConvolveKernel_8tapLowPass.ReAllocate(CShape(8, 1, 1), k_8ptI, false, 8);
    ConvolveKernel_8tapLowPass.origin[0] = -4;
}

KernelInit ConvKernelInitializer;