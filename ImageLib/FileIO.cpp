///////////////////////////////////////////////////////////////////////////
//
// NAME
//  FileIO.cpp -- image file input/output
//
// DESCRIPTION
//  Read/write image files, potentially using an interface to an
//  external package.
//
//  Currently, only Targa input/output is supported (grayscale and color
//  byte images), and only a subset of Targa formats in supported.
//
// SEE ALSO
//  FileIO.h            longer description
//
// Copyright © Richard Szeliski, 2001.  See Copyright.h for more details
//
///////////////////////////////////////////////////////////////////////////

#include "Image.h"
#include "FileIO.h"

//
//  Truevision Targa (TGA):  support 24 bit RGB and 32-bit RGBA files
//

typedef unsigned char uchar;

struct CTargaHead
{
    uchar idLength;     // number of chars in identification field
    uchar colorMapType;	// color map type
    uchar imageType;	// image type code
    uchar cMapOrigin[2];// color map origin
    uchar cMapLength[2];// color map length
    uchar cMapBits;     // color map entry size
    short x0;			// x-origin of image
    short y0;			// y-origin of image
    short width;		// width of image
    short height;		// height of image
    uchar pixelSize;    // image pixel size
    uchar descriptor;   // image descriptor byte
};

// Image data type codes
const int TargaRawColormap	= 1;
const int TargaRawRGB		= 2;
const int TargaRawBW		= 3;
const int TargaRunColormap	= 9;
const int TargaRunRGB		= 10;
const int TargaRunBW		= 11;

// Descriptor fields
const int TargaAttrBits     = 15;
const int TargaScreenOrigin = (1<<5);
const int TargaCMapSize		= 256;
const int TargaCMapBands    = 3;
/*
const int TargaInterleaveShift = 6;
const int TargaNON_INTERLEAVE	0
const int TargaTWO_INTERLEAVE	1
const int TargaFOUR_INTERLEAVE	2
const int PERMUTE_BANDS		1
*/

class CTargaRLC
{
    // Helper class to decode run-length-coded image data
public:
    CTargaRLC(bool RLC) : m_count(0), m_RLC(RLC) {}
    uchar* getBytes(int nBytes, FILE *stream);
private:
    int m_count;        // remaining count in current run
    bool m_RLC;         // is stream run-length coded?
    bool m_isRun;       // is current stream of pixels a run?
    uchar m_buffer[4];  // internal buffer
};

inline uchar* CTargaRLC::getBytes(int nBytes, FILE *stream)
{
    // Get one pixel, which consists of nBytes
    if (nBytes > 4)
        throw CError("ReadFileTGA: only support pixels up to 4 bytes long");

    if (! m_RLC)
    {
    	if (fread(m_buffer, sizeof(uchar), nBytes, stream) != nBytes)
    	    throw CError("ReadFileTGA: file is too short");
    }
    else
    {
        if (m_count == 0)
        {
            // Read in the next run count
            m_count = fgetc(stream);
            m_isRun = (m_count & 0x80) != 0;
            m_count = (m_count & 0x7f)  + 1;
            if (m_isRun)  // read the pixels for this run
    	        if (fread(m_buffer, sizeof(uchar), nBytes, stream) != nBytes)
    	            throw CError("ReadFileTGA: file is too short");
        }
        if (! m_isRun)
        {
    	    if (fread(m_buffer, sizeof(uchar), nBytes, stream) != nBytes)
    	        throw CError("ReadFileTGA: file is too short");
        }
        m_count -= 1;
    }
    return m_buffer;
}

void ReadFileTGA(CByteImage& img, const char* filename)
{
    // Open the file and read the header
    FILE *stream = fopen(filename, "rb");
    if (stream == 0)
        throw CError("ReadFileTGA: could not open %s", filename);
    CTargaHead h;
    if (fread(&h, sizeof(CTargaHead), 1, stream) != 1)
	    throw CError("ReadFileTGA(%s): file is too short", filename);

    // Throw away the image descriptor
    if (h.idLength > 0)
    {
        char* tmp = new char[h.idLength];
        int nread = fread(tmp, sizeof(uchar), h.idLength, stream);
        delete tmp;   // throw away this data
        if (nread != h.idLength)
	        throw CError("ReadFileTGA(%s): file is too short", filename);
    }
    bool isRun = (h.imageType & 8) != 0;
    bool reverseRows = (h.descriptor & TargaScreenOrigin) != 0;
    int fileBytes = (h.pixelSize + 7) / 8;

    // Read the colormap
    uchar colormap[TargaCMapSize][TargaCMapBands];
    int cMapSize = 0;
    bool grayRamp = false;
    if (h.colorMapType == 1)
    {
        cMapSize = (h.cMapLength[1] << 8) + h.cMapLength[0];
        if (h.cMapBits != 24)
            throw CError("ReadFileTGA(%s): only 24-bit colormap currently supported", filename);
	    int l = fileBytes * cMapSize;
        if (l > TargaCMapSize * TargaCMapBands)
	        throw CError("ReadFileTGA(%s): colormap is too large", filename);
	    if (fread(colormap, sizeof(uchar), l, stream) != l)
	        throw CError("ReadFileTGA(%s): could not read the colormap", filename);

        // Check if it's just a standard gray ramp
	int i;
        for (i = 0; i < cMapSize; i++) {
            for (int j = 0; j < TargaCMapBands; j++)
                if (colormap[i][j] != i)
                    break;
        }
        grayRamp = (i == cMapSize);    // didn't break out too soon
    }
    bool isGray = 
        h.imageType == TargaRawBW || h.imageType == TargaRunBW ||
        grayRamp &&
        (h.imageType == TargaRawColormap || h.imageType == TargaRunColormap);
    bool isRaw = h.imageType == TargaRawBW || h.imageType == TargaRawRGB ||
        h.imageType == TargaRawRGB && isGray;

    // Determine the image shape
    CShape sh(h.width, h.height, (isGray) ? 1 : 4);
    
    // Allocate the image if necessary
    img.ReAllocate(sh, false);

    // Construct a run-length code reader
    CTargaRLC rlc(! isRaw);

    // Read in the rows
    for (int y = 0; y < sh.height; y++)
    {
        int yr = reverseRows ? sh.height-1-y : y;
        uchar* ptr = (uchar *) img.PixelAddress(0, yr, 0);
        if (fileBytes == sh.nBands && isRaw)
        {
            // Special case for raw image, same as destination
            int n = sh.width*sh.nBands;
    	    if (fread(ptr, sizeof(uchar), n, stream) != n)
    	        throw CError("ReadFileTGA(%s): file is too short", filename);
        }
        else
        {
            // Read one pixel at a time
            for (int x = 0; x < sh.width; x++, ptr += sh.nBands)
            {
                uchar* buf = rlc.getBytes(fileBytes, stream);
                if (fileBytes == 1 && sh.nBands == 1)
                {
                    ptr[0] = buf[0];
                }
                else if (fileBytes == 1 && sh.nBands == 4)
                {
                    for (int i = 0; i < 3; i++)
                        ptr[i] = (isGray) ? buf[0] : colormap[buf[0]][i];
                    ptr[3] = 255;   // full alpha;
                }
                else if ((fileBytes == 3 || fileBytes == 4) && sh.nBands == 4)
                {
                    int i;
                    for (i = 0; i < fileBytes; i++)
                        ptr[i] = buf[i];
                    if (i == 3) // missing alpha channel
                        ptr[3] = 255;   // full alpha;
                }
                else
            	    throw CError("ReadFileTGA(%s): unhandled pixel depth or # of bands", filename);
            }
        }
    }

    if (fclose(stream))
        throw CError("ReadFileTGA(%s): error closing file", filename);
}

void WriteFileTGA(CImage img, const char* filename)
{
    // Only 1, 3, or 4 bands supported
    CShape sh = img.Shape();
    int nBands = sh.nBands;
    if (nBands != 1 && nBands != 3 && nBands != 4)
        throw CError("WriteFileTGA(%s): can only write 1, 3, or 4 bands", filename);

    // Only unsigned_8 supported directly
#if 0   // broken for now
    if (img.PixType() != unsigned_8)
    {
        CImage u8img(sh, unsigned_8);
        TypeConvert(img, u8img);
        img = u8img;
    }
#endif

    // Fill in the header structure
    CTargaHead h;
    memset(&h, 0, sizeof(h));
    h.imageType = (nBands == 1) ? TargaRawBW : TargaRawRGB;
        // TODO:  is TargaRawBW the right thing, or only binary?
    h.width     = sh.width;
    h.height    = sh.height;
    h.pixelSize = 8 * nBands;
    bool reverseRows = false;   // TODO: when is this true?

    // Open the file and write the header
    FILE *stream = fopen(filename, "wb");
    if (stream == 0)
        throw CError("WriteFileTGA: could not open %s", filename);
    if (fwrite(&h, sizeof(CTargaHead), 1, stream) != 1)
	    throw CError("WriteFileTGA(%s): file is too short", filename);

    // Write out the rows
    for (int y = 0; y < sh.height; y++)
    {
        int yr = reverseRows ? sh.height-1-y : y;
        char* ptr = (char *) img.PixelAddress(0, yr, 0);
        int n = sh.width*sh.nBands;
    	if (fwrite(ptr, sizeof(uchar), n, stream) != n)
    	    throw CError("WriteFileTGA(%s): file is too short", filename);
    }

    if (fclose(stream))
        throw CError("WriteFileTGA(%s): error closing file", filename);
}

void ReadFile (CImage& img, const char* filename)
{
    // Determine the file extension
    const char *dot = strrchr(filename, '.');
    if (strcmp(dot, ".tga") == 0 || strcmp(dot, ".tga") == 0)
    {
        if ((&img.PixType()) == 0)
            img.ReAllocate(CShape(), typeid(uchar), sizeof(uchar), true);
        if (img.PixType() == typeid(uchar))
            ReadFileTGA(*(CByteImage *) &img, filename);
        else
           throw CError("ReadFile(%s): haven't implemented conversions yet", filename);
    }
    else
        throw CError("ReadFile(%s): file type not supported", filename);
}

void WriteFile(CImage& img, const char* filename)
{
    // Determine the file extension
    const char *dot = strrchr(filename, '.');
    if (strcmp(dot, ".tga") == 0 || strcmp(dot, ".tga") == 0)
    {
        if (img.PixType() == typeid(uchar))
            WriteFileTGA(*(CByteImage *) &img, filename);
        else
           throw CError("ReadFile(%s): haven't implemented conversions yet", filename);
    }
    else
        throw CError("WriteFile(%s): file type not supported", filename);
}
