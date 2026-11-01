// Container.h: CipSoft's container templates (c:\mud\tibiaplayer\Container.h).
//
// Only the parts whose instantiations have been reconstructed are here. The
// assertion texts and line numbers come from the instantiations; member names
// other than space and block are ours.
//
// vector<T> is a growable array indexed from min to max. Indexing outside the
// current range grows it by block entries at a time (by its current size
// when block is 0) and, if initialized is set, fills new entries with init.
//
// Instantiations reconstructed so far:
//   vector<TMessage>        constructor 0x4e7f50, operator() 0x4e7390 (Status.cpp)
//   vector<TErrorReport>    constructor 0x4e8230, operator() 0x4e7860 (Status.cpp)
//   vector<TContentWindowSize>  constructor 0x4dbe90, operator() 0x4db880 (Preferences.cpp)
//   vector<TBuddy>          constructor 0x4dc170, operator() 0x4dba60 (Preferences.cpp)
//   vector<TTalkFilter *>   initialized constructor 0x4fd6e0 (Talk.cpp),
//                           operator() folded with vector<int> at 0x452ac0
//   vector<TMark>           constructor 0x426170, operator() 0x425b20 (Map.cpp; the
//                           linker kept the copies of an object before Map.obj)
// Preferences.cpp also uses vector<int>, whose code the linker kept from an
// earlier object: constructors 0x453300 (with init) and 0x4a2b90,
// operator() 0x452ac0. 0x453300 and 0x452ac0 sit at the start of
// DialogElements.obj (the first user), next to its other instance:
//   vector<TListBoxLine>    constructor 0x453600, operator() 0x452eb0
//                           (DialogElements.h's TFontListBox lines, 0x18 bytes,
//                           the implicit element constructor 0x452c70)
// DialogElements.cpp also uses vector<int> (TEditText's line starts) and
// vector<TEditLine *> and vector<char *> (folded with vector<int>'s code).
// The destructor's out-of-line instances are 0x452c50 (vector<int>, entry at
// +0x1c) and 0x452c60 (vector<TListBoxLine>, entry at +0x30), called from
// unwind funclets; the static initialisers register atexit handlers
// (0x55a320 and following) that free entry inline. Dialogs.obj adds a vector
// of a 31-byte element (destructor 0x46bb60, operator() 0x46bb70,
// constructor 0x46c0d0).

#pragma once

#include "common.h"
#include <sstream>

// array<T> is a fixed array indexed from min to min+d-1 (lines 152-178;
// it comes before vector in the original file). The class name is ours;
// min, d, i, low() and high() are named in the assertions and in the
// error message of operator(). Instantiations reconstructed so far:
//   array<TProperties>  constructor 0x4cfa30, operator() 0x4d1a10 (Objects.cpp)
//   array<TMapPoint>    constructor 0x4b0bc0, operator() 0x4b5900 (Map.cpp)
template <class T> class array {
public:
	array(int min, int max);
	// Inlined into Objects.cpp's ReleaseObjects (delete[] entry, then
	// operator delete on the array itself).
	~array() { delete[] entry; }
	T &operator()(int i);
	int low() { return min; }
	int high() { return min + d - 1; }

	int min;  // +0x00
	int d;    // +0x04, number of entries
	T *entry; // +0x08
};

// Line 152. No try block; the assertion is the only EH state.
template <class T> array<T>::array(int min, int max)
{
	this->min = min;
	d = max - min + 1;
	TASSERT1(d>0, d);
	entry = new T[d];
}

// Line 178. The error has no try block either; its message is built in a
// stringstream.
template <class T> T &array<T>::operator()(int i)
{
	i -= min;
	if (i < 0 || i >= d) {
		std::stringstream s;
		s << "index out of range (" << "original i: " << i + min << ", i: " << i << ", min: " << min
			<< ", d: " << d << ")";
		TERROR(s.str().c_str());
	}
	return entry[i];
}

// matrix<T> is a fixed two-dimensional array indexed from xmin/ymin (lines
// 217-253); matrix3d<T> its three-dimensional counterpart (lines 282-328).
// The class names are ours; xmin, ymin, zmin, dx, dy, dz, x, y and z are named
// in the assertions and error messages. x is the fastest index. The
// destructors free the entries; their only out-of-line copy is matrix's at
// 0x4b0b20, called when an exception unwinds Map.cpp's FindPath.
// Instantiations reconstructed so far (Map.cpp):
//   matrix<TPathNode>  constructor 0x4b1100, operator() 0x4b5e90, destructor 0x4b0b20
//   matrix3d<int>      constructor 0x4b0d70, operator() 0x4b5b40
template <class T> class matrix {
public:
	matrix(int xmin, int xmax, int ymin, int ymax);
	~matrix() { delete[] entry; }
	T &operator()(int x, int y);

	int xmin; // +0x00
	int ymin; // +0x04
	int dx;   // +0x08
	int dy;   // +0x0c
	T *entry; // +0x10
};

// Lines 217-218. No try block; the assertions are the only EH states.
template <class T> matrix<T>::matrix(int xmin, int xmax, int ymin, int ymax)
{
	this->xmin = xmin;
	this->ymin = ymin;
	dx = xmax - xmin + 1;
	dy = ymax - ymin + 1;
	TASSERT1(dx>0, dx);
	TASSERT1(dy>0, dy);
	entry = new T[dx * dy];
}

// Line 253.
template <class T> T &matrix<T>::operator()(int x, int y)
{
	x -= xmin;
	y -= ymin;
	if (x < 0 || x >= dx || y < 0 || y >= dy) {
		std::stringstream s;
		s << "index out of range (" << "original x: " << x + xmin << ", original y: " << y + ymin
			<< ", x: " << x << ", y: " << y << ", xmin: " << xmin << ", ymin: " << ymin << ", dx: " << dx
			<< ", dy: " << dy << ")";
		TERROR(s.str().c_str());
	}
	return entry[y * dx + x];
}

template <class T> class matrix3d {
public:
	matrix3d(int xmin, int xmax, int ymin, int ymax, int zmin, int zmax);
	~matrix3d() { delete[] entry; }
	T &operator()(int x, int y, int z);

	int xmin; // +0x00
	int ymin; // +0x04
	int zmin; // +0x08
	int dx;   // +0x0c
	int dy;   // +0x10
	int dz;   // +0x14
	T *entry; // +0x18
};

// Lines 282-284.
template <class T> matrix3d<T>::matrix3d(int xmin, int xmax, int ymin, int ymax, int zmin, int zmax)
{
	this->xmin = xmin;
	this->ymin = ymin;
	this->zmin = zmin;
	dx = xmax - xmin + 1;
	dy = ymax - ymin + 1;
	dz = zmax - zmin + 1;
	TASSERT1(dx>0, dx);
	TASSERT1(dy>0, dy);
	TASSERT1(dz>0, dz);
	entry = new T[dx * dy * dz];
}

// Line 328. Unlike the other containers, the error names a bug number as its
// expression.
template <class T> T &matrix3d<T>::operator()(int x, int y, int z)
{
	x -= xmin;
	y -= ymin;
	z -= zmin;
	if (x < 0 || x >= dx || y < 0 || y >= dy || z < 0 || z >= dz) {
		std::stringstream s;
		s << "index out of range (" << "original x: " << x + xmin << ", original y: " << y + ymin
			<< ", original z: " << z + zmin << ", x: " << x << ", y: " << y << ", z: " << z
			<< ", xmin: " << xmin << ", ymin: " << ymin << ", zmin: " << zmin << ", dx: " << dx
			<< ", dy: " << dy << ", dz: " << dz << ")";
		throw TMYEXCEPTION(s.str().c_str(), "", 0, "", 0, "[bug0000013]");
	}
	return entry[(z * dy + y) * dx + x];
}

template <class T> class vector {
public:
	vector(int min, int max, int block, T init);
	vector(int min, int max, int block);
	// Inlined everywhere: the atexit thunks of the static vectors
	// (0x55a320-0x55a3c0) and Map.cpp's TAutomapBlock destructor 0x4b0b30
	// free entry.
	~vector() { delete[] entry; }
	T &operator()(int i);

	int min;          // +0x00, lowest index used so far
	int max;          // +0x04, highest index used so far
	int start;        // +0x08, index of entry[0]
	int space;        // +0x0c, allocated entries
	int block;        // +0x10, growth step, 0 = double
	bool initialized; // +0x14, fill new entries with init
	T init;           // +0x15 or the next aligned offset
	T *entry;
};

// Lines 353-358 (vector<int> at 0x453300). Every entry starts as init.
template <class T> vector<T>::vector(int min, int max, int block, T init)
{
	this->min = min;
	this->max = max;
	start = min;
	space = max - min + 1;
	this->block = block;
	initialized = true;
	this->init = init;
	TASSERT1(space>0, space);
	TASSERT1(block>=0, block);
	entry = new T[space];
	for (int i = 0; i < space; i++)
		entry[i] = init;
}

// Lines 368-370. No try block; the two assertions are the only EH states.
template <class T> vector<T>::vector(int min, int max, int block)
{
	this->min = min;
	this->max = max;
	start = min;
	space = max - min + 1;
	this->block = block;
	initialized = false;
	TASSERT1(space>0, space);
	TASSERT1(block>=0, block);
	entry = new T[space];
}

// Grows the array until i is inside it and returns the entry. Existing
// entries are moved by swapping them with the fresh ones.
template <class T> T &vector<T>::operator()(int i)
{
	while (i < start) {
		int b = block;
		if (b == 0)
			b = space;
		T *help = new T[space + b];
		for (int j = min; j <= max; j++) {
			T h = help[j - start + b];
			help[j - start + b] = entry[j - start];
			entry[j - start] = h;
		}
		delete[] entry;
		entry = help;
		start -= b;
		space += b;
	}
	while (i >= start + space) {
		int b = block;
		if (b == 0)
			b = space;
		T *help = new T[space + b];
		for (int j = min; j <= max; j++) {
			T h = help[j - start];
			help[j - start] = entry[j - start];
			entry[j - start] = h;
		}
		delete[] entry;
		entry = help;
		space += b;
	}
	while (i < min) {
		min--;
		if (initialized)
			entry[min - start] = init;
	}
	while (i > max) {
		max++;
		if (initialized)
			entry[max - start] = init;
	}
	return entry[i - start];
}
