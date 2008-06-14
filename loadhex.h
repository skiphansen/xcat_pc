#if !defined(AFX_LOADHEX_H__4FBCADAF_1DBE_4C46_AFE9_23D89AF7C91C__INCLUDED_)
#define AFX_LOADHEX_H__4FBCADAF_1DBE_4C46_AFE9_23D89AF7C91C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// loadhex.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CLoadHex dialog

class CLoadHex : public CDialog
{
// Construction
public:
	CLoadHex(CWnd* pParent = NULL);   // standard constructor
	void OnTimer();
	void OnDownloadChar(char HandShakeChar);
	void CommSetupAck(bool bSuccess);
	void FwVersionUpdated();

// Dialog Data
	//{{AFX_DATA(CLoadHex)
	enum { IDD = IDD_DOWNLOAD };
	CEdit	mMsgWin;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLoadHex)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void EndDownload(bool bSuccess);
	void EnterDownloadMode();
	void ManualDownloadStart();
	void SendLine();
	void PutMsg(const char *Msg);


	// Generated message map functions
	//{{AFX_MSG(CLoadHex)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	enum {
		INACTIVE,
		AUTO_START_WAIT,
		START_WAIT,
		LINE_WAIT,
		SET_COMM_19200,
		RESTORE_COMM_PARAMS,
	} mDownloadState;
	FILE *mDownloadFp;
	int mTimeoutTimer;
	int mXcatAdr;
	int mVerifiedBlocks;
	int mProgrammedBlocks;
	bool mDownloadSuccess;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LOADHEX_H__4FBCADAF_1DBE_4C46_AFE9_23D89AF7C91C__INCLUDED_)
