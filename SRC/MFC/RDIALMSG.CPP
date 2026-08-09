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

//////////////////////////////////////////////////////////////////////
//
// Module:      rdialmsg.cpp
//
// Created:     02/11/99 by RDH
//
// Description: DIALOG MESSAGE HANDLERS REQUIRED BY A VARIEY OF SCREENS
//				DIALCLASS should be #defined or typedeffed to the current class.
//
//////////////////////////////////////////////////////////////////////
#ifdef	DIALCLASS

extern CFont* (g_AllFonts[][4]);
extern	CDC g_OffScreenDC;
#include <afxpriv.h>
#include "miles.h"
CFont* DIALCLASS::OnGetGlobalFont(int fontnum)
{
    if (fontnum<0||fontnum>MAXFONTS)
		fontnum=0;
	if (m_scalingfactor>1.5) 
		return ::g_AllFonts[fontnum][3];
	else 
		return ::g_AllFonts[fontnum][0];
}

BYTE* DIALCLASS::OnGetFile(int filenum)
{
	delete m_pfileblock;
	m_pfileblock=NULL;
	if (filenum>=0x10000)
	{
		RDialog::AXicondesc=IconsUI(filenum);
		return (BYTE*)&RDialog::AXicondesc;
	}
	else
	{
//		if (!(filenum>0x6600 && filenum<0x7100))  //only directories 104..113
		if (!(filenum>0x6600 && filenum<0x7200))  //only directories 104..113
			return NULL;
#if defined(BOB_LINUX)
		/* S159 (§8-MA84): the engine allows ONE open per FileNum globally, but this block is
		   held in m_pfileblock, a PER-DIALOG member -- so as soon as two different parents draw
		   controls sharing one piece of art, the second to paint opens a block the first still
		   holds and the game exits ("Opened file block (%x) again without closing!"). Nothing
		   collided while only one thing painted per frame, which is why enabling the message
		   dispatch (S158) is what exposed it: the bug is created by making a subsystem work.
		   If somebody already holds it, BORROW their data -- valid for the draw, and deliberately
		   NOT stored in m_pfileblock, because releasing a block we do not own would turn a clean
		   quit into a use-after-free.

		   DEFAULT-OFF, on evidence (S159): it does NOT fix the fatal it was written for -- that
		   collision never reaches this handler (backtrace: bob_fp_repaint -> ... -> makelink,
		   with OnGetFile absent, and `borrows: 0` measured) -- and default-on it REGRESSED the
		   gate: config-control DIFF bbox=(552,489,648,505), 13/14 byte-identical. A change that
		   alters rendering while not fixing its target ships opt-in. Enable: BOB_FILEBLK_SHARE=1. */
		static const int bobFileblkShare = getenv("BOB_FILEBLK_SHARE") ? 1 : 0;   /* S160b: draw path */
		if (bobFileblkShare)
		{
			void* shared=fileblocklink::BOB_GetOpenFileData((FileNum)filenum);
			if (shared)
			{
				if (getenv("BOB_TRACE_FILEBLK"))
					fprintf(stderr,"[fileblk] %x already open -- borrowing (dialog %p keeps m_pfileblock NULL)\n",
						(unsigned)filenum,(void*)this);
				return (BYTE*)shared;
			}
		}
#endif
		m_pfileblock=new fileblock((FileNum)filenum);//,PalTrans);
		return (BYTE*)getdata(m_pfileblock);
	}
}

void DIALCLASS::OnReleaseLastFile()
{
	delete m_pfileblock;
	m_pfileblock=NULL;
}

CDC* DIALCLASS::OnGetOffScreenDC()
{
	return &::g_OffScreenDC;
}

CRListBox* DIALCLASS::OnGetComboListbox()
{
	OnGetComboDialog();
	return (CRListBox*)RDialog::m_pListBox->GetDlgItem(IDC_RLISTBOXCTRL1);
}

CListBx* DIALCLASS::OnGetComboDialog()
{
	if (!RDialog::m_pListBox)
		RDialog::m_pListBox=new CListBx;
	if (!RDialog::m_pListBox->m_hWnd)
		RDialog::m_pListBox->Create(CListBx::IDD,NULL);
	return RDialog::m_pListBox;
}

CHintBox* DIALCLASS::OnGetHintBox()
{
	if (!RDialog::m_pHintBox)
		RDialog::m_pHintBox=new CHintBox;
	if (!RDialog::m_pHintBox->m_hWnd)
		RDialog::m_pHintBox->Create(CHintBox::IDD,this);
	RDialog::m_pHintBox->parentshandle=m_hWnd;
	return RDialog::m_pHintBox;
}

void DIALCLASS::OnPlaySound(int filenum)
{
	_Miles.UIPlaySample((FileNum)filenum);						//RJS 12Mar99
}

int DIALCLASS::OnGetString(int resourceID,char* workbuffer)
{
	return AfxLoadString(resourceID,workbuffer,(UByte)workbuffer[0]);
}

BOOL DIALCLASS::OnGetX2Flag()
{
	if (m_scalingfactor>1.5) return TRUE;
	else return FALSE;
}


#endif
