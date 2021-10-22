#pragma once
 

class SerialPort
{
public:
	SerialPort(void);
	~SerialPort(void);
private:
	//串口句柄
	HANDLE hCom;
	//获取串口
	int GetComBaudRate(int _baudRate);
	//串口是否打开
	bool isOpen;
public:
	//打开串口
	bool OpenCom(UINT prot,
		UINT _baudRate = 1500000       /*波特率*/
		);
	//关闭串口
	void CloseCom();
	//同步发送串口数据
	int SendDate(LPVOID data, DWORD dataSize);
	//异步发送串口数据
	int AsySendDate(LPVOID data, DWORD dataSize);
	//同步读串口数据
	int ReadDate(LPVOID data, DWORD dataSize);
	//异步读串口数据
	int AsyReadDate(LPVOID data, DWORD dataSize);
	//判断串口是否打开
	bool IsOpen();
};
