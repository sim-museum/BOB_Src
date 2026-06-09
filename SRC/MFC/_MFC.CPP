/*
	 Battle of Britain
	 Copyright (C) 2000, 2001 Empire Interactive (Europe) Ltd,
	 677 High Road, North Finchley, London N12 0DA

	 Please see the document licence.doc for the full licence agreement

2. LICENCE
 2.1 	
 	Subject to the provisions of this Agreement we now grant to you the 
 	following rights in respect of the Source Code:
  2.1.1 
  	the non-exclusive right to Exploit  the Source Code and Executable 
  	Code on any medium; and 
  2.1.2 
  	the non-exclusive right to create and distribute Derivative Works.
 2.2 	
 	Subject to the provisions of this Agreement we now grant you the
	following rights in respect of the Object Code:
  2.2.1 
	the non-exclusive right to Exploit the Object Code on the same
	terms and conditions set out in clause 3, provided that any
	distribution is done so on the terms of this Agreement and is
	accompanied by the Source Code and Executable Code (as
	applicable).

3. GENERAL OBLIGATIONS
 3.1 
 	In consideration of the licence granted in clause 2.1 you now agree:
  3.1.1 
	that when you distribute the Source Code or Executable Code or
	any Derivative Works to Recipients you will also include the
	terms of this Agreement;
  3.1.2 
	that when you make the Source Code, Executable Code or any
	Derivative Works ("Materials") available to download, you will
	ensure that Recipients must accept the terms of this Agreement
	before being allowed to download such Materials;
  3.1.3 
	that by Exploiting the Source Code or Executable Code you may
	not impose any further restrictions on a Recipient's subsequent
	Exploitation of the Source Code or Executable Code other than
	those contained in the terms and conditions of this Agreement;
  3.1.4 
	not (and not to allow any third party) to profit or make any
	charge for the Source Code, or Executable Code, any
	Exploitation of the Source Code or Executable Code, or for any
	Derivative Works;
  3.1.5 
	not to place any restrictions on the operability of the Source 
	Code;
  3.1.6 
	to attach prominent notices to any Derivative Works stating
	that you have changed the Source Code or Executable Code and to
	include the details anddate of such change; and
  3.1.7 
  	not to Exploit the Source Code or Executable Code otherwise than
	as expressly permitted by  this Agreement.

questions about this file may be asked at bob@rowansoftware.com a
better place to ask is http://www.simhq.com/ or even :-
http://www.simhq.com/cgi-bin/boards/cgi-bin/forumdisplay.cgi?action=topics&forum=Battle+of+Britain&number=40&DaysPrune=20&LastLogin=
*/

/* Linux port: select the file-enum groups this unity needs BEFORE the first
   files.g inclusion (via stdafx.h). Defining F_BATTLE excludes F_BATTLE.G and
   pulls in F_COMMON.G (FIL_MAP_TABLE) + F_GRAFIX.G (map tiles) + F_SOUNDS.G,
   matching RDIALOG.H's intent for the MFC dialog/map UI. */
#if defined(BOB_LINUX) && !defined(F_BATTLE)
#define F_BATTLE
#include "dosdefs.h"
#include "files.g"		/* lock FileNum with F_COMMON.G(FIL_MAP_TABLE)+F_GRAFIX.G+F_SOUNDS.G first */
/* bob's CString MUST be complete before stdafx.h: stdafx->afxwin.h defines
   __AFX_H__, which flips bob headers (savegame.h etc.) into their "MFC present"
   branch that declares `CString` members — but CString is bob's own (cstring.h),
   not provided by our MFC layer. */
#include "cstring.h"
#endif

#include "stdafx.h"
#include "_mfc.h"

#if defined(BOB_LINUX)
/* bob's UI base classes — included early so every fragment below sees the full
   definitions (the headers that use them, e.g. MapDlg.h/FullPane.h, don't
   self-include them; on Windows the PCH/build order covered this). */
#include "uniqueid.h"
/* leaf ActiveX-control wrappers FIRST (composites below derive from these) */
#include "rbutton.h"
#include "rstatic.h"
#include "rcombo.h"
#include "rspinbut.h"
#include "rlistbox.h"
#include "rtabs.h"
#include "rspltbar.h"
/* composites / dialogs / bars that use the leaf controls */
#include "rcombox.h"
#include "rdialog.h"
#include "rmdldlg.h"
#include "listbx.h"
#include "hintbox.h"
#include "maintbar.h"
#include "titlebar.h"
#include "sysbox.h"
#include "rtestsh1.h"
#include "squick1.h"
#include "movement.h"
#include "mapfltrs.h"
#endif

#include "./MIGView.cpp"

//DEADCODE DAW 24/11/99 #include ".\CntrItem.cpp"
//DEADCODE MS 14/12/99 #include ".\Fullpane.cpp"
#include "../Input/Keystub.cpp"
#include "./MainFrm.cpp"
#include "./MapDlg.cpp"

#include "./MIG.cpp"
#include "./MIGDoc.cpp"
#include "uiicons.cpp"
#include "./RDEmptyP.cpp"
#include "./RDEmptyD.cpp"

#include "RDEmpty.cpp"
#include "./RDialog.cpp"
//DEADCODE MS 02/02/00 #include ".\Stub3d.cpp"
#include "hintbox.cpp"
#include "listbx.cpp"	

#ifndef	BOB_DEMO_VER
	#include "./thumnail.cpp"
#endif
#include "rdiallog.cpp"
#include "./RowTasks.cpp"
#include "./Rradio.cpp"
#include "./rstatic.cpp"

#include "./Rtestsh1.cpp"
#include "./rmdldlg.cpp"
#include "./MTChild.cpp"
#include	"fullpsys.cpp"


