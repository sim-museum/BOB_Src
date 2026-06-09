// Linux port shim: pre-standard <fstream.h> -> modern <fstream>.
// See iostream.h shim: selective `using` (not `using namespace std`) so BoB's
// global `typedef char* string` (dosdefs.h) is not made ambiguous by std::string.
#ifndef BOB_COMPAT_FSTREAM_H
#define BOB_COMPAT_FSTREAM_H
#include <fstream>
#include "iostream.h"
using std::filebuf;
using std::ifstream;
using std::ofstream;
using std::fstream;
#endif
