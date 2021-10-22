#include "stdafx.h"
#include "serialport.h"


SerialPort::SerialPort() :hCom(INVALID_HANDLE_VALUE), isOpen(false)
{
}

SerialPort::~SerialPort()
{
}

/**
*�жϴ����Ƿ��
*/
bool SerialPort::IsOpen()
{
	return this->isOpen;
}

/**
*��ȡ������,У�鴫��Ĳ������Ƿ��������ʽֵ
*/
int SerialPort::GetComBaudRate(int _baudRate)
{
	int baudRate = -1;
	switch (_baudRate)
	{
	case 110:
		baudRate = CBR_110;
		break;
	case 300:
		baudRate = CBR_300;
		break;
	case 600:
		baudRate = CBR_600;
		break;
	case 1200:
		baudRate = CBR_1200;
		break;
	case 2400:
		baudRate = CBR_2400;
		break;
	case 4800:
		baudRate = CBR_4800;
		break;
	case 9600:
		baudRate = CBR_9600;
		break;
	case 14400:
		baudRate = CBR_14400;
		break;
	case 19200:
		baudRate = CBR_19200;
		break;
	case 38400:
		baudRate = CBR_38400;
		break;
	case 56000:
		baudRate = CBR_56000;
		break;
	case 57600:
		baudRate = CBR_57600;
		break;
	case 115200:
		baudRate = CBR_115200;
		break;
 
	case 128000:
		baudRate = CBR_128000;
		break;
	case 256000:
		baudRate = CBR_256000;
		break;
	default:
		break;
	}
	return baudRate;
}



/**
*�򿪴���
*/
bool SerialPort::OpenCom(UINT port, UINT _baudRate)
{
	TCHAR buff[20];
	memset(buff, 0, sizeof buff);
	_stprintf(buff, _T("\\\\.\\COM%d"), port);
	hCom = CreateFile(buff,         //COM1��
		GENERIC_READ | GENERIC_WRITE, //�������д
		0,                          //��ռ��ʽ
		NULL,                       //���ð�ȫ�����Խṹ
		OPEN_EXISTING,              //�򿪶����Ǵ���
		0,                          //ͬ����ʽ
		NULL);
	if (INVALID_HANDLE_VALUE == hCom){
		return FALSE;
	}

	SetupComm(hCom, 3000, 3000);                 //���뻺����������������Ĵ�С����3000

	DCB dcb;
	if (FALSE == ::GetCommState(hCom, &dcb)){
		::CloseHandle(hCom);
		return FALSE;
	}

	COMMTIMEOUTS TimeOuts;
	GetCommState(hCom, &dcb);

	// ����DCB����
	dcb.BaudRate = _baudRate;               // ������
	dcb.ByteSize = 8;                        // ÿ���ֽ���8λ
	dcb.fBinary = TRUE;						// ������ģʽ
	dcb.fParity = FALSE;					// ������żУ��
	dcb.StopBits = ONESTOPBIT;				// ֹͣλ(1)
	dcb.Parity = NOPARITY;				    // ��żУ�鷽��
	dcb.fAbortOnError = TRUE;               // �д�����ʱ��ֹ��д����
	dcb.fOutxCtsFlow = FALSE;				// ָ��CTS�Ƿ����ڼ�ⷢ�Ϳ��ơ���ΪTRUEʱCTSΪOFF�����ͽ������𡣣����������
	dcb.fRtsControl = RTS_CONTROL_DISABLE;  // RTS��ΪOFF 
	dcb.fDtrControl = DTR_CONTROL_DISABLE;  // DTR��ΪOFF 
	dcb.fOutxDsrFlow = FALSE;				// ָ��DSR�Ƿ����ڼ�ⷢ�Ϳ��ơ���ΪTRUEʱDSRΪOFF�����ͽ�������
	dcb.fDsrSensitivity = FALSE;			// ����ֵΪTRUEʱDSRΪOFFʱ���յ��ֽڱ����� 

	// ���ô���״̬
	SetCommState(hCom, &dcb);

	memset(&TimeOuts, 0, sizeof(TimeOuts));     //�趨����ʱ
	TimeOuts.ReadTotalTimeoutMultiplier = 3;    //1000;
	TimeOuts.ReadTotalTimeoutConstant = 0;      //timeout for read operation
	TimeOuts.ReadIntervalTimeout = 20;          //MAXDWORD;

	TimeOuts.WriteTotalTimeoutConstant = 0;     //�趨д��ʱ
	TimeOuts.WriteTotalTimeoutMultiplier = 3;   //100;
	SetCommTimeouts(hCom, &TimeOuts);            //���ó�ʱ

	::PurgeComm(hCom, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);//��ջ�����
	this->isOpen = true;
	return true;
}

//ͬ�����ʹ�������
int SerialPort::SendDate(LPVOID data, DWORD size)
{
	if (!this->isOpen)return -1;
	DWORD dwBytesWrite = 0;
	COMSTAT ComStat;
	DWORD dwErrorFlags;
	::ClearCommError(this->hCom, &dwErrorFlags, &ComStat);
	bool bRet = ::WriteFile(this->hCom, data, size, &dwBytesWrite, NULL);
	if (!bRet) return -1;
	return dwBytesWrite;
}

//�첽���ʹ�������
int SerialPort::AsySendDate(LPVOID data, DWORD size)
{
	if (!this->isOpen)return -1;
	DWORD dwBytesWrite = 0;
	OVERLAPPED overlapped;
	memset(&overlapped, 0, sizeof(OVERLAPPED));
	overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	bool bRet = ::WriteFile(this->hCom, data, size, &dwBytesWrite, &overlapped);
	DWORD dwError = GetLastError();
	if (!bRet && ::GetLastError() == ERROR_IO_PENDING)
	{
		::WaitForSingleObject(overlapped.hEvent, 1000);
	}
	return 1;
}

//ͬ������������
int SerialPort::ReadDate(LPVOID data, DWORD size)
{
	if (!this->isOpen)return -1;
	DWORD dwBytesRead = 0;
	bool bRet = ::ReadFile(this->hCom, data, size, &dwBytesRead, NULL);
	if (!bRet)
	{
		::PurgeComm(this->hCom, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
		return -1;
	}
	::PurgeComm(this->hCom, PURGE_TXCLEAR | PURGE_TXABORT);
	return dwBytesRead;
}

//�첽����������
int SerialPort::AsyReadDate(LPVOID data, DWORD size)
{
	if (!this->isOpen)return -1;
	DWORD readLen = 0;
	OVERLAPPED overlapped;
	memset(&overlapped, 0, sizeof(OVERLAPPED));
	overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	bool bRet = ::ReadFile(this->hCom, data, size, &readLen, &overlapped);
	if (!bRet && ::GetLastError() == ERROR_IO_PENDING)
	{
		::WaitForSingleObject(overlapped.hEvent, 500);
	}
	PurgeComm(hCom, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
	return 1;
}




/**
*�رմ���
*/
void SerialPort::CloseCom()
{
	::CloseHandle(this->hCom);
	this->isOpen = false;
	this->hCom = INVALID_HANDLE_VALUE;
}