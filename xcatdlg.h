// $Log: xcatdlg.h,v $
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
   //}}AFX_MSG
   DECLARE_MESSAGE_MAP()

   void InitButtons();
   void OnSetConfig();
   void OnGetConfig();
   void EnableSplitItems();
   void OnRestoreCodePlug();
   void OnSaveCodePlug();
   void SendModeData();
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

// Dialog Data
   //{{AFX_DATA(ManualPage)
   enum { IDD = IDD_VFO };
   CComboBox   mTxOffset;
   CComboBox   mTxPL;
   CComboBox   mRxPL;
   double   mRxFrequency;
   double   mTxOffsetFreq;
   //}}AFX_DATA


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
   //}}AFX_MSG

   float mLastEncodePL;
   float mLastDecodePL;
   double mLastTxOffsetFreq;
   bool mForcedSet;
   void OnManualSet();
   void OnSaveMode();
   void OnRecallMode();
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
   //}}AFX_MSG
   void OnStart();
   void OnLockout();
   void ChangeFrequency();
   bool LockedOutFreq();
   void OnDelLockout();
   void ChangePL(bool bSetPl);
   void OnSkip();

   DECLARE_MESSAGE_MAP()

   int   mFreqStep;        // In hz
   bool  m_bListSelected;
   int mPlFreq;
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

// Dialog Data
   //{{AFX_DATA(CAbout)
   enum { IDD = IDD_ABOUTBOX };
   CStatic  mCompiled;
   //}}AFX_DATA

   bool m_bHaveFWVer;

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
   CCommSetup CCommSetup;
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
   UINT  mMode;
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
   //}}AFX_MSG
   DECLARE_MESSAGE_MAP()
};
