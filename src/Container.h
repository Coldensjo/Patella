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
//   vector<TContentWindow>  constructor 0x4dbe90, operator() 0x4db880 (Preferences.cpp)
//   vector<TBuddy>          constructor 0x4dc170, operator() 0x4dba60 (Preferences.cpp)
//   vector<TTalkFilter *>   initialized constructor 0x4fd6e0 (Talk.cpp),
//                           operator() folded with vector<int> at 0x452ac0
// Preferences.cpp also uses vector<int>, whose code the linker kept from an
// earlier object: constructors 0x453300 (with init) and 0x4a2b90,
// operator() 0x452ac0.

#pragma once

#include "common.h"
#include <sstream>

// array<T> is a fixed array indexed from min to min+d-1 (lines 152-178;
// it comes before vector in the original file). The class name is ours;
// min, d, i, low() and high() are named in the assertions and in the
// error message of operator(). Instantiations reconstructed so far:
//   array<TProperties>  constructor 0x4cfa30, operator() 0x4d1a10 (Objects.cpp)
// Map.cpp has another one (element size 0xac): constructor 0x4b0bc0,
// operator() 0x4b5900.
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

template <class T> class vector {
public:
	vector(int min, int max, int block, T init);
	vector(int min, int max, int block);
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
