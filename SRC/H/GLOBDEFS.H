//////////////////////////////////////////////////////////////////////
//
// Module:      Globdefs.h
//
// Created:     04/02/00 by DAW
//
// Description: Very poorly named include file
//				This redefinition of the ON_MESSAGE macro protects
//				the system against the user procedures having the wrong 
//				number of parameters.
//				The system that MFC uses for it's own purposes makes 
//				this look high-tech!
//////////////////////////////////////////////////////////////////////
#pragma once
#ifndef Globdefs_DEFINED_4_2_2000
#define Globdefs_DEFINED_4_2_2000

#undef	ON_MESSAGE
#if defined(BOB_LINUX)
/* S158: the map entries are REAL now.
   They used to expand to nothing here (and separately in compat/afxwin.h -- the stub
   existed in TWO layers, and this one wins because of the #undef above, so fixing only
   afxwin.h registered nothing at all). S157 measured the cost: compat's SendMessage was
   an allowlist of three, 16 of the 20 WM_* routes the game sends died returning 0, and
   four were caught firing in a single ordinary run.

   The registration mirrors what the Windows branch below does -- it takes the address of
   the generated MSG2_* adapter, which normalises the handlers' 0/1/2-arg, void/non-void
   signatures into a uniform LRESULT(int,int). Emitted inside theClass::bob_mm_register()
   (declared by DECLARE_MESSAGE_MAP), which is what gives it access to the private MSG2_*
   members. Dispatch is gated on BOB_MSG_DISPATCH; registration is inert until then. */
#include <typeinfo>
extern "C" void bob_msgmap_add(const void* tinfo, unsigned msg, long (*fn)(void*, int, int));
extern "C" void bob_msgmap_probe(const void* tinfo, int (*fn)(void*));
#define ON_MESSAGE(message, memberFxn) \
	bob_msgmap_add(&typeid(BobMsgThisClass), (unsigned)(message), \
		[](void* w, int a, int b) -> long { \
			return (long)((BobMsgThisClass*)w)->MSG2_##memberFxn(a, b); });
/* ON_MESSAGE_CLASS names the class the handler belongs to explicitly (e.g. FULLPSYS.CPP
   registers WM_SELECTTAB on RFullPanelDial). Register under THAT type, not the map's owner. */
#define ON_MESSAGE_CLASS(msg,type,routine) \
	bob_msgmap_add(&typeid(type), (unsigned)(msg), \
		[](void* w, int a, int b) -> long { \
			return (long)((type*)w)->MSG2_##routine(a, b); });
#else
#define ON_MESSAGE(message, memberFxn) \
	{ message, 0, 0, 0, AfxSig_lwl, \
		(AFX_PMSG)(AFX_PMSGW)(LRESULT (AFX_MSG_CALL CWnd::*)(WPARAM, LPARAM))&MSG2_##memberFxn },
#define ON_MESSAGE_CLASS(msg,type,routine)\
	{ msg, 0, 0, 0, AfxSig_lwl, \
		(AFX_PMSG)(AFX_PMSGW)(LRESULT (AFX_MSG_CALL CWnd::*)(WPARAM, LPARAM))(LRESULT (AFX_MSG_CALL type::*)(WPARAM, LPARAM))&MSG2_##routine },
#endif
#define MSG2_0(memberFxn)	LRESULT MSG2_##memberFxn(int,int) {return (int)memberFxn();}
#define MSG2_1(memberFxn)	LRESULT MSG2_##memberFxn(int a,int) {return (int)memberFxn(a);}
#define MSG2_2(memberFxn)	LRESULT MSG2_##memberFxn(int a,int b) {return (int)memberFxn(a,b);}
#define MSG2_0v(memberFxn)	LRESULT MSG2_##memberFxn(int,int) {memberFxn();return 0;}
#define MSG2_1v(memberFxn)	LRESULT MSG2_##memberFxn(int a,int) {memberFxn(a);return 0;}
#define MSG2_2v(memberFxn)	LRESULT MSG2_##memberFxn(int a,int b) {memberFxn(a,b);return 0;}
#define MSG2(memberFxn)		LRESULT MSG2_##memberFxn(int a,int b)

#endif define Globdefs_DEFINED_4_2_2000
