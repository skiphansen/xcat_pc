// xcat.cpp : Defines the class behaviors for the application.
//
// $Log: xcat.cpp,v $
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

#include "stdafx.h"
#include "xcat.h"
#include "xcatDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static char *MonthNames[] = {
   "Jan",
   "Feb",
   "Mar",
   "Apr",
   "May",
   "Jun",
   "Jul",
   "Aug",
   "Sep",
   "Oct",
   "Nov",
   "Dec"
};


/////////////////////////////////////////////////////////////////////////////
// CXcatApp

BEGIN_MESSAGE_MAP(CXcatApp, CWinApp)
   //{{AFX_MSG_MAP(CXcatApp)
      // NOTE - the ClassWizard will add and remove mapping macros here.
      //    DO NOT EDIT what you see in these blocks of generated code!
   //}}AFX_MSG
   ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CXcatApp construction

CXcatApp::CXcatApp()
{
   // TODO: add construction code here,
   // Place all significant initialization in InitInstance
}



/////////////////////////////////////////////////////////////////////////////
// The one and only CXcatApp object

CXcatApp theApp;

int gComPort;
int gBaudrate;
int gBandScanStep;
double gBandScanTop;
double gBandScanBottom;
double gBandScanHangTime;
double gRxFrequency;
double gTxOffsetFreq;
int gRxCTSS;
int gTxCTSS;
int gTxOffset;
int gDebugMode;
int gXcatAdr;
CString gSaveFilename;
CString gRestoreFilename;
unsigned char gModeData[16];

RegVariable RegVars[] = {
   {"ComPort",&gComPort,REG_VAR_TYPE_INT,"1"},
   {"Baudrate",&gBaudrate,REG_VAR_TYPE_INT,"19200"},
   {"BandScanTop",&gBandScanTop,REG_VAR_TYPE_DOUBLE,"148.00"},
   {"BandScanBottom",&gBandScanBottom,REG_VAR_TYPE_DOUBLE,"144.00"},
   {"BandScanStep",&gBandScanStep,REG_VAR_TYPE_INT,"0"},
   {"BandScanHangTime",&gBandScanHangTime,REG_VAR_TYPE_DOUBLE,"0.0"},
   {"RxFrequency",&gRxFrequency,REG_VAR_TYPE_DOUBLE,"146.52"},
   {"RxCTSS",&gRxCTSS,REG_VAR_TYPE_INT,"0"},
   {"TxCTSS",&gTxCTSS,REG_VAR_TYPE_INT,"0"},
   {"TxOffset",&gTxOffset,REG_VAR_TYPE_INT,"1"},
   {"TxOffsetFreq",&gTxOffsetFreq,REG_VAR_TYPE_DOUBLE,".6"},
   {"SaveFilename",&gSaveFilename,REG_VAR_TYPE_CSTRING,"codeplug.bin"},
   {"RestoreFilename",&gRestoreFilename,REG_VAR_TYPE_CSTRING,"codeplug.bin"},
   {"DebugMode",&gDebugMode,REG_VAR_TYPE_INT,"0"},
   {"gXcatAdr",&gXcatAdr,REG_VAR_TYPE_INT,"32"},
   {NULL}
};

/////////////////////////////////////////////////////////////////////////////
// CXcatApp initialization

BOOL CXcatApp::InitInstance()
{
   // Standard initialization
   // If you are not using these features and wish to reduce the size
   //  of your final executable, you should remove from the following
   //  the specific initialization routines you do not need.

   time_t ltime;
   struct tm *tm;

   time(&ltime);
   tm = localtime(&ltime);

   LOG(("Xcat compiled " __DATE__ " starting on %s %d, %d\n",
        MonthNames[tm->tm_mon],tm->tm_mday,tm->tm_year+1900));

#ifdef _AFXDLL
   Enable3dControls();        // Call this when using MFC in a shared DLL
#else
   Enable3dControlsStatic();  // Call this when linking to MFC statically
#endif

   SetRegistryKey(_T("Xcat"));

// Restore global variables from the registry

   RegRestoreVar("Config",RegVars,0);

   CXcatDlg dlg("cat");

   m_pMainWnd = &dlg;
   int nResponse = dlg.DoModal();
   if (nResponse == IDOK)
   {
      // TODO: Place code here to handle when the dialog is
      //  dismissed with OK
   }
   else if (nResponse == IDCANCEL)
   {
      // TODO: Place code here to handle when the dialog is
      //  dismissed with Cancel
   }

   RegSaveVar("Config",RegVars,0);

   // Since the dialog has been closed, return FALSE so that we exit the
   //  application, rather than start the application's message pump.
   return FALSE;
}

void CXcatApp::RegSaveVar(const char *KeyName,RegVariable *pVarTable,void *Offset)
{
   CString Value;

   for(int i = 0; pVarTable[i].Name != NULL; i++) {
      LPBYTE cp = (LPBYTE) ((int) Offset + (int) pVarTable[i].Variable);
      switch(pVarTable[i].Type) {
         case REG_VAR_TYPE_INT:
         case REG_VAR_TYPE_UINT:
            WriteProfileInt(KeyName,pVarTable[i].Name,*((int *)cp));
            break;

         case REG_VAR_TYPE_CSTRING:
            WriteProfileString(KeyName,pVarTable[i].Name,*((CString *) cp));
            break;

         case REG_VAR_TYPE_DOUBLE: {
            CString Value;
            Value.Format("%lf",*((double *) cp));
            WriteProfileString(KeyName,pVarTable[i].Name,Value);
            break;
         }

         default:
            ASSERT(FALSE);
      }
   }
}

void CXcatApp::RegRestoreVar(const char *KeyName,RegVariable *pVarTable,void *Offset)
{
   CString Value;
   int InitValue;

   for(int i = 0; pVarTable[i].Name != NULL; i++) {
      LPBYTE cp = (LPBYTE) ((int) Offset + (int) pVarTable[i].Variable);
      switch(pVarTable[i].Type) {
         case REG_VAR_TYPE_INT:
            InitValue = atoi(pVarTable[i].InitValue);
            *((int *)cp) = GetProfileInt(KeyName,pVarTable[i].Name,InitValue);
            break;

         case REG_VAR_TYPE_UINT:
            sscanf(pVarTable[i].InitValue,"%u",&InitValue);
            *((int *)cp) = GetProfileInt(KeyName,pVarTable[i].Name,InitValue);
            break;

         case REG_VAR_TYPE_CSTRING:
            *((CString *) cp) = GetProfileString(KeyName,pVarTable[i].Name,
                                                 pVarTable[i].InitValue);
            break;

         case REG_VAR_TYPE_DOUBLE:
            Value = GetProfileString(KeyName,pVarTable[i].Name,
                                     pVarTable[i].InitValue);
            sscanf(Value,"%lf",cp);
            break;

         default:
            ASSERT(FALSE);
      }
   }
}

char *Err2String(int err)
{
   static char Ret[80];
   char *ErrMsg = strerror(err);

   if(ErrMsg != NULL) {
      _snprintf(Ret,sizeof(Ret),"%s (%d)\n",strerror(err),err);
   }
   else {
      _snprintf(Ret,sizeof(Ret),"err=%d\n",err);
   }
   return Ret;
}


FILE *LogFp = NULL;

void LogIt(char *fmt,...)
{
   char  Temp[1024];
   struct _timeb TimeNow;
   struct tm *tm;
   va_list args;
   static int bOpenErrReported = FALSE;
   
   va_start(args,fmt);
   
   _ftime(&TimeNow);
   tm = localtime(&TimeNow.time);

   if(gDebugMode && LogFp == NULL) {
      LogFp = fopen("c:\\xcat.log","a");

      if(LogFp == NULL) {
         if(!bOpenErrReported) {
            bOpenErrReported = TRUE;
            CString ErrMsg;
            ErrMsg.Format("Unable to open log file c:\\xcat.log\n"
                          "%s",Err2String(errno));
            AfxMessageBox(ErrMsg);
         }
      }
      else if(ftell(LogFp) > 1000000) {
      // Don't fill up the guys hard disk !
         fclose(LogFp);
         LogFp = NULL;
      }
   }

   if(LogFp != NULL) {
   // one big file log date & time
      _vsnprintf(Temp,sizeof(Temp),fmt,args);
      fprintf(LogFp,"%02d:%02d:%02d.%03d %s",tm->tm_hour,tm->tm_min,
              tm->tm_sec,TimeNow.millitm,Temp);
      fflush(LogFp);
   }
   va_end(args);
}

void LogHex(void *AdrIn,int Len)
{
   char Line[80];
   char *cp;
   unsigned char *Adr = (unsigned char *) AdrIn;
   int i = 0;
   int j;

   while(i < Len) {
      cp = Line;
      for(j = 0; j < 16; j++) {
         if((i + j) == Len) {
            break;
         }
         sprintf(cp,"%02x ",Adr[i+j]);
         cp += 3;
      }

      *cp++ = ' ';
      for(j = 0; j < 16; j++) {
         if((i + j) == Len) {
            break;
         }
         if(isprint(Adr[i+j])) {
            *cp++ = Adr[i+j];
         }
         else {
            *cp++ = '.';
         }
      }
      *cp = 0;
      LogIt("%04x: %s\n",Adr + i,Line);
      i += 16;
   }
}

