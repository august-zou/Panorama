#ifndef FEATURESET_H
#define FEATURESET_H

#include <vector>

using namespace std;

// FeatureMatch is used by your feature matching routine to store the
// ID of the two matching features, as well as the score
// of the match.
struct FeatureMatch {
	int id1, id2;  // IDs of the matching features.  id1 is the feature in the 
                   // first image, id2 in the second image.  The IDs start with 1,
                   // so the feature corresponding to id1 is actually f1[id1-1].
	double score;
};

// The Feature class stores the feature ID, location, and a vector of
// whatever attributes you choose to use.  It also has methods for
// drawing the feature and printing its description to the console.
// Feel free to change these.
class Feature {
public:
	int type;
	int id;
	int x;
	int y;
    double angleRadians;

	vector<double> data;

	bool selected;

public:
	// Create a feature.
	Feature();

	// Print the feature, currently just prints the location.
	void print() const;

	// Reads a SIFT feature.
	void read_sift(istream &is);
};

// You don't have to modify these.
ostream &operator<<(ostream &os, const Feature &f);
istream &operator>>(istream &is, Feature &f);

// The FeatureSet class represents a vector of features for a single
// image.  You don't need to modify it.
class FeatureSet : public vector<Feature> {
public:
	// Create a feature set.
	FeatureSet();

	// Load a feature set from a file.
	bool load(const char *name);

	// Load a SIFT feature set.
	bool load_sift(const char *name);

	// Save a feature set to file.
	bool save(const char *name) const;

	// Select (or deselect) features at a location.
	void select_point(int x, int y);

	// Select (or deselect) features inside a box.
	void select_box(int xMin, int xMax, int yMin, int yMax);

	// Select all features.
	void select_all();

	// Deselect all features.
	void deselect_all();

	// Take only the selected features.
	void get_selected_features(FeatureSet &f);
};

#endif