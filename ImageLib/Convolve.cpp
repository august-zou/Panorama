///////////////////////////////////////////////////////////////////////////
//
// NAME
//  Convolve.cpp -- separable and non-separable linear convolution
//
// DESIGN NOTES
//
//  Upsampling is not supported:  zero-pad, then filter (see Pyramid.h).
//
//  The current version is quite inefficient, especially for separable
//  convolution, where vertical kernels use a complete extra set of iterators
//  that aren't necessary.
//
// SEE ALSO
//  Convolve.h          longer description of these routines
//
// Copyright ?Richard Szeliski, 2001.  See Copyright.h for more details
// Some bug fixes by Jonathan Beall, 2008
//
///////////////////////////////////////////////////////////////////////////

#include "Image.h"
#include "Convolve.h"

static int TrimIndex(int k, EBorderMode e, int n)
{
    // Compute the index value 0 <= k < n (return -1 for Zero mode)
    while (k < 0 || k >= n)
    {
        switch (e)
        {
        case eBorderZero:       // zero padding
            return -1;
        case eBorderReplicate:  // replicate border values
            return k = __max(0, __min(n-1, k));
        case eBorderReflect:    // reflect border pixels
            k = (k < 0) ? -k : (k < n) ? k : 2*(n-1)-k;
            break;              // may need to iterate if n < |k|
        case eBorderCyclic:     // wrap pixel values
            k = (k + n) % n;    // may need to iterate if k + n < 0
        }
    }
    return k;
}

template <class T>
static void FillRowBuffer(T buf[], CImageOf<T>& src, CFloatImage& kernel, int k, int n)
{
    // Compute the real row address
    CShape sShape = src.Shape();
    int nB = sShape.nBands;
    int k0 = TrimIndex(k + kernel.origin[1], src.borderMode, sShape.height);
    if (k0 < 0)
    {
        memset(buf, 0, n * sizeof(T));
        return;
    }

    // Fill the row
    T* srcP = &src.Pixel(0, k0, 0);
    int m = n / nB;
    for (int l = 0; l < m; l++, buf += nB)
    {
        int l0 = TrimIndex(l + kernel.origin[0], src.borderMode, sShape.width);
        if (l0 < 0)
            memset(buf, 0, nB * sizeof(T));
        else
            memcpy(buf, &srcP[l0*nB], nB * sizeof(T));
    }
}

template <class T>
void ConvolveRow(CImageOf<T> buffer, CFloatImage kernel, T* dst,
                 int n, T minVal, T maxVal)
{
    CShape kShape = kernel.Shape();
    int kX  = kShape.width;
    int kY  = kShape.height;
    CShape bShape = buffer.Shape();
    int nB  = bShape.nBands;

    for (int i = 0; i < n; i++)
    {
        for (int b = 0; b < nB; b++)
        {
            float sum = 0.0f;
            for (int k = 0; k < kY; k++)
            {
                float* kPtr = &kernel.Pixel(0, k, 0);
                T*     bPtr = &buffer.Pixel(i, k, b);
                for (int l = 0; l < kX; l++, bPtr += nB)
                    sum += kPtr[l] * bPtr[0];
            }
            *dst++ = (T) __max(minVal, __min(maxVal, sum));
        }
    }
}

template <class T>
void Convolve(CImageOf<T> src, CImageOf<T>& dst,
              CFloatImage kernel)
{
    // Determine the shape of the kernel and source image
    CShape kShape = kernel.Shape();
    CShape sShape = src.Shape();

    // Allocate the result, if necessary
    dst.ReAllocate(sShape, false);
    if (sShape.width * sShape.height * sShape.nBands == 0)
        return;

    // Do the convolution
    for (int y = 0; y < sShape.height; y++)
		for (int x = 0; x < sShape.width; x++)
			for (int c = 0; c < sShape.nBands; c++)
			{
				double sum = 0;
				for (int kx = 0; kx < kShape.width; kx++)
					for (int ky = 0; ky < kShape.height; ky++)
						if ((x-kernel.origin[0]+kx >= 0) && (x-kernel.origin[0]+kx < sShape.width) && (y-kernel.origin[1]+ky >= 0) && (y-kernel.origin[1]+ky < sShape.height))
							sum += kernel.Pixel(kx,ky,0) * src.Pixel(x-kernel.origin[0]+kx,y-kernel.origin[1]+ky,c);
				dst.Pixel(x,y,c) = (T) __max(dst.MinVal(), __min(dst.MaxVal(), sum));
			}
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

    // Allocate the intermediate images
    CImageOf<T> tmpImg1(src.Shape());
    CImageOf<T> tmpImg2(src.Shape());

    // Create a proper vertical convolution kernel
    CFloatImage v_kernel(1, y_kernel.Shape().width, 1);
    for (int k = 0; k < y_kernel.Shape().width; k++)
        v_kernel.Pixel(0, k, 0) = y_kernel.Pixel(k, 0, 0);
    v_kernel.origin[1] = y_kernel.origin[0];

    // Perform the two convolutions
    Convolve(src, tmpImg1, x_kernel);
    Convolve(tmpImg1, tmpImg2, v_kernel);
				
    // Downsample or copy
    for (int y = 0; y < dShape.height; y++)
    {
        T* sPtr = &tmpImg2.Pixel(0, y * subsample, 0);
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

template <class T>
void InstantiateConvolutionOf(CImageOf<T> img)
{
    CFloatImage kernel;
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
CFloatImage ConvolveKernel_7x7;
CFloatImage ConvolveKernel_SobelX;
CFloatImage ConvolveKernel_SobelY;

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

    static float k_7x7[49] = { 1.0, 4.0, 7.0, 10.0, 7.0, 4.0, 1.0, 
                               4.0, 12.0, 26.0, 33.0, 26.0, 12.0, 4.0,
                               7.0, 26.0, 55.0, 71.0, 55.0, 26.0, 7.0, 
                               10.0, 33.0, 71.0, 91.0, 71.0, 33.0, 10.0, 
                               7.0, 26.0, 55.0, 71.0, 55.0, 26.0, 7.0, 
                               4.0, 12.0, 26.0, 33.0, 26.0, 12.0, 4.0,
                               1.0, 4.0, 7.0, 10.0, 7.0, 4.0, 1.0 };

    for (int i = 0; i < 49; i++) {
        k_7x7[i] /= 1115.0;
    }

    ConvolveKernel_121.ReAllocate(CShape(3, 1, 1), k_121, false, 3);
    ConvolveKernel_121.origin[0] = 1;
    ConvolveKernel_14641.ReAllocate(CShape(5, 1, 1), k_14641, false, 5);
    ConvolveKernel_14641.origin[0] = 2;
    ConvolveKernel_8tapLowPass.ReAllocate(CShape(8, 1, 1), k_8ptI, false, 8);
    ConvolveKernel_8tapLowPass.origin[0] = 4;
    ConvolveKernel_7x7.ReAllocate(CShape(7, 7, 1), k_7x7, false, 7);

    /* Sobel filters */
    static float k_SobelX[9] = { -1, 0, 1,
                                 -2, 0, 2,
                                 -1, 0, 1 };

    static float k_SobelY[9] = { -1, -2, -1,
                                  0,  0,  0,
                                  1,  2,  1 };    
            
    ConvolveKernel_SobelX.ReAllocate(CShape(3, 3, 1), k_SobelX, false, 3);
    ConvolveKernel_SobelX.origin[0] = 1;
    ConvolveKernel_SobelX.origin[1] = 1;
    ConvolveKernel_SobelY.ReAllocate(CShape(3, 3, 1), k_SobelY, false, 3);
    ConvolveKernel_SobelY.origin[0] = 1;
    ConvolveKernel_SobelY.origin[1] = 1;
}

KernelInit ConvKernelInitializer;
