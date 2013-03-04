///////////////////////////////////////////////////////////////////////////
//
// NAME
//  Project2.cpp -- command-line (shell) interface to project 2 code
//
// SYNOPSIS
//  Project2 sphrWarp input.tga output.tga f [k1 k2]
//  Project2 alignPair input1.f input2.f nRANSAC RANSACthresh [sift]
//  Project2 blendPairs pairlist.txt outfile.tga blendWidth
//  Project2 script script.cmd
//
// PARAMTERS
//  input.tga       input image
//  output.tga      output image
//
//  f               focal length in pixels
//  k1, k2          radial distortion parameters
//
//  input*.f        input feature set
//
//  nRANSAC         number of RANSAC iterations
//  RANSACthresh    RANSAC distance threshold for inliers
//  sift            the word "sift"
//
//  pairlist.txt    file of image pair names and relative translations
//                  this is usually the concatenation of outputs from alignPair
//
//  blendWidth		width of the horizontal blending function
//
//  script.cmd      script file (command line file)
//
// DESCRIPTION
//  This file contains a set of simple command line processes from which
//  you can build a simple automatic cylindrical stitching application.
//
//  Use the programs here to perform a series of operations such as:
//  1. warp all of the images into spherical coordinate (and undo radial distortion)
//  2. align pairs of images using a feature matcher
//  3. read in all of the images and perform pairwise blends
//     to obtain a final (rectified and trimmed) mosaic
//
// TIPS
//  To become familiar with this code, single-step through a couple of
//  examples.  Also, if you are running inside the debugger, place
//  a breakpoint inside the catch statement and at the last statement
//  in main() so you can see the error message before the program exits.
//
// SEE ALSO
//  Image.h             basic image class documentation
//
// (modified for CSE576 Spring 2005)
///////////////////////////////////////////////////////////////////////////

#include <math.h>
#include <assert.h>
#include <cstring>

#include <fstream>
#include <FL/Fl.H>
#include <FL/Fl_Shared_Image.H>

#include "ImageLib/ImageLib.h"
#include "WarpSpherical.h"
//#include "FeatureMatch.h"
#include "FeatureAlign.h"
#include "BlendImages.h"

int main(int argc, const char *argv[]);     // forward declaration
bool LoadImageFile(const char *filename, CByteImage &image);
void convertToByteImage(CFloatImage &floatImage, CByteImage &byteImage);
void convertToFloatImage(CByteImage &byteImage, CFloatImage &floatImage);
bool convertImage(const Fl_Image *image, CByteImage &convertedImage);


int SphrWarp(int argc, const char *argv[])
{
    // Warp the input image to correct for radial distortion and/or map to spherical coordinates
    if (argc < 5)
    {
        printf("usage: %s input.tga output.tga f [k1 k2]\n", argv[1]);
        return -1;
    }
    const char *infile  = argv[2];
	const char *outfile = argv[3];
	float f             = (float) atof(argv[4]);
	float k1            = (argc > 5) ? (float) atof(argv[5]) : 0.0f;
	float k2            = (argc > 6) ? (float) atof(argv[6]) : 0.0f;

	CByteImage src, dst, temp;

	/* Added by Adarsh */
	{
		bool success = LoadImageFile(infile, src);
		if (!success) {
			printf("couldn't load image 1\n");
			return -1;
		}
		else
		{
			printf("Done loading file\n");
		}
	}

	CShape sh = src.Shape();

    CTransform3x3 R;

#define THETA 0.0 // 0.1638
    R[0][0] = 1.0;  R[0][1] = 0.0;  R[0][2] = 0.0;
    R[1][0] = 0.0; R[1][1] = cos(THETA); R[1][2] = -sin(THETA);
    R[2][0] = 0.0; R[2][1] = sin(THETA);  R[2][2] = cos(THETA);

    // CFloatImage uv = WarpSphericalField(sh, sh, f, k1, k2, CTransform3x3());
    CFloatImage uv = WarpSphericalField(sh, sh, f, k1, k2, R);
    WarpLocal(src, dst, uv, false, eWarpInterpLinear);
    WriteFile(dst, outfile);
    return 0;
}


bool ReadFeatureMatches(const char *filename, vector<FeatureMatch> &matches)
{
    FILE *f = fopen(filename, "r");

    if (f == NULL)
        return false;

    int num_matches;
    fscanf(f, "%d\n", &num_matches);

    matches.resize(num_matches);

    for (int i = 0; i < num_matches; i++) {
        FeatureMatch match;

        int id1, id2;
        double score;
        fscanf(f, "%d %d %lf\n", &id1, &id2, &score);

        match.id1 = id1;
        match.id2 = id2;
        match.score = score;
        matches[i] = match;
    }

    return true;
}

int AlignPair(int argc, const char *argv[])
{
    // Align two images using feature matching
    if (argc < 7)
        {
            printf("usage: %s input1.f input2.f matchfile nRANSAC RANSACthresh [sift]\n", argv[1]);
            return -1;
        }
    const char *infile1 = argv[2];
    const char *infile2 = argv[3];
    const char *matchfile = argv[4];
    int nRANSAC         = atoi(argv[5]);
    double RANSACthresh = atof(argv[6]);

    FeatureSet f1, f2;

    // Read in the feature sets
    if ((argc >= 8) && (strcmp(argv[7], "sift") == 0)) {
        f1.load_sift(infile1);
        f2.load_sift(infile2);
    }
    else {
        f1.load(infile1);
        f2.load(infile2);
    }

    CTransform3x3 M;

    // Read in the feature matches
    vector<FeatureMatch> matches;
    bool success = ReadFeatureMatches(matchfile, matches);

    if (!success) {
        printf("Error opening match file %s for reading\n", matchfile);
        return -1;
    }

#if 0
    // Align two images using feature matching
    if (argc < 6)
    {
        printf("usage: %s input1.f input2.f nRANSAC RANSACthresh [sift]\n", argv[1]);
        return -1;

    }
    const char *infile1 = argv[2];
    const char *infile2 = argv[3];
    int nRANSAC         = atoi(argv[4]);
    double RANSACthresh = atof(argv[5]);

    FeatureSet f1, f2;

    //printf("Reading features\n");

    // Read in the feature sets
    if ((argc >= 7) && (strcmp(argv[6], "sift") == 0)) {
	f1.load_sift(infile1);
	f2.load_sift(infile2);
    } else {
	f1.load(infile1);
	f2.load(infile2);
    }

    vector<FeatureMatch> matches;
    CTransform3x3 M;

    // call your feature matching routine and store the result
    // in matches
    // printf("Finding matches\n");
    fastMatchFeatures(f1, f2, matches);
#endif

    // Performs the alignment.
    // printf("Aligning pair\n");
    // alignPair(f1, f2, matches, eTranslate, 0.0f, nRANSAC, RANSACthresh, M);
    alignPair(f1, f2, matches, eTranslate, 0.0f, nRANSAC, RANSACthresh, M);

    // Print out the result
    if ((argc >= 7) && (strcmp(argv[6], "sift") == 0)) {
        printf("%.2f %.2f\n", M[0][2], -M[1][2]);
    } else {
        printf("%.2f %.2f\n", M[0][2], M[1][2]);
        // printf("%0.3f %0.3f %0.3f %0.3f %0.3f %0.3f %0.3f %0.3f %0.3f\n",
        //         M[0][0], M[0][1], M[0][2],
        //         M[1][0], M[1][1], M[1][2],
        //         M[2][0], M[2][1], M[2][2]);
    }

    return 0;
}

int BlendPairs(int argc, const char *argv[])
{
    // Blend a sequence of images given the pairwise transformations
    if (argc < 5)
    {
        printf("usage: %s pairlist.txt outimg.tga blendWidth\n", argv[1]);
        return -1;
    }
    const char *pairlist= argv[2];
    const char *outfile = argv[3];
    float blendWidth    = (float) atof(argv[4]);

    // Open the list of image pairs
    FILE *stream = fopen(pairlist, "r");
    if (stream == 0)
        throw CError("%s: could not open the file %s", argv[1], pairlist);

    // Construct the list of images and translations
    CImagePositionV ipList;
    char line[1024], infile1[1024], infile2[1024];
    // float rel_t[2];
    CTransform3x3 M;

    int n;
    for (n = 0; fgets(line, 1024, stream); n++)
    {
        // Compute the position from the PREVIOUS displacement
        CImagePosition ip;

	if (n == 0) {
	    ip.position = CTransform3x3::Translation(0.0, 0.0); /* Ident matrix */
	    // ip.position = CTransform3x3::Rotation(45.0); /* Ident matrix */
	} else {
	    ip.position = ipList[n-1].position * M;
	}

        // for (int k = 0; k < 2; k++)
        //    ip.position[k] = (n > 0) ? ipList[n-1].position[k] - rel_t[k] : 0.0f;

        // Read the line and push the image onto the list
        // if (sscanf(line, "%s %s %f %f", infile1, infile2,
        //           &rel_t[0], &rel_t[1]) != 4)
        //    throw CError("%s: error reading %s", argv[1], pairlist);

        if (sscanf(line, "%s %s %lf %lf", infile1, infile2,
			&(M[0][2]), &(M[1][2])) != 4)
			throw CError("%s: error reading %s\n", argv[1], pairlist);

		M[1][2] = -M[1][2];

		ReadFile(ip.img, infile1);
		ipList.push_back(ip);
	}

	// Read in the last image
	CImagePosition ip;

	// for (int k = 0; k < 2; k++)
	//     ip.position[k] = ipList[n-1].position[k] - rel_t[k];
	ip.position = ipList[n-1].position * M;

	ReadFile(ip.img, infile2);
	ipList.push_back(ip);
	fclose(stream);

	CByteImage result = BlendImages(ipList, blendWidth);
	WriteFile(result, outfile);
	return 0;
}

int Script(int argc, const char *argv[])
{
    // Read a series of commands from a script file
    //  (an alternative to a shell-level command file)
    if (argc < 3)
    {
        printf("usage: %s script.cmd\n", argv[1]);
        return -1;
    }
    FILE *stream = fopen(argv[2], "r");
    if (stream == 0)
        throw CError("Could not open %s", argv[2]);

    // Process each command line
    char line[1024];
    while (fgets(line, 1024, stream))
    {
        fprintf(stderr, line);
        if (line[0] == '/' && line[1] == '/')
            continue;   // skip the comment line
        char *ptr = line;
        char *argv2[256];
        int argc2;
        for (argc2 = 0; argc2 < 256 && *ptr; argc2++)
        {
            while (*ptr && isspace(*ptr)) ptr++;
            argv2[argc2] = ptr;
            while (*ptr && !isspace(*ptr)) ptr++;
            if (*ptr)
                *ptr++ = 0;     // null terminate the argument
        }
        if (argc2 < 2)
            continue;

        // Call the dispatch routine
        int code = main(argc2, (const char **) argv2);
        if (code)
            return code;
    }
    fclose(stream);
    return 0;
}

int main(int argc, const char *argv[])
{
	// This lets us load various image formats.
	fl_register_images();
	try
	{
		// Branch to processing code based on first argument
		if (argc > 1 && strcmp(argv[1], "sphrWarp") == 0)
			return SphrWarp(argc, argv);
		else if (argc > 1 && strcmp(argv[1], "alignPair") == 0)
			return AlignPair(argc, argv);
		else if (argc > 1 && strcmp(argv[1], "blendPairs") == 0)
			return BlendPairs(argc, argv);
		else if (argc > 1 && strcmp(argv[1], "script") == 0)
			return Script(argc, argv);
		else {
			printf("usage: \n");
	        printf("	%s sphrWarp input.tga output.tga f [k1 k2]\n", argv[0]);
			printf("	%s alignPair input1.f input2.f matchfile nRANSAC RANSACthresh [sift]\n", argv[0]);
			printf("	%s blendPairs pairlist.txt outimg.tga blendWidth\n", argv[0]);
			printf("	%s script script.cmd\n", argv[0]);
		}
    }
    catch (CError &err) {
        printf(err.message);
        return -1;
    }
    return 0;
}
bool LoadImageFile(const char *filename, CByteImage &image)
{
	// Load the query image.
	printf("%s\n", filename);
	Fl_Shared_Image *fl_image = Fl_Shared_Image::get(filename);

	if (fl_image == NULL) {
		printf("TGA\n");
		ReadFile(image, filename);
		return true;
	} else {
		printf("Not TGA\n");
		CShape sh(fl_image->w(), fl_image->h(), 4);
		image = CByteImage(sh);

	    // Convert the image to the CImage format.
	    if (!convertImage(fl_image, image)) {
		    printf("couldn't convert image to RGB format\n");
		    return false;
	    }

		/* Set the alpha channel to 1 everywhere */
		int w = sh.width;
		int h = sh.height;

		sh = image.Shape();
		// printf("shape: %d, %d, %d\n", sh.width, sh.height, sh.nBands);
		for (int y = 0; y < h; y++) {
			for (int x = 0; x < w; x++) {
				image.Pixel(x, y, 3) = 255;
			}
		}

        return true;
    }
}

// Convert CFloatImage to CByteImage.
void convertToByteImage(CFloatImage &floatImage, CByteImage &byteImage) {
	CShape sh = floatImage.Shape();

	//printf("%d\n", floatImage.Shape().nBands);
	//printf("%d\n", byteImage.Shape().nBands);

    assert(floatImage.Shape().nBands == min(byteImage.Shape().nBands, 3));
	for (int y=0; y<sh.height; y++) {
		for (int x=0; x<sh.width; x++) {
			for (int c=0; c<sh.nBands; c++) {
				float value = floor(255*floatImage.Pixel(x,y,c) + 0.5f);

				if (value < byteImage.MinVal()) {
					value = byteImage.MinVal();
				}
				else if (value > byteImage.MaxVal()) {
					value = byteImage.MaxVal();
				}

				// We have to flip the image and reverse the color
				// channels to get it to come out right.  How silly!
				byteImage.Pixel(x,sh.height-y-1,sh.nBands-c-1) = (uchar) value;
			}
		}
	}
}

void convertToFloatImage(CByteImage &byteImage, CFloatImage &floatImage) {
	CShape sh = byteImage.Shape();
	//printf("%d\n", floatImage.Shape().nBands);
	//printf("%d\n", byteImage.Shape().nBands);

    assert(floatImage.Shape().nBands == min(byteImage.Shape().nBands, 3));
	for (int y=0; y<sh.height; y++) {
		for (int x=0; x<sh.width; x++) {
			for (int c=0; c<min(3,sh.nBands); c++) {
				float value = byteImage.Pixel(x,y,c) / 255.0f;

				if (value < floatImage.MinVal()) {
					value = floatImage.MinVal();
				}
				else if (value > floatImage.MaxVal()) {
					value = floatImage.MaxVal();
				}

				// We have to flip the image and reverse the color
				// channels to get it to come out right.  How silly!
				floatImage.Pixel(x,sh.height-y-1,min(3,sh.nBands)-c-1) = value;
			}
		}
	}
}

// Convert Fl_Image to CFloatImage.
bool convertImage(const Fl_Image *image, CByteImage &convertedImage) {
	if (image == NULL) {
		return false;
	}

	// Let's not handle indexed color images.
	if (image->count() != 1) {
		return false;
	}

	int w = image->w();
	int h = image->h();
	int d = image->d();

	// Get the image data.
	const char *const *data = image->data();

	int index = 0;

	for (int y=0; y<h; y++) {
		for (int x=0; x<w; x++) {
			if (d < 3) {
				// If there are fewer than 3 channels, just use the
				// first one for all colors.
				convertedImage.Pixel(x,y,0) = data[0][index];
				convertedImage.Pixel(x,y,1) = data[0][index];
				convertedImage.Pixel(x,y,2) = data[0][index];
			}
			else if (d == 3) {
				// Otherwise, use the first 3.
				convertedImage.Pixel(x,h-y-1,2) = data[0][index];
				convertedImage.Pixel(x,h-y-1,1) = data[0][index+1];
				convertedImage.Pixel(x,h-y-1,0) = data[0][index+2];
			}

			index += d;
		}
	}

	return true;
}
