#pragma once
 

class SerialPort
{
public:
	SerialPort(void);
	~SerialPort(void);
private:
	//���ھ��
	HANDLE hCom;
	//��ȡ����
	int GetComBaudRate(int _baudRate);
	//�����Ƿ��
	bool isOpen;
public:
	//�򿪴���
	bool OpenCom(UINT prot,
		UINT _baudRate = 1500000       /*������*/
		);
	//�رմ���
	void CloseCom();
	//ͬ�����ʹ�������
	int SendDate(LPVOID data, DWORD dataSize);
	//�첽���ʹ�������
	int AsySendDate(LPVOID data, DWORD dataSize);
	//ͬ������������
	int ReadDate(LPVOID data, DWORD dataSize);
	//�첽����������
	int AsyReadDate(LPVOID data, DWORD dataSize);
	//�жϴ����Ƿ��
	bool IsOpen();
};
