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

#include "stdafx.h"

// this global function searches F_GRAFIX.G and returns the file
// number of a given file equate

#if BOB_LINUX
/* Linux port: the original opens a hardcoded Windows source path (\bob\src\h\F_GRAFIX.G),
   which the fopen redirect maps under drive_c and never finds. Read the repo's F_GRAFIX.G
   (BOB_SRC_DIR from CMake, or $BOB_RC_DIR) and parse "NAME =0xHHHH,". Skew fallback: this
   source drop's F_GRAFIX.G renamed the icon equates FIL_ICON_* -> FIL_xICON_* (an 'x' after
   FIL_), but the shipped .rc/DLGINIT still say FIL_ICON_*; so if an exact match fails, retry
   with the 'x' inserted. Returns the FileNum, or 0 if unknown. */
#ifndef BOB_SRC_DIR
#define BOB_SRC_DIR "."
#endif
#undef fopen   /* bypass the game's fopen->fopen_nocase drive_c redirect: F_GRAFIX.G is a repo
                  source file (absolute BOB_SRC_DIR path), not a drive_c game-data path. */
static int bob_grafix_lookup(FILE* fp, const char* name) {
    rewind(fp);
    char line[512];
    size_t nl = strlen(name);
    while (fgets(line, sizeof line, fp)) {
        const char* p = line;
        while (*p==' '||*p=='\t') p++;
        if (strncmp(p, name, nl) != 0) continue;
        const char* q = p + nl;                       /* must be a full token, then '=' */
        while (*q==' '||*q=='\t') q++;
        if (*q != '=') continue;
        if (!(p[nl]==' '||p[nl]=='\t'||p[nl]=='=')) continue;
        return (int)strtol(q+1, NULL, 16);
    }
    return -1;
}
extern "C" int bob_getfilenum(const char* text) {
    if (!text || !text[0]) return 0;
    char path[1024];
    const char* base = getenv("BOB_RC_DIR"); if (!base || !base[0]) base = BOB_SRC_DIR;
    snprintf(path, sizeof path, "%s/h/F_GRAFIX.G", base);
    FILE* fp = fopen(path, "rb");
    if (!fp) { snprintf(path,sizeof path,"%s/H/F_GRAFIX.G",base); fp=fopen(path,"rb"); }
    static int traced=0; if(getenv("BOB_TRACE_OLE")&&!traced){traced=1;
        fprintf(stderr,"[ole] GetFileNum F_GRAFIX.G path=%s open=%s\n", path, fp?"OK":"FAIL");}
    if (!fp) return 0;
    int v = bob_grafix_lookup(fp, text);
    if (v < 0 && strncmp(text, "FIL_", 4) == 0) {     /* skew fallback: FIL_ -> FIL_x */
        char alt[128]; snprintf(alt, sizeof alt, "FIL_x%s", text + 4);
        v = bob_grafix_lookup(fp, alt);
    }
    fclose(fp);
    return v < 0 ? 0 : v;
}
int GetFileNum(const char* text) { return bob_getfilenum(text); }

/* S90: resolve an "ICON_*" name to its ICON_PAGE value. The strategic-map toolbar buttons'
   faces are sprite-sheet icons (iconset1.bmp), addressed by IconsUI page/entry, NOT standalone
   art files -- the per-file FileNum (F_GRAFIX.G) for these is renamed/absent in this drop. The
   entry names + order live in h/iconnum.g (+ iconnum2.g), which the IconsUI enum #includes after
   ICON_PAGE_1_BEFORE (=ICON_PAGE_1-1); so entry k = ICON_PAGE_1 (0x10000) + k. Build that map once
   and look names up -> the value OnDraw's transparent/IconDescUI path (the map-icon renderer) draws. */
struct IconEnt { char name[48]; int val; };
static IconEnt s_icons[512]; static int s_nicons = 0; static int s_iconsLoaded = 0;
static void bob_icons_loadfile(const char* path, int* idx) {
    FILE* f = fopen(path, "rb");
    if (!f) return;
    char line[256];
    while (fgets(line, sizeof line, f)) {
        const char* p = line; while (*p==' '||*p=='\t') p++;
        if (strncmp(p, "ICON_", 5) != 0) continue;
        int n = 0; char nm[48];
        while ((isalnum((unsigned char)p[n]) || p[n]=='_') && n < 47) { nm[n]=p[n]; n++; }
        nm[n] = 0;
        if (n < 5) continue;
        if (s_nicons < 512) { strncpy(s_icons[s_nicons].name, nm, 47); s_icons[s_nicons].name[47]=0;
            s_icons[s_nicons].val = 0x10000 + (*idx); s_nicons++; }
        (*idx)++;
    }
    fclose(f);
}
extern "C" int bob_icon_pagenum(const char* name) {
    if (!name || !name[0]) return 0;
    if (!s_iconsLoaded) { s_iconsLoaded = 1; int idx = 0; char path[1024];
        const char* base = getenv("BOB_RC_DIR"); if (!base || !base[0]) base = BOB_SRC_DIR;
        snprintf(path, sizeof path, "%s/h/iconnum.g",  base); bob_icons_loadfile(path, &idx);
        snprintf(path, sizeof path, "%s/h/iconnum2.g", base); bob_icons_loadfile(path, &idx);
    }
    for (int i = 0; i < s_nicons; i++) if (strcmp(s_icons[i].name, name) == 0) return s_icons[i].val;
    return 0;
}
#else
int GetFileNum(const char* text)
{
	// ok first open F_GRAFIX.G which we'll assume is in
	// \mig\src\h
	FILE* fp;
	fp=fopen("\\bob\\src\\h\\F_GRAFIX.G","rb");
	if (fp==NULL) 
	{
		MessageBox(AfxGetMainWnd()->m_hWnd,"Cannot find \\bob\\src\\mfc\\f_grafix.g","ERROR",MB_OK);
		return -1;
	}
	// now parse it
	char c=0;
	int textlen=strlen(text);
	char buffer[255];
// first search for first letter of string...
	while (true)
	{
		while(true)
		{
			if(fread(&c,1,1,fp)==NULL) {fclose(fp);return 0;} // failed to find it
			if (c==text[0]) break;
		}
		buffer[0]=c;
		// now check if the rest of the string is after it
		if(fread((buffer+1),1,textlen,fp)<textlen-1) {fclose(fp);return 0;}
		if (buffer[textlen]=='\t') // prevents triggering on partial successes...
		{
			buffer[textlen]=NULL; // NULL terminate the buffer
			if (strcmp(buffer,text)==0)
				break;
		}
		fseek(fp,-textlen,SEEK_CUR); // failed so backup and carry on...
	}
	// ok we have a match...
	// so find the string
	// first skip over spaces...
	while(true)
	{
		if(fread(&c,1,1,fp)==NULL) {fclose(fp);return 0;} // so near yet so far...
		if (c!='\t') break;
	}
	// extract string...
	int x=0;
	while(true)
	{
		if (c==13)
		{
			buffer[x]=NULL;
			break;
		}
		if (c!='=' && c!=',')
		{
			buffer[x]=c;
			x++;
		}
		if(fread(&c,1,1,fp)==NULL) {fclose(fp);return 0;} // so near yet so far...
	}
	fclose(fp);
	return strtol(buffer,NULL,16); // success!!!!;
}
#endif /* BOB_LINUX */

