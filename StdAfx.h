// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//
// $Log: StdAfx.h,v $
// Revision 1.1  2004/07/09 23:12:30  Skip
// Initial revision
//

#if !defined(AFX_STDAFX_H__0D8158B6_CD87_4D68_89E1_E6484B9BFC45__INCLUDED_)
#define AFX_STDAFX_H__0D8158B6_CD87_4D68_89E1_E6484B9BFC45__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define VC_EXTRALEAN    // Exclude rarely-used stuff from Windows headers

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdtctl.h>      // MFC support for Internet Explorer 4 Common Controls
#include <sys/timeb.h>

#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>        // MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#include <afxmt.h>         // Multitread extensions

#endif // !defined(AFX_STDAFX_H__0D8158B6_CD87_4D68_89E1_E6484B9BFC45__INCLUDED_)
