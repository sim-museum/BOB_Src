// Linux port shim: pre-standard <iostream.h> -> modern <iostream>.
// VC6's <iostream.h> exposed the stream names in the GLOBAL namespace but did
// NOT define std::string there. BoB has its own `typedef char* string`
// (dosdefs.h), so we must NOT `using namespace std` (that would make std::string
// ambiguous with BoB's). Instead bring in only the stream-related names.
#ifndef BOB_COMPAT_IOSTREAM_H
#define BOB_COMPAT_IOSTREAM_H
#include <iostream>
#include <iomanip>
using std::ios;
using std::ios_base;
using std::istream;
using std::ostream;
using std::iostream;
using std::streambuf;
using std::streampos;
using std::streamoff;
using std::streamsize;
using std::cin;
using std::cout;
using std::cerr;
using std::clog;
using std::endl;
using std::ends;
using std::flush;
using std::ws;
using std::dec;
using std::hex;
using std::oct;
using std::setw;
using std::setfill;
using std::setprecision;
using std::setbase;
using std::setiosflags;
using std::resetiosflags;
#endif
