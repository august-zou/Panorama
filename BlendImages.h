///////////////////////////////////////////////////////////////////////////
//
// NAME
//  BlendImages.h -- blend together a set of overlapping images
//
// SPECIFICATION
//  CByteImage BlendImages(CImagePositionV ipv, float blendWidth);
//
// PARAMETERS
//  ipv                 list (vector) of images and their locations
//  blendWidth          half-width of transition region (in pixels)
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
//  BlendImages.cpp     implementation
//
// Copyright © Richard Szeliski, 2001.  See Copyright.h for more details
// (modified for CSE455 Winter 2003)
//
///////////////////////////////////////////////////////////////////////////

struct CImagePosition
{
    CByteImage img;         // image
    //float position[2];      // position relative to first image
	CTransform3x3 position;
};

typedef std::vector<CImagePosition> CImagePositionV;

CByteImage BlendImages(CImagePositionV& ipv, float blendWidth);
