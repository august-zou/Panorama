///////////////////////////////////////////////////////////////////////////
//
// NAME
//  FileIO.h -- image file input/output
//
// DESCRIPTION
//  Read/write image files, potentially using an interface to an
//  external package.
//
//  You can either pass and empty (unitialized) image to ReadFile,
//  or one you have already allocated (with a specific pixel type).
//  
//  If you don't initialize the image, the type of the returned image
//  (e.g., 1 band vs. 4 band) will be determined by the image file.
//  If you do initialize the image, it will be re-allocated if necessary,
//  and the data will be coerced into the type you specified.
//
// SEE ALSO
//  FileIO.cpp          implementation
//
// Copyright © Richard Szeliski, 2001.  See Copyright.h for more details
//
///////////////////////////////////////////////////////////////////////////

void ReadFile (CImage& img, const char* filename);
void WriteFile(CImage& img, const char* filename);
