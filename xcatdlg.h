// $Log: xcatdlg.h,v $
// Revision 1.11  2009/01/06 23:23:42  Skip
// 1. Added support for low power transmit Power Control bit used by some T53 and
//    T83 model VHF radios.
// 2. Corrected error in calculation of code plug data for inverted transmit DPL
//    codes.
//
// Revision 1.10  2008/06/14 14:36:47  Skip
// 1. Added pDownloader member to CAbout class.
// 2. Added OnDownloadChar member to CXcatDlg class.
//
// Revision 1.9  2008/06/01 14:03:09  Skip
// Added UpdateIO5and7, OnUpdateFirmware.
//
// Revision 1.8  2008/02/02 17:58:22  Skip
// Added support for volume pot (not tested).
//
// Revision 1.7  2007/07/15 14:26:21  Skip
// Added squelch pot support.
//
// Revision 1.6  2007/01/02 17:28:56  Skip
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
// Revision 1.5  2005/01/08 19:31:30  Skip
// 1. Replaced CCommSetup with new dialog that allows XCat's address and
//    baudrate to be configured.
// 2. Replaced recall mode dialog with new class that supports labeling modes.
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
//
#if !defined(AFX_XCATDIALOG_H__712BDAC8_AF96_4012_A7AA_75BC2D3AC619__INCLUDED_)
#define AFX_XCATDIALOG_H__712BDAC8_AF96_4012_A7AA_75BC2D3AC619__INCLUDED_
#include "loadhex.h"

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// XcatDialog.h : header file
//

class CScanEnable;

/////////////////////////////////////////////////////////////////////////////
// CConfigure dialog

class CConfigure : public CPropertyPage
{
   DECLARE_DYNCREATE(CConfigure)

// Construction
public:
   CConfigure();
   ~CConfigure();
   void ConfigMsgRx(unsigned char *Config);
   void VCOSplits(unsigned char *Splits);
   void ModeData(unsigned char *Data);

// Dialog Data
   //{{AFX_DATA(CConfigure)
	enum { IDD = IDD_CONFIGURE };
	CComboBox	mOut7;
	CComboBox	mOut6;
	CComboBox	mOut4;
	CComboBox	mOut3;
   CStatic  mTransferStatus;
   CComboBox   mOut5;
   CComboBox   mOut2;
   CComboBox   mOut1;
   CComboBox   mOut0;
   CComboBox   mControlSys;
   CComboBox   mBand;
   double   mRxVcoSplitFreq;
   BOOL  mSendCosMsg;
   double   mTxVcoSplitFreq;
	BOOL	mUFasSquelch;
	BOOL	mEnableVolumePot;
	//}}AFX_DATA

   int mMode;
   FILE *mFp;
   bool m_bSaving;
   unsigned char mModeData[16];

// Overrides
   // ClassWizard generate virtual function overrides
   //{{AFX_VIRTUAL(CConfigure)
   public:
   virtual BOOL OnSetActive();
   protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   //}}AFX_VIRTUAL

// Implementation
protected:
   // Generated message map functions
   //{{AFX_MSG(CConfigure)
   afx_msg void OnRange1();
   afx_msg void OnRange2();
   afx_msg void OnSelchangeBand();
	afx_msg void OnSelchangeOut3();
	afx_msg void OnSelchangeOut4();
	afx_msg void OnSelchangeOut6();
	afx_msg void OnSelchangeControlSys();
	//}}AFX_MSG
   DECLARE_MESSAGE_MAP()

   void InitButtons();
   void OnSetConfig();
   void OnGetConfig();
   void EnableSplitItems();
   void OnRestoreCodePlug();
   void OnSaveCodePlug();
   void SendModeData();
	int  GetSelectedConfig();
	void SaveSplits();
	void OnSelchangeOut3_4_6(int NewSelection);
	void UpdateUFasSquelch();
	void UpdateEnableVolumePot();
	void UpdateIO5and7();
};

/////////////////////////////////////////////////////////////////////////////
// ManualPage dialog

class ManualPage : public CPropertyPage
{
   DECLARE_DYNCREATE(ManualPage)

// Construction
public:
   ManualPage();
   ~ManualPage();

	void ModeData();
	void UpdatePots();

// Dialog Data
   //{{AFX_DATA(ManualPage)
	enum { IDD = IDD_VFO };
	CSliderCtrl	mVolumePot;
	CSliderCtrl	mSquelchPot;
	CComboBox	mTxTimeout;
   CComboBox   mTxOffset;
   CComboBox   mTxPL;
   CComboBox   mRxPL;
   double   mRxFrequency;
   double   mTxOffsetFreq;
	BOOL	mLowPowerTx;
	//}}AFX_DATA
	bool mbRxDPL;
	bool mbInvRxDPL;
	bool mbTxDPL;
	bool mbInvTxDPL;

	bool mbTxCBMode;
	bool mbLastTxCBMode;
	
	bool mbRxCBMode;
	bool mbLastRxCBMode;

// Overrides
   // ClassWizard generate virtual function overrides
   //{{AFX_VIRTUAL(ManualPage)
   public:
   virtual BOOL OnSetActive();
   protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   //}}AFX_VIRTUAL

// Implementation
protected:
   // Generated message map functions
   //{{AFX_MSG(ManualPage)
   virtual BOOL OnInitDialog();
	afx_msg void OnRxPlEnable();
	afx_msg void OnTxDplEnable();
	afx_msg void OnRxDplEnable();
	afx_msg void OnTxPlEnable();
	afx_msg void OnTxDplEnableInv();
	afx_msg void OnRxDplEnableInv();
	afx_msg void OnSelchangeRxPl();
	afx_msg void OnSelchangeTxPl();
	afx_msg void OnSelchangeTxOffset();
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	//}}AFX_MSG

   float mLastEncodePL;
   float mLastDecodePL;
   double mLastTxOffsetFreq;
   bool mForcedSet;
   void OnManualSet();
   void OnSaveMode();
   void OnRecallMode();
	void UpdateRxCB();
	void UpdateTxCB();

   DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CScanEnable dialog

class CScanEnable : public CPropertyPage
{
   DECLARE_DYNCREATE(CScanEnable)

// Construction
public:
   CScanEnable();
   ~CScanEnable();
	void ModeData();

// Dialog Data
   //{{AFX_DATA(CScanEnable)
	enum { IDD = IDD_SCAN_ENABLE };
	CComboBox	m2ndPriorityMode;
	CComboBox	mPriorityMode;
	BOOL	m_bScanEnabled;
	BOOL	m_bTalkbackEnabled;
	BOOL	m_bFixedScan;
	//}}AFX_DATA


// Overrides
   // ClassWizard generate virtual function overrides
   //{{AFX_VIRTUAL(CScanEnable)
	public:
	virtual BOOL OnSetActive();
	protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
   // Generated message map functions
   //{{AFX_MSG(CScanEnable)
	virtual BOOL OnInitDialog();
	afx_msg void OnScanEnabled();
	afx_msg void OnFixedScan();
	afx_msg void OnSelchangePriorityChan();
	//}}AFX_MSG
   DECLARE_MESSAGE_MAP()

	void EnableDisableControls();
	void OnRecallMode();
	void OnSaveMode();
	void OnManualSet();

};
/////////////////////////////////////////////////////////////////////////////
// CBandScan dialog

class CBandScan : public CPropertyPage
{
   DECLARE_DYNCREATE(CBandScan)

// Construction
public:
   CBandScan();
   ~CBandScan();

   void OnTimer();
   void CarrierDetectChange(bool bHaveSignal);
   void SaveLockedOutList();

   bool m_bScanActive;
   bool m_bPlScan;
	bool m_bDPlScan;
   bool  m_bHaveSignal;
   double mCurrentFreq;     // In Mhz
   double mLastBottom;
   double mLastTop;

// Dialog Data
   //{{AFX_DATA(CBandScan)
   enum { IDD = IDD_BAND_SCAN };
   CListBox    mLockoutList;
   CComboBox   mStep;
   double   mBandScanTop;
   double   mBandScanBottom;
   CString  mScanFreq;
   double   mHangTime;
   CString  mPLFreqText;
   //}}AFX_DATA


// Overrides
   // ClassWizard generate virtual function overrides
   //{{AFX_VIRTUAL(CBandScan)
   public:
   virtual BOOL OnSetActive();
   virtual BOOL OnKillActive();
   protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   //}}AFX_VIRTUAL

// Implementation
protected:
   void InitButtons();
   // Generated message map functions
   //{{AFX_MSG(CBandScan)
   virtual BOOL OnInitDialog();
   afx_msg void OnSetfocusBandScanTop();
   afx_msg void OnSetfocusBandScanBottom();
   afx_msg void OnSetfocusBandScanStep();
   afx_msg void OnSetfocusLockoutList();
   afx_msg void OnDestroy();
   afx_msg void OnBandScan();
   afx_msg void OnDoPlScan();
	afx_msg void OnDoDplScan();
	//}}AFX_MSG
   void OnStart();
   void OnLockout();
   void ChangeFrequency();
   bool LockedOutFreq();
   void OnDelLockout();
   void ChangePL(bool bSetPl);
	void ChangeDPL();
   void OnSkip();

   DECLARE_MESSAGE_MAP()

   int   mFreqStep;        // In hz
   bool  m_bListSelected;
   int mPlFreq;
   int mDPlFreq;
};

/////////////////////////////////////////////////////////////////////////////
// CCommSetup1 dialog

class CCommSetup1 : public CPropertyPage
{
   DECLARE_DYNCREATE(CCommSetup1)

// Construction
public:
	CCommSetup1();   // standard constructor

// Dialog Data
	//{{AFX_DATA(CCommSetup1)
	enum { IDD = IDD_COMM_SETUP1 };
	CString	mXCatAdr;
	//}}AFX_DATA

	int mNewXcatAdr;
	int mNewBaudrate;
	int mNewComPort;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCommSetup1)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	#define MAX_COM_PORTS	16
	int mPortLookup[MAX_COM_PORTS];

	// Generated message map functions
	//{{AFX_MSG(CCommSetup1)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	void OnSet();
};

/////////////////////////////////////////////////////////////////////////////
// CCommSetup dialog

class CCommSetup : public CPropertyPage
{
   DECLARE_DYNCREATE(CCommSetup)

// Construction
public:
   CCommSetup();
   ~CCommSetup();

// Dialog Data
   BYTE mComPort;

   //{{AFX_DATA(CCommSetup)
	enum { IDD = IDD_COMM_SETUP };
	CString	mXCatAdr;
	//}}AFX_DATA


// Overrides
   // ClassWizard generate virtual function overrides
   //{{AFX_VIRTUAL(CCommSetup)
	public:
   virtual BOOL OnSetActive();
	protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
   // Generated message map functions
   //{{AFX_MSG(CCommSetup)
      // NOTE: the ClassWizard will add member functions here
   afx_msg void OnCom1();
   afx_msg void OnCom2();
   afx_msg void OnCom3();
   afx_msg void OnCom4();
   afx_msg void OnProperties();
   virtual BOOL OnInitDialog();
   //}}AFX_MSG
   DECLARE_MESSAGE_MAP()

	void OnSet();

   void GrayCommButtons();
   static int ComPortResourceID[];
};

/////////////////////////////////////////////////////////////////////////////
// CAbout dialog

class CAbout : public CPropertyPage
{
   DECLARE_DYNCREATE(CAbout)

// Construction
public:
   CAbout();
   ~CAbout();
	CLoadHex *pDownloader;

// Dialog Data
   //{{AFX_DATA(CAbout)
   enum { IDD = IDD_ABOUTBOX };
   CStatic  mCompiled;
   //}}AFX_DATA

// Overrides
   // ClassWizard generate virtual function overrides
   //{{AFX_VIRTUAL(CAbout)
   public:
   virtual BOOL OnSetActive();
   protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   //}}AFX_VIRTUAL

// Implementation
protected:
   // Generated message map functions
   //{{AFX_MSG(CAbout)
   virtual BOOL OnInitDialog();
	afx_msg void OnUpdateFirmware();
	//}}AFX_MSG
   DECLARE_MESSAGE_MAP()

};

/////////////////////////////////////////////////////////////////////////////
// CDebugMsgs dialog

class CDebugMsgs : public CPropertyPage
{
   DECLARE_DYNCREATE(CDebugMsgs)

// Construction
public:
   CDebugMsgs();
   ~CDebugMsgs();
   void ModeData(unsigned char *Data);
   void SignalReport(int Mode,int bSignal);
	void SyncDebugData(unsigned char *Data);

// Dialog Data
   //{{AFX_DATA(CDebugMsgs)
   enum { IDD = IDD_DEBUG};
   CEdit mEdit;
   //}}AFX_DATA


// Overrides
   // ClassWizard generate virtual function overrides
   //{{AFX_VIRTUAL(CDebugMsgs)
   public:
   virtual BOOL OnSetActive();
   protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   //}}AFX_VIRTUAL

// Implementation
protected:
	void OnGetSyncData();
	void OnGetCodePlugData();
	void OnGetSigReport();

   // Generated message map functions
   //{{AFX_MSG(CDebugMsgs)
      // NOTE: the ClassWizard will add member functions here
   //}}AFX_MSG
   DECLARE_MESSAGE_MAP()

};

// CXcatDlg

class CXcatDlg : public CPropertySheet
{
   DECLARE_DYNAMIC(CXcatDlg)

// Construction
public:
   CXcatDlg(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

// Attributes

public:

   ManualPage ManualPage;
   CScanEnable ScanPage;
   CBandScan BandScan;
   CCommSetup1 CommSetup;
   CDebugMsgs DebugMsgs;
   CConfigure Configure;
   CAbout CAbout;

// Operations
public:

// Overrides
   // ClassWizard generated virtual function overrides
   //{{AFX_VIRTUAL(CXcatDlg)
   public:
   virtual BOOL OnInitDialog();
   protected:
   //}}AFX_VIRTUAL

// Implementation
public:
   virtual ~CXcatDlg();

   // Generated message map functions
protected:
   //{{AFX_MSG(CXcatDlg)
   afx_msg void OnPaint();
   afx_msg HCURSOR OnQueryDragIcon();
   afx_msg void OnTimer(UINT nIDEvent);
   //}}AFX_MSG
   DECLARE_MESSAGE_MAP()

   bool bHaveSignal;
   time_t   SignalLostTime;

   LRESULT OnRxMsg(WPARAM wParam,LPARAM lParam);
   LRESULT OnCommError(WPARAM /* wParam*/, LPARAM lParam);
	LRESULT OnDownloadChar(WPARAM /* wParam*/, LPARAM lParam);
};

/////////////////////////////////////////////////////////////////////////////
#endif // !defined(AFX_XCATDIALOG_H__712BDAC8_AF96_4012_A7AA_75BC2D3AC619__INCLUDED_)
/////////////////////////////////////////////////////////////////////////////
// CModeSel dialog

class CModeSel : public CDialog
{
// Construction
public:
	CModeSel(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CModeSel)
	enum { IDD = IDD_MODE_SEL };
	CComboBox	mMode;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CModeSel)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CModeSel)
	afx_msg void OnEditName();
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////
// CEditModeName dialog

class CEditModeName : public CDialog
{
// Construction
public:
	CEditModeName(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CEditModeName)
	enum { IDD = IDD_NAME_EDIT };
	CString	mName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEditModeName)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CEditModeName)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////
// CXcatFileDialog dialog

class CXcatFileDialog : public CFileDialog
{
	DECLARE_DYNAMIC(CXcatFileDialog)

public:
	CXcatFileDialog(BOOL bOpenFileDialog, // TRUE for FileOpen, FALSE for FileSaveAs
		LPCTSTR lpszDefExt = NULL,
		LPCTSTR lpszFileName = NULL,
		DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		LPCTSTR lpszFilter = NULL,
		CWnd* pParentWnd = NULL);

protected:
	BOOL OnFileNameOK();
	//{{AFX_MSG(CXcatFileDialog)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
