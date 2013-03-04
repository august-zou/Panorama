///////////////////////////////////////////////////////////////////////////
//
// NAME
//  BlendImages.cpp -- blend together a set of overlapping images
//
// DESCRIPTION
//  This routine takes a collection of images aligned more or less horizontally
//  and stitches together a mosaic.
//
//  The images can be blended together any way you like, but I would recommend
//  using a soft halfway blend of the kind Steve presented in the first lecture.
//
//  Once you have blended the images together, you should crop the resulting
//  mosaic at the halfway points of the first and last image.  You should also
//  take out any accumulated vertical drift using an affine warp.
//  Lucas-Kanade Taylor series expansion of the registration error.
//
// SEE ALSO
//  BlendImages.h       longer description of parameters
//
// Copyright ?Richard Szeliski, 2001.  See Copyright.h for more details
// (modified for CSE455 Winter 2003)
//
///////////////////////////////////////////////////////////////////////////

#include "ImageLib/ImageLib.h"
#include "BlendImages.h"
#include <float.h>
#include <math.h>



#define MAX(x,y) (((x) < (y)) ? (y) : (x))
#define MIN(x,y) (((x) < (y)) ? (x) : (y))

/* Return the closest integer to x, rounding up */
static int iround(double x) {
    if (x < 0.0) {
	return (int) (x - 0.5);
    } else {
	return (int) (x + 0.5);
    }
}

void ImageBoundingBox(CImage &image, CTransform3x3 &M, 
		      int &min_x, int &min_y, int &max_x, int &max_y)
{
    float min_xf = FLT_MAX, min_yf = FLT_MAX;
    float max_xf = 0.0, max_yf = 0.0;

    CVector3 corners[4];

    int width = image.Shape().width;
    int height = image.Shape().height;
	
    corners[0][0] = 0.0;
    corners[0][1] = 0.0;
    corners[0][2] = 1.0;
	
    corners[1][0] = width - 1;
    corners[1][1] = 0.0;
    corners[1][2] = 1.0;

    corners[2][0] = 0.0;
    corners[2][1] = height - 1;
    corners[2][2] = 1.0;

    corners[3][0] = width - 1;
    corners[3][1] = height - 1;
    corners[3][2] = 1.0;

    corners[0] = M * corners[0];
    corners[1] = M * corners[1];
    corners[2] = M * corners[2];
    corners[3] = M * corners[3];

    corners[0][0] /= corners[0][2];
    corners[0][1] /= corners[0][2];

    corners[1][0] /= corners[0][2];
    corners[1][1] /= corners[0][2];

    corners[2][0] /= corners[0][2];
    corners[2][1] /= corners[0][2];

    corners[3][0] /= corners[0][2];
    corners[3][1] /= corners[0][2];
	

    min_xf = (float) MIN(min_xf, corners[0][0]);
    min_xf = (float) MIN(min_xf, corners[1][0]);
    min_xf = (float) MIN(min_xf, corners[2][0]);
    min_xf = (float) MIN(min_xf, corners[3][0]);

    min_yf = (float) MIN(min_yf, corners[0][1]);
    min_yf = (float) MIN(min_yf, corners[1][1]);
    min_yf = (float) MIN(min_yf, corners[2][1]);
    min_yf = (float) MIN(min_yf, corners[3][1]);
	
    max_xf = (float) MAX(max_xf, corners[0][0]);
    max_xf = (float) MAX(max_xf, corners[1][0]);
    max_xf = (float) MAX(max_xf, corners[2][0]);
    max_xf = (float) MAX(max_xf, corners[3][0]);
	
    max_yf = (float) MAX(max_yf, corners[0][1]);
    max_yf = (float) MAX(max_yf, corners[1][1]);
    max_yf = (float) MAX(max_yf, corners[2][1]);
    max_yf = (float) MAX(max_yf, corners[3][1]);    

    min_x = (int) floor(min_xf);
    min_y = (int) floor(min_yf);
    max_x = (int) ceil(max_xf);
    max_y = (int) ceil(max_yf);
}


/******************* TO DO 4 *********************
 * AccumulateBlend:
 *	INPUT:
 *		img: a new image to be added to acc
 *		acc: portion of the accumulated image where img is to be added
 *		M: translation matrix for calculating a bounding box
 *		blendWidth: width of the blending function (horizontal hat function;
 *	    try other blending functions for extra credit)
 *	OUTPUT:
 *		add a weighted copy of img to the subimage specified in acc
 *		the first 3 band of acc records the weighted sum of pixel colors
 *		the fourth band of acc records the sum of weight
 */
static void AccumulateBlend(CByteImage& img, CFloatImage& acc, CTransform3x3 M, float blendWidth)
{

	
    /* Compute the bounding box of the image of the image */
    int bb_min_x, bb_min_y, bb_max_x, bb_max_y;
    ImageBoundingBox(img, M, bb_min_x, bb_min_y, bb_max_x, bb_max_y);

    CTransform3x3 Minv = M.Inverse();

    for (int y = bb_min_y; y <= bb_max_y; y++) {
        for (int x = bb_min_x; x < bb_max_x; x++) {
            /* Check bounds in destination */
            if (x < 0 || x >= acc.Shape().width || 
                y < 0 || y >= acc.Shape().height)
                continue;

            /* Compute source pixel and check bounds in source */
            CVector3 p_dest, p_src;
            p_dest[0] = x;
            p_dest[1] = y;
            p_dest[2] = 1.0;

            p_src = Minv * p_dest;

            float x_src = (float) (p_src[0] / p_src[2]);
            float y_src = (float) (p_src[1] / p_src[2]);

            if (x_src < 0.0 || x_src >= img.Shape().width - 1 ||
                y_src < 0.0 || y_src >= img.Shape().height - 1)
                continue;

            int xf = (int) floor(x_src);
            int yf = (int) floor(y_src);
            int xc = xf + 1;
            int yc = yf + 1;

            /* Skip black pixels */
            if (img.Pixel(xf, yf, 0) == 0x0 && 
                img.Pixel(xf, yf, 1) == 0x0 && 
                img.Pixel(xf, yf, 2) == 0x0)
                continue;

            if (img.Pixel(xc, yf, 0) == 0x0 && 
                img.Pixel(xc, yf, 1) == 0x0 && 
                img.Pixel(xc, yf, 2) == 0x0)
                continue;

            if (img.Pixel(xf, yc, 0) == 0x0 && 
                img.Pixel(xf, yc, 1) == 0x0 && 
                img.Pixel(xf, yc, 2) == 0x0)
                continue;

            if (img.Pixel(xc, yc, 0) == 0x0 && 
                img.Pixel(xc, yc, 1) == 0x0 && 
                img.Pixel(xc, yc, 2) == 0x0)
                continue;

            
            double weight = 1.0;

			// *** BEGIN TODO ***
			// set weight properly
			//(see mosaics lecture slide on "feathering") 
	
			

            
			// *** END TODO ***	

			acc.Pixel(x, y, 0) += (float) (weight * img.PixelLerp(x_src, y_src, 0));
            acc.Pixel(x, y, 1) += (float) (weight * img.PixelLerp(x_src, y_src, 1));
            acc.Pixel(x, y, 2) += (float) (weight * img.PixelLerp(x_src, y_src, 2));
            acc.Pixel(x, y, 3) += (float) weight;

			
        }
    }
}







/******************* TO DO 5 *********************
 * NormalizeBlend:
 *	INPUT:
 *		acc: input image whose alpha channel (4th channel) contains
 *		     normalizing weight values
 *		img: where output image will be stored
 *	OUTPUT:
 *		normalize r,g,b values (first 3 channels) of acc and store it into img
 */
static void NormalizeBlend(CFloatImage& acc, CByteImage& img)
{
	// *** BEGIN TODO ***
	// fill in this routine..
	

	// *** END TODO ***
}

/******************* TO DO 5 *********************
 * BlendImages:
 *	INPUT:
 *		ipv: list of input images and their relative positions in the mosaic
 *		blendWidth: width of the blending function
 *	OUTPUT:
 *		create & return final mosaic by blending all images
 *		and correcting for any vertical drift
 */
CByteImage BlendImages(CImagePositionV& ipv, float blendWidth)
{
    // Assume all the images are of the same shape (for now)
    CByteImage& img0 = ipv[0].img;
    CShape sh        = img0.Shape();
    int width        = sh.width;
    int height       = sh.height;
    int nBands       = sh.nBands;
    int dim[2]       = {width, height};

    // Compute the bounding box for the mosaic
    int n = ipv.size();
    float min_x = 0, min_y = 0;
    float max_x = 0, max_y = 0;
    int i;
    for (i = 0; i < n; i++)
    {
		
		CTransform3x3 &pos = ipv[i].position;

        CVector3 corners[4];

        corners[0][0] = 0.0;
        corners[0][1] = 0.0;
        corners[0][2] = 1.0;

        corners[1][0] = width - 1;
        corners[1][1] = 0.0;
        corners[1][2] = 1.0;

        corners[2][0] = 0.0;
        corners[2][1] = height - 1;
        corners[2][2] = 1.0;

        corners[3][0] = width - 1;
        corners[3][1] = height - 1;
        corners[3][2] = 1.0;

        corners[0] = pos * corners[0];
        corners[1] = pos * corners[1];
        corners[2] = pos * corners[2];
        corners[3] = pos * corners[3];

        corners[0][0] /= corners[0][2];
        corners[0][1] /= corners[0][2];

        corners[1][0] /= corners[0][2];
        corners[1][1] /= corners[0][2];

        corners[2][0] /= corners[0][2];
        corners[2][1] /= corners[0][2];

        corners[3][0] /= corners[0][2];
        corners[3][1] /= corners[0][2];
        
		// *** BEGIN TODO #1 ***
		// add some code here to update min_x, ..., max_y

		// *** END TODO #1 ***
    }

    // Create a floating point accumulation image
    CShape mShape((int)(ceil(max_x) - floor(min_x)),
                  (int)(ceil(max_y) - floor(min_y)), nBands);
    CFloatImage accumulator(mShape);
    accumulator.ClearPixels();

	double x_init, x_final;
    double y_init, y_final;

	// Add in all of the images
    for (i = 0; i < n; i++) {
        
        CTransform3x3 &M = ipv[i].position;

        CTransform3x3 M_t = CTransform3x3::Translation(-min_x, -min_y) * M;

        CByteImage& img = ipv[i].img;

        // Perform the accumulation
		AccumulateBlend(img, accumulator, M_t, blendWidth);

        if (i == 0) {
            CVector3 p;
            p[0] = 0.5 * width;
            p[1] = 0.0;
            p[2] = 1.0;

            p = M_t * p;
            x_init = p[0];
            y_init = p[1];
        } else if (i == n - 1) {
            CVector3 p;
            p[0] = 0.5 * width;
            p[1] = 0.0;
            p[2] = 1.0;

            p = M_t * p;
            x_final = p[0];
            y_final = p[1];
        }
    }



    // Normalize the results
    CByteImage compImage(mShape);
    NormalizeBlend(accumulator, compImage);
    bool debug_comp = false;
    if (debug_comp)
        WriteFile(compImage, "tmp_comp.tga");

    // Allocate the final image shape
    CShape cShape(mShape.width - width, height, nBands);
    CByteImage croppedImage(cShape);

    // Compute the affine deformation
    CTransform3x3 A;
    
	// *** BEGIN TODO #2 ***
    // fill in the right entries in A to trim the left edge and
    // to take out the vertical drift


	// *** END TODO #2 ***

    // Warp and crop the composite
    WarpGlobal(compImage, croppedImage, A, eWarpInterpLinear);

    return croppedImage;
}
