// $Log: Comm.cpp,v $
// Revision 1.10  2008/02/02 17:58:21  Skip
// Added support for volume pot (not tested).
//
// Revision 1.9  2007/07/15 14:14:23  Skip
// Added squelch pot support.
//
// Revision 1.8  2007/07/06 13:43:59  Skip
// 1. Changed MAX_RETRIES from 5 to 3.
// 2. Changed response timeout from 2 seconds to .5 seconds.
// 3. Added code to Comm::SendNextMessage to send a fake reply to
//    the main window when an set communications parameters message
//    fails.  This is necessary so the PC will actually change baudrates,
//    otherwise we can get stuck out of sync with the Xcat unless we
//    whack the baudrate variable in the registry manually.
//
// Revision 1.7  2007/01/26 00:28:40  Skip
// 1. Modified debug code in Comm::SendNextMessage to lookup and display the
//    description of the Xcat Cmd byte.
// 2. Increased reponse timeout set by Comm::SendNextMessage from .5 seconds to
//    2 seconds.
// 3. Modified debug code in Comm::IOThreadMainLoop to lookup and display the
//    description of the Xcat Cmd byte.
// 4. Modified Comm::SetModeData to save mode raw mode data to c:\xcat_tx.bin
//    when debug mode is enabled.
//
// Revision 1.6  2005/01/08 19:17:32  Skip
// "The Xcat is Naked" -> "The Xcat Naked".
//
// Revision 1.5  2005/01/06 16:06:25  Skip
// 1. Modified Init() to wait for communications thread to start running before
//    returning.  Fixes ASSERT failures when SendNextMessage() is called
//    from the mainline thread by SendMessage() before ProcessRx() is called
//    the first time.
// 2. Doubled response timeout to 500 milliseconds to prvent premature
//    timeouts at 1200 baud.
// 3. Added gInvertedModeSel support to SelectMode().
// 4. Added SetCommParameters().
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
// Revision 1.3  2004/08/28 22:31:30  Skip
// Added the ability to change the serial port baudrate and the address used
// by the Xcat on the bus.
//
// Revision 1.2  2004/08/08 23:41:27  Skip
// Complete implementation of mode scan.
//
// Revision 1.1.1.1  2004/07/09 23:12:30  Skip
// Initial import of Xcat PC control program - V0.09.
// Shipped with first 10 Xcat boards.
//
//
#include "stdafx.h"
#include "resource.h"
#include "xcat.h"
#include "comm.h"

Comm CComm;
#define MAX_RETRIES  3
int tracecounter = 0;

int g_bSettingSquelch = FALSE;
int gSquelchLevel = 0;
int gPendingSquelchLevel = 0;

int g_bSettingVolume = FALSE;
int gVolumeLevel = 0;
int gPendingVolumeLevel = 0;

static char *XcatCmdLookup[] = {
	"get vfo raw data",					// 0x00
	"set raw vfo data",					// 0x01
	"get firmware version number",	// 0x02
	"get configuration data",			// 0x03
	"set configuration data",			// 0x04
	"get Tx offset",						// 0x05
	"set Tx offset",						// 0x06
	"get VCO split frequencies",		// 0x07
	"set VCO split frequencies",		// 0x08
	"get sync data debug info",		// 0x09
	"Set communications parameters",	// 0x0a
	"Set Squelch Level",					// 0x0b
};

static char *XcatResponseLookup[] = {
	"response to get vfo raw data",	// 0x80
	"Signal/mode report to PC",	// 0x81
	"get firmware version number response",	// 0x82
	"response to get configuration data",	// 0x83
	"0x84",
	"get Tx offset response",	// 0x85
	"0x86",
	"get VCO split frequencies response",	// 0x87
	"0x88",
	"get sync data debug response",	// 0x89
	"Set communications parameters ACK",	// 0x8a
	"0x8b",
};


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
      mIOThreadEnable = 3;

      if(!PurgeComm(mComDev,PURGE_TXABORT | PURGE_RXABORT)){
         ASSERT(FALSE);
      }
      // Wait for the IO Thread to terminate

      while(mIOThreadEnable == 3) {
			Sleep(10);
		}

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
      mIOThreadEnable = 3;

      if(!PurgeComm(mComDev,PURGE_TXABORT | PURGE_RXABORT)){
         ASSERT(FALSE);
      }

      // Wait for the IO Thread to terminate

      while(mIOThreadEnable == 3) {
			Sleep(10);
		}

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

// Wait for IO thread to startup
	while(mIOThreadEnable == 1) {
		Sleep(10);
	}
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
		if(mTxHead->Hdr.Cmd == 0xaa && mTxHead->Data[0] == 0xa) {
		// Set baudrate message failed.  
		// Send a phoney ack so we'll change the PC's baudrate anyway.  
		// Otherwise we can never sync baudrates unless we whack the registry.
			AppMsg *pMsg = (AppMsg *) new char[sizeof(AppMsg)+mRxCount];
			pMsg->Hdr.Cmd = 0xaa;
			pMsg->Data[0] = 0x8a;

			if(!m_pMainWnd->PostMessage(ID_RX_MSG,0,(LPARAM) pMsg)){
				ASSERT(FALSE);
			}
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
			if(pMsg->Hdr.Cmd == ICOM_CMD_XCAT) {
				BYTE XcatCmd = pMsg->Data[0];
				if(XcatCmd < 0xc) {
					LOG(("sending %s:\n",XcatCmdLookup[XcatCmd]));
				}
				else if(XcatCmd > 0x7f && XcatCmd < 0x8c) {
					LOG(("sending %s:\n",XcatResponseLookup[XcatCmd-0x80]));
				}
				else {
					LOG(("sending:\n"));
				}
			}
			else {
				LOG(("sending:\n"));
			}
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
      SetTimeout(&mTxTimeout,500);
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

   mIOThreadEnable++;
   while(mIOThreadEnable == 2) {
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
		if(pMsg->Hdr.Cmd == 0xaa && pMsg->Data[0] == 0xb) {
		// Set squelch level message sent
			g_bSettingSquelch = FALSE;
			if(gSquelchLevel != gPendingSquelchLevel) {
            SetSquelchLevel(gPendingSquelchLevel);
			}
		}
      delete pMsg;
      if(mTxHead != NULL && mTxState == TX_IDLE) {
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
                  if(pMsg != NULL) {
                     pMsg->DataLen = mRxCount;
                     memcpy(&pMsg->Hdr,mRxMsg,mRxCount);

							if(pMsg->Hdr.Cmd == ICOM_CMD_XCAT) {
								BYTE XcatCmd = pMsg->Data[0];
								if(XcatCmd < 0xb) {
									LOG(("Received %s:\n",XcatCmdLookup[XcatCmd]));
								}
								else if(XcatCmd > 0x7f && XcatCmd < 0x8b) {
									LOG(("Received %s:\n",XcatResponseLookup[XcatCmd-0x80]));
								}
								else {
									LOG(("Received:\n"));
								}
							}
							else {
								LOG(("Received:\n"));
							}
							LOG_HEX(mRxMsg,mRxCount);

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
                           pErrMsg->Format("The Xcat NAK'ed\n"
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
      InitMsgHeader(&pMsg->Hdr,gXcatAdr,0xaa);
      pMsg->Data[0] = 0;
      pMsg->Data[1] = 0xfd;
      pMsg->DataLen = sizeof(CI_V_Hdr) + 2;
#else
      InitMsgHeader(&pMsg->Hdr,gXcatAdr,5);

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
      InitMsgHeader(&pMsg->Hdr,gXcatAdr,0x1b);

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
      InitMsgHeader(&pMsg->Hdr,gXcatAdr,0xaa);
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
      InitMsgHeader(&pMsg->Hdr,gXcatAdr,0xaa);
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
      InitMsgHeader(&pMsg->Hdr,gXcatAdr,0xaa);
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
      InitMsgHeader(&pMsg->Hdr,gXcatAdr,0xaa);
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
      InitMsgHeader(&pMsg->Hdr,gXcatAdr,0xaa);
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
      InitMsgHeader(&pMsg->Hdr,gXcatAdr,0xaa);
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
      InitMsgHeader(&pMsg->Hdr,gXcatAdr,0xaa);
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
      InitMsgHeader(&pMsg->Hdr,gXcatAdr,0xaa);
      pMsg->Data[0] = 4;
      pMsg->Data[1] = Config[0];
      pMsg->Data[2] = Config[1];
      pMsg->Data[3] = Config[2];
      pMsg->Data[4] = 0xfd;
      pMsg->DataLen = sizeof(CI_V_Hdr) + 5;
      SendMessage(pMsg);
   }
}

void Comm::SetDuplex(unsigned char DuplexType)
{
   AppMsg *pMsg = new AppMsg;

   if(pMsg != NULL) {
      memset(pMsg,0,sizeof(AppMsg));
      InitMsgHeader(&pMsg->Hdr,gXcatAdr,0xf);
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
      InitMsgHeader(&pMsg->Hdr,gXcatAdr,9);
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
      InitMsgHeader(&pMsg->Hdr,gXcatAdr,0xa);
      pMsg->Data[0] = 0xfd;
      pMsg->DataLen = sizeof(CI_V_Hdr) + 1;
      SendMessage(pMsg);
   }
}

/* Mode 1 -> 32 */
void Comm::SelectMode(unsigned char Mode)
{
   AppMsg *pMsg = new AppMsg;
	int Bcd = INVERT_MODE(Mode-1);

   Bcd = (Bcd % 10) | ((Bcd / 10) << 4);

   if(pMsg != NULL) {
      memset(pMsg,0,sizeof(AppMsg));
      InitMsgHeader(&pMsg->Hdr,gXcatAdr,8);
      pMsg->Data[0] = (unsigned char) Bcd;
      pMsg->Data[1] = 0xfd;
      pMsg->DataLen = sizeof(CI_V_Hdr) + 2;
      SendMessage(pMsg);
   }
}


void Comm::SetModeData(unsigned char *Data)
{
	if(gDebugMode) {
		FILE *fp;

		if((fp = fopen("c:\\xcat_tx.bin","w")) != NULL) {
			fwrite(Data,16,1,fp);
			fclose(fp);
		}
	}
   
	AppMsg *pMsg = new AppMsg;

   if(pMsg != NULL) {
      memset(pMsg,0,sizeof(AppMsg));
      InitMsgHeader(&pMsg->Hdr,gXcatAdr,0xaa);
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

void Comm::GetSigReport()
{
   AppMsg *pMsg = new AppMsg;

   if(pMsg != NULL) {
      memset(pMsg,0,sizeof(AppMsg));
      InitMsgHeader(&pMsg->Hdr,gXcatAdr,0xaa);
      pMsg->Data[0] = 9;
      pMsg->Data[1] = 0xfd;
      pMsg->DataLen = sizeof(CI_V_Hdr) + 2;
      SendMessage(pMsg);
   }
}

void Comm::GetSyncData()
{
   AppMsg *pMsg = new AppMsg;

   if(pMsg != NULL) {
      memset(pMsg,0,sizeof(AppMsg));
      InitMsgHeader(&pMsg->Hdr,gXcatAdr,0xaa);
      pMsg->Data[0] = 9;
      pMsg->Data[1] = 0xfd;
      pMsg->DataLen = sizeof(CI_V_Hdr) + 2;
      SendMessage(pMsg);
   }
}

void Comm::SetCommParameters(int Baudrate,int Adr)
{
   AppMsg *pMsg = new AppMsg;

   if(pMsg != NULL) {
      memset(pMsg,0,sizeof(AppMsg));
      InitMsgHeader(&pMsg->Hdr,gXcatAdr,0xaa);
      int i = 0;
      pMsg->Data[i++] = 0xa;
      pMsg->Data[i++] = (BYTE) Baudrate;
      pMsg->Data[i++] = (BYTE) Adr;
      pMsg->Data[i++] = 0xfd;
      pMsg->DataLen = sizeof(CI_V_Hdr) + i;
      SendMessage(pMsg);
   }
}

// Set squelch level messages are sent by dragging the squelch pot control
// so we need to make sure we don't send a flood of squelch level message.
// Wait for the last one to be acked before sending another
void Comm::SetSquelchLevel(int SquelchLevel)
{
	if(!g_bSettingSquelch) {
	// No Squelch set message active
		g_bSettingSquelch = TRUE;
		gSquelchLevel = SquelchLevel;
		gPendingSquelchLevel = SquelchLevel;
		
		AppMsg *pMsg = new AppMsg;

		if(pMsg != NULL) {
			memset(pMsg,0,sizeof(AppMsg));
			InitMsgHeader(&pMsg->Hdr,gXcatAdr,0xaa);
			int i = 0;
			pMsg->Data[i++] = 0xb;
			pMsg->Data[i++] = (BYTE) SquelchLevel;
			pMsg->Data[i++] = 0xfd;
			pMsg->DataLen = sizeof(CI_V_Hdr) + i;
			SendMessage(pMsg);
		}
	}
	else {
		gPendingSquelchLevel = SquelchLevel;
	}
}

// Set volume level messages are sent by dragging the volume pot control
// so we need to make sure we don't send a flood of volume level message.
// Wait for the last one to be acked before sending another
void Comm::SetVolumeLevel(int VolumeLevel)
{
	if(!g_bSettingVolume) {
	// No Volume set message active
		g_bSettingVolume = TRUE;
		gVolumeLevel = VolumeLevel;
		gPendingVolumeLevel = VolumeLevel;
		
		AppMsg *pMsg = new AppMsg;

		if(pMsg != NULL) {
			memset(pMsg,0,sizeof(AppMsg));
			InitMsgHeader(&pMsg->Hdr,gXcatAdr,0xaa);
			int i = 0;
			pMsg->Data[i++] = 0xc;
			pMsg->Data[i++] = (BYTE) VolumeLevel;
			pMsg->Data[i++] = 0xfd;
			pMsg->DataLen = sizeof(CI_V_Hdr) + i;
			SendMessage(pMsg);
		}
	}
	else {
		gPendingVolumeLevel = VolumeLevel;
	}
}



