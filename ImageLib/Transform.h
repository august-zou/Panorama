///////////////////////////////////////////////////////////////////////////
//
// NAME
//  Transform.h -- coordinate transformation matrices
//
// DESCRIPTION
//  These classes are used to implement 3x3 and 4x4 coordinate transformation
//  matrices.  Basic operations supported are:
//   . multiplication
//   . inverses
//   . translation and rotation
//
// SEE ALSO
//  Transform.cpp       implementation
//
// Copyright © Richard Szeliski, 2001.  See Copyright.h for more details
// (modified for CSE576 Spring 2005)
//
///////////////////////////////////////////////////////////////////////////

class CVector3
{
public:
	CVector3();                         // default constructor (zero)
	double &operator[](int i);          // element access
	const double &operator[](int i) const;    // element access
private:
	double m_array[3];
};

inline double &CVector3::operator[](int i)
{
    return m_array[i];
}

inline const double &CVector3::operator[](int i) const
{
    return m_array[i];
}

class CTransform3x3
{
public:
    CTransform3x3();                    // default constructor (identity)
    static CTransform3x3 Translation(float tx, float ty);
    static CTransform3x3 Rotation(float degrees);
    CTransform3x3 Inverse(void  );      // matrix inverse
	CVector3 operator*(const CVector3& v) const;
    CTransform3x3 operator*(const CTransform3x3& m);
    double* operator[](int i);          // access the elements
    const double* operator[](int i) const;    // access the elements
private:
    double m_array[3][3];               // data array
};

inline double* CTransform3x3::operator[](int i)
{
    return m_array[i];
}

inline const double* CTransform3x3::operator[](int i) const
{
    return m_array[i];
}
