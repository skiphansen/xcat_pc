// $Log: Comm.cpp,v $
// Revision 1.1  2004/07/09 23:12:30  Skip
// Initial revision
//
//
#include "stdafx.h"
#include "resource.h"
#include "xcat.h"
#include "comm.h"

Comm CComm;
#define MAX_RETRIES  5
int tracecounter = 0;

// Set time in pTimeout TimeoutMs milliseconds from now
void SetTimeout(struct _timeb *pTimeout,int TimeoutMs)
{
   struct _timeb TimeNow;
   _ftime(&TimeNow);

   __int64 x;
   x = ((__int64) TimeNow.time * 1000) + TimeNow.millitm + TimeoutMs;

   pTimeout->time = (time_t) (x / 1000);
   pTimeout->millitm = (unsigned short) (x % 1000);
}

Comm::Comm()
{
   mComDev = INVALID_HANDLE_VALUE;
   mRxState = SYNC1_WAIT;
   mTxState = TX_IDLE;
// m_pComStats = NULL;
   mTxHead = NULL;
   mTxTail = NULL;
   mOurAdr = 0xe0;
   m_bReportErrors = TRUE;

   // create the events for overlapped read and write and thread control

   mRxOverlapped.hEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
   ASSERT(mRxOverlapped.hEvent != NULL);

   mTxOverlapped.hEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
   ASSERT(mTxOverlapped.hEvent != NULL);

   mIOWaitHandleArray[0] = mRxOverlapped.hEvent;
   mIOWaitHandleArray[1] = mTxOverlapped.hEvent;
   mIOWaitHandleArray[2] = CreateEvent(NULL,FALSE,FALSE,NULL);
   ASSERT(mIOWaitHandleArray[2] != NULL);
   mLastWasRetry = FALSE;
}

Comm::~Comm()
{
   if(mComDev != INVALID_HANDLE_VALUE){
      // Kill the IO Thread
      mIOThreadEnable = 2;

      if(!PurgeComm(mComDev,PURGE_TXABORT | PURGE_RXABORT)){
         ASSERT(FALSE);
      }
      // Wait for the IO Thread to terminate

      while(mIOThreadEnable == 2);

      CloseHandle(mComDev);
      CloseHandle(mRxOverlapped.hEvent);
      CloseHandle(mTxOverlapped.hEvent);
      CloseHandle(mIOWaitHandleArray[2]);
   }
}

BOOL Comm::Init(int ComPort, int Baudrate)
{
   // init variables

   ClearStats();
   mComState = IDLE;
   m_pMainWnd = AfxGetApp()->m_pMainWnd;

   if(mComDev != INVALID_HANDLE_VALUE){
      // reinit, close the old port

      // Kill the IO Thread
      mIOThreadEnable = 2;

      if(!PurgeComm(mComDev,PURGE_TXABORT | PURGE_RXABORT)){
         ASSERT(FALSE);
      }

      // Wait for the IO Thread to terminate

      while(mIOThreadEnable == 2);

      if(!CloseHandle(mComDev))
         ASSERT(FALSE);
      mComDev = INVALID_HANDLE_VALUE;
   }

   if(ComPort == -1) {
      ComPort = theApp.GetProfileInt("Settings","Port",-1);
   }

   if(Baudrate <= 0) {
      Baudrate = theApp.GetProfileInt("Settings","Baudrate",-1);
   }

   if(ComPort == -1 || Baudrate <= 0) {
      return FALSE;
   }

   char ComPortName[6];
   sprintf(ComPortName,"COM%d",ComPort);
   LOG(("Opening %s at %d baud.\n",ComPortName,Baudrate));
   mComDev = CreateFile(ComPortName,GENERIC_READ | GENERIC_WRITE,0,NULL,
                   OPEN_EXISTING,FILE_FLAG_OVERLAPPED,0);
   if(mComDev == INVALID_HANDLE_VALUE){
      LOG(("Open failed.\n"));
      return FALSE;
   }
   if(!GetCommState(mComDev,&mDcb)){
      LOG(("GetCommState() failed.\n"));
      CloseHandle(mComDev);
      mComDev = INVALID_HANDLE_VALUE;
      return FALSE;
   }

   mDcb.BaudRate = Baudrate;     /* Baudrate */
   mDcb.fBinary = 1;    /* Binary Mode (skip EOF check)    */
   mDcb.fParity = 0;    /* Disable parity checking          */
   mDcb.fOutxCtsFlow = 0;  /* No CTS handshaking on output    */
   mDcb.fOutxDsrFlow = 0;  /* DSR handshaking on output       */
   mDcb.fDtrControl = 0;   /* DTR Flow control                */
   mDcb.fDsrSensitivity = 0; /* DSR Sensitivity              */
   mDcb.fTXContinueOnXoff = 1; /* Continue TX when Xoff sent */
   mDcb.fOutX = 0;         /* Disable output X-ON/X-OFF        */
   mDcb.fInX = 0;       /* Enable input X-ON/X-OFF         */
   mDcb.fErrorChar = 0; /* Enable Err Replacement          */
   mDcb.fNull = 0;         /* Enable Null stripping           */
   mDcb.fRtsControl = RTS_CONTROL_TOGGLE;  /* Rts Flow control                */
   mDcb.fAbortOnError = 1; /* Abort all reads and writes on Error */
   mDcb.ByteSize = 8;
   mDcb.Parity = NOPARITY;
   mDcb.StopBits = ONESTOPBIT;

   if(!SetCommState(mComDev,&mDcb)){
      DWORD err = GetLastError();
      LOG(("SetCommState() failed: %d.\n",err));
      ASSERT(FALSE);
      CloseHandle(mComDev);
      mComDev = INVALID_HANDLE_VALUE;
      return FALSE;
   }

   if(!SetupComm(mComDev,1024,1024)){
      DWORD err = GetLastError();
      LOG(("SetupComm() failed: %d.\n",err));
      ASSERT(FALSE);
   }
   mComPort = ComPort;
   
   COMMTIMEOUTS   commTimeouts; // Timeout structure for our COM port
   if (!GetCommTimeouts(mComDev, &commTimeouts)) {
      DWORD err = GetLastError();
      return 1;
   }

/* Make sure comm timeouts are disabled! */

   commTimeouts.ReadIntervalTimeout = 0;
   commTimeouts.ReadTotalTimeoutConstant = 0;
   commTimeouts.ReadTotalTimeoutMultiplier = 0;
   commTimeouts.WriteTotalTimeoutMultiplier = 0;

   if (!SetCommTimeouts(mComDev, &commTimeouts)) {
      DWORD err = GetLastError();
      return 1;
   }

   // Start the IO Thread

   CWinThread* IOThread;

   mIOThreadEnable = 1;
   IOThread = AfxBeginThread(RxThreadControl,(LPVOID) this,THREAD_PRIORITY_ABOVE_NORMAL);

   ASSERT(IOThread != NULL);
   return TRUE;
}

void Comm::ClearStats()
{
   mTxMsgCount = 0;
   mRxMsgCount = 0;
   mSendErrors = 0;
   mUnknownMsgs = 0;
   mLineErrors = 0;
   mNoHandlerCount = 0;
   mGarbageCharCount = 0;
   mRxEventsLost = 0;
   mTxEventsLost = 0;
   mIllegalFrame = 0;
   mTooBig = 0;
   mTotalRawBytes = 0;
   mTotalReadCalls = 0;
   mTxFramesSent = 0;
}

void Comm::SendMessage(AppMsg *pMsg)
{
   pMsg->Link = NULL;

   mTxQueueLock.Lock();
   if(mTxHead == NULL) {
      mTxHead = pMsg;
      mTxTail = pMsg;
      mTxQueueLock.Unlock();
      SendNextMessage(FALSE);
   }
   else {
      mTxTail->Link = pMsg;
      mTxTail = pMsg;
      mTxQueueLock.Unlock();
   }
}

void Comm::SendNextMessage(bool bRetry)
{
   ASSERT(mTxState == TX_IDLE);
   BYTE Temp[ICOM_MAX_MSG_SIZE];

   AppMsg *pMsg = mTxHead;
   ASSERT(pMsg != NULL);

   if(!bRetry) {
      mRetries = -1;
   }

   if(++mRetries > MAX_RETRIES) {
      // oops !
      if(m_bReportErrors) {
         CString *pErrMsg = new CString;
         pErrMsg->Format("The Xcat is not responding.\n"
                         "Please check the cable and\n"
                         "make sure the radio is\n"
                         "turned on.\n\n"
                         "Command: 0x%02X, 0x%02X\n",
                         pMsg->Hdr.Cmd,pMsg->Data[0]);
         if(!m_pMainWnd->PostMessage(ID_COMM_ERROR,0,(LPARAM) pErrMsg)) {
            ASSERT(FALSE);
         }
         m_bReportErrors = FALSE;
      }
      DeleteTxMsg();
   }
   else {
      mTicker = 0;

   // escape any special characters in the message

      memcpy(Temp,&pMsg->Hdr,sizeof(pMsg->Hdr));
      int j = sizeof(pMsg->Hdr);
      int LoopCount = pMsg->DataLen - sizeof(pMsg->Hdr) - 1;

      for(int i = 0; i < LoopCount; i++) {
         if(pMsg->Data[i] >= 0xfd) {
            Temp[j++] = 0xff;
            Temp[j++] = pMsg->Data[i] & 0xf;
         }
         else {
            Temp[j++] = pMsg->Data[i];
         }
      }
      Temp[j++] = pMsg->Data[i];

      if(bRetry) {
         TRACE3("resending %d byte message 0x%x, 0x%x\n",j,Temp[4],Temp[5]);
         LOG(("resending:\n"));

      }
      else {
         TRACE3("sending %d byte message 0x%x, 0x%x\n",j,Temp[4],Temp[5]);
         LOG(("sending:\n"));
      }
      LOG_HEX(Temp,j);
      mComState = SENDING;
      mTxState = TX_SENDING;
      if(!WriteFile(mComDev,Temp,j,&mBytesSent,&mTxOverlapped)){
         DWORD err = GetLastError();
         if(err != ERROR_IO_PENDING){
            ASSERT(FALSE);
            mSendErrors++;
            mComState = IDLE;
            mTxState = TX_IDLE;
         }
         else {
            mTxFramesSent++;
         }
      }
      SetTimeout(&mTxTimeout,250);
   }
}


UINT Comm::RxThreadControl(LPVOID pCommClass)
{
   ((Comm *) pCommClass)->IOThreadMainLoop();

   return TRUE;
}

void Comm::IOThreadMainLoop()
{
   DWORD Event;
   DWORD err;
   BOOL DidSomething;

   // Kick off the receiver

   ProcessRx();

   while(mIOThreadEnable == 1) {
      DWORD WaitTimeout;

      if(mTxTimeout.time == 0) {
         WaitTimeout = INFINITE;
      }
      else {
         struct _timeb TimeNow;
         _ftime(&TimeNow);

         __int64 x;
         x = ((mTxTimeout.time - TimeNow.time) * 1000) +
             mTxTimeout.millitm - TimeNow.millitm;
         
         if(x > 0) {
            WaitTimeout = (DWORD) x;
         }
         else {
            WaitTimeout = 0;
         }
      }
      
      Event = WaitForMultipleObjects(3,mIOWaitHandleArray,FALSE,WaitTimeout);
      if(Event == WAIT_FAILED){
         err = GetLastError();
         ASSERT(FALSE);
      }
      do {
         DidSomething = FALSE;
         if(mTxState == TX_SENDING) {
            if(!GetOverlappedResult(mComDev,&mTxOverlapped,&mBytesSent,FALSE)){
               err = GetLastError();
               if(err != ERROR_IO_INCOMPLETE){
                  ASSERT(FALSE);
               }
            }
            else{
               // Write has completed
               if(!ResetEvent(mTxOverlapped.hEvent)){
                  err = GetLastError();
                  ASSERT(FALSE);
               }
               DidSomething = TRUE;
               mComState = RX_WAIT;
               mTxState = TX_IDLE;
            }
         }

         if(!GetOverlappedResult(mComDev,&mRxOverlapped,&mBytesRead,FALSE)){
            err = GetLastError();
            if(err != ERROR_IO_INCOMPLETE && err != ERROR_OPERATION_ABORTED) {
               ASSERT(FALSE);
            }
         }
         else{
            // Read has completed
            if(!ResetEvent(mRxOverlapped.hEvent)){
               err = GetLastError();
               ASSERT(FALSE);
            }
            ProcessRx();
            DidSomething = TRUE;
         }

         if(mTxTimeout.time != 0) {
         // Check for response timeout
            struct _timeb TimeNow;
            _ftime(&TimeNow);

            __int64 x;
            x = ((mTxTimeout.time - TimeNow.time) * 1000) +
                mTxTimeout.millitm - TimeNow.millitm;
            if(x <= 0) {
            // Response timeout
               LOG(("Reponse timeout.\n"));
               TRACE("T");
               SendNextMessage(TRUE);
            }
         }

      } while(DidSomething);
   }
   mIOThreadEnable = 0;
}

void Comm::DeleteTxMsg()
{
   mTxQueueLock.Lock();

   mTxTimeout.time = 0;
   AppMsg *pMsg = mTxHead;
   if(pMsg != NULL) {
      mTxHead = pMsg->Link;
      mTxQueueLock.Unlock();
      delete pMsg;
      if(mTxHead != NULL) {
         SendNextMessage(FALSE);
      }
   }
   else {
      mTxQueueLock.Unlock();
      ASSERT(FALSE);
   }
}

void Comm::ProcessRx()
{
   for( ; ; ) {
      mTotalRawBytes += mBytesRead;
      for(DWORD i = 0; i < mBytesRead; i++){
         BYTE RxByte = mRxBuf[i];
         if(RxByte == 0xfe && mRxState != SYNC1_WAIT && mRxState != SYNC2_WAIT) {
            LOG(("0xfe received while in middle of receiving frame:\n"));
            LOG_HEX(mRxMsg,mRxCount);
            mRxCount = 0;
            mRxState = SYNC1_WAIT;
         }
         mRxMsg[mRxCount++] = RxByte;
         switch(mRxState) {
            case SYNC1_WAIT:
               if(RxByte == 0xfe) {
                  mRxState = SYNC2_WAIT;
               }
               else {
                  mRxCount = 0;
               }
               break;

            case SYNC2_WAIT:
               if(RxByte == 0xfe) {
                  mRxState = GET_TO;
               }
               else {
                  mRxCount = 0;
                  mRxState = SYNC1_WAIT;
               }
               break;

            case GET_TO:
               if(RxByte == 0 || RxByte == mOurAdr) {
                  mRxState = GET_DATA;
               }
               else {
                  mRxState = WAIT_END;
               }
               break;

            case GET_DATA:
               if(RxByte == 0xfd) {
               // We have a complete message
                  AppMsg *pMsg = (AppMsg *) new char[sizeof(AppMsg)+mRxCount];
                  LOG(("Received:\n"));
                  LOG_HEX(mRxMsg,mRxCount);
                  if(pMsg != NULL) {
                     if(!m_bReportErrors) {
                        m_bReportErrors = TRUE; // turn back on error reporting
                        CString *pErrMsg = new CString;
                        pErrMsg->Format("Communications (re)established\n"
                                        "with the Xcat.",
                                        pMsg->Hdr.Cmd,pMsg->Data[0]);
                        if(!m_pMainWnd->PostMessage(ID_COMM_ERROR,0,
                                                    (LPARAM) pErrMsg)) 
                        {
                           ASSERT(FALSE);
                        }
                     }

                     pMsg->DataLen = mRxCount;
                     memcpy(&pMsg->Hdr,mRxMsg,mRxCount);
                     if(pMsg->Hdr.Cmd == 0xfb) {
                     // Ack
                        if(tracecounter++ > 64) {
                           tracecounter = 0;
                           TRACE("A\n");
                        }
                        else {
                           TRACE("A");
                        }
                        delete pMsg;
                        DeleteTxMsg();
                     }
                     else if(pMsg->Hdr.Cmd == 0xfa) {
                     // Nak
                        if(mTxHead != NULL && mTxHead->Hdr.Cmd != 5) {
                           CString *pErrMsg = new CString;
                           pErrMsg->Format("The Xcat is NAK'ed\n"
                                           "Command: 0x%02X, 0x%02X\n",
                                           mTxHead->Hdr.Cmd,mTxHead->Data[0]);
                           if(!m_pMainWnd->PostMessage(ID_COMM_ERROR,0,
                                                       (LPARAM) pErrMsg)) 
                           {
                              ASSERT(FALSE);
                           }
                        }
                        if(tracecounter++ > 64) {
                           tracecounter = 0;
                           TRACE("N\n");
                        }
                        else {
                           TRACE("N");
                        }
                        ASSERT(FALSE);
                        delete pMsg;
                        DeleteTxMsg();
                     }
                     else {
                        if(pMsg->Hdr.To == mOurAdr) {
                        // Must be something we asked for
                           DeleteTxMsg();
                        }

                        if(!m_pMainWnd->PostMessage(ID_RX_MSG,0,(LPARAM) pMsg)){
                           ASSERT(FALSE);
                        }
                     }
                  }

                  mRxCount = 0;
                  mRxState = SYNC1_WAIT;
               }
               else if(RxByte == 0xff) {
               // escape character
                  mRxCount--;
                  mRxState = GET_ESC_DATA;
               }
               break;

            case GET_ESC_DATA:
               mRxMsg[mRxCount - 1] |= 0xf0;
               mRxState = GET_DATA;
               break;

            case WAIT_END:
               mRxCount = 0;
               if(RxByte == 0xfd) {
                  mRxState = SYNC1_WAIT;
               }
               break;

            default:
               ASSERT(FALSE);
         }

         if(mRxState != WAIT_END && mRxCount == sizeof(mRxMsg)) {
         // buffer overflow
            mTooBig++;
            mRxState = WAIT_END;
         }
      }

      // Set the amount to read next
      switch(mRxState){
         case SYNC1_WAIT:
            mBytesToRead = ICOM_MIN_MSG_SIZE;
            break;

         case SYNC2_WAIT:
            mBytesToRead = ICOM_MIN_MSG_SIZE - 1;
            break;

         case GET_TO:
            mBytesToRead = ICOM_MIN_MSG_SIZE - 2;
            break;

         case GET_DATA:
            if(mRxCount < ICOM_MIN_MSG_SIZE) {
               mBytesToRead = ICOM_MIN_MSG_SIZE - mRxCount;
            }
            else {
               mBytesToRead = 1;
            }
            break;

         case GET_ESC_DATA:
         case WAIT_END:
            mBytesToRead = 1;
            break;

         default:
            ASSERT(FALSE);
      }

      DWORD RxErrors;
      COMSTAT ComStat;

      if(!ClearCommError(mComDev,&RxErrors,&ComStat)){
         DWORD err = GetLastError();
         ASSERT(FALSE);
      }

      if(RxErrors & CE_RXOVER){
         mRxEventsLost++;
      }
      if(RxErrors & CE_OVERRUN){
         mRxEventsLost++;
      }
      if(RxErrors & CE_TXFULL){
         mTxEventsLost++;
      }

      /*

  add code to test the following:

CE_BREAK The hardware detected a break condition.
CE_FRAME The hardware detected a framing error.
CE_IOE   An I/O error occurred during communications with the device.
CE_MODE  The requested mode is not supported, or the hFile parameter is invalid. If this value is specified, it is the only valid error.
CE_TXFULL   The application tried to transmit a character, but the output buffer was full.
*/
      // Read the amount of data needed to complete this message or
      // everything that is there, whichever is greater.

      mBytesToRead = max(mBytesToRead,ComStat.cbInQue);
      mBytesToRead = min(mBytesToRead,sizeof(mRxBuf));
      mBytesRead = 0;

      mTotalReadCalls++;
      if(!ReadFile(mComDev,mRxBuf,1 /* mBytesToRead*/ ,&mBytesRead,&mRxOverlapped)){
         DWORD err = GetLastError();
         if(err == ERROR_IO_PENDING){
            return;
         }
         else {
            ASSERT(FALSE);
         }
      }
   }
}


void InitMsgHeader(CI_V_Hdr *pHdr,BYTE To,BYTE Cmd)
{
   pHdr->Fe_1 = 0xfe;
   pHdr->Fe_2 = 0xfe;
   pHdr->To   = To;
   pHdr->From = CComm.mOurAdr;
   pHdr->Cmd  = Cmd;
}

/* Command 5: write frequency data to vfo or memory

  Cmd     Data_0        Data_1         Data_2           Data_3
 <0x5> [10Hz | 1Hz] [1Khz | 100Hz] [100Khz | 10Khz] [10Mhz | 1Mhz]

     Data_4
 [1Ghz | 100 Mhz]
*/
void Comm::SetFreq(double Freq)
{
   AppMsg *pMsg = new AppMsg;

   if(pMsg != NULL) {
      memset(pMsg,0,sizeof(AppMsg));
#if 0
      InitMsgHeader(&pMsg->Hdr,0x20,0xaa);
      pMsg->Data[0] = 0;
      pMsg->Data[1] = 0xfd;
      pMsg->DataLen = sizeof(CI_V_Hdr) + 2;
#else
      InitMsgHeader(&pMsg->Hdr,0x20,5);

      int iFreq = (int) ((Freq * 1000000.0) + 0.5);

      for(int i = 0; i < 5; i++) {
         pMsg->Data[i] = (BYTE) (iFreq % 10);
         iFreq /= 10;
         pMsg->Data[i] |= (BYTE) ((iFreq % 10) << 4);
         iFreq /= 10;
      }
      pMsg->Data[i] = 0xfd;
      pMsg->DataLen = sizeof(CI_V_Hdr) + 6;
#endif

      SendMessage(pMsg);
   }
}


/* Command 1B:

  Cmd     Data_0         Data_1         Data_2
 <0x1b> [0x00 | 0x01] [100Hz | 10Hz] [1Hz | .1Hz]

 Data_0
 0x00 - set encode CTSS frequency
 0x01 - set decode CTSS frequency
*/
void Comm::SetCTSSFreq(double Freq,BYTE Decode)
{
   AppMsg *pMsg = new AppMsg;

   if(pMsg != NULL) {
      memset(pMsg,0,sizeof(AppMsg));
      InitMsgHeader(&pMsg->Hdr,0x20,0x1b);

      int iFreq = (int) ((Freq * 10.0) + 0.5);

      pMsg->Data[0] = Decode;

      for(int i = 1; i < 3; i++) {
         pMsg->Data[3-i] = (BYTE) (iFreq % 10);
         iFreq /= 10;
         pMsg->Data[3-i] |= (BYTE) ((iFreq % 10) << 4);
         iFreq /= 10;
      }
      pMsg->Data[i++] = 0xfd;
      pMsg->DataLen = sizeof(CI_V_Hdr) + i;

      SendMessage(pMsg);
   }
}

void Comm::RequestFWVer()
{
   AppMsg *pMsg = new AppMsg;

   if(pMsg != NULL) {
      memset(pMsg,0,sizeof(AppMsg));
      InitMsgHeader(&pMsg->Hdr,0x20,0xaa);
      pMsg->Data[0] = 2;
      pMsg->Data[1] = 0xfd;
      pMsg->DataLen = sizeof(CI_V_Hdr) + 2;
      SendMessage(pMsg);
   }
}

void Comm::GetModeData()
{
   AppMsg *pMsg = new AppMsg;

   if(pMsg != NULL) {
      memset(pMsg,0,sizeof(AppMsg));
      InitMsgHeader(&pMsg->Hdr,0x20,0xaa);
      pMsg->Data[0] = 0;
      pMsg->Data[1] = 0xfd;
      pMsg->DataLen = sizeof(CI_V_Hdr) + 2;
      SendMessage(pMsg);
   }
}

void Comm::GetTxOffset()
{
   AppMsg *pMsg = new AppMsg;

   if(pMsg != NULL) {
      memset(pMsg,0,sizeof(AppMsg));
      InitMsgHeader(&pMsg->Hdr,0x20,0xaa);
      pMsg->Data[0] = 5;
      pMsg->Data[1] = 0xfd;
      pMsg->DataLen = sizeof(CI_V_Hdr) + 2;
      SendMessage(pMsg);
   }
}

void Comm::SetTxOffset(int TxOffset)
{
   AppMsg *pMsg = new AppMsg;

   if(pMsg != NULL) {
      memset(pMsg,0,sizeof(AppMsg));
      InitMsgHeader(&pMsg->Hdr,0x20,0xaa);
      pMsg->Data[0] = 6;
      for(int i = 1; i < 5; i++) {
         pMsg->Data[i] = (BYTE) (TxOffset & 0xff);
         TxOffset >>= 8;
      }
      pMsg->Data[i++] = 0xfd;
      pMsg->DataLen = sizeof(CI_V_Hdr) + i;
      SendMessage(pMsg);
   }
}

void Comm::GetVCOSplits()
{
   AppMsg *pMsg = new AppMsg;

   if(pMsg != NULL) {
      memset(pMsg,0,sizeof(AppMsg));
      InitMsgHeader(&pMsg->Hdr,0x20,0xaa);
      pMsg->Data[0] = 7;
      pMsg->Data[1] = 0xfd;
      pMsg->DataLen = sizeof(CI_V_Hdr) + 2;
      SendMessage(pMsg);
   }
}

void Comm::SetVCOSplits(unsigned int RxSplitF,unsigned int TxSplitF)
{
   AppMsg *pMsg = new AppMsg;

   if(pMsg != NULL) {
      memset(pMsg,0,sizeof(AppMsg));
      InitMsgHeader(&pMsg->Hdr,0x20,0xaa);
      pMsg->Data[0] = 8;
      for(int i = 1; i < 5; i++) {
         pMsg->Data[i] = (BYTE) (RxSplitF & 0xff);
         RxSplitF >>= 8;
      }
      for(; i < 9; i++) {
         pMsg->Data[i] = (BYTE) (TxSplitF & 0xff);
         TxSplitF >>= 8;
      }
      pMsg->Data[i++] = 0xfd;
      pMsg->DataLen = sizeof(CI_V_Hdr) + i;
      SendMessage(pMsg);
   }
}

void Comm::GetConfig()
{
   AppMsg *pMsg = new AppMsg;

   if(pMsg != NULL) {
      memset(pMsg,0,sizeof(AppMsg));
      InitMsgHeader(&pMsg->Hdr,0x20,0xaa);
      pMsg->Data[0] = 3;
      pMsg->Data[1] = 0xfd;
      pMsg->DataLen = sizeof(CI_V_Hdr) + 2;
      SendMessage(pMsg);
   }
}

void Comm::SetConfig(unsigned char *Config)
{
   AppMsg *pMsg = new AppMsg;

   if(pMsg != NULL) {
      memset(pMsg,0,sizeof(AppMsg));
      InitMsgHeader(&pMsg->Hdr,0x20,0xaa);
      pMsg->Data[0] = 4;
      pMsg->Data[1] = Config[0];
      pMsg->Data[2] = Config[1];
      pMsg->Data[3] = 0xfd;
      pMsg->DataLen = sizeof(CI_V_Hdr) + 4;
      SendMessage(pMsg);
   }
}

void Comm::SetDuplex(unsigned char DuplexType)
{
   AppMsg *pMsg = new AppMsg;

   if(pMsg != NULL) {
      memset(pMsg,0,sizeof(AppMsg));
      InitMsgHeader(&pMsg->Hdr,0x20,0xf);
      pMsg->Data[0] = DuplexType;
      pMsg->Data[1] = 0xfd;
      pMsg->DataLen = sizeof(CI_V_Hdr) + 2;
      SendMessage(pMsg);
   }
}

void Comm::StoreVFO()
{
   AppMsg *pMsg = new AppMsg;

   if(pMsg != NULL) {
      memset(pMsg,0,sizeof(AppMsg));
      InitMsgHeader(&pMsg->Hdr,0x20,9);
      pMsg->Data[0] = 0xfd;
      pMsg->DataLen = sizeof(CI_V_Hdr) + 1;
      SendMessage(pMsg);
   }
}

void Comm::RecallMode()
{
   AppMsg *pMsg = new AppMsg;

   if(pMsg != NULL) {
      memset(pMsg,0,sizeof(AppMsg));
      InitMsgHeader(&pMsg->Hdr,0x20,0xa);
      pMsg->Data[0] = 0xfd;
      pMsg->DataLen = sizeof(CI_V_Hdr) + 1;
      SendMessage(pMsg);
   }
}

void Comm::SelectMode(unsigned char Mode)
{
   AppMsg *pMsg = new AppMsg;
   int Bcd = Mode - 1;

   Bcd = (Bcd % 10) | ((Bcd / 10) << 4);

   if(pMsg != NULL) {
      memset(pMsg,0,sizeof(AppMsg));
      InitMsgHeader(&pMsg->Hdr,0x20,8);
      pMsg->Data[0] = (unsigned char) Bcd;
      pMsg->Data[1] = 0xfd;
      pMsg->DataLen = sizeof(CI_V_Hdr) + 2;
      SendMessage(pMsg);
   }
}


void Comm::SetModeData(unsigned char *Data)
{
   AppMsg *pMsg = new AppMsg;

   if(pMsg != NULL) {
      memset(pMsg,0,sizeof(AppMsg));
      InitMsgHeader(&pMsg->Hdr,0x20,0xaa);
      int i = 0;
      pMsg->Data[i++] = 1;
      for(int j = 0; j < 16; j++) {
         pMsg->Data[i++] = Data[j];
      }
      pMsg->Data[i++] = 0xfd;
      pMsg->DataLen = sizeof(CI_V_Hdr) + i;
      SendMessage(pMsg);
   }
}


