///////////////////////////////////////////////////////////////////////////
//
// NAME
//  FeatureAlign.h -- image registration using feature matching
//
// SPECIFICATION
//  int alignPair(const FeatureSet &f1, const FeatureSet &f2, const vector<FeatureMatch> &matches, MotionModel m, float f, int nRANSAC, double RANSACthresh, CTransform3x3& M);
//
// PARAMETERS
//  f1, f2              source feature sets
//  matches				correspondences between f1 and f2
//  m                   motion model
//  f                   focal length
//  nRANSAC             number of RANSAC iterations
//  RANSACthresh        RANSAC distance threshold
//  M                   transformation matrix (output)
//
// DESCRIPTION
//  These routines compute the alignment between two images using
//  feature-based motion estimation.  The features and their
//  correspondences have already been computed.
//
// SEE ALSO
//  FeatureAlign.cpp    implementation
//
// Copyright © Richard Szeliski, 2001.  See Copyright.h for more details
// (modified for CSE576 Spring 2005)
//
///////////////////////////////////////////////////////////////////////////

#include "FeatureSet.h"

enum MotionModel
{
    eTranslate           = 0,    // images are translated only
    eRotate3D            = 1     // images are translated and rotated
};

// Compute transformation between two images.
int alignPair(const FeatureSet &f1, const FeatureSet &f2,
			  const vector<FeatureMatch> &matches, MotionModel m, float f,
			  int nRANSAC, double RANSACthresh, CTransform3x3& M);

// Count the number of feature matches concurring with transformation.
int countInliers(const FeatureSet &f1, const FeatureSet &f2,
				 const vector<FeatureMatch> &matches, MotionModel m, float f,
				 CTransform3x3 M, double RANSACthresh, vector<int> &inliers);

// Compute the least squares optimal transformation from inlier feature
// matches.
int leastSquaresFit(const FeatureSet &f1, const FeatureSet &f2,
					const vector<FeatureMatch> &matches, MotionModel m, float f,
					const vector<int> &inliers, CTransform3x3& M);
