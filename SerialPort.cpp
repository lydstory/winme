#include "stdafx.h"
#include "serialport.h"


SerialPort::SerialPort() :hCom(INVALID_HANDLE_VALUE), isOpen(false)
{
}

SerialPort::~SerialPort()
{
}

/**
*判断串口是否打开
*/
bool SerialPort::IsOpen()
{
	return this->isOpen;
}

/**
*获取波特率,校验传入的波特率是否是正规格式值
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
*打开串口
*/
bool SerialPort::OpenCom(UINT port, UINT _baudRate)
{
	TCHAR buff[20];
	memset(buff, 0, sizeof buff);
	_stprintf(buff, _T("\\\\.\\COM%d"), port);
	hCom = CreateFile(buff,         //COM1口
		GENERIC_READ | GENERIC_WRITE, //允许读和写
		0,                          //独占方式
		NULL,                       //引用安全性属性结构
		OPEN_EXISTING,              //打开而不是创建
		0,                          //同步方式
		NULL);
	if (INVALID_HANDLE_VALUE == hCom){
		return FALSE;
	}

	SetupComm(hCom, 3000, 3000);                 //输入缓冲区和输出缓冲区的大小都是3000

	DCB dcb;
	if (FALSE == ::GetCommState(hCom, &dcb)){
		::CloseHandle(hCom);
		return FALSE;
	}

	COMMTIMEOUTS TimeOuts;
	GetCommState(hCom, &dcb);

	// 设置DCB参数
	dcb.BaudRate = _baudRate;               // 波特率
	dcb.ByteSize = 8;                        // 每个字节有8位
	dcb.fBinary = TRUE;						// 二进制模式
	dcb.fParity = FALSE;					// 允许奇偶校验
	dcb.StopBits = ONESTOPBIT;				// 停止位(1)
	dcb.Parity = NOPARITY;				    // 奇偶校验方法
	dcb.fAbortOnError = TRUE;               // 有错误发生时中止读写操作
	dcb.fOutxCtsFlow = FALSE;				// 指定CTS是否用于检测发送控制。当为TRUE时CTS为OFF，发送将被挂起。（发送清除）
	dcb.fRtsControl = RTS_CONTROL_DISABLE;  // RTS置为OFF 
	dcb.fDtrControl = DTR_CONTROL_DISABLE;  // DTR置为OFF 
	dcb.fOutxDsrFlow = FALSE;				// 指定DSR是否用于检测发送控制。当为TRUE时DSR为OFF，发送将被挂起。
	dcb.fDsrSensitivity = FALSE;			// 当该值为TRUE时DSR为OFF时接收的字节被忽略 

	// 设置串口状态
	SetCommState(hCom, &dcb);

	memset(&TimeOuts, 0, sizeof(TimeOuts));     //设定读超时
	TimeOuts.ReadTotalTimeoutMultiplier = 3;    //1000;
	TimeOuts.ReadTotalTimeoutConstant = 0;      //timeout for read operation
	TimeOuts.ReadIntervalTimeout = 20;          //MAXDWORD;

	TimeOuts.WriteTotalTimeoutConstant = 0;     //设定写超时
	TimeOuts.WriteTotalTimeoutMultiplier = 3;   //100;
	SetCommTimeouts(hCom, &TimeOuts);            //设置超时

	::PurgeComm(hCom, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);//清空缓冲区
	this->isOpen = true;
	return true;
}

//同步发送串口数据
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

//异步发送串口数据
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

//同步读串口数据
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

//异步读串口数据
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
*关闭串口
*/
void SerialPort::CloseCom()
{
	::CloseHandle(this->hCom);
	this->isOpen = false;
	this->hCom = INVALID_HANDLE_VALUE;
}