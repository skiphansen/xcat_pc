// xcat.h : main header file for the XCAT application
//
// $Log: xcat.h,v $
// Revision 1.2  2004/08/08 23:41:28  Skip
// Complete implementation of mode scan.
//
// Revision 1.1.1.1  2004/07/09 23:12:30  Skip
// Initial import of Xcat PC control program - V0.09.
// Shipped with first 10 Xcat boards.
//

#if !defined(AFX_XCAT_H__B8E5B984_12DA_4FF5_B2D5_A938DB6FBA76__INCLUDED_)
#define AFX_XCAT_H__B8E5B984_12DA_4FF5_B2D5_A938DB6FBA76__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
   #error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"      // main symbols

// global variables
extern int gComPort;
extern int gBaudrate;
extern int gBandScanStep;
extern double gBandScanTop;
extern double gBandScanBottom;
extern double gBandScanHangTime;
extern double gRxFrequency;
extern double gTxOffsetFreq;
extern int gRxCTSS;
extern int gTxCTSS;
extern int gTxOffset;
extern int gDebugMode;
extern CString gSaveFilename;
extern CString gRestoreFilename;
extern unsigned char gModeData[16];

typedef struct {
   const char *Name;
   void *Variable;
   int   Type;
      #define REG_VAR_TYPE_INT      1
      #define REG_VAR_TYPE_UINT     2
      #define REG_VAR_TYPE_CSTRING  3
      #define REG_VAR_TYPE_DOUBLE   4
   const char *InitValue;
} RegVariable;

/////////////////////////////////////////////////////////////////////////////
// CXcatApp:
// See xcat.cpp for the implementation of this class
//

class CXcatApp : public CWinApp
{
public:
   CXcatApp();

// Overrides
   // ClassWizard generated virtual function overrides
   //{{AFX_VIRTUAL(CXcatApp)
   public:
   virtual BOOL InitInstance();
   //}}AFX_VIRTUAL

// Implementation

   //{{AFX_MSG(CXcatApp)
      // NOTE - the ClassWizard will add and remove member functions here.
      //    DO NOT EDIT what you see in these blocks of generated code !
   //}}AFX_MSG
   DECLARE_MESSAGE_MAP()

private:

   void RegRestoreVar(const char *KeyName,RegVariable *pVarTable,void *Offset);
   void RegSaveVar(const char *KeyName,RegVariable *pVarTable,void *Offset);
};

extern CXcatApp theApp;

enum XCAT_PRIVATE_IDS {
   ID_RX_MSG = WM_APP,
   ID_MANUAL_SET,
   ID_BAND_SCAN_START,
   ID_BAND_SCAN_LOCKOUT,
   ID_BAND_SCAN_DEL_LOCKOUT,
   ID_GET_CONFIG,
   ID_SET_CONFIG,
   ID_SAVE_MODE,
   ID_RECALL_MODE,
   ID_SKIP,
   ID_SAVE_CODEPLUG,
   ID_RESTORE_CODEPLUG,
   ID_COMM_ERROR,
};

extern HWND hMainWindow;

char *Err2String(int err);

void LogIt(char *fmt,...);
void LogHex(void *AdrIn,int Len);

#define LOG(x)       LogIt x
#define LOG_HEX(x,y) LogHex(x,y)

extern char DateCompiled[];
extern char TimeCompiled[];

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_XCAT_H__B8E5B984_12DA_4FF5_B2D5_A938DB6FBA76__INCLUDED_)
