// XcatDialog.cpp : implementation file
//
// $Log: xcatDlg.cpp,v $
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
#include "XcatDlg.h"
#include "comm.h"

#define CONFIG_BAND_MASK 7
#define CONFIG_10M      0
#define CONFIG_6M       1
#define CONFIG_2M       2
#define CONFIG_10_6M    3
#define CONFIG_440      4

#define CONFIG_CTRL_MASK 0x3
#define CONFIG_GENERIC  0x1
#define CONFIG_CACTUS   0x2

#define SYNTOR_SCAN_TYPE_MASK    0xc0
#define SYNTOR_SCAN_TYPE_DOUBLE  0x00
#define SYNTOR_SCAN_TYPE_SINGLE  0x40
#define SYNTOR_SCAN_TYPE_NONE    0x80
#define SYNTOR_SCAN_TYPE_NONPRI  0xc0

#define SYNTOR_32MODE_MASK       0x1f


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

HWND hMainWindow;

void SetButtonMode(int OrigID,int ID,const char *Label,bool bEnabled,bool bHide)
{
   static CWnd *pApply = NULL;      // Property sheet's apply button
   static CWnd *pOk = NULL;
   static CWnd *pCancel = NULL;
   static CWnd *pHelp = NULL;
   CWnd *pCWnd;
   
   if(pApply == NULL) {
      pCWnd = AfxGetApp()->m_pMainWnd;
      pApply  = pCWnd->GetDlgItem(ID_APPLY_NOW);
      pOk     = pCWnd->GetDlgItem(IDOK);
      pCancel = pCWnd->GetDlgItem(IDCANCEL);
      pHelp   = pCWnd->GetDlgItem(IDHELP);
   }

   switch(OrigID) {
      case ID_APPLY_NOW:
         pCWnd = pApply;
         break;

      case IDOK:
         pCWnd = pOk;
         break;

      case IDCANCEL:
         pCWnd = pCancel;
         break;

      case IDHELP:
         pCWnd = pHelp;
         break;
   }

   if(pCWnd != NULL) {
      if(ID != 0) {
         pCWnd->SetDlgCtrlID(ID);
      }
      if(Label != NULL) {
         pCWnd->SetWindowText(Label);
      }
      pCWnd->EnableWindow(bEnabled);
      pCWnd->ShowWindow(bHide ? SW_HIDE : SW_SHOWNORMAL);
   }
}

void EnableItems(CWnd* pDialogWnd, int ResourceID[],BOOL EnableFlag)
{
   ASSERT_VALID(pDialogWnd);
   for(int i=0; ResourceID[i] != 0; i++){
      CWnd* pWnd = pDialogWnd->GetDlgItem(ResourceID[i]);
      ASSERT_VALID(pWnd);
      pWnd->EnableWindow(EnableFlag);
   }
}

/////////////////////////////////////////////////////////////////////////////
// CXcatDlg

IMPLEMENT_DYNAMIC(CXcatDlg, CPropertySheet)

CXcatDlg::CXcatDlg(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
   :CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{
   bHaveSignal = FALSE;
   SignalLostTime = 0;
   AddPage(&ManualPage);
   AddPage(&ScanPage);
   AddPage(&BandScan);
   AddPage(&CCommSetup);
   AddPage(&Configure);
   if(gDebugMode) {
      AddPage(&DebugMsgs);
   }
   AddPage(&CAbout);
}

CXcatDlg::~CXcatDlg()
{
}


BEGIN_MESSAGE_MAP(CXcatDlg, CPropertySheet)
   //{{AFX_MSG_MAP(CXcatDlg)
   ON_WM_PAINT()
   ON_WM_QUERYDRAGICON()
   ON_WM_TIMER()
   //}}AFX_MSG_MAP
   ON_MESSAGE(ID_RX_MSG,OnRxMsg)
   ON_MESSAGE(ID_COMM_ERROR,OnCommError)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CXcatDlg message handlers

BOOL CXcatDlg::OnInitDialog() 
{
   BOOL bResult = CPropertySheet::OnInitDialog();
#if 0
   GetDlgItem(IDOK)->ShowWindow(SW_HIDE);
   GetDlgItem(IDCANCEL)->ShowWindow(SW_HIDE);
   GetDlgItem(ID_APPLY_NOW)->ShowWindow(SW_HIDE);
   GetDlgItem(IDHELP)->ShowWindow(SW_HIDE);
#endif
   
   hMainWindow = m_hWnd;
   if(!CComm.Init(gComPort,gBaudrate)) {
      CString ErrMsg;
      ErrMsg.Format("Unable to open COM%d,\n"
                    "please check that no other\n"
                    "programs are using COM%d.",gComPort,gComPort);
      AfxMessageBox(ErrMsg);
   }
   ModifyStyle(WS_MAXIMIZEBOX,WS_MINIMIZEBOX);
   GetSystemMenu(FALSE)->InsertMenu(-1,MF_BYPOSITION | MF_STRING,SC_ICON,
                                    "Minimize" );
   SetIcon(AfxGetApp()->LoadIcon(IDR_MAINFRAME),TRUE);

   // Start a 200 millisecond timer
   if(!SetTimer(1,200,NULL)) {
      AfxMessageBox("Fatal error: Unable to create timer.");
      PostMessage(WM_QUIT,0,0);
   }
#if 0
   CComm.SetVCOSplits(0x01234567,0x89abcdef);
   CComm.GetVCOSplits();
#endif

   return bResult;
}

LRESULT CXcatDlg::OnRxMsg(WPARAM /* wParam*/, LPARAM lParam)
{
   AppMsg *p= (AppMsg *) lParam;
   CI_VMsg *pMsg = (CI_VMsg *) &p->Hdr;

   if(pMsg->Hdr.Cmd == 0xaa) {
   // Private Xcat command
      switch(pMsg->Data[0]) {
         case 0x80:  // response to get vfo raw data
            memcpy(gModeData,&pMsg->Data[1],sizeof(gModeData));
            ScanPage.ModeData();
            if(GetActivePage() == &DebugMsgs) {
               DebugMsgs.ModeData(&pMsg->Data[1]);
            }
            else if(GetActivePage() == &Configure) {
               Configure.ModeData(&pMsg->Data[1]);
            }
            break;

         case 0x81:  // Mode/signal change
            if(pMsg->Data[2]) {
            // Carrier detected
               bHaveSignal = TRUE;
               CString Mode;
               if(BandScan.m_bScanActive && pMsg->Data[1] == 0) {
                  Mode.Format("%3.4f",BandScan.mCurrentFreq);
               }
               else {
                  Mode.Format("cat - Mode %d",pMsg->Data[1] + 1);
               }
               SetTitle(Mode);
            }
            else {
            // Carrier lost
               bHaveSignal = FALSE;
               SignalLostTime = time(NULL);
               SetTitle("cat");
            }
            BandScan.m_bHaveSignal = bHaveSignal;
            if(GetActivePage() == &BandScan) {
               BandScan.CarrierDetectChange(bHaveSignal);
            }
            break;

         case 0x82:  // get firmware version number response 
         {  pMsg->Data[p->DataLen - sizeof(CI_V_Hdr)-1] = 0;
            CString Version;

            Version.Format("Firmware version: %s",(char *) &pMsg->Data[1]);
            CAbout.m_bHaveFWVer = TRUE;
            CAbout.GetDlgItem(IDC_FIRMWARE_VER)->SetWindowText(Version);
            break;
         }

         case 0x83:  // Get config
         {
            Configure.ConfigMsgRx(&pMsg->Data[1]);
            break;
         }

         case 0x85:  // get Tx offset
         {  int TxOffset = 0;
            int i;
            for(i = 0; i < 4; i++) {
               TxOffset <<= 4;
               TxOffset += pMsg->Data[i+1];
            }

            break;
         }

         case 0x87:  // get VCO splits
         {
            Configure.VCOSplits(&pMsg->Data[1]);
            break;
         }

         case 0x89:  // Debug info
         {
            break;
         }
      }
   }

   delete p;
   return 0;
}

LRESULT CXcatDlg::OnCommError(WPARAM /* wParam*/, LPARAM lParam)
{
   CString *pErrMsg = (CString *) lParam;
   AfxMessageBox(*pErrMsg);
   delete pErrMsg;

   return 0;
}


void CXcatDlg::OnTimer(UINT nIDEvent) 
{
   if(!bHaveSignal) {
      if(BandScan.m_bScanActive && 
         (BandScan.m_bPlScan  || 
          (time(NULL) - SignalLostTime) > BandScan.mHangTime))
      {
         BandScan.OnTimer();
      }
   }
   
   CPropertySheet::OnTimer(nIDEvent);
}

/////////////////////////////////////////////////////////////////////////////
// ManualPage property page

IMPLEMENT_DYNCREATE(ManualPage, CPropertyPage)

ManualPage::ManualPage() : CPropertyPage(ManualPage::IDD)
{
   mLastDecodePL = -1.0;
   mLastEncodePL = -1.0;
   mForcedSet = TRUE;

   //{{AFX_DATA_INIT(ManualPage)
   mRxFrequency = 0.0;
   mTxOffsetFreq = 0.0;
   //}}AFX_DATA_INIT
}

ManualPage::~ManualPage()
{
}

void ManualPage::DoDataExchange(CDataExchange* pDX)
{
   CPropertyPage::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(ManualPage)
   DDX_Control(pDX, IDC_TX_OFFSET, mTxOffset);
   DDX_Control(pDX, IDC_TX_PL, mTxPL);
   DDX_Control(pDX, IDC_RX_PL, mRxPL);
   DDX_Text(pDX, IDC_RX_FREQ, mRxFrequency);
   DDX_Text(pDX, IDC_TX_OFFSET_FREQ, mTxOffsetFreq);
   DDV_MinMaxDouble(pDX, mTxOffsetFreq, 0., 10000000.);
   //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(ManualPage, CPropertyPage)
   //{{AFX_MSG_MAP(ManualPage)
   //}}AFX_MSG_MAP
   ON_BN_CLICKED(ID_MANUAL_SET,OnManualSet)
   ON_BN_CLICKED(ID_SAVE_MODE,OnSaveMode)
   ON_BN_CLICKED(ID_RECALL_MODE,OnRecallMode)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// ManualPage message handlers

char *PLTones[] =
{
   "67.0 (XZ)","69.3 (WZ)","71.9 (XA)","74.4 (WA)","77.0 (XB)","79.7 (WB)",
   "82.5 (YZ)","85.4 (YA)","88.5 (YB)","91.5 (ZZ)","94.8 (ZA)","97.4 (ZB)",
   "100.0 (1Z)","103.5 (1A)","107.2 (1B)","110.9 (2Z)","114.8 (2A)",
   "118.8 (2B)","123.0 (3Z)","127.3 (3A)","131.8 (3B)","136.5 (4Z)",
   "141.3 (4A)","146.2 (4B)","151.4 (5Z)","156.7 (5A)","162.2 (5B)",
   "167.9 (6Z)","173.8 (6A)","179.9 (6B)","186.2 (7Z)","192.8 (7A)",
   "203.5 (M1)","206.5 (8Z)","210.7 (M2)","218.1 (M3)","225.7 (M4)",
   "229.1 (9Z)","233.6 (M5)","241.8 (M6)","250.3 (M7)","254.1 (0Z)",NULL
};

void FillPLBox(CComboBox* pCB,char *NoToneString)
{
   pCB->ResetContent();

   pCB->InsertString(-1,NoToneString);
   for(int i = 0; PLTones[i] != NULL; i++) {
      pCB->InsertString(-1,PLTones[i]);
   }
}

BOOL ManualPage::OnInitDialog() 
{
   CPropertyPage::OnInitDialog();
   
   FillPLBox(&mTxPL,"Disabled");
   mTxPL.SetCurSel(gTxCTSS);
   FillPLBox(&mRxPL,"Carrier");
   mRxPL.SetCurSel(gRxCTSS);
   mTxOffset.SetCurSel(gTxOffset);
   mRxFrequency = gRxFrequency;
   mTxOffsetFreq = gTxOffsetFreq;
   UpdateData(FALSE);
   
   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL ManualPage::OnSetActive() 
{
   SetButtonMode(IDOK,ID_MANUAL_SET,"Set",TRUE,FALSE);
   SetButtonMode(IDCANCEL,ID_SAVE_MODE,"Store",TRUE,FALSE);
   SetButtonMode(ID_APPLY_NOW,ID_RECALL_MODE,"Recall",TRUE,!gDebugMode);
   SetButtonMode(IDHELP,0,NULL,FALSE,TRUE);
   
   mForcedSet = TRUE;
   return CPropertyPage::OnSetActive();
}

void ManualPage::OnManualSet()
{
   UpdateData(TRUE);
   gRxFrequency = mRxFrequency;
   gTxOffsetFreq = mTxOffsetFreq;

   if(mForcedSet || gTxOffsetFreq != mLastTxOffsetFreq) {
      mLastTxOffsetFreq = gTxOffsetFreq;
      CComm.SetTxOffset((int)(gTxOffsetFreq * 1000000.0));
   }

   if(mForcedSet || gTxOffset != mTxOffset.GetCurSel()) {
      gTxOffset = mTxOffset.GetCurSel();
      switch(gTxOffset) {
         case 0:  // + offset
            CComm.SetDuplex(0x12);
            break;

         case 1:  // simplex
            CComm.SetDuplex(0x10);
            break;

         case 2:  // - offset
            CComm.SetDuplex(0x11);
            break;
      }
   }

   CComm.SetFreq(mRxFrequency);

   float PlTone = 0.0;
   gTxCTSS = mTxPL.GetCurSel();
   if(gTxCTSS != 0) {
      sscanf(PLTones[gTxCTSS-1],"%f",&PlTone);
   }
   if(mForcedSet || PlTone != mLastEncodePL) {
      mLastEncodePL = PlTone;
      CComm.SetCTSSFreq(PlTone,FALSE);
   }

   PlTone = 0.0;
   gRxCTSS = mRxPL.GetCurSel();
   if(gRxCTSS != 0) {
      sscanf(PLTones[gRxCTSS-1],"%f",&PlTone);
   }
   if(mForcedSet || PlTone != mLastDecodePL) {
      mLastDecodePL = PlTone;
      CComm.SetCTSSFreq(PlTone,TRUE);
   }
   mForcedSet = FALSE;
}

void ManualPage::OnSaveMode()
{
   CModeSel dlg;

   OnManualSet();
   if(dlg.DoModal() == IDOK) {
      CComm.SelectMode(dlg.mMode);
      CComm.StoreVFO();
   }
}

void ManualPage::OnRecallMode()
{
   CModeSel dlg;

   if(dlg.DoModal() == IDOK) {
      mForcedSet = TRUE; // send everything next time !
      CComm.SelectMode(dlg.mMode);
      CComm.RecallMode();
   }
}

/////////////////////////////////////////////////////////////////////////////
// CScanEnable property page

IMPLEMENT_DYNCREATE(CScanEnable, CPropertyPage)

CScanEnable::CScanEnable() : CPropertyPage(CScanEnable::IDD)
{
   //{{AFX_DATA_INIT(CScanEnable)
   m_bScanEnabled = FALSE;
   m_bTalkbackEnabled = FALSE;
   m_bFixedScan = FALSE;
   //}}AFX_DATA_INIT
}

CScanEnable::~CScanEnable()
{
}

void CScanEnable::DoDataExchange(CDataExchange* pDX)
{
   CPropertyPage::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CScanEnable)
   DDX_Control(pDX, IDC_2ND_PRIORITY, m2ndPriorityMode);
   DDX_Control(pDX, IDC_PRIORITY_CHAN, mPriorityMode);
   DDX_Check(pDX, IDC_SCAN_ENABLED, m_bScanEnabled);
   DDX_Check(pDX, IDC_TB_ENABLED, m_bTalkbackEnabled);
   DDX_Check(pDX, IDC_FIXED_SCAN, m_bFixedScan);
   //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CScanEnable, CPropertyPage)
   //{{AFX_MSG_MAP(CScanEnable)
   ON_BN_CLICKED(IDC_SCAN_ENABLED, OnScanEnabled)
   ON_BN_CLICKED(IDC_FIXED_SCAN, OnFixedScan)
   ON_CBN_SELCHANGE(IDC_PRIORITY_CHAN, OnSelchangePriorityChan)
   //}}AFX_MSG_MAP
   ON_BN_CLICKED(ID_MANUAL_SET,OnManualSet)
   ON_BN_CLICKED(ID_SAVE_MODE,OnSaveMode)
   ON_BN_CLICKED(ID_RECALL_MODE,OnRecallMode)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CScanEnable message handlers

int NPResoursesIDs[] = {
   IDC_SCAN_MODE1,
   IDC_SCAN_MODE2,
   IDC_SCAN_MODE3,
   IDC_SCAN_MODE4,
   IDC_SCAN_MODE5,
   IDC_SCAN_MODE6,
   IDC_SCAN_MODE7,
   IDC_SCAN_MODE8,
   IDC_SCAN_MODE9,
   IDC_SCAN_MODE10,
   IDC_SCAN_MODE11,
   IDC_SCAN_MODE12,
   IDC_SCAN_MODE13,
   IDC_SCAN_MODE14,
   IDC_SCAN_MODE15,
   IDC_SCAN_MODE16,
   IDC_SCAN_MODE17,
   IDC_SCAN_MODE18,
   IDC_SCAN_MODE19,
   IDC_SCAN_MODE20,
   IDC_SCAN_MODE21,
   IDC_SCAN_MODE22,
   IDC_SCAN_MODE23,
   IDC_SCAN_MODE24,
   IDC_SCAN_MODE25,
   IDC_SCAN_MODE26,
   IDC_SCAN_MODE27,
   IDC_SCAN_MODE28,
   IDC_SCAN_MODE29,
   IDC_SCAN_MODE30,
   IDC_SCAN_MODE31,
   IDC_SCAN_MODE32,
   0
};

int ScanResoursesIDs[] = {
   IDC_PRIORITY_CHAN,
   IDC_TB_ENABLED,
   IDC_FIXED_SCAN,
   0
};

int SecondPriorityResoursesID[] = {
	IDC_2ND_PRIORITY,
	0
};

BOOL CScanEnable::OnInitDialog() 
{
   CPropertyPage::OnInitDialog();
   
   mPriorityMode.InsertString(-1,"None");
   m2ndPriorityMode.InsertString(-1,"None");
   for(int i = 0; i < 32; i++) {
      CString ModeString;
      ModeString.Format("Mode %d",i+1);
      mPriorityMode.InsertString(-1,ModeString);
      m2ndPriorityMode.InsertString(-1,ModeString);
   }

   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CScanEnable::OnSetActive() 
{
   SetButtonMode(IDOK,ID_MANUAL_SET,"Set",TRUE,FALSE);
   SetButtonMode(IDCANCEL,ID_SAVE_MODE,"Store",TRUE,FALSE);
   SetButtonMode(ID_APPLY_NOW,ID_RECALL_MODE,"Recall",TRUE,FALSE);
   SetButtonMode(IDHELP,0,NULL,FALSE,TRUE);
   
   CComm.GetModeData();

   return CPropertyPage::OnSetActive();
}


void CScanEnable::ModeData()
{
   CButton *pRB;

   switch(gModeData[9] & SYNTOR_SCAN_TYPE_MASK) {
      case SYNTOR_SCAN_TYPE_DOUBLE:
         m_bScanEnabled = TRUE;
         mPriorityMode.SetCurSel((gModeData[0xa] & SYNTOR_32MODE_MASK) + 1);
         m2ndPriorityMode.SetCurSel((gModeData[9] & SYNTOR_32MODE_MASK) + 1);
         break;

      case SYNTOR_SCAN_TYPE_SINGLE:
         m_bScanEnabled = TRUE;
         mPriorityMode.SetCurSel((gModeData[0xa] & SYNTOR_32MODE_MASK) + 1);
         m2ndPriorityMode.SetCurSel(0);
         break;

      case SYNTOR_SCAN_TYPE_NONE:
         m_bScanEnabled = FALSE;
         mPriorityMode.SetCurSel(0);
         m2ndPriorityMode.SetCurSel(0);
         break;

      case SYNTOR_SCAN_TYPE_NONPRI:
         m_bScanEnabled = TRUE;
         mPriorityMode.SetCurSel(0);
         m2ndPriorityMode.SetCurSel(0);
         break;
   }

   if(gModeData[9] & 0x20) {
      m_bTalkbackEnabled = FALSE;
   }
   else {
   // Talk back scan enabled
      m_bTalkbackEnabled = TRUE;
   }

   if(gModeData[0xa] & 0x80) {
      m_bFixedScan = TRUE;
   }
   else {
      m_bFixedScan = FALSE;
   }

   int NPBits = (gModeData[0] << 24) | (gModeData[1] << 16) | (gModeData[2] << 8) | gModeData[3];
   unsigned int Mask = 0x80000000;

   for(int i = 0; i < 32; i++) {
      pRB = (CButton *) GetDlgItem(NPResoursesIDs[i]);
      pRB->SetCheck(NPBits & Mask ? FALSE : TRUE);
      Mask >>= 1;
   }
   UpdateData(FALSE);
   EnableDisableControls();
}

void CScanEnable::OnManualSet()
{
   UpdateData(TRUE);

   int NPBits = 0xffffffff;
   unsigned int Mask = 0x80000000;

   CButton *pRB;

   for(int i = 0; i < 32; i++) {
      pRB = (CButton *) GetDlgItem(NPResoursesIDs[i]);
      if(pRB->GetCheck()) {
         NPBits &= ~Mask;
      }
      Mask >>= 1;
   }

   gModeData[0] = (unsigned char) ((NPBits >> 24) & 0xff);
   gModeData[1] = (unsigned char) ((NPBits >> 16) & 0xff);
   gModeData[2] = (unsigned char) ((NPBits >> 8) & 0xff);
   gModeData[3] = (unsigned char) (NPBits & 0xff);

   if(m_bTalkbackEnabled) {
      gModeData[9] &= ~0x20;
   }
   else {
      gModeData[9] |= 0x20;
   }

   if(m_bFixedScan) {
      gModeData[0xa] |= 0x80;
   }
   else {
      gModeData[0xa] &= ~0x80;
   }

   int PrioritySel = mPriorityMode.GetCurSel();
   int SecondPrioritySel = m2ndPriorityMode.GetCurSel();

   gModeData[9] &= ~SYNTOR_SCAN_TYPE_MASK;
   gModeData[9] &=  ~SYNTOR_32MODE_MASK;
   gModeData[0xa] &=  ~SYNTOR_32MODE_MASK;

   if(m_bScanEnabled) {
      if(PrioritySel == 0) {
      // No priority channel selected
         gModeData[9] |= SYNTOR_SCAN_TYPE_NONPRI;
      }
      else if(SecondPrioritySel == 0) {
      // No 2nd priority channel selected
         gModeData[9] |= SYNTOR_SCAN_TYPE_SINGLE;
         gModeData[0xa] |= (PrioritySel - 1);
      }
      else {
      // Both priority channels selected
         gModeData[9] |= SYNTOR_SCAN_TYPE_DOUBLE;
         gModeData[9] |= (SecondPrioritySel - 1);
         gModeData[0xa] |= (PrioritySel - 1);
      }
   }
   else {
      gModeData[9] |= SYNTOR_SCAN_TYPE_NONE;
   }
   CComm.SetModeData(&gModeData[0]);

}

void CScanEnable::EnableDisableControls() 
{
   UpdateData(TRUE);
   
   EnableItems(this,NPResoursesIDs,m_bScanEnabled && m_bFixedScan);
   EnableItems(this,ScanResoursesIDs,m_bScanEnabled);
   EnableItems(this,SecondPriorityResoursesID,
					m_bScanEnabled && mPriorityMode.GetCurSel() != 0);
}

void CScanEnable::OnScanEnabled() 
{
   EnableDisableControls();
}

void CScanEnable::OnFixedScan() 
{
   EnableDisableControls();
}

void CScanEnable::OnSelchangePriorityChan() 
{
   EnableDisableControls();
}

void CScanEnable::OnSaveMode()
{
   CModeSel dlg;

   OnManualSet();
   if(dlg.DoModal() == IDOK) {
      CComm.SelectMode(dlg.mMode);
      CComm.StoreVFO();
   }
}

void CScanEnable::OnRecallMode()
{
   CModeSel dlg;

   if(dlg.DoModal() == IDOK) {
      CComm.SelectMode(dlg.mMode);
      CComm.RecallMode();
      CComm.GetModeData();
   }
}

/////////////////////////////////////////////////////////////////////////////
// CBandScan property page

IMPLEMENT_DYNCREATE(CBandScan, CPropertyPage)

CBandScan::CBandScan() : CPropertyPage(CBandScan::IDD)
{
   m_bScanActive = FALSE;
   m_bPlScan = FALSE;
   m_bHaveSignal = FALSE;
   m_bListSelected = FALSE;
   mPlFreq = 0;
   mLastBottom = 0.0;
   mLastTop = 0.0;
   //{{AFX_DATA_INIT(CBandScan)
   mBandScanTop = gBandScanTop;
   mBandScanBottom = gBandScanBottom;
   mScanFreq = _T("");
   mHangTime = gBandScanHangTime;
   mPLFreqText = _T("");
   //}}AFX_DATA_INIT
}

CBandScan::~CBandScan()
{
}

void CBandScan::DoDataExchange(CDataExchange* pDX)
{
   CPropertyPage::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CBandScan)
   DDX_Control(pDX, IDC_LOCKOUT_LIST, mLockoutList);
   DDX_Control(pDX, IDC_BAND_SCAN_STEP, mStep);
   DDX_Text(pDX, IDC_BAND_SCAN_TOP, mBandScanTop);
   DDX_Text(pDX, IDC_BAND_SCAN_BOTTOM, mBandScanBottom);
   DDX_Text(pDX, IDC_SCAN_FREQ, mScanFreq);
   DDX_Text(pDX, IDC_HANG_TIME, mHangTime);
   DDV_MinMaxDouble(pDX, mHangTime, 0., 120.);
   DDX_Text(pDX, IDC_PL_SCAN, mPLFreqText);
   //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CBandScan, CPropertyPage)
   //{{AFX_MSG_MAP(CBandScan)
   ON_EN_SETFOCUS(IDC_BAND_SCAN_TOP, OnSetfocusBandScanTop)
   ON_EN_SETFOCUS(IDC_BAND_SCAN_BOTTOM, OnSetfocusBandScanBottom)
   ON_CBN_SETFOCUS(IDC_BAND_SCAN_STEP, OnSetfocusBandScanStep)
   ON_LBN_SETFOCUS(IDC_LOCKOUT_LIST, OnSetfocusLockoutList)
   ON_WM_DESTROY()
   ON_BN_CLICKED(IDC_BAND_SCAN, OnBandScan)
   ON_BN_CLICKED(IDC_DO_PL_SCAN, OnDoPlScan)
   //}}AFX_MSG_MAP
   ON_BN_CLICKED(ID_BAND_SCAN_START,OnStart)
   ON_BN_CLICKED(ID_BAND_SCAN_LOCKOUT,OnLockout)
   ON_BN_CLICKED(ID_BAND_SCAN_DEL_LOCKOUT,OnDelLockout)
   ON_BN_CLICKED(ID_SKIP,OnSkip)
END_MESSAGE_MAP()

BOOL CBandScan::OnInitDialog() 
{
   CPropertyPage::OnInitDialog();
   
   mStep.SetCurSel(gBandScanStep);
   for(int i = 0; ; i++) {
      CString Temp;
      CString KeyName;

      KeyName.Format("BandLockout%d",i);

      Temp = theApp.GetProfileString("Config",KeyName,"");
      if(strlen(Temp) == 0 || Temp == "0") {
         break;
      }
      mLockoutList.AddString(Temp);
   }

   if(m_bPlScan) {
      CButton *pRB = (CButton *) GetDlgItem(IDC_DO_PL_SCAN);
      pRB->SetCheck(TRUE);
   }
   else {
      CButton *pRB = (CButton *) GetDlgItem(IDC_BAND_SCAN);
      pRB->SetCheck(TRUE);
   }
   
   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}

/////////////////////////////////////////////////////////////////////////////
// CBandScan message handlers
void CBandScan::InitButtons() 
{
   SetButtonMode(IDOK,ID_BAND_SCAN_START,m_bScanActive ? "Stop" : "Start",
                 TRUE,FALSE);
   SetButtonMode(IDCANCEL,ID_BAND_SCAN_LOCKOUT,"Lockout",
                 m_bHaveSignal && !m_bPlScan,FALSE);
   SetButtonMode(ID_APPLY_NOW,ID_BAND_SCAN_DEL_LOCKOUT,"UnLock",
                 m_bListSelected,FALSE);
   SetButtonMode(IDHELP,ID_SKIP,"Skip",m_bHaveSignal,FALSE);
}

BOOL CBandScan::OnSetActive() 
{
   InitButtons();
// Band scan in carrier squelch
   ChangePL(FALSE);

   return CPropertyPage::OnSetActive();
}

void CBandScan::OnStart()
{
   static int BandScanStepLookup[] = {
      5000,    // 5 Khz
      6250,    // 6.25 Khz
      10000,   // 10 Khz
      12500,   // 12.5 Khz
      15000,   // 15 Khz
      20000,   // 20 Khz
      25000,   // 25 Khz
      30000,   // 30 Khz
   };

   m_bScanActive = !m_bScanActive;
   if(m_bScanActive) {
      UpdateData(TRUE);
      if(m_bPlScan) {
         ChangePL(TRUE);
      }
      else {
         if(mBandScanBottom != mLastBottom || mBandScanTop != mLastTop) {
            mLastBottom = mBandScanBottom;
            mLastTop = mBandScanTop;
            CComm.SetFreq(mBandScanBottom);
            mCurrentFreq = mBandScanBottom;
         }

         mScanFreq.Format("%3.4f",mCurrentFreq);
         gBandScanStep = mStep.GetCurSel();
         mFreqStep = BandScanStepLookup[gBandScanStep];
         gBandScanTop = mBandScanTop;
         gBandScanBottom = mBandScanBottom;
         gBandScanHangTime = mHangTime;
         UpdateData(FALSE);
      }
   }
   InitButtons();
}

bool CBandScan::LockedOutFreq()
{
   CString Freq;
   CString Temp;
   bool Ret = FALSE;

   Freq.Format("%3.4f",mCurrentFreq);

   int LockOutCount = mLockoutList.GetCount();
   for(int i = 0; i < LockOutCount; i++) {
      mLockoutList.GetText(i,Temp);
      if(Freq == Temp) {
         Ret = TRUE;
         break;
      }
   }
   
   return Ret;
}

void CBandScan::SaveLockedOutList()
{
   CString Freq;
   CString Temp;
   CString KeyName;
   bool Ret = FALSE;

   int LockOutCount = mLockoutList.GetCount();
   for(int i = 0; i < LockOutCount; i++) {
      KeyName.Format("BandLockout%d",i);
      mLockoutList.GetText(i,Temp);
      theApp.WriteProfileString("Config",KeyName,Temp);
   }
   
   KeyName.Format("BandLockout%d",i);
   theApp.WriteProfileString("Config",KeyName,"0");
}

void CBandScan::OnLockout()
{
   if(!LockedOutFreq()) {
   // New entry
      CString Freq;
      Freq.Format("%3.4f",mCurrentFreq);
      mLockoutList.AddString(Freq);
      ChangeFrequency();
   }
}

void CBandScan::OnDelLockout()
{
   int Selection = mLockoutList.GetCurSel();

   if(Selection != -1) {
      mLockoutList.DeleteString(mLockoutList.GetCurSel());
      UpdateData(FALSE);
      if(Selection < mLockoutList.GetCount()) {
         mLockoutList.SetCurSel(Selection);
      }
      else {
         m_bListSelected = FALSE;
         InitButtons();
      }
   }
}

void CBandScan::OnSkip()
{
   if(m_bPlScan) {
      ChangePL(TRUE);
   }
   else {
      ChangeFrequency();
   }
}

void CBandScan::ChangePL(bool bSetPl)
{
   float PlTone = 0.0;

   UpdateData(TRUE);
   if(bSetPl) {
      if(PLTones[mPlFreq] == NULL) {
         mPlFreq = 0;
      }
      mPLFreqText = PLTones[mPlFreq];
      sscanf(PLTones[mPlFreq++],"%f",&PlTone);
   }
   else {
      mPLFreqText = "Carrier Squelch";
   }
   UpdateData(FALSE);
   CComm.SetCTSSFreq(PlTone,TRUE);
}


void CBandScan::ChangeFrequency()
{
   UpdateData(TRUE);
   bool bWrapped = FALSE;

   do {
      mCurrentFreq = ((mCurrentFreq * 1000000) + mFreqStep) / 1000000;
      if(mCurrentFreq > mBandScanTop) {
         mCurrentFreq = mBandScanBottom;
         if(bWrapped) {
         // prevent infinite loop
            break;
         }
         bWrapped = TRUE;
      }
   } while(LockedOutFreq());

   CComm.SetFreq(mCurrentFreq);
   mScanFreq.Format("%3.4f",mCurrentFreq);
   UpdateData(FALSE);
}

void CBandScan::OnTimer()
{  // Next frequency
   if(m_bScanActive) {
      if(m_bPlScan) {
         ChangePL(TRUE);
      }
      else {
         ChangeFrequency();
      }
   }
}

void CBandScan::OnSetfocusLockoutList() 
{
   m_bListSelected = TRUE;
   InitButtons();
}

void CBandScan::CarrierDetectChange(bool bHaveSignal)
{
   InitButtons();
}

void CBandScan::OnSetfocusBandScanTop() 
{
   if(m_bListSelected) {
      m_bListSelected = FALSE;
      InitButtons();
   }
}

void CBandScan::OnSetfocusBandScanBottom() 
{
   if(m_bListSelected) {
      m_bListSelected = FALSE;
      InitButtons();
   }
}

void CBandScan::OnSetfocusBandScanStep() 
{
   if(m_bListSelected) {
      m_bListSelected = FALSE;
      InitButtons();
   }
}

void CBandScan::OnDestroy() 
{
   SaveLockedOutList();
   CPropertyPage::OnDestroy();
}

BOOL CBandScan::OnKillActive() 
{
   m_bScanActive = FALSE;
   if(m_bPlScan) {
   // Return to carrier squelch
      ChangePL(FALSE);
   }
   return CPropertyPage::OnKillActive();
}


void CBandScan::OnBandScan() 
{
   if(m_bPlScan) {
   // Return to carrier squelch
      ChangePL(FALSE);
   }
   m_bPlScan = FALSE;
   m_bScanActive = FALSE;
   InitButtons();
}

void CBandScan::OnDoPlScan() 
{
   m_bPlScan = TRUE;
   m_bScanActive = FALSE;
   InitButtons();
}

/////////////////////////////////////////////////////////////////////////////
// CCommSetup property page

IMPLEMENT_DYNCREATE(CCommSetup, CPropertyPage)

CCommSetup::CCommSetup() : CPropertyPage(CCommSetup::IDD)
{
   //{{AFX_DATA_INIT(CCommSetup)
	mXCatAdr = _T("");
	//}}AFX_DATA_INIT
}

CCommSetup::~CCommSetup()
{
}

void CCommSetup::DoDataExchange(CDataExchange* pDX)
{
   CPropertyPage::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CCommSetup)
	DDX_Text(pDX, IDC_XCAT_ADR, mXCatAdr);
	DDV_MaxChars(pDX, mXCatAdr, 2);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCommSetup, CPropertyPage)
   //{{AFX_MSG_MAP(CCommSetup)
      // NOTE: the ClassWizard will add message map macros here
   ON_BN_CLICKED(IDC_COM1, OnCom1)
   ON_BN_CLICKED(IDC_COM2, OnCom2)
   ON_BN_CLICKED(IDC_COM3, OnCom3)
   ON_BN_CLICKED(IDC_COM4, OnCom4)
   ON_BN_CLICKED(IDC_PROPERTIES, OnProperties)
   //}}AFX_MSG_MAP
   ON_BN_CLICKED(ID_SET_XCAT_ADR,OnSet)
END_MESSAGE_MAP()

int CCommSetup::ComPortResourceID[] = {
   IDC_COM1,
   IDC_COM2,
   IDC_COM3,
   IDC_COM4
};

BOOL CCommSetup::OnInitDialog()
{
   CDialog::OnInitDialog();
   // select the appropriate radio button

   if(mComPort < 1 || mComPort > 4) // force to a valid range
      mComPort = 1;
   GrayCommButtons();
	mXCatAdr.Format("%02X",gXcatAdr);
   UpdateData(FALSE);

   return FALSE;  // focus set
}

void CCommSetup::GrayCommButtons()
{
   // gray out ports which do not exist

   char ComString[5];
   BOOL EnableButton;

   for(int i = 0; i < 4; i++){
      sprintf(ComString,"COM%d",i+1);
      HANDLE idComDev = CreateFile(ComString,GENERIC_READ | GENERIC_WRITE,0,NULL,
                     OPEN_EXISTING,FILE_FLAG_OVERLAPPED,0);
      if(idComDev == INVALID_HANDLE_VALUE) {
         DWORD err = GetLastError();
         if(err == ERROR_ACCESS_DENIED) {
         // Port exists, but is in use
            EnableButton = TRUE;
         }
         else {
            EnableButton = FALSE;
         }
      }
      else{
         // close the port
         CloseHandle(idComDev);
         EnableButton = TRUE;
      }
      CButton *pRB = (CButton *) GetDlgItem(ComPortResourceID[i]);
      ASSERT_VALID(pRB);
      pRB->EnableWindow(EnableButton);
      pRB->SetCheck(i+1 == mComPort && EnableButton);
   }
}

void CCommSetup::OnCom1()
{
   mComPort = 1;
}

void CCommSetup::OnCom2()
{
   mComPort = 2;
}

void CCommSetup::OnCom3()
{
   mComPort = 3;
}

void CCommSetup::OnCom4()
{
   mComPort = 4;
}

BOOL CCommSetup::OnSetActive() 
{
   SetButtonMode(IDOK,ID_SET_XCAT_ADR,"Set",TRUE,FALSE);
   SetButtonMode(IDCANCEL,0,NULL,FALSE,TRUE);
   SetButtonMode(ID_APPLY_NOW,0,NULL,FALSE,TRUE);
   SetButtonMode(IDHELP,0,NULL,FALSE,TRUE);
   
   return CPropertyPage::OnSetActive();
}

void CCommSetup::OnProperties()
{
   char ComString[5];
   COMMCONFIG cc;

   sprintf(ComString,"COM%d",mComPort);
   cc.dwSize = sizeof(cc);
   cc.wVersion = 1;
   cc.dcb = CComm.mDcb;

   if(CommConfigDialog(ComString,m_hWnd,&cc)) {
		gBaudrate = cc.dcb.BaudRate;
		if(!CComm.Init(gComPort,gBaudrate)) {
			CString ErrMsg;
			ErrMsg.Format("Unable to open COM%d,\n"
							  "please check that no other\n"
							  "programs are using COM%d.",gComPort,gComPort);
			AfxMessageBox(ErrMsg);
		}
   }
}


void CCommSetup::OnSet() 
{
   UpdateData(TRUE);
	
	if(sscanf((LPCSTR) mXCatAdr,"%x",&gXcatAdr) != 1) {
      CString ErrMsg;
      ErrMsg.Format("Error: the Xcat CI-V address is invalid.\n"
						  "Please enter a valid Hex address.");
      AfxMessageBox(ErrMsg);
	}
	else {
		CPropertyPage::OnOK();
	}
}

void CXcatDlg::OnPaint() 
{
   CPaintDC dc(this); // device context for painting

   if(IsIconic()) {
      // Erase the icon background when placed over other app window
      DefWindowProc(WM_ICONERASEBKGND, (WORD)dc.m_hDC, 0L);

      // Center the icon
      CRect rc;
      GetClientRect(&rc);
      rc.left = (rc.right  - ::GetSystemMetrics(SM_CXICON)) >> 1;
      rc.top  = (rc.bottom - ::GetSystemMetrics(SM_CYICON)) >> 1;

      // Draw the icon
      HICON hIcon = AfxGetApp()->LoadIcon(AFX_IDI_STD_FRAME);
      dc.DrawIcon(rc.left, rc.top, hIcon);
   }

   // Do not call CPropertySheet::OnPaint() for painting messages
}

HCURSOR CXcatDlg::OnQueryDragIcon() 
{
   // TODO: Add your message handler code here and/or call default
    return AfxGetApp()->LoadIcon(AFX_IDI_STD_FRAME);  
   // return CPropertySheet::OnQueryDragIcon();
}



/////////////////////////////////////////////////////////////////////////////
// CAbout property page

IMPLEMENT_DYNCREATE(CAbout, CPropertyPage)

CAbout::CAbout() : CPropertyPage(CAbout::IDD)
{
   m_bHaveFWVer = FALSE;
   //{{AFX_DATA_INIT(CAbout)
   //}}AFX_DATA_INIT
}

CAbout::~CAbout()
{
}

void CAbout::DoDataExchange(CDataExchange* pDX)
{
   CPropertyPage::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CAbout)
   DDX_Control(pDX, IDC_COMPILED, mCompiled);
   //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAbout, CPropertyPage)
   //{{AFX_MSG_MAP(CAbout)
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAbout message handlers


BOOL CAbout::OnSetActive() 
{
   if(!m_bHaveFWVer)
   {
      CComm.RequestFWVer();
   }

   SetButtonMode(IDOK,0,NULL,FALSE,TRUE);
   SetButtonMode(IDCANCEL,0,NULL,FALSE,TRUE);
   SetButtonMode(ID_APPLY_NOW,0,NULL,FALSE,TRUE);
   SetButtonMode(IDHELP,0,NULL,FALSE,TRUE);
   

   return CPropertyPage::OnSetActive();
}
/////////////////////////////////////////////////////////////////////////////
// CDebugMsgs property page

IMPLEMENT_DYNCREATE(CDebugMsgs, CPropertyPage)

CDebugMsgs::CDebugMsgs() : CPropertyPage(CDebugMsgs::IDD)
{
   //{{AFX_DATA_INIT(CDebugMsgs)
      // NOTE: the ClassWizard will add member initialization here
   //}}AFX_DATA_INIT
}

CDebugMsgs::~CDebugMsgs()
{
}

void CDebugMsgs::DoDataExchange(CDataExchange* pDX)
{
   CPropertyPage::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CDebugMsgs)
   DDX_Control(pDX, IDC_DEBUG_MSGS, mEdit);
   //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDebugMsgs, CPropertyPage)
   //{{AFX_MSG_MAP(CDebugMsgs)
      // NOTE: the ClassWizard will add message map macros here
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDebugMsgs message handlers

BOOL CDebugMsgs::OnSetActive() 
{
   CComm.GetModeData();
#if 0
   CComm.GetSigReport();
#endif

   mEdit.SetWindowText("");
   return CPropertyPage::OnSetActive();
}

void CDebugMsgs::ModeData(unsigned char *Data)
{
   char Text[80];
   char *cp = Text;

   for(int i = 0; i < 8; i++) {
      cp += sprintf(cp,"%02X ",Data[i]);
   }
   strcat(cp,"\r\n");
   cp += 2;

   for(; i < 16; i++) {
      cp += sprintf(cp,"%02X ",Data[i]);
   }
   strcat(cp,"\r\n");

   mEdit.SetWindowText(Text);

}

void CDebugMsgs::SignalReport(int Mode,int bSignal)
{
   CString Text;
   CString Report;

   Report.Format("\r\nMode %d, Carrier %d\r\n",Mode,bSignal);

   mEdit.GetWindowText(Text);

   Text += Report;
   mEdit.SetWindowText(Text);
}

/////////////////////////////////////////////////////////////////////////////
// CConfigure property page

IMPLEMENT_DYNCREATE(CConfigure, CPropertyPage)

CConfigure::CConfigure() : CPropertyPage(CConfigure::IDD)
{
   //{{AFX_DATA_INIT(CConfigure)
   mRxVcoSplitFreq = 215.6;
   mSendCosMsg = FALSE;
   mTxVcoSplitFreq = 161.8;
   //}}AFX_DATA_INIT

   mFp = NULL;
}

CConfigure::~CConfigure()
{
}

void CConfigure::DoDataExchange(CDataExchange* pDX)
{
   CPropertyPage::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CConfigure)
   DDX_Control(pDX, IDC_TRANSFER_STATUS, mTransferStatus);
   DDX_Control(pDX, IDC_OUT_5, mOut5);
   DDX_Control(pDX, IDC_OUT_2, mOut2);
   DDX_Control(pDX, IDC_OUT_1, mOut1);
   DDX_Control(pDX, IDC_OUT_0, mOut0);
   DDX_Control(pDX, IDC_CONTROL_SYS, mControlSys);
   DDX_Control(pDX, IDC_BAND, mBand);
   DDX_Text(pDX, IDC_RX_VCO_SPLIT_F, mRxVcoSplitFreq);
   DDV_MinMaxDouble(pDX, mRxVcoSplitFreq, 100., 250.);
   DDX_Check(pDX, IDC_SEND_COS, mSendCosMsg);
   DDX_Text(pDX, IDC_TX_VCO_SPLIT_F, mTxVcoSplitFreq);
   DDV_MinMaxDouble(pDX, mTxVcoSplitFreq, 100., 250.);
   //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CConfigure, CPropertyPage)
   //{{AFX_MSG_MAP(CConfigure)
   ON_BN_CLICKED(IDC_RANGE1, OnRange1)
   ON_BN_CLICKED(IDC_RANGE2, OnRange2)
   ON_CBN_SELCHANGE(IDC_BAND, OnSelchangeBand)
   //}}AFX_MSG_MAP
   ON_BN_CLICKED(ID_GET_CONFIG,OnGetConfig)
   ON_BN_CLICKED(ID_SET_CONFIG,OnSetConfig)
   ON_BN_CLICKED(ID_SAVE_CODEPLUG,OnSaveCodePlug)
   ON_BN_CLICKED(ID_RESTORE_CODEPLUG,OnRestoreCodePlug)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConfigure message handlers

void CConfigure::InitButtons() 
{
   SetButtonMode(IDOK,ID_GET_CONFIG,"Get Config",TRUE,FALSE);
   SetButtonMode(IDCANCEL,ID_SET_CONFIG,"Set Config",TRUE,FALSE);
   SetButtonMode(ID_APPLY_NOW,ID_SAVE_CODEPLUG,"Save Modes",TRUE,FALSE);
   SetButtonMode(IDHELP,ID_RESTORE_CODEPLUG,"Load Modes",TRUE,FALSE);
}


BOOL CConfigure::OnSetActive() 
{
   InitButtons();
   return CPropertyPage::OnSetActive();
}

void CConfigure::OnGetConfig()
{
   CComm.GetVCOSplits();
   CComm.GetConfig();
}

void CConfigure::OnSetConfig()
{
   UpdateData(TRUE);
   int Config[2];

   Config[0] = mBand.GetCurSel();

   switch(mControlSys.GetCurSel()) {
      case 0:  // No control system
         break;

      case 1:  // Doug Hall
         Config[0] |= 0x10;
         break;

      case 2:  // Palomar Telecom / Cactus / Remote base #1
         Config[0] |= 0x20;
         break;

      case 3:  // Palomar Telecom / Cactus / Remote base #2
         Config[0] |= 0x60;
         break;

      case 4:  // Palomar Telecom / Cactus / Remote base #3
         Config[0] |= 0xa0;
         break;

      case 5:  // Palomar Telecom / Cactus / Remote base #4
         Config[0] |= 0xd0;
         break;
   }
   
   if(mSendCosMsg) {
      Config[0] |= 8;
   }

   Config[1] = 0xff;

   if(!mOut0.GetCurSel()) {
      Config[1] &= ~1;
   }

   if(!mOut1.GetCurSel()) {
      Config[1] &= ~2;
   }

   if(!mOut2.GetCurSel()) {
      Config[1] &= ~4;
   }

   if(!mOut5.GetCurSel()) {
      Config[1] &= ~0x20;
   }

   unsigned char ConfigBytes[2];

   ConfigBytes[0] = (unsigned char) Config[0];
   ConfigBytes[1] = (unsigned char) Config[1];

   CComm.SetConfig(&ConfigBytes[0]);
   CComm.SetVCOSplits((unsigned int) (mRxVcoSplitFreq * 1e6),
                      (unsigned int) (mTxVcoSplitFreq * 1e6));
}


void CConfigure::ConfigMsgRx(unsigned char *Config)
{
   UpdateData(TRUE);
   int BandSelection;

   BandSelection = Config[0] & CONFIG_BAND_MASK;
   if(BandSelection > 4) {
      BandSelection = 0;
   }
   mBand.SetCurSel(BandSelection);
   mSendCosMsg = (Config[0] & 0x8) ? TRUE : FALSE;
   
   int ControlSystemSel = 0;

   switch(Config[0] >> 4) {
      case 1:  // Doug Hall
         ControlSystemSel = 1;
         break;

      case 2:  // Palomar Telecom / Cactus / Remote base #1
         ControlSystemSel = 2;
         break;

      case 6:  // Palomar Telecom / Cactus / Remote base #2
         ControlSystemSel = 3;
         break;

      case 0xa:   // Palomar Telecom / Cactus / Remote base #3
         ControlSystemSel = 4;
         break;

      case 0xd:   // Palomar Telecom / Cactus / Remote base #4
         ControlSystemSel = 5;
         break;
   }
   mControlSys.SetCurSel(ControlSystemSel);

   mOut0.SetCurSel((Config[1] & 0x1) ? 1 : 0);
   mOut1.SetCurSel((Config[1] & 0x2) ? 1 : 0);
   mOut2.SetCurSel((Config[1] & 0x4) ? 1 : 0);
   mOut5.SetCurSel((Config[1] & 0x20) ? 1 : 0);

   EnableSplitItems();

   UpdateData(FALSE);
}

void CConfigure::VCOSplits(unsigned char *Splits)
{
   
   UpdateData(TRUE);
   unsigned int RxSplitF = 0;
   unsigned int TxSplitF = 0;
   
   for(int i = 0; i < 4; i++) {
      RxSplitF >>= 8;
      RxSplitF |= *Splits++ << 24;
   }

   for(i = 0; i < 4; i++) {
      TxSplitF >>= 8;
      TxSplitF |= *Splits++ << 24;
   }
   mRxVcoSplitFreq = (double) RxSplitF / 1e6;
   mTxVcoSplitFreq = (double) TxSplitF / 1e6;

   UpdateData(FALSE);
}


void CConfigure::OnRange1() 
{
   mRxVcoSplitFreq = 198.5;
   mTxVcoSplitFreq = 144.0;

   UpdateData(FALSE);
}

void CConfigure::OnRange2() 
{
   mRxVcoSplitFreq = 215.9;
   mTxVcoSplitFreq = 161.8;

   UpdateData(FALSE);
}

void CConfigure::EnableSplitItems() 
{
   static int TwoMeterItemIDs[] = {
      IDC_TX_VCO_SPLIT_F,
      IDC_RX_VCO_SPLIT_F,
      IDC_RANGE1,
      IDC_RANGE2,
      IDC_DEFAULTS,
      0
   };
   
   EnableItems(this,TwoMeterItemIDs,mBand.GetCurSel() == 3);
}

void CConfigure::OnSelchangeBand() 
{
   EnableSplitItems();
}

void CConfigure::OnSaveCodePlug()
{
   static char Filter[] = "Binary Files (*.bin)|*.bin|All Files (*.*)|*.*||";

   CFileDialog dlg(FALSE,NULL,gSaveFilename,OFN_CREATEPROMPT|OFN_OVERWRITEPROMPT,
                   Filter);

   if(dlg.DoModal() == IDOK) {
      gSaveFilename = dlg.GetPathName();
      if((mFp = fopen(gSaveFilename,"wb")) == NULL) {
         CString ErrMsg;
         ErrMsg.Format("Open failed: %s",Err2String(errno));
         AfxMessageBox(ErrMsg);
      }
      else {
         mMode = 1;
         m_bSaving = TRUE;

         mTransferStatus.SetWindowText("Saving Mode 1");

         CComm.SelectMode(mMode);
         CComm.RecallMode();
         CComm.GetModeData();
      }
   }
}

void CConfigure::OnRestoreCodePlug()
{
   static char Filter[] = "Binary Files (*.bin)|*.bin|All Files (*.*)|*.*||";

   CFileDialog dlg(TRUE,NULL,gSaveFilename,OFN_FILEMUSTEXIST|OFN_HIDEREADONLY,
                   Filter);

   if(dlg.DoModal() == IDOK) {
      gRestoreFilename = dlg.GetPathName();
      if((mFp = fopen(gRestoreFilename,"rb")) == NULL) {
         CString ErrMsg;
         ErrMsg.Format("Open failed: %s",Err2String(errno));
         AfxMessageBox(ErrMsg);
      }
      else {
         mMode = 1;
         m_bSaving = FALSE;
         SendModeData();
      }
   }
}

void CConfigure::ModeData(unsigned char *Data)
{
   if(mFp != NULL && m_bSaving) {
      if(fwrite(Data,16,1,mFp) != 1) {
      // Write error
         CString ErrMsg;
         mTransferStatus.SetWindowText("Save failed");
         ErrMsg.Format("Write failed: %s",Err2String(errno));
         AfxMessageBox(ErrMsg);
         fclose(mFp);
         mFp = NULL;
      }
      else {
         if(++mMode > 32) {
         // We be done !
            fclose(mFp);
            mFp = NULL;
            mTransferStatus.SetWindowText("Save complete");
         }
         else {
            CString Status;

            Status.Format("Saving Mode %d",mMode);
            mTransferStatus.SetWindowText(Status);
            CComm.SelectMode(mMode);
            CComm.RecallMode();
            CComm.GetModeData();
         }
      }
   }

   if(mFp != NULL && !m_bSaving) {
   // Verify the read back
      for(int i = 0; i < 16; i ++) {
         if(mModeData[i] != Data[i]) {
            CString ErrMsg;
            ErrMsg.Format("Verify error!\n"
                          "Mode %d, byte %d is 0x%02x,\n"
                          "it should be 0x%02x",mMode,i,Data[i],mModeData[i]);
            AfxMessageBox(ErrMsg);
            fclose(mFp);
            mFp = NULL;
            mTransferStatus.SetWindowText("Load failed");
            break;
         }
      }

      if(i == 16) {
         if(++mMode > 32) {
         // We be done !
            fclose(mFp);
            mFp = NULL;
            mTransferStatus.SetWindowText("Load complete");
         }
         else {
            SendModeData();
         }
      }
   }
}

void CConfigure::SendModeData()
{
   CString Status;

   if(mFp != NULL) {
      if(fread(mModeData,16,1,mFp) != 1) {
         CString ErrMsg;
         ErrMsg.Format("Write failed: %s",Err2String(errno));
         AfxMessageBox(ErrMsg);
         fclose(mFp);
         mFp = NULL;
      }
      else {
         Status.Format("Loading Mode %d",mMode);
         mTransferStatus.SetWindowText(Status);

      // Download new data
         CComm.SetModeData(mModeData);
      // Select the mode
         CComm.SelectMode(mMode);
      // Write it
         CComm.StoreVFO();
      // Read it
         CComm.RecallMode();
      // Send it back
         CComm.GetModeData();
      }
   }
}



/////////////////////////////////////////////////////////////////////////////
// CModeSel dialog


CModeSel::CModeSel(CWnd* pParent /*=NULL*/)
   : CDialog(CModeSel::IDD, pParent)
{
   //{{AFX_DATA_INIT(CModeSel)
   mMode = 1;
   //}}AFX_DATA_INIT
}


void CModeSel::DoDataExchange(CDataExchange* pDX)
{
   CDialog::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CModeSel)
   DDX_Text(pDX, IDC_MODE, mMode);
   DDV_MinMaxUInt(pDX, mMode, 1, 32);
   //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CModeSel, CDialog)
   //{{AFX_MSG_MAP(CModeSel)
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CModeSel message handlers




BOOL CAbout::OnInitDialog() 
{
   CPropertyPage::OnInitDialog();
   
   CString TimeStamp;

   TimeStamp.Format("Compiled:  %s, %s",DateCompiled,TimeCompiled);
   mCompiled.SetWindowText(TimeStamp);
   
   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}


