///////////////////////////////////////////////////////////////////////////
//
// NAME
//  IPLInterface.h -- interface to IPL routines
//
// SPECIFICATION
//  IplImage* IPLCreateImage(CImage img);
//  
//  class CIPLImage
//  {
//  public:
//      CIPLImage();
//      CIPLImage(CImage img);
//      IplImage* operator*(void);      // pointer to an IplImage*
//      CImage& Image(void);            // reference to a copy of the original image
//  };
//
//  bool IPLiUseIPL    // use IPL routines when possible
//
// PARAMETERS
//  img                 image to which you need an IPL pointer
//
// DESCRIPTION
//  IPLCreateImage calls iplCreateImageHeader to create an IPL image header.
//  You must call iplDeallocateImage explicitly when you are done with this
//  pointer (but do NOT ask for the image data to be de-allocated).
//  Also, should img get deallocated and the last reference count to
//  the memory disappear, the image data in the return pointer will
//  get deallocated.
//
//  The CIPLImage class is a safer way to get a pointer to an IPL
//  image that you can then use in IPL calls.  The IplImage* header that
//  is constructed during creation is reference counted.  The header
//  gets de-allocated when the CIPLImage object gets deallocated (e.g.,
//  when it goes out of scope).  However, because of the way that
//  the default copy constructor works, you can also return a
//  CIPLImage from a function, and the header will not get deallocated.
//
//  The boolean IPLiUseIPL is used by other routines to control whether
//  IPL gets called.
//
// SEE ALSO
//  Image.h             image class definition
//
// Copyright © Richard Szeliski, 2001.  See Copyright.h for more details
//
///////////////////////////////////////////////////////////////////////////

#include "ipl.h"

// Return an IPL image pointer to CImage data
//  The user will have to call iplDeallocateImage explicitly.

IplImage* IPLCreateImage(CImage img);

// Create a wrapper that contains and IPL image pointer.
//  The header is deallocated automatically when the class object is destroyed

class CIPLImage
{
public:
    CIPLImage();
    CIPLImage(CImage img);
    // uses system-supplied copy constructor, assignment operator, and destructor
    IplImage* operator*(void);      // pointer to an IplImage*
    CImage& Image(void);            // reference to a copy of the original image
private:
    CImage m_img;                   // copy of the initial image
    IplImage* m_iplImage;           // initialized IPL image header
    CRefCntMem m_rcm;               // reference counted pointer (for deallocation)

    static void DeallocateHeader(void *ptr);    // call iplDeallocateImage
};

inline CImage& CIPLImage::Image(void)
{
    // Reference to a copy of the original image
    return m_img;
}

inline IplImage* CIPLImage::operator*(void)
{
    // Pointer to an IplImage*
    return m_iplImage;
}


extern bool IPLiUseIPL;    // use IPL routines when possible
