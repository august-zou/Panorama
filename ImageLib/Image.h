///////////////////////////////////////////////////////////////////////////
//
// NAME
//  Image.h -- a simple reference-counted image structure
//
// DESCRIPTION
//  The CImage class is used to create and access weakly typed
//  two-dimensional images.  
//
//  The templated CImageOf<T> classes are used to create strongly typed images.
//  The currently supported pixel types are:
//      unsigned char, int, and float.
//
//  The images can have an arbitrary width, height, and also an arbitrary
//  number of bands (channels) per pixel.  For example, traditional RGBA
//  images can be represented using a 4-channel unsigned_8 image.
//
//  Images are normally allocated on the stack (NOT on the heap, i.e.,
//  "new Image" should not be used).  They can be freely returned from
//  functions and put into other data structures.  Assignment and copy
//  construction share memory (to copy pixel values from one image to
//  another one, use CopyPixels()).
//
// SEE ALSO
//  Image.cpp           implementation
//  RefCntMem.h         reference-counted memory object used by CImage
//
// Copyright © Richard Szeliski, 2001.  See Copyright.h for more details
//
///////////////////////////////////////////////////////////////////////////

#include "RefCntMem.h"
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <cstring>

#ifdef WIN32
#include <typeinfo.h>
#else
#include <typeinfo>
#define __max(a,b)  (((a) > (b)) ? (a) : (b))
#define __min(a,b)  (((a) < (b)) ? (a) : (b))
#endif

typedef unsigned char uchar;

using namespace std;


// Shape of an image: width x height x nbands

struct CShape
{
    int width, height;      // width and height in pixels
    int nBands;             // number of bands/channels
    
    // Constructors and helper functions 
    CShape(void) : width(0), height(0), nBands(0) {}
    CShape(int w, int h, int nb) : width(w), height(h), nBands(nb) {}
    bool InBounds(int x, int y);            // is given pixel address valid?
    bool InBounds(int x, int y, int band);  // is given pixel address valid?
    bool operator==(const CShape& ref);     // are two shapes the same?
    bool SameIgnoringNBands(const CShape& ref); // " ignoring the number of bands?
    bool operator!=(const CShape& ref);     // are two shapes not the same?
};

// Padding (border) mode for neighborhood operations like convolution

enum EBorderMode
{
    eBorderZero         = 0,    // zero padding
    eBorderReplicate    = 1,    // replicate border values
    eBorderReflect      = 2,    // reflect border pixels
    eBorderCyclic       = 3     // wrap pixel values
};

// Image attributes

struct CImageAttributes
{
    int alphaChannel;       // which channel contains alpha (for compositing)
    int origin[2];          // x and y coordinate origin (for some operations)
    EBorderMode borderMode; // border behavior for neighborhood operations...
    // char colorSpace[4];     // RGBA, YUVA, etc.: not currently used
};


// Generic (weakly typed) image

class CImage : public CImageAttributes
{
public:
    CImage(void);               // default constructor
    CImage(CShape s, const type_info& ti, int bandSize);
    // uses system-supplied copy constructor, assignment operator, and destructor

    void ReAllocate(CShape s, const type_info& ti, int bandSize,
                    void *memory, bool deleteWhenDone, int rowSize);
    void ReAllocate(CShape s, const type_info& ti, int bandSize,
                    bool evenIfShapeDiffers = false);
    void DeAllocate(void);      // release the memory & set to default values

    CShape Shape(void)              { return m_shape; }
    const type_info& PixType(void)  { return *m_pTI; }
    int BandSize(void)              { return m_bandSize; }

    void* PixelAddress(int x, int y, int band);

    void SetSubImage(int xO, int yO, int width, int height);   // sub-image sharing memory
    void ClearPixels(void); // set all the pixels to 0

private:
    void SetDefaults(void); // set internal state to default values

    CShape m_shape;         // image shape (dimensions)
    const type_info* m_pTI; // pointer to type_info class
    int m_bandSize;         // size of each band in bytes
    int m_pixSize;          // stride between pixels in bytes
    int m_rowSize;          // stride between rows in bytes
    char* m_memStart;       // start of addressable memory
    CRefCntMem m_memory;    // reference counted memory
};

//  Strongly typed image

template <class T>
class CImageOf : public CImage
{
public:
    CImageOf(void);
    CImageOf(CShape s);
    CImageOf(int width, int height, int nBands);
    // uses system-supplied copy constructor, assignment operator, and destructor

    void ReAllocate(CShape s, bool evenIfShapeDiffers = false);
    void ReAllocate(CShape s, T *memory, bool deleteWhenDone, int rowSize);

    T& Pixel(int x, int y, int band);

    CImageOf SubImage(int x, int y, int width, int height);   // sub-image sharing memory

    double PixelLerp(double x, double y, int band);

    T MinVal(void);     // minimum allowable value (for clipping)
    T MaxVal(void);     // maximum allowable value (for clipping)
};


// Commonly used types (supported in type conversion routines):

typedef CImageOf<uchar> CByteImage;
typedef CImageOf<int>   CIntImage;
typedef CImageOf<float> CFloatImage;

// Color pixel support

template <class PixType>
struct RGBA
{
    PixType B, G, R, A;     // A channel is highest one
};

// Simple error handling class

struct CError : public exception
{
    CError(const char* msg)                 { strcpy(message, msg); }
    CError(const char* fmt, int d)          { sprintf(message, fmt, d); }
    CError(const char* fmt, const char *s)  { sprintf(message, fmt, s); }
    CError(const char* fmt, const char *s,
           int d)                           { sprintf(message, fmt, s, d); }
    CError(const char* fmt, const char *s,
           const char*s2)                   { sprintf(message, fmt, s, s2); }
    char message[1024];         // longest allowable message
};


#include "Image.inl"
