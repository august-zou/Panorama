///////////////////////////////////////////////////////////////////////////
//
// NAME
//  IPLInterface.cpp -- interface to IPL routines
//
// SEE ALSO
//  IPLInterface.h      full description of this functionality
//  Image.h             image class definition
//
// Copyright © Richard Szeliski, 2001.  See Copyright.h for more details
//
///////////////////////////////////////////////////////////////////////////

#include "Image.h"
#include "IPLInterface.h"

//
// Return an IPL image pointer to CImage data
//  The user will have to call iplDeallocateImage explicitly.
//

IplImage* IPLCreateImage(CImage img)
{
    // Compute the required parameters
    CShape sh = img.Shape();
    int nChannels = sh.nBands;      // Number of channels in the image.
    int alphaChannel =              // Alpha channel number (0 if there is no alpha channel in the image).
        (nChannels == 4) ? img.alphaChannel+1 : 0;
    int depth =                     // Bit depth of pixels. Can be one of:
        (img.PixType() == typeid(uchar)) ? IPL_DEPTH_8U :
        (img.PixType() == typeid(char))  ? IPL_DEPTH_8S :
        (img.PixType() == typeid(unsigned short)) ? IPL_DEPTH_16U :
        (img.PixType() == typeid(short)) ? IPL_DEPTH_16S :
        (img.PixType() == typeid(int  )) ? IPL_DEPTH_32S :
        (img.PixType() == typeid(float)) ? IPL_DEPTH_32F : 0;
    char* colorModel =              // A four-character string describing the color model.
        (nChannels == 1) ? "Gray" : "RGBA";
    char* channelSeq =              // The sequence of color channels.
        (nChannels == 1) ? "GRAY" : "BGRA";
    int dataOrder = IPL_DATA_ORDER_PIXEL;
    int origin = IPL_ORIGIN_TL;     // The origin of the image.
    int align =                     // Alignment of image data.
        ((((int) img.PixelAddress(0, 0, 0)) | ((int) img.PixelAddress(0, 1, 0))) & 7) ?
        IPL_ALIGN_DWORD : IPL_ALIGN_QWORD;
    int width = sh.width;           // Width of the image in pixels.
    int height = sh.height;         // Height of the image in pixels.
    IplROI* roi = 0;                // Pointer to an ROI (region of interest) structure.
    IplImage* maskROI = 0;          // Pointer to the header of another image that specifies the mask ROI.
    void* imageID = 0;              // The image ID (field reserved for the application).
    IplTileInfo* tileInfo = 0;      // The pointer to the IplTileInfo structure

    // Create the header
    IplImage* ptr = iplCreateImageHeader(
        nChannels,
        alphaChannel, depth,  colorModel,
        channelSeq, dataOrder, origin, align,
        width, height, roi, maskROI,
        imageID, tileInfo);
    if (ptr == 0)
        throw CError("IPLCreateImage: could not create the header");

    // Fill in the image data pointers
    char* imgData   = ((char *) img.PixelAddress(0, 0, 0));
    int nBytes      = ((char *) img.PixelAddress(0, 1, 0)) - imgData;
	ptr->imageSize = nBytes * sh.height;    // useful size in bytes
	ptr->imageData = imgData;               // pointer to aligned image
	ptr->widthStep = nBytes;                // size of aligned line in bytes
	ptr->imageDataOrigin = imgData;         // ptr to full, nonaligned image

    // Set the border mode
    int mode = 
        (img.borderMode == eBorderZero)     ? IPL_BORDER_CONSTANT :
        (img.borderMode == eBorderReplicate)? IPL_BORDER_REPLICATE :
        (img.borderMode == eBorderReflect)  ? IPL_BORDER_REFLECT :
        (img.borderMode == eBorderCyclic)   ? IPL_BORDER_WRAP : 0;
    iplSetBorderMode(ptr, mode, IPL_SIDE_ALL, 0);

    return ptr;
}

//
// Create a wrapper that contains and IPL image pointer.
//  The header is deallocated automatically when the class object is destroyed
//

CIPLImage::CIPLImage() : m_iplImage(0)
{
    // Default constructor
}

CIPLImage::CIPLImage(CImage img) : m_img(img)
{
    // Create and initialize the header
    m_iplImage = IPLCreateImage(m_img);

    // Fill in the reference counted pointer, for automatic deallocation
    m_rcm.ReAllocate(sizeof(IplImage), m_iplImage, true, DeallocateHeader);
}

void CIPLImage::DeallocateHeader(void *hdr)
{
    // Deallocate the header (called inside ref-counted memory class object)
    int flag =              // Flag indicating what memory area to free:
        IPL_IMAGE_HEADER;   //  Free header structure.
    iplDeallocate((IplImage *) hdr, flag);
}

bool IPLiUseIPL = true;    // use IPL routines when possible
