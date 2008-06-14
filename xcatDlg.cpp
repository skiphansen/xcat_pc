// XcatDialog.cpp : implementation file
//
// $Log: xcatDlg.cpp,v $
// Revision 1.15  2008/06/14 14:35:11  Skip
// 1. Added firmware update support to the About page.
// 2. Modified OnRxMsg to set gRawVerString with version number received
//    from Xcat (Same as gVerString less "Firmware version: ".
// 3. Modified OnRxMsg to call FwVersionUpated when firmware version is
//    received from the Xcat and the downloader is active.
// 4. Modified OnRxMsg to call the downloader's CommSetupAck function when
//    an ack is received for a comm setup message and the download is active.
// 5. Added routine to pass ID_DOWNLOAD_CHAR events to the downloader
//    classes OnDownloadChar when the downloader is active.
// 6. Modified OnTimer to call the downloader classes OnTimer function when
//    the downloader is active.
// 7. Modified SetDPL to handle the carrier squelch correctly.
//
// Revision 1.14  2008/06/01 14:01:54  Skip
// 1. Added code to display the loader's firmware version if available.
// 2. Added code to round transmitter offset to OnManualSet.
// 3. Modified CAbout::SetActive to always request firmware version info.
// 4. Enabled Palomar control system to be selected for firmware > 0.30.
// 5. Added code to configure I/O 7 as a PTT input in Palomar mode.
//
// Revision 1.13  2008/05/13 15:43:51  Skip
// Added support for the new debug variables in 0.29 (srxlen (Palomar
// code debug var), wd_count, bo_count and unk_count.
//
// Revision 1.12  2008/02/03 15:53:36  Skip
// Corrected display of out of band receive frequencies.
//
// Revision 1.11  2008/02/02 17:58:22  Skip
// Added support for volume pot (not tested).
//
// Revision 1.10  2007/07/15 14:25:53  Skip
// 1. Added checks for firmware features against Xcat firmware version to
//    CConfigure::OnSetConfig.
// 2. Modified CXcatDlg::OnRxMsg to set new globals gFirmwareVer and
//    gFirmwareVerString when a firmware version response is received.
// 3. Added squelch pot support: slider in VFO view, configuration support on
//    configuration page, initial slider position update from 3'rd byte of
//    configuration data.
// 4. Added firmware version requests to Debug and configuration pages.
// 5. Modified SyncDebugData to display data properly for all versions of
//    firmware.  Starting with V 0.27 there are a maximum of 7 bytes of sync
//    data, previously there were 5.  Sync data is now displayed in order
//    received rather than reverse byte order.
//
// Revision 1.9  2007/01/26 00:24:00  Skip
// 1. Moved global ctable into ManualPage::ModeData(), ctable from syntorxdecode
//    and syntorxgen aren't the same animal!
// 2. Corrected TxOffset calculation in ManualPage::ModeData for response 0x85,
//    the byte order was backwards.  (The result wasn't used any where, it
//    was just for debug).
// 3. Added ctable from syntorxgen to SetCodePlugRxFrequency.
// 4. Added code to SetCodePlugRxFrequency to set ModeData[8] based on the
//    selected reference frequency.
// 5. Corrected clearing of code plug Rx bits in SetCodePlugRxFrequency.
// 6. Corrected rounding errors in calculation of TxOffsetFreq in
//    ManualPage::ModeData.
//
// Revision 1.8  2007/01/02 17:28:56  Skip
// 1. Added support for DPL (DCS) encoding and decoding to the VFO page.
// 2. Added a DPL scan option to Scan page.
// 3. Added support for UHF range 1 radios (420).  Updated Xcat Firmware required.
// 4. Added receive only support to the VFO page.  Receive only is now offered
//    as a choice in the transmitter offset box.
// 5. Added support for the transmitter timeout timer to the VFO page.
// 6. Added code to save retrieved code plug data to c:\xcat.bin when the code
//    plug data is read via the Debug Page.
// 7. Enabled VCO split frequencies to be specified for UHF range radios. Updated
//    Xcat Firmware is required.
// 8. Corrected the default value of the receive VCO split frequency for VHF range
//    2 radios (was 215.9, should be 203.9).
//
// Revision 1.7  2005/01/08 19:42:59  Skip
// Minor comment edits.
//
// Revision 1.6  2005/01/08 19:30:06  Skip
// Version 0.15 changes:
// 1. Replaced CCommSetup with new dialog that allows XCat's address and
//    baudrate to be configured.
// 2. Added code to read Xcat's configuration and mode data at program
//    startup so VFO tab will have current data.
// 3. Added code to ManualPage to set dialog values based on code plug data
//    when it is available.
// 4. Added support for inverted mode select switches.
// 5. Enabled recall model button on manual page. Previously it was only 
//    enabled in debug mode.
// 6. Added display of invalid frame counter to sync data debug output.
// 7. Replaced recall mode dialog with new class that supports labeling modes.
// 8. Added ability to save and reload mode labels.
//
// Revision 1.5  2004/12/31 00:49:18  Skip
// Version 0.14 changes:
// 1. Disabled signal reports in debug screen.
// 2. Added display of successfully rx, tx frequency sets to sync data
//    debug screen.
//
// Revision 1.4  2004/12/27 05:55:24  Skip
// Version 0.13:
// 1. Fixed crash in Debug mode caused by calling ScanPage.ModeData()
//    being called when not on the scan page.
// 2. Added support for sync rx debug data (requires firmware update as well).
// 3. Added request buttons to Debug mode for code plug data and sync rx
//    debug data.
// 4. Corrected bug in configuration of remote base #4 in Palomar mode.
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

#include "stdafx.h"
#include "xcat.h"
#include "XcatDlg.h"
#include "comm.h"

#define CONFIG_BAND_MASK 7
#define CONFIG_10M      0
#define CONFIG_6M       1
#define CONFIG_10_6M    2
#define CONFIG_2M       3
#define CONFIG_440      4
#define CONFIG_420      5

#define CONFIG_CTRL_MASK 	0x30
#define CONFIG_GENERIC  	0x10
#define CONFIG_CACTUS   	0x20

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


// bit swap & invert lookup table
unsigned int dpltable[8] = { 7, 3, 5, 1, 6, 2, 4, 0 };

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
   AddPage(&CommSetup);
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
   ON_MESSAGE(ID_DOWNLOAD_CHAR,OnDownloadChar)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CXcatDlg message handlers

BOOL CXcatDlg::OnInitDialog() 
{
   BOOL bResult = CPropertySheet::OnInitDialog();
   
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

   // Start a 500 millisecond timer
   if(!SetTimer(1,500,NULL)) {
      AfxMessageBox("Fatal error: Unable to create timer.");
      PostMessage(WM_QUIT,0,0);
   }

   CComm.GetConfig();
	CComm.GetModeData();

   return bResult;
}

LRESULT CXcatDlg::OnRxMsg(WPARAM wParam,LPARAM lParam)
{
   AppMsg *p= (AppMsg *) lParam;
   CI_VMsg *pMsg = (CI_VMsg *) &p->Hdr;

   if(pMsg->Hdr.Cmd == 0xaa) {
   // Private Xcat command
      switch(pMsg->Data[0]) {
         case 0x80:  // response to get vfo raw data
				TRACE("Got Mode data\n");
            memcpy(gModeData,&pMsg->Data[1],sizeof(gModeData));
				g_bHaveModeData = TRUE;

            if(g_bHaveConfig && GetActivePage() == &ManualPage) {
					ManualPage.ModeData();
				}
            else if(GetActivePage() == &ScanPage) {
					ScanPage.ModeData();
				}
            else if(GetActivePage() == &DebugMsgs) {
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
               CString Temp;
               if(BandScan.m_bScanActive && pMsg->Data[1] == 0) {
                  Temp.Format("%3.4f",BandScan.mCurrentFreq);
               }
               else {
						int Mode = INVERT_MODE(pMsg->Data[1]);
						Temp.Format("%d: %s",Mode+1,gModeName[Mode]);
               }
               SetTitle(Temp);
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
				else if(GetActivePage() == &DebugMsgs) {
#if 0
               DebugMsgs.SignalReport(pMsg->Data[1],pMsg->Data[2]);
#endif
				}
            break;

         case 0x82:  // get firmware version number response 
         {  pMsg->Data[p->DataLen - sizeof(CI_V_Hdr)-1] = 0;
				gRawVerString = (char *) &pMsg->Data[1];
			// format "V x.xx"
				gFirmwareVer = atoi((char *) &pMsg->Data[5]);
				gFirmwareVer += 100 * atoi((char *) &pMsg->Data[3]);
				g_bHaveFWVer = TRUE;
				int SlashIndex;
				if((SlashIndex = gRawVerString.Find('/')) != -1) {
					gLoaderVerString = "Loader Ver: ";
					gLoaderVerString += gRawVerString.Mid(SlashIndex+1);
					gRawVerString = gRawVerString.Left(SlashIndex);
				}
            gFirmwareVerString.Format("Firmware version: %s",gRawVerString);

            if(GetActivePage() == &CAbout) {
					if(gLoaderVerString.IsEmpty()) {
						CAbout.GetDlgItem(IDC_LOADER_VER)->ShowWindow(SW_HIDE);
					}
					else {
						CAbout.GetDlgItem(IDC_LOADER_VER)->SetWindowText(gLoaderVerString);
						CAbout.GetDlgItem(IDC_LOADER_VER)->ShowWindow(SW_SHOWNORMAL);
					}
               CAbout.GetDlgItem(IDC_FIRMWARE_VER)->SetWindowText(gFirmwareVerString);
					if(CAbout.pDownloader != NULL) {
						CAbout.pDownloader->FwVersionUpdated();
					}
            }
            break;
         }

         case 0x83:  // Get config
         {
            memcpy(gConfig,&pMsg->Data[1],CONFIG_LEN);
				g_bHaveConfig = TRUE;
				ManualPage.UpdatePots();

            if(GetActivePage() == &Configure) {
					Configure.ConfigMsgRx(&pMsg->Data[1]);
				}
            break;
         }

         case 0x85:  // get Tx offset
         {
            int TxOffset = *((int *) &pMsg->Data[1]);
            break;
         }

         case 0x87:  // get VCO splits
         {
            Configure.VCOSplits(&pMsg->Data[1]);
            break;
         }

         case 0x89:  // Sync data debug info
         {
            if(GetActivePage() == &DebugMsgs) {
               DebugMsgs.SyncDebugData(&pMsg->Data[1]);
            }
            break;
         }

			case 0x8a:	// Ack for communications parameter set message
				if(GetActivePage() == &CAbout && CAbout.pDownloader != NULL) {
					CAbout.pDownloader->CommSetupAck(wParam == 0);
				}
				else if(GetActivePage() == &CommSetup) {
					gXcatAdr = CommSetup.mNewXcatAdr;
					if(gBaudrate != CommSetup.mNewBaudrate || 
						gComPort != CommSetup.mNewComPort) 
					{
						gBaudrate = CommSetup.mNewBaudrate;
						gComPort = CommSetup.mNewComPort;
						if(!CComm.Init(gComPort,gBaudrate)) {
							CString ErrMsg;
							ErrMsg.Format("Unable to open COM%d,\n"
											  "please check that no other\n"
											  "programs are using COM%d.",gComPort,
											  gComPort);
							AfxMessageBox(ErrMsg);
						}
					}
				}
				break;
      }
   }

   delete p;
   return 0;
}

LRESULT CXcatDlg::OnDownloadChar(WPARAM /* wParam*/, LPARAM lParam)
{
	if(GetActivePage() == &CAbout  && CAbout.pDownloader != NULL) {
		CAbout.pDownloader->OnDownloadChar((char) lParam);
	}
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
   
	if(GetActivePage() == &CAbout && CAbout.pDownloader != NULL) {
		CAbout.pDownloader->OnTimer();
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
	mbRxDPL = mbInvRxDPL = mbTxDPL = mbInvTxDPL = FALSE;

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
	DDX_Control(pDX, IDC_VOLUME, mVolumePot);
	DDX_Control(pDX, IDC_SQUELCH, mSquelchPot);
	DDX_Control(pDX, IDC_TX_TIMEOUT, mTxTimeout);
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
	ON_BN_CLICKED(IDC_RX_PL_ENABLE, OnRxPlEnable)
	ON_BN_CLICKED(IDC_TX_DPL_ENABLE, OnTxDplEnable)
	ON_BN_CLICKED(IDC_RX_DPL_ENABLE, OnRxDplEnable)
	ON_BN_CLICKED(IDC_TX_PL_ENABLE, OnTxPlEnable)
	ON_BN_CLICKED(IDC_TX_DPL_ENABLE_INV, OnTxDplEnableInv)
	ON_BN_CLICKED(IDC_RX_DPL_ENABLE_INV, OnRxDplEnableInv)
	ON_CBN_SELCHANGE(IDC_RX_PL, OnSelchangeRxPl)
	ON_CBN_SELCHANGE(IDC_TX_PL, OnSelchangeTxPl)
	ON_CBN_SELCHANGE(IDC_TX_OFFSET, OnSelchangeTxOffset)
	ON_WM_VSCROLL()
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

typedef struct
{
	unsigned int dpl;
	unsigned int funnybits;
} Dplfunnybits;

Dplfunnybits dplfunnybits[] =
{
   { 0023, 1 }, { 0025, 0 }, { 0026, 2 /* 6 */}, { 0031, 0 }, { 0032, 2 },
	{ 0036, 4 }, { 0043, 4 }, { 0047, 2 }, { 0051, 5 }, { 0054, 6 }, 
	{ 0065, 3 }, { 0071, 3 /* 7*/ }, { 0072, 1 }, { 0073, 4 },
	{ 0074, 0 /* 4 */}, { 0114, 4 }, { 0115, 1 }, { 0116, 3 }, 
	{ 0125, 1 }, { 0131, 1 }, { 0132, 3 }, { 0134, 2 }, { 0143, 5 },
	{ 0152, 6 /* 7 */ }, { 0155, 2 }, { 0156, 0 }, { 0162, 6 /* 7 */},
	{ 0165, 2 }, { 0172, 0 }, { 0174, 1 }, { 0205, 3 },
	{ 0223, 4 }, { 0225, 5 }, { 0226, 7 }, { 0243, 1 }, { 0244, 5 },
	{ 0245, 0 /* 4 */ }, { 0251, 4 }, 
	{ 0261, 0 }, { 0263, 7 }, { 0265, 6 }, { 0271, 6 },
	{ 0306, 0 }, { 0311, 2 }, { 0315, 4 }, 
	{ 0331, 4 }, { 0343, 0 }, { 0346, 3 }, { 0351, 1 },
	{ 0364, 2 }, { 0365, 7 }, { 0371, 7 }, { 0411, 4 },
	{ 0412, 6 }, { 0413, 3 }, { 0423, 7 }, { 0431, 2 }, { 0432, 0 },
	{ 0445, 7 }, { 0464, 4 }, { 0465, 1 }, { 0466, 3 }, { 0503, 4 },
	{ 0506, 7 }, { 0516, 1 }, { 0532, 1 },
	{ 0546, 4 }, { 0565, 0 }, { 0606, 3 }, { 0612, 3 }, { 0624, 2 },
	{ 0627, 0 }, { 0631, 7 }, { 0632, 5 }, { 0654, 1 }, { 0662, 0 },
	{ 0664, 1 }, { 0703, 1 }, { 0712, 2 }, { 0723, 7 }, { 0731, 6 },
	{ 0732, 4 }, { 0743, 2 }, { 0754, 0 }, { 0000, 0 }
};

void FillPLBox(CComboBox* pCB,char *NoToneString,int bDPL)
{
   pCB->ResetContent();

   pCB->InsertString(-1,NoToneString);
	if(bDPL) {
		CString DPLCode;
		for(int i = 0; dplfunnybits[i].dpl != 0; i++) {
			DPLCode.Format("%03o",dplfunnybits[i].dpl);
			pCB->InsertString(-1,DPLCode);
		}
	}
	else {
		for(int i = 0; PLTones[i] != NULL; i++) {
			pCB->InsertString(-1,PLTones[i]);
		}
	}
}

int FindPl(double pl)
{
	float Freq;

	for(int i = 0; PLTones[i] != NULL; i++) {
		sscanf(PLTones[i],"%f",&Freq);
		if(fabs(Freq - pl) < .05) {
		// Close enough for government work
			return i + 1;
		}
	}

	return 0;
}

int FindDCS(unsigned int code)
{
	for(int i = 0; dplfunnybits[i].dpl != 0; i++) {
		if(dplfunnybits[i].dpl == code) {
			return i + 1;
		}
	}

	return 0;
}

int GetRxVbits(double vcofreq)
{
	int ConfigBand = gConfig[0] & CONFIG_BAND_MASK;
	int Ret;

	switch(ConfigBand)
	{
		case CONFIG_10M:
		case CONFIG_6M:
		case CONFIG_10_6M:
			if(vcofreq < 113.8) {
				Ret = 0;
			}
			else if(vcofreq < 122.8) {
				Ret = 2;
			}
			else if(vcofreq < 132.6) {
				Ret = 1;
			}
			else {
				Ret = 3;
			}
			break;

		case CONFIG_2M:
			if(vcofreq < gVCORxSplitVHF) {
				Ret = 0;
			}
			else {
				Ret = 2;
			}
			break;

		case CONFIG_420:
			if(vcofreq < gVCORxSplit420) {
				Ret = 2;
			}
			else {
				Ret = 0;
			}
			break;

		case CONFIG_440:
			if(vcofreq < gVCORxSplit440) {
				Ret = 2;
			}
			else {
				Ret = 0;
			}
			break;
	}
	return Ret;
}

void SetCodePlugRxFrequency(double RxFreq,unsigned char *ModeData)
{
	unsigned int rxif;
	unsigned int rxvcofreq;
	unsigned int rxa, rxb, rxc, rxn, rxn1, rxn2;
	unsigned int refreq = 6250;
	unsigned int rxcix;
	unsigned char accum = ModeData[0xb] & 0xf0;
	bool bLowBand = FALSE;
	unsigned int rxfreq = (unsigned int) floor((RxFreq * 1000000.0L) + 0.5L);
	int ConfigBand = gConfig[0] & CONFIG_BAND_MASK;
	static unsigned char ctable[3] = { 2, 1, 3 };

/* Default: "VHF RSS prefers the 5 KHz reference frequency. All other
	radios prefer the 6.25 kHz frequency." -- From Pakman's code plug
	documentation at http://home.xnet.com/~pakman/syntor/syntorx.htm */

	switch(ConfigBand)
	{
		case CONFIG_10M:
		case CONFIG_6M:
		case CONFIG_10_6M:
			bLowBand = TRUE;
			rxif = 75700000;
			break;

		case CONFIG_2M:
			refreq = 5000;
			rxif = 53900000;
			break;

		case CONFIG_440:
			rxif = -53900000;
			break;

		case CONFIG_420:
			rxif = 53900000;
			break;
	}

	if(rxfreq % refreq != 0) { 
	// Try the other reference frequency
		refreq = refreq == 5000 ? 6250 : 5000;
		if(rxfreq % refreq != 0) { 
		// hmmm...
			CString ErrMsg;
			ErrMsg.Format("Unable to find reference frequency for a receive "
							  "frequency of %u\n",rxfreq);
			AfxMessageBox(ErrMsg);
			return;
		}
	}

	ModeData[8] &= 0xfc;
	switch(refreq)
	{
		case 5000:
			ModeData[8] |= 3;
			break;

		case 6250:
			break;

		case 4166:
			ModeData[8] |= 2;
			break;

		default:
			ModeData[8] |= 1;
			break;
	}

	rxvcofreq = rxfreq + rxif;

	rxn = rxvcofreq / refreq;
	rxc = rxn % 3;
	rxcix = rxc;

	if(bLowBand) {
		rxn1 = rxn;
	}
	else {
		rxn1 = rxn / 3;
		if (rxc == 0)
			rxn1--;
	}
	rxa = rxn1 % 63;
	if (rxa == 0) {
		rxa = 63;
		rxn1 -= 63;
	}
	rxn2 = rxn1 / 63;
	rxb = rxn2 - rxa;
	
	for(int i = 0xb; i < 0x10; i++) {
	// Clear Rx bits
		ModeData[i] &= 0xf0;
	}
	
	accum |= GetRxVbits((double) rxvcofreq / 1000000.0) << 2;
	if(bLowBand) {
		accum |= 3;
	}
	else {
		accum |= ctable[rxcix];
	}

	ModeData[0xb] = accum;
	ModeData[0xc] |= ((rxb & 0x03c0) >> 6);
	ModeData[0xd] |= ((rxb & 0x003c) >> 2);
	ModeData[0xe] |= ((rxb & 0x0003) << 2);
	ModeData[0xe] |= ((rxa & 0x0030) >> 4);
	ModeData[0xf] |= (rxa & 0x000f);
}

void SetDPL(int Index,unsigned char *Data)
{
	if(Index == 0) {
	// Carrier
		Data[0] = 0xff;
		Data[1] = 0xdf;
	}
	else {
		unsigned int DplCode = dplfunnybits[Index-1].dpl;

		Data[0] = (dpltable[(DplCode & 0007)]) << 4;
		Data[0] |= (dpltable[(DplCode & 0070) >> 3]) << 1;
		Data[0] |= (dpltable[(DplCode & 0700) >> 6] & 0004) >> 2;

		Data[1] = 0xe0;
		Data[1] |= (dpltable[(DplCode & 0700) >> 6] & 0003) << 3;
		Data[1] |= dplfunnybits[Index-1].funnybits;
	}
}


BOOL ManualPage::OnInitDialog() 
{
   CPropertyPage::OnInitDialog();
   
   FillPLBox(&mTxPL,"Disabled",FALSE);
   mTxPL.SetCurSel(gTxCTSS);
   FillPLBox(&mRxPL,"Carrier",FALSE);
   mRxPL.SetCurSel(gRxCTSS);
   mTxOffset.SetCurSel(gTxOffset);
   mRxFrequency = gRxFrequency;
   mTxOffsetFreq = gTxOffsetFreq;

   mSquelchPot.SetRange(0,0xff);
   UpdateData(FALSE);
   
   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL ManualPage::OnSetActive() 
{
   SetButtonMode(IDOK,ID_MANUAL_SET,"Set",TRUE,FALSE);
   SetButtonMode(IDCANCEL,ID_SAVE_MODE,"Store",TRUE,FALSE);
   SetButtonMode(ID_APPLY_NOW,ID_RECALL_MODE,"Recall",TRUE,FALSE);
   SetButtonMode(IDHELP,0,NULL,FALSE,TRUE);
   
   mForcedSet = TRUE;

	if(g_bHaveConfig) {
		UpdatePots();
		if(g_bHaveModeData) {
			ModeData();
		}
	}

   return CPropertyPage::OnSetActive();
}

void ManualPage::OnManualSet()
{
   UpdateData(TRUE);
   gRxFrequency = mRxFrequency;
   gTxOffsetFreq = mTxOffsetFreq;

	int TxOffset = mTxOffset.GetCurSel();

	unsigned char ModeData[16];
	memcpy(ModeData,gModeData,sizeof(ModeData));

// NB: Set the parameters that are set using via raw code plug data first
	if(mbTxDPL) {
		mLastEncodePL = -1.0;	// clobber last encode PL tone
		gTxDCS = mTxPL.GetCurSel();
		SetDPL(gTxDCS,&ModeData[4]);
		if(gTxDCS != 0 && mbInvTxDPL) {
			ModeData[4] = 0x80;
		}
	}

	if(mbRxDPL) {
		mLastDecodePL = -1.0;	// clobber last decode PL tone
		gRxDCS = mRxPL.GetCurSel();
		SetDPL(gRxDCS,&ModeData[6]);
		if(mbInvRxDPL) {
			ModeData[6] |= 0x80;
		}
	}

	gTxTimeout = mTxTimeout.GetCurSel();
	ModeData[8] &= ~0xf8;
	ModeData[8] |= (0x1f - gTxTimeout) << 3;

	if(TxOffset == 3) {
	// Receive only mode.  We must set the Rx frequency via raw data
	// because the CI-V set frequency command will also set the Tx frequency
		SetCodePlugRxFrequency(mRxFrequency,ModeData);

		ModeData[0xb] = (ModeData[0xb] & 0xf) | 0xc0;
		ModeData[0xc] |= 0xf0;
		ModeData[0xd] |= 0xf0;
		ModeData[0xe] |= 0xf0;
		ModeData[0xf] |= 0xf0;
	}

	if(mForcedSet || memcmp(ModeData,gModeData,sizeof(ModeData)) != 0) {
	// Mode data has changed, send it
		memcpy(gModeData,ModeData,sizeof(gModeData));
		CComm.SetModeData(&gModeData[0]);
	}

// NB: Set the parameters that are set using standandard CI-V commands

   float PlTone = 0.0;
	if(!mbTxDPL) {
		gTxCTSS = mTxPL.GetCurSel();
		if(gTxCTSS != 0) {
			sscanf(PLTones[gTxCTSS-1],"%f",&PlTone);
		}
		if(mForcedSet || PlTone != mLastEncodePL) {
			mLastEncodePL = PlTone;
			CComm.SetCTSSFreq(PlTone,FALSE);
		}
	}

   PlTone = 0.0;
	if(!mbRxDPL) {
		gRxCTSS = mRxPL.GetCurSel();
		if(gRxCTSS != 0) {
			sscanf(PLTones[gRxCTSS-1],"%f",&PlTone);
		}
		if(mForcedSet || PlTone != mLastDecodePL) {
			mLastDecodePL = PlTone;
			CComm.SetCTSSFreq(PlTone,TRUE);
		}
	}

   if(mForcedSet || gTxOffsetFreq != mLastTxOffsetFreq) {
      mLastTxOffsetFreq = gTxOffsetFreq;
      CComm.SetTxOffset((int)((gTxOffsetFreq * 1000000.0) + .5));
   }

   if(mForcedSet || gTxOffset != TxOffset) {
		gTxOffset = TxOffset;
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

	if(gTxOffset != 3) {
		CComm.SetFreq(mRxFrequency);
	}

// Update the mode data for next time.  We set DPL codes via mode data,
// but we set PL codes using the standard CI-V command (for historical
// and testing purposes) so we need to re-read the mode data to keep
// it in sync.

	CComm.GetModeData();

   mForcedSet = FALSE;
}

void ManualPage::OnSaveMode()
{
   CModeSel dlg;

   OnManualSet();
   if(dlg.DoModal() == IDOK) {
      CComm.SelectMode(gLastModeSel);
      CComm.StoreVFO();
   }
}

void ManualPage::OnRecallMode()
{
   CModeSel dlg;

   if(dlg.DoModal() == IDOK) {
      mForcedSet = TRUE; // send everything next time !
      CComm.SelectMode(gLastModeSel);
      CComm.RecallMode();
		CComm.GetModeData();
   }
}

// Decode mode data and set controls
void ManualPage::ModeData()
{
	unsigned int txa, txb, txc, txv;
	unsigned int rxa, rxb, rxc, rxv;
	unsigned int refreq;
	unsigned int bit, bits;
	int fvco;
	int frx = 0;
	int ftx = 0;
	double pl;
   double   TxOffsetFreq;
   CButton *pRB;
	int ButtonID;
	static unsigned int ctable[4] = { 0, 1, 3, 2 };

	static int TxButtons[] = {
		IDC_TX_PL_ENABLE,
		IDC_TX_DPL_ENABLE,
		IDC_TX_DPL_ENABLE_INV,
		0
	};

	static int RxButtons[] = {
		IDC_RX_PL_ENABLE,
		IDC_RX_DPL_ENABLE,
		IDC_RX_DPL_ENABLE_INV,
		0
	};


// Send everything on the next set
	mForcedSet = TRUE;
	switch(gModeData[8] & 0x03)
	{
		case 0:
			refreq = 6250;
			break;
		case 1:
			refreq = 0;
			break;
		case 2:
			refreq = 4166;
			break;
		case 3:
			refreq = 5000;
			break;
	}

// Calculate current transmit and receive frequencies

	txv = (gModeData[11] & 0x00c0) >> 6;
	txc = (gModeData[11] & 0x0030) >> 4;
	rxv = (gModeData[11] & 0x000c) >> 2;
	rxc = (gModeData[11] & 0x0003);
	txb = (gModeData[12] & 0x00f0) << 2;
	rxb = (gModeData[12] & 0x000f) << 6;
	txb |= (gModeData[13] & 0x00f0) >> 2;
	rxb |= (gModeData[13] & 0x000f) << 2;
	txb |= (gModeData[14] & 0x00c0) >> 6;
	rxb |= (gModeData[14] & 0x000c) >> 2;
	txa = gModeData[14] & 0x0030;
	rxa = (gModeData[14] & 0x0003) << 4;
	txa |= (gModeData[15] & 0x00f0) >> 4;
	rxa |= gModeData[15] & 0x000f;

	switch(gConfig[0] & CONFIG_BAND_MASK)
	{
		case CONFIG_10M:
		case CONFIG_6M:
		case CONFIG_10_6M:
			fvco = ((64 * rxa) + (63 * rxb)) * refreq;
			frx = fvco - 75700000;
			fvco = ((64 * txa) + (63 * txb)) * refreq;
			ftx = 172800000 - fvco;
			break;

		case CONFIG_2M:
			fvco = ((((64 * rxa) + (63 * rxb)) * 3) + ctable[rxc]) * refreq;
			frx = fvco - 53900000;
			fvco = ((((64 * txa) + (63 * txb)) * 3) + ctable[txc]) * refreq;
			ftx = fvco;
			break;

		case CONFIG_440:
			fvco = ((((64 * rxa) + (63 * rxb)) * 3) + ctable[rxc]) * refreq;
			frx = fvco + 53900000;
			fvco = ((((64 * txa) + (63 * txb)) * 3) + ctable[txc]) * refreq;
			ftx = fvco;
			break;

		case CONFIG_420:
			fvco = ((((64 * rxa) + (63 * rxb)) * 3) + ctable[rxc]) * refreq;
			frx = fvco - 53900000;
			fvco = ((((64 * txa) + (63 * txb)) * 3) + ctable[txc]) * refreq;
			ftx = fvco;
			break;
	}

	if((gModeData[0xb] & 0xf0) == 0xc0 &&
		(gModeData[0xc] & 0xf0) == 0xf0 &&
		(gModeData[0xd] & 0xf0) == 0xf0 &&
		(gModeData[0xe] & 0xf0) == 0xf0 &&
		(gModeData[0xf] & 0xf0) == 0xf0)
	{	// special tx code for receive only
		ftx = 0;
		mTxOffset.SetCurSel(3);
	}

	mRxFrequency = (double) frx / 1000000.0;
	gRxFrequency = mRxFrequency;

	if(ftx != 0 && frx != 0) {
		TxOffsetFreq = (ftx - frx) / 1000000.0;

		if(TxOffsetFreq < 0) {
		// Negative offset
			gTxOffsetFreq = mTxOffsetFreq = -TxOffsetFreq; 
			mTxOffset.SetCurSel(2);
		}
		else if(TxOffsetFreq > 0) {
		// Positive offset
			gTxOffsetFreq = mTxOffsetFreq = TxOffsetFreq; 
			mTxOffset.SetCurSel(0);
		}
		else {
		// Simplex
			mTxOffset.SetCurSel(1);
		}
	}

// Decode Tx Pl tone
	switch((gModeData[5] & 0x60) >> 5)
	{
		case 3:	/* DPL */
			mbTxDPL = TRUE;
			if(gModeData[4] & 0x80) {
				ButtonID = IDC_TX_DPL_ENABLE_INV;
			}
			else {
				ButtonID = IDC_TX_DPL_ENABLE;
			}
			bits = dpltable[(gModeData[4] & 0x70) >> 4]; 			/* C */
			bits |= dpltable[((gModeData[4] & 0x0e) >> 1)] << 3; 	/* B */
			bit = (gModeData[4] & 0x01) << 2; 	/* A0 */
			bit |= (gModeData[5] & 0x18) >> 3; 	/* A1-2 */
			bits |= dpltable[bit] << 6;			/* A */
			gTxDCS = FindDCS(bits);
			TRACE2("Rx DPL code: %09o, gTxDCS: %d\n",bits,gTxDCS);
			break;

		case 2:	/* NONE */
			ButtonID = IDC_TX_PL_ENABLE;
			gTxCTSS = 0;
			break;

		case 1:	/* PL */
			/* yes, it's supposed to fall through */
		case 0:	/* PL */
			ButtonID = IDC_TX_PL_ENABLE;
			mbTxDPL = FALSE;
			bits = gModeData[4];
			bits |= (gModeData[5] & 0x3f) << 8;
			pl = (float)(~bits & 0x3fff) / 18.0616;
			gTxCTSS = FindPl(pl);
			break;
	}
	
	for(int i = 0; TxButtons[i] != 0; i++) {
		pRB = (CButton *) GetDlgItem(TxButtons[i]);
		pRB->SetCheck(TxButtons[i] == ButtonID);
	}
	
// Decode Rx Pl tone

	switch((gModeData[7] & 0x60) >> 5)
	{
		case 3:	/* DPL */
			mbRxDPL = TRUE;
			if(gModeData[6] & 0x80) {
				ButtonID = IDC_RX_DPL_ENABLE_INV;
			}
			else {
				ButtonID = IDC_RX_DPL_ENABLE;
			}

			bits = dpltable[(gModeData[6] & 0x70) >> 4]; 			/* C */
			bits |= dpltable[((gModeData[6] & 0x0e) >> 1)] << 3; 	/* B */
			bit = (gModeData[6] & 0x01) << 2; 	/* A0 */
			bit |= (gModeData[7] & 0x18) >> 3; 	/* A1-2 */
			bits |= dpltable[bit] << 6;			/* A */
			gRxDCS = FindDCS(bits);
			TRACE2("Tx DPL code: %09o, gRxDCS: %d\n",bits,gRxDCS);
			break;

		case 2:	/* NONE */
			ButtonID = IDC_RX_PL_ENABLE;
			mbRxDPL = FALSE;
			gRxCTSS = 0;
			break;

		case 1:	/* PL */
			/* yes, it's supposed to fall through */
		case 0:	/* PL */
			mbRxDPL = FALSE;
			ButtonID = IDC_RX_PL_ENABLE;
			bits = gModeData[6];
			bits |= (gModeData[7] & 0x3f) << 8;
			pl = (float)(~bits & 0x3fff) / 61.17;
			gRxCTSS = FindPl(pl);
			break;
	}

// Set Tx timeout

	gTxTimeout = 0x1f - ((gModeData[8] >> 3) & 0x1f);
	mTxTimeout.SetCurSel(gTxTimeout);

	for(i = 0; RxButtons[i] != 0; i++) {
		pRB = (CButton *) GetDlgItem(RxButtons[i]);
		pRB->SetCheck(RxButtons[i] == ButtonID);
	}

	OnSelchangeTxOffset();
	UpdateRxCB();
	UpdateTxCB();
	UpdateData(FALSE);
}

void ManualPage::OnSelchangeRxPl() 
{
	if(mbRxDPL) {
		gRxDCS = mRxPL.GetCurSel();
	}
	else {
		gRxCTSS = mRxPL.GetCurSel();
	}
}

void ManualPage::OnSelchangeTxPl() 
{
	if(mbTxDPL) {
		gTxDCS = mTxPL.GetCurSel();
	}
	else {
		gTxCTSS = mTxPL.GetCurSel();
	}
}

void ManualPage::UpdateRxCB() 
{
	if(mbRxCBMode != mbRxDPL) {
		mbRxCBMode = mbRxDPL;
		FillPLBox(&mRxPL,"Carrier",mbRxCBMode);
	}
	mRxPL.SetCurSel(mbRxDPL ? gRxDCS : gRxCTSS);
}

void ManualPage::UpdateTxCB() 
{
	if(mbTxCBMode != mbTxDPL) {
		mbTxCBMode = mbTxDPL;
		FillPLBox(&mTxPL,"disabled",mbTxCBMode);
	}
	mTxPL.SetCurSel(mbTxDPL ? gTxDCS : gTxCTSS);
}

void ManualPage::OnRxPlEnable() 
{
	mbRxDPL = FALSE;
	UpdateRxCB();
}

void ManualPage::OnRxDplEnable() 
{
	mbRxDPL = TRUE;
	mbInvRxDPL = FALSE;
	UpdateRxCB();
}

void ManualPage::OnRxDplEnableInv() 
{
	mbRxDPL = TRUE;
	mbInvRxDPL = TRUE;
	UpdateRxCB();
}

void ManualPage::OnTxPlEnable() 
{
	mbTxDPL = FALSE;
	UpdateTxCB();
}

void ManualPage::OnTxDplEnable() 
{
	mbTxDPL = TRUE;
	mbInvTxDPL = FALSE;
	UpdateTxCB();
}

void ManualPage::OnTxDplEnableInv() 
{
	mbTxDPL = TRUE;
	mbInvTxDPL = TRUE;
	UpdateTxCB();
}

int TxResourseIDs[] = {
   IDC_TX_PL,
   IDC_TX_PL_ENABLE,
   IDC_TX_DPL_ENABLE,
   IDC_TX_DPL_ENABLE_INV,
   IDC_TX_OFFSET_FREQ,
   IDC_TX_TIMEOUT,
   IDC_TX_OFFSET_LABEL,
   IDC_TX_TIMEOUT_LABEL,
   0
};

void ManualPage::OnSelchangeTxOffset() 
{
	int TxOffset = mTxOffset.GetCurSel();
	
   EnableItems(this,TxResourseIDs,TxOffset != 3);
}

void ManualPage::UpdatePots()
{
   CWnd *pCWnd = GetDlgItem(IDC_SQUELCH_LABEL);

	int nCmdShow;
	nCmdShow = (gConfig[1] & CONFIG_SQU_POT_MASK) == 0 ? 
						SW_SHOWNORMAL : SW_HIDE;

	if((gConfig[1] & CONFIG_SQU_POT_MASK) == 0) {
		nCmdShow = SW_SHOWNORMAL;
		mSquelchPot.SetPos(gConfig[2]);
	}
	else {
		nCmdShow = SW_HIDE;
	}
	mSquelchPot.ShowWindow(nCmdShow);
	pCWnd->ShowWindow(nCmdShow);

	if(!gEnableVolumePot) {
		nCmdShow = SW_HIDE;
	}

	mVolumePot.SetPos(gConfig[3]);
	mVolumePot.ShowWindow(nCmdShow);
   pCWnd = GetDlgItem(IDC_VOLUME_LABEL);
	pCWnd->ShowWindow(nCmdShow);
}

void ManualPage::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	int ID = pScrollBar->GetDlgCtrlID();

	if(ID == IDC_VOLUME) {
		gConfig[3] = (unsigned char) mVolumePot.GetPos();
		TRACE2("0x%x/0x%x\n",nSBCode,gConfig[3]);
		CComm.SetVolumeLevel(gConfig[3]);
	}
	else if(ID == IDC_SQUELCH) {
		gConfig[2] = (unsigned char) mSquelchPot.GetPos();
		TRACE2("0x%x/0x%x\n",nSBCode,gConfig[2]);
		CComm.SetSquelchLevel(gConfig[2]);
	}
	CPropertyPage::OnVScroll(nSBCode, nPos, pScrollBar);
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
   
	if(g_bHaveModeData) {
		ModeData();
	}
   return CPropertyPage::OnSetActive();
}


void CScanEnable::ModeData()
{
   CButton *pRB;
	int x;

   switch(gModeData[9] & SYNTOR_SCAN_TYPE_MASK) {
      case SYNTOR_SCAN_TYPE_DOUBLE:
         m_bScanEnabled = TRUE;
			x = INVERT_MODE(gModeData[0xa] & SYNTOR_32MODE_MASK);
         mPriorityMode.SetCurSel(x+1);
         x = INVERT_MODE(gModeData[9] & SYNTOR_32MODE_MASK);
         m2ndPriorityMode.SetCurSel(x+1);
         break;

      case SYNTOR_SCAN_TYPE_SINGLE:
         m_bScanEnabled = TRUE;
         x = INVERT_MODE(gModeData[0xa] & SYNTOR_32MODE_MASK);
         mPriorityMode.SetCurSel(x+1);
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

   if(gInvertedModeSel) {
		unsigned int Mask = 1;
		for(int i = 0; i < 32; i++) {
			pRB = (CButton *) GetDlgItem(NPResoursesIDs[i]);
			pRB->SetCheck(NPBits & Mask ? FALSE : TRUE);
			Mask <<= 1;
		}
	}
	else {
		unsigned int Mask = 0x80000000;
		for(int i = 0; i < 32; i++) {
			pRB = (CButton *) GetDlgItem(NPResoursesIDs[i]);
			pRB->SetCheck(NPBits & Mask ? FALSE : TRUE);
			Mask >>= 1;
		}
	}
   UpdateData(FALSE);
   EnableDisableControls();
}

void CScanEnable::OnManualSet()
{
   UpdateData(TRUE);

   int NPBits = 0xffffffff;

   CButton *pRB;

	unsigned int Mask = gInvertedModeSel ? 1 : 0x80000000;
	for(int i = 0; i < 32; i++) {
		pRB = (CButton *) GetDlgItem(NPResoursesIDs[i]);
		if(pRB->GetCheck()) {
			NPBits &= ~Mask;
		}
		if(gInvertedModeSel) {
			Mask <<= 1;
		}
		else {
			Mask >>= 1;
		}
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
         gModeData[0xa] |= INVERT_MODE(PrioritySel - 1);
      }
      else {
      // Both priority channels selected
         gModeData[9] |= SYNTOR_SCAN_TYPE_DOUBLE;
         gModeData[9] |= INVERT_MODE(SecondPrioritySel - 1);
         gModeData[0xa] |= INVERT_MODE(PrioritySel - 1);
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
      CComm.SelectMode(gLastModeSel);
      CComm.StoreVFO();
   }
}

void CScanEnable::OnRecallMode()
{
   CModeSel dlg;

   if(dlg.DoModal() == IDOK) {
      CComm.SelectMode(gLastModeSel);
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
	m_bDPlScan = FALSE;
   m_bHaveSignal = FALSE;
   m_bListSelected = FALSE;
   mPlFreq = 0;
   mDPlFreq = 1;
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
	ON_BN_CLICKED(IDC_DO_DPL_SCAN, OnDoDplScan)
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
		else if(m_bDPlScan) {
			ChangeDPL();
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

void CBandScan::ChangeDPL()
{
   UpdateData(TRUE);
	if(dplfunnybits[mDPlFreq].dpl == 0) {
		mDPlFreq = 1;
	}
	mPLFreqText.Format("%03o",dplfunnybits[mDPlFreq].dpl);
   UpdateData(FALSE);
	SetDPL(mDPlFreq++,&gModeData[6]);
	CComm.SetModeData(&gModeData[0]);
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
		else if(m_bDPlScan) {
			ChangeDPL();
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
   if(m_bPlScan || m_bDPlScan) {
   // Return to carrier squelch
      ChangePL(FALSE);
   }
   m_bPlScan = FALSE;
	m_bDPlScan = FALSE;
   m_bScanActive = FALSE;
   InitButtons();
}

void CBandScan::OnDoPlScan() 
{
	m_bPlScan = TRUE;
   m_bDPlScan = FALSE;
   m_bScanActive = FALSE;
   InitButtons();
}

void CBandScan::OnDoDplScan() 
{
	m_bDPlScan = TRUE;
   m_bPlScan = FALSE;
   m_bScanActive = FALSE;
   InitButtons();
// Update gModeData 'cuz we'll be using it to set the DPL tones later
	CComm.GetModeData();
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
	pDownloader = NULL;
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
	ON_BN_CLICKED(IDC_UPDATE_FIRMWARE, OnUpdateFirmware)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAbout message handlers


BOOL CAbout::OnSetActive() 
{
// Always request firmware version, we may have changed radions since
// the last time we were here...
	CComm.RequestFWVer();
   if(g_bHaveFWVer) {
		if(gLoaderVerString.IsEmpty()) {
			GetDlgItem(IDC_LOADER_VER)->ShowWindow(SW_HIDE);
		}
		else {
			GetDlgItem(IDC_LOADER_VER)->SetWindowText(gLoaderVerString);
			GetDlgItem(IDC_LOADER_VER)->ShowWindow(SW_SHOWNORMAL);
		}
		GetDlgItem(IDC_FIRMWARE_VER)->SetWindowText(gFirmwareVerString);
	}

   SetButtonMode(IDOK,0,NULL,FALSE,TRUE);
   SetButtonMode(IDCANCEL,0,NULL,FALSE,TRUE);
   SetButtonMode(ID_APPLY_NOW,0,NULL,FALSE,TRUE);
   SetButtonMode(IDHELP,0,NULL,FALSE,TRUE);
   

   return CPropertyPage::OnSetActive();
}


void CAbout::OnUpdateFirmware() 
{
	pDownloader = new CLoadHex;

   pDownloader->DoModal();
	delete pDownloader;
	pDownloader = NULL;
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
   ON_BN_CLICKED(ID_GET_CODEPLUG_DATA,OnGetCodePlugData)
   ON_BN_CLICKED(ID_GET_SYNC_DEBUG,OnGetSyncData)
   ON_BN_CLICKED(ID_GET_SIG_REPORT,OnGetSigReport)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDebugMsgs message handlers

BOOL CDebugMsgs::OnSetActive() 
{
   if(!g_bHaveFWVer)
   {
      CComm.RequestFWVer();
   }

   SetButtonMode(IDOK,ID_GET_CODEPLUG_DATA,"Code Plug",TRUE,FALSE);
   SetButtonMode(IDCANCEL,ID_GET_SYNC_DEBUG,"Sync Data",TRUE,FALSE);
   SetButtonMode(ID_APPLY_NOW,ID_GET_SIG_REPORT,"Sig report",FALSE,TRUE);
   SetButtonMode(IDHELP,0,NULL,FALSE,TRUE);
   
	return CPropertyPage::OnSetActive();
}

void CDebugMsgs::OnGetCodePlugData()
{
   CComm.GetModeData();
}

void CDebugMsgs::OnGetSyncData()
{
   CComm.GetSyncData();
}

void CDebugMsgs::OnGetSigReport()
{
   CComm.GetSigReport();
}

void CDebugMsgs::ModeData(unsigned char *Data)
{
   CString Text;
	CString Temp;
	FILE *fp;

	if((fp = fopen("c:\\xcat.bin","w")) != NULL) {
		fwrite(Data,16,1,fp);
		fclose(fp);
	}

	Text.Format("Raw mode 1 code plug data:\r\n");
   for(int i = 0; i < 8; i++) {
      Temp.Format("%02X ",Data[i]);
		Text += Temp;
   }
   Text += "\r\n";

   for(; i < 16; i++) {
      Temp.Format("%02X ",Data[i]);
		Text += Temp;
   }
   Text += "\r\n";

   mEdit.SetWindowText(Text);
}

void CDebugMsgs::SyncDebugData(unsigned char *Data)
{
   CString Text;
	CString Temp;
	int Bytes;
	int Bits;
	int OffNumBits;
                  
	if(gFirmwareVer < 27) {
	// before version 0.27 there were only 5 bytes of serial data
		OffNumBits = 5;
	}
	else {
	// after version 0.27 there were are 7 bytes of serial data
		OffNumBits = 7;
	}

	Bits = Data[OffNumBits];
	Bytes = Bits/8;

	if((Bits % 8) != 0) {
		Bytes++;
	}

	Text.Format("Received %d bits (%d bytes):\r\n",Bits,Bytes);
	if(Bytes > 7) {
		Bytes = 7;
	}
   for(int i = Bytes-1; i >= 0; i--) {
      Temp.Format("%02X ",Data[i]);
		Text += Temp;
   }
	Temp.Format("\r\nTotal frames %d\r\n",Data[OffNumBits+1]);
	Text += Temp;
	
	Temp.Format("Invalid frames %d\r\n",Data[OffNumBits+5]);
	Text += Temp;
	
	Temp.Format("Frames that successfully set rx frequency %d\r\n",Data[OffNumBits+3]);
	Text += Temp;

	Temp.Format("Frames that successfully set tx frequency %d\r\n",Data[OffNumBits+4]);
	Text += Temp;

	Temp.Format("Serial input is disabled after %d bits\r\n",Data[OffNumBits+2]);
	Text += Temp;
   
	if(gFirmwareVer == 23 || gFirmwareVer > 28) {
	// Palomar specific variable
		Temp.Format("Serial frame size %d bits\r\n",Data[OffNumBits+6]);
		Text += Temp;
	}

	if(gFirmwareVer > 28) {
		Temp.Format("Watchdog resets: %d\r\n",Data[OffNumBits+7]);
		Text += Temp;
	
		Temp.Format("Brownout resets: %d\r\n",Data[OffNumBits+8]);
		Text += Temp;

		Temp.Format("Unknown resets: %d\r\n",Data[OffNumBits+9]);
		Text += Temp;

	}

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
	mUFasSquelch = FALSE;
	mEnableVolumePot = FALSE;
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
	DDX_Control(pDX, IDC_OUT_7, mOut7);
	DDX_Control(pDX, IDC_OUT_6, mOut6);
	DDX_Control(pDX, IDC_OUT_4, mOut4);
	DDX_Control(pDX, IDC_OUT_3, mOut3);
   DDX_Control(pDX, IDC_TRANSFER_STATUS, mTransferStatus);
   DDX_Control(pDX, IDC_OUT_5, mOut5);
   DDX_Control(pDX, IDC_OUT_2, mOut2);
   DDX_Control(pDX, IDC_OUT_1, mOut1);
   DDX_Control(pDX, IDC_OUT_0, mOut0);
   DDX_Control(pDX, IDC_CONTROL_SYS, mControlSys);
   DDX_Control(pDX, IDC_BAND, mBand);
   DDX_Text(pDX, IDC_RX_VCO_SPLIT_F, mRxVcoSplitFreq);
	DDV_MinMaxDouble(pDX, mRxVcoSplitFreq, 100., 500.);
   DDX_Check(pDX, IDC_SEND_COS, mSendCosMsg);
   DDX_Text(pDX, IDC_TX_VCO_SPLIT_F, mTxVcoSplitFreq);
	DDV_MinMaxDouble(pDX, mTxVcoSplitFreq, 100., 500.);
	DDX_Check(pDX, IDC_UF_AS_SQUELCH, mUFasSquelch);
	DDX_Check(pDX, IDC_UF_HAS_VOLUME, mEnableVolumePot);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CConfigure, CPropertyPage)
   //{{AFX_MSG_MAP(CConfigure)
   ON_BN_CLICKED(IDC_RANGE1, OnRange1)
   ON_BN_CLICKED(IDC_RANGE2, OnRange2)
   ON_CBN_SELCHANGE(IDC_BAND, OnSelchangeBand)
	ON_CBN_SELCHANGE(IDC_OUT_3, OnSelchangeOut3)
	ON_CBN_SELCHANGE(IDC_OUT_4, OnSelchangeOut4)
	ON_CBN_SELCHANGE(IDC_OUT_6, OnSelchangeOut6)
	ON_CBN_SELCHANGE(IDC_CONTROL_SYS, OnSelchangeControlSys)
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
	if(g_bHaveConfig) {
		ConfigMsgRx((unsigned char *) &gConfig);
	}
   if(!g_bHaveFWVer)
   {
      CComm.RequestFWVer();
   }
   return CPropertyPage::OnSetActive();
}

void CConfigure::OnGetConfig()
{
   CComm.GetConfig();
   CComm.GetVCOSplits();
}

int CConfigure::GetSelectedConfig()
{
	int CurrentConfig = mBand.GetCurSel();
// kludge: swap 440 and 420 
// (I can't stand the frequencies being out of order and 420 was added later.)

	if(CurrentConfig == CONFIG_420) {
		CurrentConfig = CONFIG_440;
	}
	else if(CurrentConfig == CONFIG_440) {
		CurrentConfig = CONFIG_420;
	}

	return CurrentConfig;
}


void CConfigure::SaveSplits()
{
	switch(GetSelectedConfig()) {
		case CONFIG_2M:
			gVCORxSplitVHF = mRxVcoSplitFreq;
			gVCOTxSplitVHF = mTxVcoSplitFreq;
			break;

		case CONFIG_420:
			gVCORxSplit420 = mRxVcoSplitFreq;
			gVCOTxSplit420 = mTxVcoSplitFreq;
			break;

		case CONFIG_440:
			gVCORxSplit440 = mRxVcoSplitFreq;
			gVCOTxSplit440 = mTxVcoSplitFreq;
			break;
	}
}

void CConfigure::OnSetConfig()
{
	int bSetConfig = TRUE;
   UpdateData(TRUE);

	int CurrentConfig = GetSelectedConfig();
   gConfig[0] = CurrentConfig;

   switch(mControlSys.GetCurSel()) {
      case 0:  // No control system
         break;

      case 1:  // Doug Hall
         gConfig[0] |= 0x10;
			if(mUFasSquelch) { 
				gConfig[0] |= 0x80;
			}
			else {
				gConfig[0] &= ~0x80;
			}
         break;

      case 2:  // Palomar Telecom / Cactus / Remote base #1
         gConfig[0] |= 0x20;
         break;

      case 3:  // Palomar Telecom / Cactus / Remote base #2
         gConfig[0] |= 0x60;
         break;

      case 4:  // Palomar Telecom / Cactus / Remote base #3
         gConfig[0] |= 0xa0;
         break;

      case 5:  // Palomar Telecom / Cactus / Remote base #4
         gConfig[0] |= 0xe0;
         break;
   }
   
   if(mSendCosMsg) {
      gConfig[0] |= 8;
   }

   gConfig[1] = 0xff;

   if(!mOut0.GetCurSel()) {
      gConfig[1] &= ~1;
   }

   if(!mOut1.GetCurSel()) {
      gConfig[1] &= ~2;
   }

   if(!mOut2.GetCurSel()) {
      gConfig[1] &= ~4;
   }

   if(!mOut3.GetCurSel()) {
      gConfig[1] &= ~8;
   }

   if(!mOut4.GetCurSel()) {
      gConfig[1] &= ~0x10;
   }

   if(!mOut5.GetCurSel()) {
      gConfig[1] &= ~0x20;
   }

   if(!mOut6.GetCurSel()) {
      gConfig[1] &= ~0x40;
   }

   if(!mOut7.GetCurSel()) {
      gConfig[1] &= ~0x80;
   }


	if(CurrentConfig == CONFIG_420) {
		CString ErrMsg;
		ErrMsg.Format("Warning: This firmware does not support\n"
						  "UHF range 1 (406 to 420 Mhz).\n"
						  "Please contact wb6ymh@cox.net for\n"
						  "UHF range 1 firmware.");
		AfxMessageBox(ErrMsg,MB_ICONEXCLAMATION);
		bSetConfig = FALSE;
	}

	if(g_bHaveFWVer) {
	// Check features against firmware capabilities
		if((gConfig[1] & CONFIG_SQU_POT_MASK) == 0 && gFirmwareVer < 27) {
		// Squelch pot support was added in version 0.27
			CString ErrMsg;
			ErrMsg.Format("Warning: Squelch pot support requires\n"
							  "Xcat firmware V 0.27 or better.\n"
							  "This Xcat has %s\n",gFirmwareVerString);
			AfxMessageBox(ErrMsg,MB_ICONEXCLAMATION);
			bSetConfig = FALSE;
		}
		else if((gConfig[1] & CONFIG_SQU_POT_MASK) == 0 && mEnableVolumePot &&
			gFirmwareVer < 28) 
		{
		// Volume pot support was added in version 0.28
			CString ErrMsg;
			ErrMsg.Format("Warning: Volume pot support requires\n"
							  "Xcat firmware V 0.27 or better.\n"
							  "This Xcat has %s\n",gFirmwareVerString);
			AfxMessageBox(ErrMsg,MB_ICONEXCLAMATION);
			bSetConfig = FALSE;
		}

		if((gConfig[0] & CONFIG_CTRL_MASK) == CONFIG_CACTUS) {
			CString ErrMsg;
			if(gFirmwareVer >= 27 && gFirmwareVer < 30) {
				ErrMsg.Format("Warning: Palomar Telcom support was dropped\n"
								  "in Xcat firmware V 0.27.  Please\n"
								  "contact wb6ymh@cox.net for Xcat firmware\n"
								  "that supports the Palomar control system.");
				bSetConfig = FALSE;
			}
			else {
				ErrMsg.Format("Warning: Palomar Telcom support is incomplete.\n"
								  "Please contact wb6ymh@cox.net for more\n"
								  "information.");
			}
			AfxMessageBox(ErrMsg,MB_ICONEXCLAMATION);
		}
	}


	if(bSetConfig) {
		CComm.SetConfig(&gConfig[0]);
		CComm.SetVCOSplits((unsigned int) (mRxVcoSplitFreq * 1e6),
								 (unsigned int) (mTxVcoSplitFreq * 1e6));
		SaveSplits();
		gEnableVolumePot = mEnableVolumePot;
	}
}


void CConfigure::ConfigMsgRx(unsigned char *Config)
{
   UpdateData(TRUE);
   int BandSelection;

   BandSelection = Config[0] & CONFIG_BAND_MASK;
   
// kludge: swap 440 and 420 
// (I can't stand the frequencies being out of order and 420 was added later.)

	if(BandSelection == CONFIG_420) {
		BandSelection = CONFIG_440;
	}
	else if(BandSelection == CONFIG_440) {
		BandSelection = CONFIG_420;
	}
	else if(BandSelection > CONFIG_420) {
      BandSelection = 0;
   }


   mBand.SetCurSel(BandSelection);
   mSendCosMsg = (Config[0] & 0x8) ? TRUE : FALSE;
   
   int ControlSystemSel = 0;

   switch(Config[0] >> 4) {
      case 9:  // Doug Hall w/ UF for squelch
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

      case 0xe:   // Palomar Telecom / Cactus / Remote base #4
         ControlSystemSel = 5;
         break;
   }
   mControlSys.SetCurSel(ControlSystemSel);

   mOut0.SetCurSel((Config[1] & 0x1) ? 1 : 0);
   mOut1.SetCurSel((Config[1] & 0x2) ? 1 : 0);
   mOut2.SetCurSel((Config[1] & 0x4) ? 1 : 0);
   mOut3.SetCurSel((Config[1] & 0x8) ? 1 : 0);
   mOut4.SetCurSel((Config[1] & 0x10) ? 1 : 0);
   mOut5.SetCurSel((Config[1] & 0x20) ? 1 : 0);
   mOut6.SetCurSel((Config[1] & 0x40) ? 1 : 0);
   mOut7.SetCurSel((Config[1] & 0x80) ? 1 : 0);

   EnableSplitItems();
	UpdateUFasSquelch();
	UpdateEnableVolumePot();
	UpdateIO5and7();

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
	SaveSplits();

   UpdateData(FALSE);
}


void CConfigure::OnRange1() 
{
	switch(GetSelectedConfig()) {
		case CONFIG_2M:
			mRxVcoSplitFreq = 198.5;
			mTxVcoSplitFreq = 144.0;
			break;

		case CONFIG_420:
			mRxVcoSplitFreq = 466.7;
			mTxVcoSplitFreq = 412.8;
			break;

		case CONFIG_440:
			mRxVcoSplitFreq = 405.6;
			mTxVcoSplitFreq = 459.5;
			break;
	}

   UpdateData(FALSE);
}

void CConfigure::OnRange2() 
{
   mRxVcoSplitFreq = 203.9;
   mTxVcoSplitFreq = 161.8;

   UpdateData(FALSE);
}

void CConfigure::EnableSplitItems() 
{
   static int VSplitItemIDs[] = {
      IDC_TX_VCO_SPLIT_F,
      IDC_RX_VCO_SPLIT_F,
      IDC_RANGE1,
      IDC_RANGE2,
      IDC_DEFAULTS,
      0
   };

	int CurrentConfig = GetSelectedConfig();

   EnableItems(this,VSplitItemIDs,CurrentConfig > CONFIG_10_6M);

	CWnd *pRange1 = GetDlgItem(IDC_RANGE1);
	CWnd *pRange2 = GetDlgItem(IDC_RANGE2);
	CWnd *pLabel  = GetDlgItem(IDC_DEFAULTS);

	if(CurrentConfig == CONFIG_2M) {
		pRange1->SetWindowText("Range 1");
		pRange2->ShowWindow(SW_SHOWNORMAL);
		pLabel->ShowWindow(SW_SHOWNORMAL);
		mRxVcoSplitFreq = gVCORxSplitVHF;
		mTxVcoSplitFreq = gVCOTxSplitVHF;
		UpdateData(FALSE);
	}
	else if(CurrentConfig > CONFIG_10_6M) {
	// UHF
		pRange1->SetWindowText("Set Defaults");
		pRange2->ShowWindow(SW_HIDE);
		pLabel->ShowWindow(SW_HIDE);
		if(CurrentConfig == CONFIG_420) {
			mRxVcoSplitFreq = gVCORxSplit420;
			mTxVcoSplitFreq = gVCOTxSplit420;
		}
		else {
			mRxVcoSplitFreq = gVCORxSplit440;
			mTxVcoSplitFreq = gVCOTxSplit440;
		}
		UpdateData(FALSE);
	}
}

void CConfigure::OnSelchangeBand() 
{
   EnableSplitItems();
}

void CConfigure::OnSaveCodePlug()
{
   CXcatFileDialog dlg(FALSE,NULL,gSaveFilename,
							  OFN_CREATEPROMPT|OFN_OVERWRITEPROMPT,NULL);

   if(dlg.DoModal() == IDOK) {
		CString Temp;
      Temp = dlg.GetFileExt();
		Temp.MakeLower();
      gSaveFilename = dlg.GetPathName();
		if(Temp == "bin") {
		// Code plug data
			if((mFp = fopen(gSaveFilename,"wb")) == NULL) {
				CString ErrMsg;
				ErrMsg.Format("Open failed: %s",Err2String(errno));
				AfxMessageBox(ErrMsg);
			}
			else {
				mMode = 1;
				m_bSaving = TRUE;

				mTransferStatus.SetWindowText("Saving Mode 1");
			// NB: undo inverted mode stuff so we always save/restore the code
			// plug data in the same (natural) order
				CComm.SelectMode(INVERT_MODE(mMode-1)+1);
				CComm.RecallMode();
				CComm.GetModeData();
			}
		}
		else if(Temp == "txt") {
			if((mFp = fopen(gSaveFilename,"w")) == NULL) {
				CString ErrMsg;
				ErrMsg.Format("Open failed: %s",Err2String(errno));
				AfxMessageBox(ErrMsg);
			}
			else {
				for(int i = 0; i < 32; i++) {
					fprintf(mFp,"%s\n",gModeName[i]);
				}
				fclose(mFp);
				mFp = NULL;
			}
		}
   }
}

void CConfigure::OnRestoreCodePlug()
{
   CXcatFileDialog dlg(TRUE,NULL,gSaveFilename,OFN_FILEMUSTEXIST|OFN_HIDEREADONLY,
                       NULL);

   if(dlg.DoModal() == IDOK) {
		CString Temp;
      Temp = dlg.GetFileExt();
		Temp.MakeLower();
      gRestoreFilename = dlg.GetPathName();
		if(Temp == "bin") {
		// Code plug data
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
		else if(Temp == "txt") {
			if((mFp = fopen(gRestoreFilename,"r")) == NULL) {
				CString ErrMsg;
				ErrMsg.Format("Open failed: %s",Err2String(errno));
				AfxMessageBox(ErrMsg);
			}
			else {
				char line[80];
				for(int i = 0; i < 32; i++) {
					fgets(line,sizeof(line),mFp);
					line[strlen(line)-1] = 0;	// remove new line
					gModeName[i] = line;
				}
				fclose(mFp);
				mFp = NULL;
			}
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
			// NB: undo inverted mode stuff so we always save/restore the code
			// plug data in the same (natural) order
				CComm.SelectMode(INVERT_MODE(mMode-1)+1);
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
		// NB: undo inverted mode stuff so we always save/restore the code
		// plug data in the same (natural) order
         CComm.SelectMode(INVERT_MODE(mMode-1)+1);
      // Write it
         CComm.StoreVFO();
      // Read it
         CComm.RecallMode();
      // Send it back
         CComm.GetModeData();
      }
   }
}

void CConfigure::UpdateEnableVolumePot()
{
	CButton *pRB = (CButton *) GetDlgItem(IDC_UF_HAS_VOLUME);
	ASSERT_VALID(pRB);

// Gray out "Enable volume pot" unless pots are enabled and firmware is > 0.28
	pRB->EnableWindow(mOut3.GetCurSel() == 0 /* && (!g_bHaveFWVer || gFirmwareVer >= 28*/);
}

void CConfigure::OnSelchangeOut3_4_6(int NewSelection) 
{
	mOut3.SetCurSel(NewSelection);
	mOut4.SetCurSel(NewSelection);
	mOut6.SetCurSel(NewSelection);
	UpdateEnableVolumePot();

   UpdateData(FALSE);
}

void CConfigure::OnSelchangeOut3() 
{
   UpdateData(TRUE);
	OnSelchangeOut3_4_6(mOut3.GetCurSel());
}

void CConfigure::OnSelchangeOut4() 
{
   UpdateData(TRUE);
	OnSelchangeOut3_4_6(mOut4.GetCurSel());
}

void CConfigure::OnSelchangeOut6() 
{
   UpdateData(TRUE);
	OnSelchangeOut3_4_6(mOut6.GetCurSel());
}

void CConfigure::UpdateUFasSquelch() 
{
	CButton *pRB = (CButton *) GetDlgItem(IDC_UF_AS_SQUELCH);
	ASSERT_VALID(pRB);
// Gray out "Use UF outputs for squelch" for everything other than doug hall Mode
	pRB->EnableWindow(mControlSys.GetCurSel() == 1);
}

void CConfigure::UpdateIO5and7()
{
// Gray out Out 5 and 7 in Palomar mode
	if(mControlSys.GetCurSel() < 2) {
	// UF5 = user selection
		int Save = mOut5.GetCurSel();
		mOut5.EnableWindow(TRUE);
		mOut5.DeleteString(1);
		mOut5.AddString("User Output");
		mOut5.SetCurSel(Save);
		mOut7.EnableWindow(TRUE);
	}
	else {
	// UF5 = PTT output, UF7 = PTT input
		mOut5.EnableWindow(FALSE);
		mOut5.DeleteString(1);
		mOut5.AddString("PTT Output");
		mOut5.SetCurSel(1);
		mOut7.EnableWindow(FALSE);
		mOut7.SetCurSel(0);
	}
}

void CConfigure::OnSelchangeControlSys() 
{
	UpdateUFasSquelch();
	UpdateIO5and7();
}

BOOL CAbout::OnInitDialog() 
{
   CPropertyPage::OnInitDialog();
   
   CString TimeStamp;

   TimeStamp.Format("Compiled:  %s, %s",DateCompiled,TimeCompiled);
   mCompiled.SetWindowText(TimeStamp);
   
   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}


/////////////////////////////////////////////////////////////////////////////
// CModeSel dialog


CModeSel::CModeSel(CWnd* pParent /*=NULL*/)
	: CDialog(CModeSel::IDD, pParent)
{
	//{{AFX_DATA_INIT(CModeSel)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CModeSel::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CModeSel)
	DDX_Control(pDX, IDC_MODE, mMode);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CModeSel, CDialog)
	//{{AFX_MSG_MAP(CModeSel)
	ON_BN_CLICKED(IDC_EDIT_NAME, OnEditName)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CModeSel message handlers

void CModeSel::OnEditName() 
{
	CEditModeName dlg;
	int Index = mMode.GetCurSel();

	dlg.mName = gModeName[Index];

   if(dlg.DoModal() == IDOK) {
		CString Temp;
		
		gModeName[Index] = dlg.mName;
		mMode.DeleteString(Index);
		Temp.Format("%d: %s",Index+1,gModeName[Index]);
		mMode.InsertString(Index,Temp);
		mMode.SetCurSel(Index);
	}
}

BOOL CModeSel::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	mMode.ResetContent();
	for(int i = 0; i < 32; i++) {
		CString Temp;

		Temp.Format("%d: %s",i+1,gModeName[i]);
		mMode.InsertString(-1,Temp);
	}
	mMode.SetCurSel(gLastModeSel-1);
	return TRUE;
}


void CModeSel::OnOK() 
{
	gLastModeSel = mMode.GetCurSel() + 1;
	
	CDialog::OnOK();
}

/////////////////////////////////////////////////////////////////////////////
// CEditModeName dialog


CEditModeName::CEditModeName(CWnd* pParent /*=NULL*/)
	: CDialog(CEditModeName::IDD, pParent)
{
	//{{AFX_DATA_INIT(CEditModeName)
	mName = _T("");
	//}}AFX_DATA_INIT
}


void CEditModeName::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEditModeName)
	DDX_Text(pDX, IDC_NAME, mName);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CEditModeName, CDialog)
	//{{AFX_MSG_MAP(CEditModeName)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEditModeName message handlers

/////////////////////////////////////////////////////////////////////////////
// CXcatFileDialog

IMPLEMENT_DYNAMIC(CXcatFileDialog, CFileDialog)
static char XCatFilter[] = "Code plug binary (*.bin)|*.bin|"
	                        "Mode Names (*.txt)|*.txt||";

CXcatFileDialog::CXcatFileDialog(BOOL bOpenFileDialog, LPCTSTR lpszDefExt, LPCTSTR lpszFileName,
		DWORD dwFlags, LPCTSTR lpszFilter, CWnd* pParentWnd) :
	CFileDialog(bOpenFileDialog, lpszDefExt, lpszFileName, dwFlags, XCatFilter, pParentWnd)
{

}


BEGIN_MESSAGE_MAP(CXcatFileDialog, CFileDialog)
	//{{AFX_MSG_MAP(CXcatFileDialog)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


BOOL CXcatFileDialog::OnFileNameOK()
{
	BOOL Ret = FALSE;
	CString Temp;
	
	Temp = GetFileExt();
	Temp.MakeLower();
	
	if(Temp != "bin" && Temp != "txt") {
		AfxMessageBox("Error: Invalid extension.\n\n"
						  "Please use a .bin extension for code plug data\n"
						  "or a .txt extension for mode label files.");
		Ret = TRUE;
	}

	return Ret;
}
/////////////////////////////////////////////////////////////////////////////
// CCommSetup1 dialog

IMPLEMENT_DYNCREATE(CCommSetup1, CPropertyPage)

CCommSetup1::CCommSetup1() : CPropertyPage(CCommSetup1::IDD)
{
	//{{AFX_DATA_INIT(CCommSetup1)
	mXCatAdr = _T("");
	//}}AFX_DATA_INIT
}


void CCommSetup1::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCommSetup1)
	DDX_Text(pDX, IDC_XCAT_ADR, mXCatAdr);
	DDV_MaxChars(pDX, mXCatAdr, 2);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCommSetup1, CPropertyPage)
	//{{AFX_MSG_MAP(CCommSetup1)
	//}}AFX_MSG_MAP
   ON_BN_CLICKED(ID_SET_XCAT_ADR,OnSet)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCommSetup1 message handlers

BOOL CCommSetup1::OnInitDialog() 
{
   CComboBox *pCB = (CComboBox*) GetDlgItem(IDC_COM_PORT);

   CDialog::OnInitDialog();
   
   pCB->ResetContent();

   int Index = 1;

   memset(mPortLookup,sizeof(mPortLookup),0);

   for(UINT i = 0; i < MAX_COM_PORTS; i++) {
      CString ComString;
      int bPortExists = 0;
      if(i == 0) {
         ComString = "Not Configured";
      }
      else {
         ComString.Format("COM%d",i);
         HANDLE idComDev = CreateFile(ComString,GENERIC_READ | GENERIC_WRITE,0,
                                      NULL,OPEN_EXISTING,FILE_FLAG_OVERLAPPED,0);
         if(idComDev == INVALID_HANDLE_VALUE) {
            DWORD err = GetLastError();
            if(err == ERROR_ACCESS_DENIED) {
            // Port exists, but is in use
               bPortExists = 1;
            }
         }
         else{
            // close the port
            CloseHandle(idComDev);
            bPortExists = 1;
         }

         if(!bPortExists) {
            continue;
         }
         mPortLookup[Index++] = i;
      }
      pCB->InsertString(-1,ComString);
   }

   for(i = 0; i < MAX_COM_PORTS; i++) {
      if(mPortLookup[i] == gComPort) {
         pCB->SetCurSel(i);
         break;
      }
   }

   if(i == MAX_COM_PORTS) {
   // didn't find the port
      pCB->SetCurSel(0);
   }

   pCB = (CComboBox*) GetDlgItem(IDC_BAUDRATE);
	pCB->SetCurSel(GetBaudrateValue(gBaudrate));

	mXCatAdr.Format("%02X",gXcatAdr);

   UpdateData(FALSE);
   
   return TRUE;
}

BOOL CCommSetup1::OnSetActive() 
{
   SetButtonMode(IDOK,ID_SET_XCAT_ADR,"Set",TRUE,FALSE);
   SetButtonMode(IDCANCEL,0,NULL,FALSE,TRUE);
   SetButtonMode(ID_APPLY_NOW,0,NULL,FALSE,TRUE);
   SetButtonMode(IDHELP,0,NULL,FALSE,TRUE);
   
	return CPropertyPage::OnSetActive();
}

void CCommSetup1::OnSet() 
{
   UpdateData(TRUE);
	int bReInitializeComm = FALSE;
	int bSendXcatCommConfig = FALSE;
	int bBaudrateChanged = FALSE;
	int bComPortChanged = FALSE;
	int BaudSel;

	if(sscanf((LPCSTR) mXCatAdr,"%x",&mNewXcatAdr) != 1) {
      CString ErrMsg;
      ErrMsg.Format("Error: the Xcat CI-V address is invalid.\n"
						  "Please enter a valid Hex address.");
      AfxMessageBox(ErrMsg);
	}
	else {
		CComboBox *pCB = (CComboBox*) GetDlgItem(IDC_COM_PORT);
		mNewComPort = mPortLookup[pCB->GetCurSel()];
		pCB = (CComboBox*) GetDlgItem(IDC_BAUDRATE);
		switch(BaudSel = pCB->GetCurSel()) {
			case 0:	// 1200 baud
				mNewBaudrate = 1200;
				break;

			case 1:	// 2400 baud
				mNewBaudrate = 2400;
				break;

			case 2:	// 4800 baud
				mNewBaudrate = 4800;
				break;

			case 3:	// 9600 baud
				mNewBaudrate = 9600;
				break;

			case 4:	// 19200 baud
				mNewBaudrate = 19200;
				break;
		}

		if((mNewXcatAdr != gXcatAdr || mNewBaudrate != gBaudrate) &&
			CComm.CommunicationsUp())
		{	// Change communications parameters on Xcat
				CComm.SetCommParameters(BaudSel,mNewXcatAdr);
		}
		else {
		// We don't need to change communications parameters on Xcat or 
		// We can't seem to be able to talk to the Xcat at the moment...
			gXcatAdr = mNewXcatAdr;
			if(gBaudrate != mNewBaudrate || gComPort != mNewComPort) {
				if(!CComm.Init(mNewComPort,mNewBaudrate)) {
					CString ErrMsg;
					ErrMsg.Format("Unable to open COM%d,\n"
									  "please check that no other\n"
									  "programs are using COM%d.",mNewComPort,mNewComPort);
					AfxMessageBox(ErrMsg);
				}
				else {
					gComPort = mNewComPort;
					gBaudrate = mNewBaudrate;

				// Send something to the Xcat to check the communications line
					CComm.GetConfig();
				}
			}
		}

		CPropertyPage::OnOK();
	}
}


