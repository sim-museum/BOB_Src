// Linux port shim: pre-standard <fstream.h> -> modern <fstream>.
#ifndef BOB_COMPAT_FSTREAM_H
#define BOB_COMPAT_FSTREAM_H
#include <fstream>
#include <iostream>
#include <iomanip>
using namespace std;	// faithful to the old <fstream.h> global namespace
#endif
