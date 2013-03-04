#include <fstream>
#include <math.h>
#include "FeatureSet.h"

// Create a feature.
Feature::Feature() {
	selected = false;
}

// Print the (x,y) location of a feature.  You can modify this to print
// whatever attributes you use.
void Feature::print() const {
	printf("(%d,%d)\n", x, y);
}

// Reads a SIFT feature.
void Feature::read_sift(istream &is) {
	// Let's use type 9 for SIFT features.
	type = 9;

	double xSub;
	double ySub;
	double scale;
	double rotation;

	// Read the feature location, scale, and orientation.
	is >> xSub >> ySub >> scale >> rotation;

	// They give row first, then column.
	x = (int) (ySub + 0.5);
	y = (int) (xSub + 0.5);

	data.resize(128);

	// Read the descriptor vector.
	for (int i=0; i<128; i++) {
		is >> data[i];
	}
}

// Write the feature to an output stream.
ostream &operator<<(ostream &os, const Feature &f) {
	os << f.type << '\n';
	os << f.id << '\n';
	os << f.x << ' ' << f.y << '\n';
    os << f.angleRadians << '\n';

	os << f.data.size() << '\n';

	for (unsigned int i=0; i<f.data.size(); i++) {
		os << f.data[i] << '\n';
	}

	return os;
}

// Read the feature from an input stream.
istream &operator>>(istream &is, Feature &f) {
	int n;

	is >> f.type;
	is >> f.id;
	is >> f.x >> f.y;
    is >> f.angleRadians;

	is >> n;

	f.data.clear();
	f.data.resize(n);

	for (int i=0; i<n; i++) {
		is >> f.data[i];
	}

	return is;
}

// Create a feature set.
FeatureSet::FeatureSet() {
}

// Load a feature set from a file.
bool FeatureSet::load(const char *name) {
	int n;

	// Clear the currently loaded features.
	clear();

	// Open the file.
	ifstream f(name);

	if (!f.is_open()) {
		return false;
	}

	// Read the total number of features.
	f >> n;

	// Resize the vector of features.
	resize(n);

	// Read each of the features.
	iterator i = begin();

	while (i != end()) {
		f >> (*i);
		i++;
	}

	// Close the file.
	f.close();

	return true;
}

// Load a SIFT feature set.
bool FeatureSet::load_sift(const char *name) {
	int n;
	int m;

	// Clear the currently loaded features.
	clear();

	// Open the file.
	ifstream f(name);

	if (!f.is_open()) {
		return false;
	}

	// Read the total number of features.
	f >> n;

	// Read the length of each feature.  It better be 128.
	f >> m;

	if (m != 128) {
		f.close();
		return false;
	}

	// Resize the vector of features.
	resize(n);

	// Read each of the features.
	iterator i = begin();
	int id = 1;

	while (i != end()) {
		(*i).read_sift(f);
		(*i).id = id;

		i++;
		id++;
	}

	// Close the file.
	f.close();

	return true;
}

// Save a feature set to file.
bool FeatureSet::save(const char *name) const {
	// Open the file.
	ofstream f(name);

	if (!f.is_open()) {
		return false;
	}

	// Write the number of features.
	f << size() << '\n';

	// Write each of the features.
	const_iterator i = begin();

	while (i != end()) {
		f << (*i);
		i++;
	}

	// Close the file.
	f.close();

	return true;
}

// Select (or deselect) features at a location.
void FeatureSet::select_point(int x, int y) {
	iterator i = begin();

	while (i != end()) {
		// If the given point is within 5 pixels of the feature, then
		// select it.  This can select multiple features if they are
		// nearly collocated.
	        if ((fabs((double)(*i).x-x) <= 3) && (fabs((double)(*i).y-y) <= 3)) {
			(*i).selected = (!(*i).selected);
		}

		i++;
	}
}

// Select (or deselect) features inside a box.
void FeatureSet::select_box(int xMin, int xMax, int yMin, int yMax) {
	iterator i = begin();

	while (i != end()) {
		if (((*i).x >= xMin) && ((*i).x <= xMax) && ((*i).y >= yMin) && ((*i).y <= yMax)) {
			(*i).selected = (!(*i).selected);
		}

		i++;
	}
}

// Select all features.
void FeatureSet::select_all() {
	iterator i = begin();

	while (i != end()) {
		(*i).selected = true;
		i++;
	}
}

// Deselect all features.
void FeatureSet::deselect_all() {
	iterator i = begin();

	while (i != end()) {
		(*i).selected = false;
		i++;
	}
}

// Take only the selected features.
void FeatureSet::get_selected_features(FeatureSet &f) {
	f.clear();

	iterator i = begin();

	while (i != end()) {
		if ((*i).selected) {
			f.push_back((*i));
		}

		i++;
	}
}
