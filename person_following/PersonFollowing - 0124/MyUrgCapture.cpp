/*
*�@  URG �L���v�`��
*    �}���`�X���b�h�ɐݒ肵�ăR���p�C��
*/
#include <windows.h>
#include <process.h>
#include <stdio.h>

#include "MyUrgCapture.h"

HANDLE     g_commHandle;			// �ʐM�|�[�g�n���h��
OVERLAPPED g_ReadOverlapped;		// ���[�h�p�I�[�o���b�v�\����
OVERLAPPED g_WriteOverlapped;		// ���C�g�p�I�[�o���b�v�\����
BYTE rxBuffTmp[8192],rxBuff[4096];	// �ʐM�o�b�t�@  BYTE rxBuffTmp[4096],rxBuff[2048];

// �X���b�h�̃n���h��
unsigned __stdcall ReadThread(void *lpx);
static HANDLE g_hThread;
static DWORD  g_hThreadID;

CRITICAL_SECTION g_csReadThread;
volatile BOOL g_endFlag;			// ��M�����t���O
volatile BOOL g_stopFlag;			// �X���b�h��~�t���O
volatile WORD g_totalCnt;			// �I�[�R�[�h(line feed �A��2��)������܂ł̎�M�f�[�^��(line feed ��sum ����)
volatile WORD g_distance[1080];		// �����i�[�z��

void CommClose(void);
void CommOpen(char *ComNo);

////////////////////////////////////
// �f�[�^��M�X���b�h
////////////////////////////////////
unsigned __stdcall ReadThread(void *lpx)
{
	DWORD   Event, NumberOfBytes, Error;
	COMSTAT Comstat;
	BYTE    lfCnt = 0;

	EnterCriticalSection(&g_csReadThread);
	g_totalCnt = 0;
	LeaveCriticalSection(&g_csReadThread);
	while(1)
	{
		Sleep(10);

		// �X���b�h��~
		int stopFlag = 0;
		EnterCriticalSection(&g_csReadThread);
		stopFlag = g_stopFlag;
		LeaveCriticalSection(&g_csReadThread);
		if(stopFlag) break;

		WaitCommEvent(g_commHandle,&Event,&g_ReadOverlapped);	// ��M�C�x���g�̊m�F

		if(Event & EV_RXCHAR)									// ��M�C�x���g�����������ꍇ
		{
			ClearCommError(g_commHandle,&Error,&Comstat);		// ��M�o�C�g���̎擾
			if(Comstat.cbInQue)
			{	// ��M�f�[�^�T�C�Y���o�b�t�@�ɓǂݍ���
				if(!ReadFile(g_commHandle,rxBuffTmp,Comstat.cbInQue,&NumberOfBytes,&g_ReadOverlapped)){	// ���얢�����̏ꍇ
					if(GetLastError() == ERROR_IO_PENDING)	// ��薳���̏ꍇ
						GetOverlappedResult(g_commHandle,&g_ReadOverlapped,&NumberOfBytes,TRUE);	// ���슮���܂őҋ@
				}

				for(unsigned int i = 23;i < Comstat.cbInQue;i++){ // i=23����f�[�^�isensor����̃G�R�[�o�b�N�A�^�C���X�^���v���Ƃ΂��ăf�[�^�������珈���j
					if(rxBuffTmp[i] == 0x0a){	// lf�����m�����ꍇ
						lfCnt++;				// rxBuff�� lf �̓R�s�[���Ȃ�
/**/					g_totalCnt--;			// lf �̑O�� sum ������̂� sum �����u���b�N�̐擪�f�[�^�ŏ㏑�����邽�߂� g_totalCnt �� sum �̏ꏊ�ɖ߂�
					}else{
						EnterCriticalSection(&g_csReadThread);
						rxBuff[g_totalCnt++] = rxBuffTmp[i];
						LeaveCriticalSection(&g_csReadThread);
						lfCnt = 0;
					}

					if(lfCnt == 2){				// lf��2��A�����m�����ꍇ
						lfCnt = 0;
/**/					g_totalCnt++;			//�߂����������i�߂�
						EnterCriticalSection(&g_csReadThread);
						g_endFlag = 1;			// ��M�����t���O�𗧂Ă�
						LeaveCriticalSection(&g_csReadThread);
					}
				}
			}
		}
	}

	return 0;
}

void start_URG(){ // BM�R�}���h�ŋ�������J�n
	int i;

	DWORD NumberOfByte;
	BYTE bmCommand[] = "BM";
	BYTE txBuff[64];
	BYTE *txBuffWork = txBuff;

	for(i=0;i<2;i++) *txBuffWork++ = bmCommand[i];
	*txBuffWork++ = 0xa;

	WriteFile(g_commHandle,txBuff,txBuffWork - txBuff,&NumberOfByte,&g_WriteOverlapped);

	return;
}

//////////////////////////////////////////
// URG �����f�[�^�̎擾
//////////////////////////////////////////
void GetDistanceData(int URG_Distance[])
{
	DWORD NumberOfByte;
	int i,j,k;	

	BYTE gCommand[] = "GD0000108000";			// G�R�}���h������ "G00076800";
	BYTE txBuff[64];						// ���M�o�b�t�@    BYTE txBuff[64];
	BYTE *txBuffWork = txBuff;

	for(i = 0;i < 12;i++) *txBuffWork++ = gCommand[i];//	for(i = 0;i < 9;i++) *txBuffWork++ = gCommand[i];
	*txBuffWork++ = 0xa;					// �I�[����(line feed)

	// ���M
	EnterCriticalSection(&g_csReadThread);
	g_endFlag = 0;
	LeaveCriticalSection(&g_csReadThread);
	WriteFile(g_commHandle,txBuff,txBuffWork - txBuff,&NumberOfByte,&g_WriteOverlapped);
	for(;;){
		int endFlag = 0;
		EnterCriticalSection(&g_csReadThread);
		endFlag = g_endFlag;
		LeaveCriticalSection(&g_csReadThread);
		if(endFlag) break;
		Sleep(1);
	}
	
	EnterCriticalSection(&g_csReadThread);
	j = 0;
	for(i=0;i<g_totalCnt;i+=3){ // �R�L�����N�^(�o�C�g)�łP�f�[�^
		g_distance[j++] = ((rxBuff[i] - 0x30) << 12) + ((rxBuff[i+1] - 0x30) << 6) + (rxBuff[i+2] - 0x30); // �f�R�[�h���Ȃ��狗�������i�[
	}
	g_totalCnt = 0;
	LeaveCriticalSection(&g_csReadThread);

	EnterCriticalSection(&g_csReadThread);
	for(k=0;k<1080;k++){
		URG_Distance[k] = (int)g_distance[k];
	}
	LeaveCriticalSection(&g_csReadThread);

	return;
}

//////////////////////////////////////////
// URG �̊J�n
//////////////////////////////////////////
int init_URG(char *ComNo){
	// �I�[�o���b�v�\���̂̏�����
	ZeroMemory(&g_ReadOverlapped,  sizeof(OVERLAPPED));
	ZeroMemory(&g_WriteOverlapped, sizeof(OVERLAPPED));
	g_ReadOverlapped.hEvent  = CreateEvent(NULL, TRUE, FALSE, NULL);
	g_WriteOverlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	// �ʐM�|�[�g�̃I�[�v��
	CommOpen(ComNo);

	// �X���b�h�̊J�n
	g_stopFlag = FALSE;	
	InitializeCriticalSection(&g_csReadThread);
	g_hThread = (HANDLE)_beginthreadex(NULL, 0, ReadThread, NULL, 0, (unsigned int *)&g_hThreadID);

	return 1;
}

//////////////////////////////////////////
// URG �̏I��
//////////////////////////////////////////
int close_URG(void)
{
	// �X���b�h�̏I��
	EnterCriticalSection(&g_csReadThread);
	g_stopFlag = TRUE;						// �X���b�h��~�t���O�𗧂Ă�
	LeaveCriticalSection(&g_csReadThread);
	WaitForSingleObject(g_hThread,2000);	// �X���b�h�̏I����҂�

	// �ʐM�|�[�g�̃N���[�Y
	CommClose();

	return 1;
}

////////////////////////////////////
// Comm�|�[�g���J��
////////////////////////////////////
void CommOpen(char *ComNo)
{
	DCB	 dcb;
	BOOL retVal;

	// �V���A���|�[�g�̃I�[�v�� �g�p����USB�|�[�g�ɍ��킹�Ĕԍ����w�肷��
	g_commHandle = CreateFile(ComNo,GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,FILE_FLAG_OVERLAPPED,NULL);

	// �ʐM�̏ڍאݒ�
	dcb.DCBlength = sizeof(DCB);
	dcb.BaudRate = CBR_115200;
	dcb.fBinary = TRUE;
	dcb.fParity = FALSE;
	dcb.fOutxCtsFlow = FALSE;
	dcb.fOutxDsrFlow = FALSE;
	dcb.fDtrControl = DTR_CONTROL_DISABLE;
	dcb.fDsrSensitivity = FALSE;
	dcb.fTXContinueOnXoff = FALSE;
	dcb.fOutX = FALSE;
	dcb.fInX = FALSE;
	dcb.fErrorChar = FALSE;
	dcb.fNull = FALSE;
	dcb.fRtsControl = RTS_CONTROL_DISABLE;
	dcb.fAbortOnError = FALSE;
	dcb.fDummy2 = 0;
	dcb.wReserved = 0;
	dcb.XonLim = 0;
	dcb.XoffLim = 0;
	dcb.ByteSize = 8;
	dcb.Parity = NOPARITY;
	dcb.StopBits = ONESTOPBIT;
	dcb.XonChar = 0;
	dcb.XoffChar = 0;
	dcb.ErrorChar = 0;
	dcb.EofChar = 0;
	dcb.EvtChar = 0;
	dcb.wReserved1 =0;

	retVal = SetCommState(g_commHandle,&dcb);		// DCB�\���̂̓��e���V���A���|�[�g�ɐݒ�
	retVal = SetCommMask(g_commHandle,EV_RXCHAR);	// �C�x���g�̐ݒ�i��M�C�x���g�j
	retVal = SetupComm(g_commHandle,0x1000,0x1000);	// ���o�̓o�b�t�@�̃T�C�Y�ݒ�
}

////////////////////////////////////
// Comm�|�[�g�����
////////////////////////////////////
void CommClose(void)
{
	CloseHandle(g_commHandle);
}
