// $Log: comm.h,v $
// Revision 1.1  2004/07/09 23:12:30  Skip
// Initial revision
//
//
#define ICOM_MIN_MSG_SIZE  6
#define ICOM_MAX_MSG_SIZE  1024  /* ? */
#define ICOM_MAX_DATA_SIZE 17

typedef struct {
   BYTE  Fe_1;
   BYTE  Fe_2;
   BYTE  To;
   BYTE  From;
   BYTE  Cmd;
} CI_V_Hdr;

typedef struct {
   CI_V_Hdr Hdr;
   BYTE  Data[ICOM_MAX_DATA_SIZE];
} CI_VMsg;

typedef struct {
   CI_V_Hdr Hdr;
   BYTE  Freq[5]; // Frequency in BCD.  LSB first
} CI_V_SetFreqMsg;

typedef struct AppMsgTAG {
   struct AppMsgTAG *Link;
   int   DataLen;       // actual length of Body
   CI_V_Hdr Hdr;
   BYTE  Data[ICOM_MAX_DATA_SIZE];
} AppMsg;


enum COMM_STATE {
   IDLE = 1,
   SENDING,
   RX_WAIT,
   RECEIVING,
   TX_WAIT,
};

enum RX_STATE {
   SYNC1_WAIT = 1,
   SYNC2_WAIT,
   GET_TO,
   GET_DATA,
   GET_ESC_DATA,
   WAIT_END
};

enum TX_STATE{
   TX_IDLE = 1,
   TX_SENDING
};

class Comm {
public:
   // interface
   friend class CComstats1;
   Comm();
   ~Comm();
   BOOL Init(int ComPort = -1, int BaudRate = -1);
   void ClearStats();
   void SendMessage(AppMsg *pMsg);
   void SetFreq(double Freq);
   void SetCTSSFreq(double Freq,BYTE Encode);
   void RequestFWVer();
   void GetModeData();
   void GetTxOffset();
   void SetTxOffset(int TxOffset);
   void GetVCOSplits();
   void SetVCOSplits(unsigned int RxSplitF,unsigned int TxSplitF);
   void SetVCOSplits(int RxSplitF,int TxSplitF);
   void GetConfig();
   void SetConfig(unsigned char *Config);
   void SetDuplex(unsigned char DuplexType);
   void RecallMode();
   void StoreVFO();
   void SelectMode(unsigned char Mode);
   void SetModeData(unsigned char *Data);

public:
   // Attributes
   BYTE mOurAdr;

protected:

   //Communications stuff:

   int mComPort;        // Our COM port number
   HANDLE mComDev;      // handle of our COM port
   DCB mDcb;            // device control block for our COM port

   COMM_STATE mComState;

   // communications statistics

   long mTxMsgCount;    // Calls to send2rack()
   long mSendErrors;
   long mTxFramesSent;  // Includes retrys

   long mRxMsgCount;
   long mUnknownMsgs;
   long mLineErrors;
   long mGarbageCharCount;
   long mNoHandlerCount;
   long mRxEventsLost;
   long mTxEventsLost;
   long mIllegalFrame;
   long mTooBig;
   long mTotalRawBytes;
   long mTotalReadCalls;

// CComstats1* m_pComStats;

   // receive variables

   RX_STATE mRxState;
   TX_STATE mTxState;
   BYTE* m_pRxBuf;
   int mRxCount;
   char Rx_v0;
   char Rx_v1;
   AppMsg *mTxHead;
   AppMsg *mTxTail;
   DWORD mBytesSent;
   OVERLAPPED mTxOverlapped;
   OVERLAPPED mRxOverlapped;

   HANDLE mIOWaitHandleArray[3];

   int mIOThreadEnable;
   DWORD mBytesRead;
   CCriticalSection mTxQueueLock;
   BYTE mRxBuf[ICOM_MAX_MSG_SIZE];
   BYTE mRxMsg[ICOM_MAX_MSG_SIZE];
   DWORD mBytesToRead;
   int mDataCount;
   int mRetries;
   int mTicker;
   bool mLastWasRetry;
   struct _timeb mTxTimeout;
   bool m_bReportErrors;
   CWnd* m_pMainWnd;

   // implementation
protected:
   void ProcessRx();
   void SendNextMessage(bool bRetry = FALSE);
   static UINT RxThreadControl(LPVOID pCommClass);
   void IOThreadMainLoop();
   void DeleteTxMsg();
};

extern Comm CComm;
