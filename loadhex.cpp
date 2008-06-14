#include "stdafx.h"
#include "xcat.h"
#include "comm.h"
#include "loadhex.h"

/////////////////////////////////////////////////////////////////////////////
// CLoadHex dialog


CLoadHex::CLoadHex(CWnd* pParent /*=NULL*/)
	: CDialog(CLoadHex::IDD, pParent)
{
	//{{AFX_DATA_INIT(CLoadHex)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	mDownloadState = INACTIVE;
	mDownloadFp = NULL;
	mXcatAdr = gXcatAdr;
	mVerifiedBlocks = 0;
	mProgrammedBlocks = 0;
	mDownloadSuccess = false;
}


void CLoadHex::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLoadHex)
	DDX_Control(pDX, IDC_MSGS, mMsgWin);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CLoadHex, CDialog)
	//{{AFX_MSG_MAP(CLoadHex)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLoadHex message handlers

void CLoadHex::EndDownload(bool bSuccess)
{
	mDownloadState = INACTIVE;
	mDownloadSuccess = bSuccess;

	if(mDownloadFp != NULL) {
		fclose(mDownloadFp);
		mDownloadFp = NULL;
	}

	if(!CComm.Init(gComPort,mDownloadSuccess ? 9600 : gBaudrate,FALSE)) {
		CString ErrMsg;
		ErrMsg.Format("Unable to open COM%d,\n"
						  "please check that no other\n"
						  "programs are using COM%d.",gComPort,gComPort);
		AfxMessageBox(ErrMsg);
	}

	if(mDownloadSuccess) {
		gXcatAdr = 0x20;
	// Set the Report Errors flag to suppress "(re)established"with the Xcat."
		CComm.m_bReportErrors = TRUE;
	}

// Refresh the firmware version
	CComm.RequestFWVer();
}

// Initialize comm port for download
void CLoadHex::EnterDownloadMode()
{
	if(!CComm.Init(gComPort,19200,TRUE)) {
      CString ErrMsg;
      ErrMsg.Format("Unable to open COM%d,\n"
                    "please check that no other\n"
                    "programs are using COM%d.",gComPort,gComPort);
      AfxMessageBox(ErrMsg);
		EndDownload(FALSE);
	}
	else {
		mDownloadState = START_WAIT;
	}
}

void CLoadHex::OnTimer()
{
	switch(mDownloadState) {
		case AUTO_START_WAIT:
			if(mTimeoutTimer-- == 0) {
			// Hmmm... ask the user to power cycle the radio.
				LOG(("AUTO_START_WAIT timeout\n"));
				ManualDownloadStart();
			}
			break;

		case START_WAIT:
			break;

		case LINE_WAIT:
			if(mTimeoutTimer-- == 0) {
				LOG(("LINE_WAIT timeout\n"));
				AfxMessageBox("The Xcat has stopped responding.\n"
								  "Check the connections and try again",
								  MB_ICONEXCLAMATION);
				PutMsg("The Xcat has stopped responding, download failed.\r\n");
				EndDownload(FALSE);
			}
			break;

		default:
			break;
	}
}

void CLoadHex::ManualDownloadStart()
{
	EnterDownloadMode();
	AfxMessageBox("Ready to download new code.\n"
					  "Power cycle the Syntor and Xcat\n"
					  "to continue\n");
}

void CLoadHex::CommSetupAck(bool bSuccess)
{
	if(mDownloadState == RESTORE_COMM_PARAMS) {
		if(bSuccess) {
			PutMsg("Communications parameters restored.\r\n");
			mDownloadState = INACTIVE;
			gXcatAdr = mXcatAdr;
			if(!CComm.Init(gComPort,gBaudrate,FALSE)) {
				CString ErrMsg;
				ErrMsg.Format("Unable to open COM%d,\n"
								  "please check that no other\n"
								  "programs are using COM%d.",gComPort,gComPort);
				AfxMessageBox(ErrMsg);
			}
		}
		else {
			PutMsg("Error: unable to restore prior communications parameters.\r\n");
		// Set our best guess, maybe power flamed out
			gBaudrate = 9600;
			gXcatAdr = 0x20;
		}
	}
	else if(mDownloadState != INACTIVE) {
		if(bSuccess) {
			EnterDownloadMode();
		}
		else {
			PutMsg("Error: unable to enter download mode, download failed.\r\n");
		}
	}
}

void CLoadHex::SendLine()
{
   char Line[120];
	int LineLen;

	if(fgets(Line,sizeof(Line)-1,mDownloadFp) != NULL) {
		LineLen = strlen(Line);
		Line[LineLen - 1] = '\r';
	}

	CComm.SendRaw(Line,LineLen);

	if(Line[0] == ':' && Line[7] == '0' && Line[8] == '0') {
	// Data record, hex address starts at Line[3]
		Line[7] = 0;
		unsigned int LoadAdr;
		CString Msg;
		sscanf(&Line[3],"%x",&LoadAdr);
		Msg.Format("0x%X",LoadAdr / 2);
		GetDlgItem(IDC_DOWNLOAD_ADR)->SetWindowText(Msg);
	}

	mTimeoutTimer = 4;	// give the Xcat 2 seconds to reply
	mDownloadState = LINE_WAIT;
}

void CLoadHex::OnDownloadChar(char HandShakeChar)
{
	switch(mDownloadState) {
		case AUTO_START_WAIT:
			if(HandShakeChar == 'B') {
				LOG(("Received startup character in AUTO_START_WAIT state\n"));
				CComm.SetBreak(FALSE);
				SendLine();
			}
			else {
				LOG(("Received 0x%x in state AUTO_START_WAIT\n",HandShakeChar));
			}
			break;

		case START_WAIT:
			if(HandShakeChar == 'B') {
				LOG(("Received startup character in START_WAIT state\n"));
				PutMsg("Beginning download.\r\n");
				CComm.SetBreak(FALSE);
				SendLine();
			}
			else {
				LOG(("Received 0x%x in START_WAIT state\n",HandShakeChar));
			}
			break;

		case LINE_WAIT:
			LOG(("Received \"%c\" in LINE_WAIT state\n",HandShakeChar));
         switch(HandShakeChar) {
				case 'P': {
					CString Msg;
					Msg.Format("%d",++mProgrammedBlocks);
					GetDlgItem(IDC_UPDATED_BLOCKS)->SetWindowText(Msg);
					SendLine();
					break;
				}

				case 'V': {
					CString Msg;
					Msg.Format("%d",++mVerifiedBlocks);
					GetDlgItem(IDC_VERIFIED_BLOCKS)->SetWindowText(Msg);
					SendLine();
					break;
				}

            case 'I':
            case 'R':
            // Ignore thise cases, just send the next line
					SendLine();
               break;

            case 'F':
            // We're done !
					PutMsg("Download complete.\r\n");
					EndDownload(TRUE);
               break;

            case 'E':
            // programming error
					PutMsg("The Xcat has reported programming error.\r\n"
							 "Download aborted.");
					EndDownload(FALSE);
               break;

            default:
               PutMsg("Error: Unexpected or undefined handshaking "
                      "character received (0x%x).\r\nDownload aborted.\r\n");
					EndDownload(FALSE);
               break;
         }
			break;
	}
}


void CLoadHex::PutMsg(const char *Msg) 
{
	CString Temp;

	mMsgWin.GetWindowText(Temp);
	Temp += Msg;
	mMsgWin.SetWindowText(Temp);
}

BOOL CLoadHex::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	CFileDialog dlg(TRUE,".hex",NULL,OFN_FILEMUSTEXIST|OFN_HIDEREADONLY,
						 "Firmware image files (*.hex)|*.hex|", NULL);

   if(dlg.DoModal() == IDOK) {
		CString Temp = dlg.GetPathName();

		if((mDownloadFp = fopen(Temp,"r")) == NULL) {
			CString ErrMsg;
			ErrMsg.Format("Error: Unable to open %s",Temp);
			AfxMessageBox(ErrMsg);
			ErrMsg += ", download failed.\r\n";
			PutMsg(ErrMsg);
		}
		else {
			CString Msg;
			if(CComm.CommunicationsUp()) {
			// Reinitialize the comm port for download.  
			// Send a set serial communications message to the Xcat so it'll 
			// reset, use 19200 as a baudrate because that's what the loader
			// runs at.  Then we should be ready to receive the 'B' character

				CComm.SetCommParameters(4,gXcatAdr);
				mTimeoutTimer = 4;	// give the Xcat 2 seconds to reply

				mDownloadState = gBaudrate == 19200 ? AUTO_START_WAIT : SET_COMM_19200;
			}
			else {
			// No communications, popup a dialog
				ManualDownloadStart();
			}
			if(g_bHaveFWVer) {
				Msg.Format("Firmware version before update: %s\r\n",gRawVerString);
				PutMsg(Msg);
			}
			Msg.Format("Downloading %s.\r\n",Temp);
			PutMsg(Msg);
			PutMsg("Waiting for download request from the Xcat...\r\n");
		}
   }
	else {
		OnCancel();
	}
	
	return TRUE;
}

void CLoadHex::FwVersionUpdated()
{
	CString Msg;

	Msg.Format("Firmware version after update: %s\r\n",gRawVerString);
	PutMsg(Msg);

	if(mDownloadSuccess && (gBaudrate != 9600 || gXcatAdr != 0x20)) {
	// We've successfully zapped the Xcat so we're now back to 9600 baud and
	// a CI-V address of 0x20 (assuming the firmware > 0.25) so 
	// we now need to reset the original values

		if(gBaudrate != 9600) {
			CString Msg;
			Msg.Format("Resetting baudrate to %d.\r\n",gBaudrate);
			PutMsg(Msg);
		}

		if(mXcatAdr != 0x20) {
			CString Msg;
			Msg.Format("Resetting CI-V address to 0x%x.\r\n",mXcatAdr);
			PutMsg(Msg);
		}

		CComm.SetCommParameters(GetBaudrateValue(gBaudrate),mXcatAdr);
		mDownloadState = RESTORE_COMM_PARAMS;
	}
}

