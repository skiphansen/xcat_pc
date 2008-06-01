// xcat.h : main header file for the XCAT application
//
// $Log: xcat.h,v $
// Revision 1.9  2008/06/01 13:57:15  Skip
// Added gLoaderVerString, ID_DOWNLOAD_CHAR.
//
// Revision 1.8  2008/02/02 17:58:21  Skip
// Added support for volume pot (not tested).
//
// Revision 1.7  2007/07/15 14:17:24  Skip
// 1. Added gFirmwareVer, gFirmwareVerString, CONFIG_SQU_POT_MASK
//    and g_bHaveFWVer.
// 2. Increased CONFIG_LEN to 3 for squelch pot value.
//
// Revision 1.6  2007/01/02 17:28:24  Skip
// AAdded registry backed globals gVCORxSplitVHF, gVCOTxSplitVHF, gVCORxSplit420,
// gVCOTxSplit420, gVCORxSplit440, and gVCOTxSplit440.
//
// Added globals gRxDCS, gTxDCS, and gTxTimeout.
//
// Revision 1.5  2005/01/08 19:20:24  Skip
// 1. Added global variables ModeName[], LastModelSel, gInvertedModeSel,
//    gConfig, and g_bHaveModeData.
// 2. Added INVERT_MODE macro.
//
// Revision 1.4  2004/12/27 05:55:24  Skip
// Version 0.13:
// 1. Fixed crash in Debug mode caused by calling ScanPage.ModeData()
//    being called when not on the scan page.
// 2. Added support for sync rx debug data (requires firmware update as well).
// 3. Added request buttons to Debug mode for code plug data and sync rx
//    debug data.
// 4. Corrected bug in configuration of remote base  #4 in Palomar mode.
//
// Revision 1.3  2004/08/28 22:31:31  Skip
// Added the ability to change the serial port baudrate and the address used
// by the Xcat on the bus.
//
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

#define	CONFIG_LEN	4

#define CONFIG_SQU_POT_MASK	0x58

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
extern int gRxDCS;
extern int gTxCTSS;
extern int gTxDCS;
extern int gTxOffset;
extern int gTxTimeout;
extern int gDebugMode;
extern int gXcatAdr;
extern CString gSaveFilename;
extern CString gRestoreFilename;
extern CString gModeName[32];
extern unsigned char gModeData[16];
extern int g_bHaveModeData;
extern unsigned char gConfig[CONFIG_LEN];
extern int g_bHaveConfig;
extern bool g_bHaveFWVer;
extern int gFirmwareVer;
extern CString gFirmwareVerString;
extern CString gLoaderVerString;
extern int gLastModeSel;
extern int gInvertedModeSel;
extern double gVCORxSplitVHF;
extern double gVCOTxSplitVHF;
extern double gVCORxSplit420;
extern double gVCOTxSplit420;
extern double gVCORxSplit440;
extern double gVCOTxSplit440;
extern int gEnableVolumePot;

// in x 0 -> 31, out x 31 -> 0
#define INVERT_MODE(x)  (gInvertedModeSel ? (~(x) & 0x1f) : (x))

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
	ID_SET_XCAT_ADR,
	ID_GET_CODEPLUG_DATA,
	ID_GET_SYNC_DEBUG,
	ID_GET_SIG_REPORT,
	ID_DOWNLOAD_CHAR
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
