// stdafx.h

#ifndef __STDAFX_H
#define __STDAFX_H


#include "config.h"


#define NO_INLINE /* FIXME */

#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif

#include "Common/MyWindows.h"
#include "Common/Types.h"

#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <wchar.h>
#include <stddef.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>

#ifdef __NETWARE__
#include <sys/types.h>
#endif

#undef CS /* fix for Solaris 10 x86 */

/***************************/

#define CLASS_E_CLASSNOTAVAILABLE        ((HRESULT)0x80040111L)

/************************* LastError *************************/
inline DWORD WINAPI GetLastError(void) { return errno; }
inline void WINAPI SetLastError( DWORD err ) { errno = err; }

#define AreFileApisANSI() (1)

void Sleep(unsigned millisleep);

typedef pid_t t_processID;

t_processID GetCurrentProcess(void);

#define  NORMAL_PRIORITY_CLASS (0)
#define  IDLE_PRIORITY_CLASS   (10)
void SetPriorityClass(t_processID , int priority);

#ifdef __cplusplus
class wxWindow;
typedef wxWindow *HWND;

#define MB_ICONERROR (0x00000200) // wxICON_ERROR
#define MB_YESNOCANCEL (0x00000002 | 0x00000008 | 0x00000010) // wxYES | wxNO | wxCANCEL
#define MB_ICONQUESTION (0x00000400) // wxICON_QUESTION
#define MB_TASKMODAL  (0) // FIXME
#define MB_SYSTEMMODAL (0) // FIXME

#define MB_OK (0) // FIXME !
#define MB_ICONSTOP (0) // FIXME !
#define MB_OKCANCEL (0) // FIXME !

#define MessageBox MessageBoxW
int MessageBoxW(wxWindow * parent, const TCHAR * mes, const TCHAR * title,int flag);

typedef void *HINSTANCE;

typedef          int   INT_PTR;  // FIXME 64 bits ?
typedef unsigned int  UINT_PTR;  // FIXME 64 bits ?
typedef          long LONG_PTR;  // FIXME 64 bits ?
typedef          long DWORD_PTR; // FIXME 64 bits ?
typedef UINT_PTR WPARAM;

/* WARNING
 LPARAM shall be 'long' because of CListView::SortItems and wxListCtrl::SortItems :
*/
typedef LONG_PTR LPARAM;
typedef LONG_PTR LRESULT;

#define CALLBACK /* */

/************ LANG ***********/
typedef WORD            LANGID;

LANGID GetUserDefaultLangID(void);
LANGID GetSystemDefaultLangID(void);

#define PRIMARYLANGID(l)        ((WORD)(l) & 0x3ff)
#define SUBLANGID(l)            ((WORD)(l) >> 10)

#endif

#endif 

