// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define STRICT
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#define _WTL_USE_CSTRING

#include "resource.h"
#include "targetver.h"

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501 // Target Windows XP
#endif

#ifndef WINVER
#define WINVER 0x0501
#endif

#include <Windows.h>
#include <assert.h>

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // some CString constructors will be explicit

#include <atlbase.h>        // Base ATL classes
#include <atlapp.h>

extern CAppModule _Module;  // Global _Module

#include <atlwin.h>         // ATL windowing classes
#include <atlframe.h>
#include <atlctrls.h>
#include <atldlgs.h>
#include <atlctrlw.h>
#include <atlmisc.h>
#include <atlcrack.h>
//#include <atltypes.h>

#include <atlcoll.h>


#include <stdio.h>

#include "shared/UnicodeSupport.h"
#include "shared/Singleton.h"
#include "sqlite3/sqlite3.h"

//#include "DBManager.h"
class DBManager;
extern DBManager g_DBManager; // Global DB manager



class Application;
typedef Shared::Singleton<Application> App;

#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include "Logger.h"
