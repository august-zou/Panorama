///////////////////////////////////////////////////////////////////////////
//
// NAME
//  WarpSpherical.h -- warp a flat (perspective) image into spherical
//      coordinates and/or undo radial lens distortion
//
// SPECIFICATION
//  CFloatImage WarpSphericalField(CShape sh, float f, float k1, float k2, const CTransform3x3 &r)
// PARAMETERS
//  sh                  shape of destination image
//  f                   focal length, in pixels
//  k1, k2              radial distortion parameters
//  r                   rotation matrix
//
// DESCRIPTION
//  WarpSphericalField produces a pixel coordinate image suitable
//  for correcting radial distortion in an image and/or mapping
//  the image into spherical coordinates (for image mosaics).
//
//  Use the output of WarpSphericalField as the input to 
//  WarpLocal, along with a source and destination image, to actually
//  perform the warp.
//
// SEE ALSO
//  WarpSpherical.cpp implementation
//  WarpImage.h         image warping code
//  Image.h             image class definition
//
// Copyright © Richard Szeliski, 2001.  See Copyright.h for more details
// (modified for CSE576 Spring 2005)
//
///////////////////////////////////////////////////////////////////////////

CFloatImage WarpSphericalField(CShape srcSh, CShape dstSh, float f,
                                 float k1, float k2, const CTransform3x3 &r);
