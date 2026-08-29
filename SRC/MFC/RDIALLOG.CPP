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

//
// Logging dialog inter-relationships:
//
//
//

#include "stdafx.h"

#if BOB_LINUX
/* R3.7 negative control. The fix is a one-character `()`; a gate that cannot make the bug come
   back cannot claim to guard against it. BOB_POISON_DIALSLOTS=1 refills the freshly value-
   initialised array with the kind of residue glibc actually hands back (the faulting address in
   the PO's crash was 0x3bf95c60), so the control arm reproduces the crash on demand and the fixed
   arm must survive it. File scope deliberately: `extern "C"` inside a function body is ill-formed,
   and this port has made that exact mistake four times now. */
extern "C" void bob_poison_dialslots(RDialog** a, int n)
{
	if (!a || !getenv("BOB_POISON_DIALSLOTS")) return;
	for (int k = 0; k < n; k++) a[k] = (RDialog*)(unsigned long)(0x3bf95c00u + 16u*k);
}
#else
#define bob_poison_dialslots(a,n) ((void)0)
#endif


//dummy notify routines that can be overloaded
void	RDialog::ParentDialClosed(RDialog*,int)
{}
void	RDialog::ChildDialClosed(int,RDialog*,int)
{}
void	CRToolBar::ChildDialClosed(int,RDialog*,int)
{}
void	RDialog::RefreshData(int)
{}
void	CRToolBar::RefreshData(int)
{}

//
//real management code
//

int RDialog::LogChild(RDialog* child)
{
	//first have to find right pane...
	RDialog* top=InDialAncestor();
	RDialog* chtop=child->InDialAncestor();
	int rv=0;
	/* R3.7 (PO 2026-08-28: "I ran german campaign, then tried british campaign which ended in
	   window disappearing"). SIGSEGV in CRToolBar::CloseLoggedChild(12), faulting on the element
	   read itself -- with MAX_CHILD_DIALS=16, slot 12 is IN BOUNDS, so this was never an overrun:
	   the ARRAY CONTENTS were garbage. `new RDialog*[N]` DEFAULT-initialises, which for pointers
	   leaves them indeterminate. Every consumer here assumes zeroed slots -- `while
	   (loggedchild[rv]) rv++` to find a free one, `if (loggedchild[i])` to guard a close -- so heap
	   residue reads as a live dialog and gets dereferenced. The original built under MSVC, whose
	   debug allocator hands back zeroed pages; glibc hands back whatever was last freed there,
	   which is why this surfaces only after a first campaign has churned the heap (german, THEN
	   british). The `()` makes it value-initialisation: all slots NULL. */
	if (!top->dialoglinks.loggedchild)
	{
		top->dialoglinks.loggedchild=new RDialog* [dialoglinks.MAX_CHILD_DIALS]();
		top->dialoglinks.loggedchildlauncher=new RDialog* [dialoglinks.MAX_CHILD_DIALS]();
		bob_poison_dialslots(top->dialoglinks.loggedchild, dialoglinks.MAX_CHILD_DIALS);
		for (int i=0;i<dialoglinks.MAX_CHILD_DIALS;i++)
			top->dialoglinks.loggedchild[i]=top->dialoglinks.loggedchildlauncher[i]=NULL;
	}
	while (top->dialoglinks.loggedchild[rv])	rv++;
	top->dialoglinks.loggedchild[rv]=chtop;
	top->dialoglinks.loggedchildlauncher[rv]=this;
	chtop->dialoglinks.loggedparent=top;
	chtop->dialoglinks.loggedparentlauncher=child;
	return rv;
}
bool	RDialog::LogChild(int rv,RDialog* child)
{
	//first have to find right pane...
	RDialog* top=InDialAncestor();
	RDialog* chtop=child->InDialAncestor();
	if (!top->dialoglinks.loggedchild)
	{
		top->dialoglinks.loggedchild=new RDialog* [dialoglinks.MAX_CHILD_DIALS]();
		top->dialoglinks.loggedchildlauncher=new RDialog* [dialoglinks.MAX_CHILD_DIALS]();
		bob_poison_dialslots(top->dialoglinks.loggedchild, dialoglinks.MAX_CHILD_DIALS);
		for (int i=0;i<dialoglinks.MAX_CHILD_DIALS;i++)
			top->dialoglinks.loggedchild[i]=top->dialoglinks.loggedchildlauncher[i]=NULL;
	}
	else
		if	(top->dialoglinks.loggedchild[rv])
			return false;
	top->dialoglinks.loggedchild[rv]=chtop;
	top->dialoglinks.loggedchildlauncher[rv]=this;
	chtop->dialoglinks.loggedparent=top;
	chtop->dialoglinks.loggedparentlauncher=child;
	return true;
}
int CRToolBar::LogChild(RDialog* child)
{
	//first have to find right pane...
	RDialog* chtop=child->InDialAncestor();
	int rv=0;
	if (!dialoglinks.loggedchild)
	{
		dialoglinks.loggedchild=new RDialog* [dialoglinks.MAX_CHILD_DIALS]();
		dialoglinks.loggedchildlauncher=new RDialog* [dialoglinks.MAX_CHILD_DIALS]();
		bob_poison_dialslots(dialoglinks.loggedchild, dialoglinks.MAX_CHILD_DIALS);
		for (int i=0;i<dialoglinks.MAX_CHILD_DIALS;i++)
			dialoglinks.loggedchild[i]=dialoglinks.loggedchildlauncher[i]=NULL;
	}
	while (dialoglinks.loggedchild[rv])	rv++;
	dialoglinks.loggedchild[rv]=chtop;
	dialoglinks.loggedchildlauncher[rv]=NULL;
	chtop->dialoglinks.loggedparenttool=this;
	chtop->dialoglinks.loggedparentlauncher=child;
	return rv;
}
bool	CRToolBar::LogChild(int rv,RDialog* child)
{
	//first have to find right pane...
	RDialog* chtop=child->InDialAncestor();
	if (!dialoglinks.loggedchild)
	{
		dialoglinks.loggedchild=new RDialog* [dialoglinks.MAX_CHILD_DIALS]();
		dialoglinks.loggedchildlauncher=new RDialog* [dialoglinks.MAX_CHILD_DIALS]();
		bob_poison_dialslots(dialoglinks.loggedchild, dialoglinks.MAX_CHILD_DIALS);
		for (int i=0;i<dialoglinks.MAX_CHILD_DIALS;i++)
			dialoglinks.loggedchild[i]=dialoglinks.loggedchildlauncher[i]=NULL;
	}
	else
		if	(dialoglinks.loggedchild[rv])
			return false;
	dialoglinks.loggedchild[rv]=chtop;
	dialoglinks.loggedchildlauncher[rv]=NULL;
	chtop->dialoglinks.loggedparenttool=this;
	chtop->dialoglinks.loggedparentlauncher=child;
	return true;
}

void	CRToolBar::DialExitFix(int rv)
{	//never got logged parent!
	if (dialoglinks.loggedchild)
	{
		for (int ind=0;ind<dialoglinks.MAX_CHILD_DIALS;ind++)
			if (dialoglinks.loggedchild[ind])
			{
				RDialog* ch=dialoglinks.loggedchild[ind];
				dialoglinks.loggedchild[ind]=NULL;
				ch->dialoglinks.loggedparent=NULL;
				ch->dialoglinks.loggedparentlauncher->ParentDialClosed(dialoglinks.loggedchildlauncher[ind],rv);
			}
		RDialog** lct=dialoglinks.loggedchild;
		dialoglinks.loggedchild=NULL;
		delete []lct;
		lct=dialoglinks.loggedchildlauncher;
		dialoglinks.loggedchildlauncher=NULL;
		delete []lct;
	}
}
static 	RDialog*	GlobalUpdateOngoing=NULL;
void	RDialog::DialExitFix(int rv)
{
	if (GlobalUpdateOngoing==this)
		GlobalUpdateOngoing=NULL;
	if (dialoglinks.loggedparent)
	{	//need to find child in parent's list
		int ind=0;
		RDialog* ch=dialoglinks.loggedparent;
		dialoglinks.loggedparent=NULL;
		while (ch->dialoglinks.loggedchild[ind]!=this)
			ind++;
		ch->dialoglinks.loggedchild[ind]=NULL;
		ch->dialoglinks.loggedchildlauncher[ind]->ChildDialClosed(ind,dialoglinks.loggedparentlauncher,rv);
	}
	if (dialoglinks.loggedparenttool)
	{
		int ind=0;
		CRToolBar* ch=dialoglinks.loggedparenttool;
		dialoglinks.loggedparenttool=NULL;
		while (ch->dialoglinks.loggedchild[ind]!=this)
			ind++;
		ch->dialoglinks.loggedchild[ind]=NULL;
		ch->ChildDialClosed(ind,dialoglinks.loggedparentlauncher,rv);
	}
	if (dialoglinks.loggedchild)
	{
		for (int ind=0;ind<dialoglinks.MAX_CHILD_DIALS;ind++)
			if (dialoglinks.loggedchild[ind])
			{
				RDialog* ch=dialoglinks.loggedchild[ind];
				dialoglinks.loggedchild[ind]=NULL;
				ch->dialoglinks.loggedparent=NULL;
				ch->dialoglinks.loggedparentlauncher->ParentDialClosed(dialoglinks.loggedchildlauncher[ind],rv);
			}
		RDialog** lct=dialoglinks.loggedchild;
		dialoglinks.loggedchild=NULL;
		delete []lct;
		lct=dialoglinks.loggedchildlauncher;
		dialoglinks.loggedchildlauncher=NULL;
		delete []lct;
	}
}		 

int		RDialog::LoggedChild()
{
	RDialog* top=InDialAncestor();
	if (!top->dialoglinks.loggedchild)
		return -1;
	for (int i=0;i<dialoglinks.MAX_CHILD_DIALS;i++)
		if (top->dialoglinks.loggedchild[i])
			return i;
	return -1;
}

RDialog* RDialog::LoggedChild(int i)
{
	RDialog* top=InDialAncestor();
	if (!top->dialoglinks.loggedchild) 
		return (NULL);
	else
		return (top->dialoglinks.loggedchild[i]);
}

int		CRToolBar::LoggedChild()
{
	if (!dialoglinks.loggedchild)
		return -1;
	for (int i=dialoglinks.MAX_CHILD_DIALS-1;i>=0;i--)
		if (dialoglinks.loggedchild[i])
			return i;
	return -1;
}

RDialog* CRToolBar::LoggedChild(int i)
{
	if (!dialoglinks.loggedchild) 
		return (NULL);
	else
		return (dialoglinks.loggedchild[i]);
}
#if BOB_LINUX
/* S154 (SP.23): the teardown compat's CDialog::OnCancel/EndDialog never did.
   Default OFF (BOB_DLG_TEARDOWN=1 enables) -- see the note in afxwin.h for why this is gated.

   Two things window-destruction does on Windows that matter to us:
     1. the dialog's hosted R* controls go away  -> bob_ole_release_dialog()
     2. the logged-child slot referencing it clears -> so LoggedChild() stops reporting a dialog
        that is gone, and the directive dialogs' cancel toggle-loop terminates the way it does on
        Windows instead of relying on S108's re-entrancy guard.
   We do NOT free the dialog object itself: ownership is the game's (some are members, some are
   `new`ed by Make()) and freeing the wrong one is a far worse bug than retaining it. This reclaims
   the per-re-open control storage and fixes the lifetime lie in the slot table; the object-level
   leak is a separate, smaller question. */
extern "C" int bob_ole_release_dialog(CWnd*);
extern "C" int bob_ole_count_for_dialog(CWnd*);
/* S155 (SP.23): one trace instead of a fourth guess. For every DestroyWindow, print the object's
   RTTI, how many hosted controls name IT as parent, and the same for each fchild/sibling
   descendant -- so "which object owns the 184" stops being an inference. BOB_TRACE_DESTROY. */
extern "C" void bob_dialog_destroy_trace(CWnd* dlg)
{
	static int on = -1;
	if (on < 0) on = getenv("BOB_TRACE_DESTROY") ? 1 : 0;
	if (!on || !dlg) return;
	static int budget = 0;
	if (++budget > 40) return;            /* predicate-free but bounded; 40 is plenty to see the shape */
	fprintf(stderr, "[destroy] %p rtti=%s hosts_here=%d\n",
		(void*)dlg, typeid(*dlg).name(), bob_ole_count_for_dialog(dlg));
	/* descendants, if this really is an RDialog -- guarded by the same RTTI we just printed */
	RDialog* rd = dynamic_cast<RDialog*>(dlg);
	if (!rd) { fprintf(stderr, "[destroy]    (not an RDialog -- no child walk)\n"); return; }
	int d = 0;
	for (RDialog* c = rd->fchild; c && d < 12; c = c->sibling, d++)
		fprintf(stderr, "[destroy]    child %p rtti=%s hosts=%d\n",
			(void*)c, typeid(*c).name(), bob_ole_count_for_dialog((CWnd*)c));
	fflush(stderr);
}

extern "C" void bob_dialog_teardown(CWnd* dlg)
{
	/* S155: now DEFAULT-ON. It shipped default-off in S154 because this is the lifecycle every
	   dialog runs through and S108's stack overflow lives on the same path. The evidence to flip it:
	   the leak measurement went 181,424 -> 184 hosted controls (exactly one dialog's worth, i.e. no
	   accumulation), the campaign accept/dismiss run exercised the directive dialogs' cancel toggle
	   -- S108's territory -- without incident, and the full gate suite run WITH it enabled came back
	   14/14 byte-identical, dummy==GL byte-identical, flight 98.6%. BOB_NO_DLG_TEARDOWN reverts. */
	static int on = -1;
	if (on < 0) on = getenv("BOB_NO_DLG_TEARDOWN") ? 0 : 1;
	if (!on || !dlg) return;
	/* S155: release the destroyed node AND its children. S154 measured that this hook frees only
	   1 control, and the S155 destroy trace said exactly why:
	       [destroy] 0x9d06900 rtti=RDEmptyD      hosts_here=0
	       [destroy]    child 0x9f79430 rtti=LWDirectives hosts=184
	   DestroyWindow is called on the PANEL (an RDEmptyD placeholder, or RFullPanelDial), while the
	   hosted controls belong to the dialog CONTAINED in it. Same shape for every dialog measured:
	   DirectiveResults 3, LWMissionFolder 23, TakeOverOffered 6 (x4 distinct instances -- the leak
	   in flight). The dynamic_cast is the safety check S154 said this needed: DestroyWindow is on
	   CWnd and reaches plenty of non-RDialog windows. Walk depth-first but BOUNDED -- a corrupt or
	   cyclic list must not hang a teardown. */
	int n = bob_ole_release_dialog(dlg);
	if (RDialog* rd = dynamic_cast<RDialog*>(dlg)) {
		int guard = 0;
		for (RDialog* c = rd->fchild; c && guard < 64; c = c->sibling, guard++) {
			n += bob_ole_release_dialog((CWnd*)c);
			int g2 = 0;
			for (RDialog* gc = c->fchild; gc && g2 < 64; gc = gc->sibling, g2++)
				n += bob_ole_release_dialog((CWnd*)gc);
		}
	}
	int slots = 0;
	/* clear any logged-child slot that points at this dialog (or its dial ancestor) */
	for (int t = 0; t < TB_TOTAL; t++) {
		CRToolBar* tb = CMainFrame::toolbars[t];
		if (!tb || !tb->dialoglinks.loggedchild) continue;
		for (int i = 0; i < tb->dialoglinks.MAX_CHILD_DIALS; i++) {
			RDialog* c = tb->dialoglinks.loggedchild[i];
			if (!c) continue;
			if (c == (RDialog*)dlg || c->dialoglinks.loggedparentlauncher == (RDialog*)dlg) {
				tb->dialoglinks.loggedchild[i] = NULL;
				tb->dialoglinks.loggedchildlauncher[i] = NULL;
				slots++;
			}
		}
	}
	if ((n || slots) && getenv("BOB_TRACE_OLE"))
		fprintf(stderr, "[dlg] teardown %p: released %d control(s), cleared %d slot(s)\n",
			(void*)dlg, n, slots);
}
#endif

bool	RDialog::CloseLoggedChild(int i)
{	//returns true if child still there!
	RDialog* top=InDialAncestor();
	if (!top->dialoglinks.loggedchild)
		return false;
	if (top->dialoglinks.loggedchild[i])
		top->dialoglinks.loggedchild[i]->dialoglinks.loggedparentlauncher->OnCancel();
	return (top->dialoglinks.loggedchild[i]!=NULL);
}

//������������������������������������������������������������������������������
//Procedure		CloseLoggedChildren
//Author		Jim Taylor
//Date			Fri 19 Feb 1999
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void	RDialog::CloseLoggedChildren(RDialog* skipthis)
{
	if (skipthis)
		skipthis=skipthis->InDialAncestor();
	RDialog* top=InDialAncestor();
	if (!top->dialoglinks.loggedchild)
		return;
	for (int i=0;i<dialoglinks.MAX_CHILD_DIALS;i++)
		if (top->dialoglinks.loggedchild[i] && top->dialoglinks.loggedchild[i]!=skipthis)
			top->dialoglinks.loggedchild[i]->dialoglinks.loggedparentlauncher->OnCancel();
}

void	CRToolBar::CloseLoggedChildren(RDialog* skipthis)
{
	if (skipthis)
		skipthis=skipthis->InDialAncestor();
	if (!dialoglinks.loggedchild)
		return;
	for (int i=0;i<dialoglinks.MAX_CHILD_DIALS;i++)
		if (dialoglinks.loggedchild[i] && dialoglinks.loggedchild[i]!=skipthis)
			dialoglinks.loggedchild[i]->dialoglinks.loggedparentlauncher->OnCancel();
}

//������������������������������������������������������������������������������
//Procedure		CloseLoggedChild
//Author		Jim Taylor
//Date			Fri 19 Feb 1999
//
//Description	
//
//Inputs		
//
//Returns		true if child slot still active
//
//------------------------------------------------------------------------------
bool	CRToolBar::CloseLoggedChild(int i)
{	//returns true if child still there!
	if (!dialoglinks.loggedchild)
		return false;
#if BOB_LINUX
	/* S108: per-slot re-entrancy guard. The directive dialogs' OnCancel handlers form a toggle loop
	   that re-enters CloseLoggedChild for the SAME slot -- RAFDirectives::OnCancel -> OpenEmptyDirective
	   Results -> CloseLoggedChild(DIRECTIVERESULTS) -> DirectivesNoResults::OnCancel ->
	   OpenRAFDirectivetoggle -> CloseLoggedChild(DIRECTIVES) -> RAFDirectives::OnCancel -> ... On Windows
	   this terminates because CDialog::OnCancel destroys the window (clearing the slot); our Linux
	   CDialog::OnCancel is a no-op, so the loop recurses forever -> stack overflow (a day-3 directive
	   event in a multi-day campaign, msctlbr.cpp:390). Block a re-entrant close of a slot already being
	   closed -- breaks the loop while leaving the slot state (which OnCancel reads) untouched. */
	static unsigned char s_closing[256] = {0};
	if (i >= 0 && i < 256) { if (s_closing[i]) return (dialoglinks.loggedchild[i] != NULL); s_closing[i] = 1; }
#endif
	if (dialoglinks.loggedchild[i])
		dialoglinks.loggedchild[i]->dialoglinks.loggedparentlauncher->OnCancel();
#if BOB_LINUX
	if (i >= 0 && i < 256) s_closing[i] = 0;
#endif
	return (dialoglinks.loggedchild[i]!=NULL);
}

void	RDialog::RefreshChild(int i)
{
	RDialog* top=InDialAncestor();
	if (!top->dialoglinks.loggedchild)
		return;
	if (top->dialoglinks.loggedchild[i])
		top->dialoglinks.loggedchild[i]->dialoglinks.loggedparentlauncher->RefreshData(-1);
}

void	CRToolBar::RefreshChild(int i)
{
	if (!dialoglinks.loggedchild)
		return;
	if (dialoglinks.loggedchild[i])
		dialoglinks.loggedchild[i]->dialoglinks.loggedparentlauncher->RefreshData(-1);
}

void	RDialog::RefreshParent()
{
	RDialog* top=InDialAncestor();
	if (top->dialoglinks.loggedparent)
		for (int i=0;i<dialoglinks.MAX_CHILD_DIALS;i++)
			if (top->dialoglinks.loggedparent->dialoglinks.loggedchild[i]==top)
			{
				top->dialoglinks.loggedparent->dialoglinks.loggedchildlauncher[i]->RefreshData(i);
				return;
			}
}
void	CRToolBar::GlobalRefreshData()
{	//Dialogue may try to close itself while this happens
	if (dialoglinks.loggedchild)
		for (int i=0;i<DialogLinks::MAX_CHILD_DIALS;i++)
			if (dialoglinks.loggedchild[i])
				dialoglinks.loggedchild[i]->InDialAncestor()->GlobalRefreshData();
	RefreshData(-2);
}

void	RDialog::GlobalRefreshData()
{	//Dialogue may try to close itself while this happens
	GlobalUpdateOngoing=this;
	RefreshData(-2);
	{
		if (GlobalUpdateOngoing)
			for (RDialog* c=fchild;c;c=c->sibling)
			{
				c->GlobalRefreshData();
				breakif (!GlobalUpdateOngoing);
			}
	}
	if (GlobalUpdateOngoing)
		if (dialoglinks.loggedchild)
			for (int i=0;i<DialogLinks::MAX_CHILD_DIALS;i++)
				if (dialoglinks.loggedchild[i])
					dialoglinks.loggedchild[i]->InDialAncestor()->GlobalRefreshData();
}

void	CRToolBar::MakeChildrenVisible(bool enable)
{
	if (!dialoglinks.loggedchild)
		return;
	int setting;
	if (enable)
		setting=SW_SHOW;
	else
		setting=SW_HIDE;
	for (int i=0;i<dialoglinks.MAX_CHILD_DIALS;i++)
		if (dialoglinks.loggedchild[i])
		{
			dialoglinks.loggedchild[i]->ShowWindow(setting);
			dialoglinks.loggedchild[i]->dialoglinks.loggedparentlauncher->MakeChildrenVisible(enable);
		}
}


void	RDialog::MakeChildrenVisible(bool enable)
{
	RDialog* top=InDialAncestor();
	if (!top->dialoglinks.loggedchild)
		return;
	int setting;
	if (enable)
		setting=SW_SHOW;
	else
		setting=SW_HIDE;
	for (int i=0;i<dialoglinks.MAX_CHILD_DIALS;i++)
		if (top->dialoglinks.loggedchild[i])
		{
			top->dialoglinks.loggedchild[i]->ShowWindow(setting);
			top->dialoglinks.loggedchild[i]->dialoglinks.loggedparentlauncher->MakeChildrenVisible(enable);
		}
}
