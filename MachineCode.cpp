#include "stdafx.h"
#include "MachineCode.h"
#include <Winsock2.h>
#include <vector>
#include <IPHlpApi.h>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Iphlpapi.lib")


namespace TSM
{

// Define global buffers.
BYTE IdOutCmd[sizeof (SENDCMDOUTPARAMS) + IDENTIFY_BUFFER_SIZE - 1];
int ReadPhysicalDriveInNT (std::wstring& hardDiskSerialNo)
{
	int done = FALSE;
	int drive = 0;
	bool first = true;

	for (drive = 0; drive < MAX_IDE_DRIVES; drive++)
	{
		if (!first)
		{
			break;
		}
		HANDLE hPhysicalDriveIOCTL = 0;

		//  Try to get a handle to PhysicalDrive IOCTL, report failure
		//  and exit if can't.
		WCHAR driveName[MAX_PATH] = {0};
		swprintf(driveName, MAX_PATH, L"\\\\.\\PhysicalDrive%d", drive);

		//  Windows NT, Windows 2000, must have admin rights
		hPhysicalDriveIOCTL = CreateFile (driveName,
			GENERIC_READ | GENERIC_WRITE, 
			FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
			OPEN_EXISTING, 0, NULL);
		if (hPhysicalDriveIOCTL != INVALID_HANDLE_VALUE)
		{
			GETVERSIONOUTPARAMS VersionParams;
			DWORD               cbBytesReturned = 0;

			// Get the version, etc of PhysicalDrive IOCTL
			memset ((void*) &VersionParams, 0, sizeof(VersionParams));

			if ( ! DeviceIoControl (hPhysicalDriveIOCTL, DFP_GET_VERSION,
				NULL, 
				0,
				&VersionParams,
				sizeof(VersionParams),
				&cbBytesReturned, NULL) )
			{         
				// printf ("DFP_GET_VERSION failed for drive %d\n", i);
				// continue;
			}

			// If there is a IDE device at number "i" issue commands
			// to the device
			if (VersionParams.bIDEDeviceMap > 0)
			{
				BYTE             bIDCmd = 0;   // IDE or ATAPI IDENTIFY cmd
				SENDCMDINPARAMS  scip;

				// Now, get the ID sector for all IDE devices in the system.
				// If the device is ATAPI use the IDE_ATAPI_IDENTIFY command,
				// otherwise use the IDE_ATA_IDENTIFY command
				bIDCmd = (VersionParams.bIDEDeviceMap >> drive & 0x10) ? \
IDE_ATAPI_IDENTIFY : IDE_ATA_IDENTIFY;

				memset (&scip, 0, sizeof(scip));
				memset (IdOutCmd, 0, sizeof(IdOutCmd));

				if ( DoIDENTIFY (hPhysicalDriveIOCTL, 
					&scip, 
					(PSENDCMDOUTPARAMS)IdOutCmd, 
					(BYTE) bIDCmd,
					(BYTE) drive,
					&cbBytesReturned))
				{
					DWORD diskdata [256];
					int ijk = 0;
					USHORT *pIdSector = (USHORT *)
						((PSENDCMDOUTPARAMS) IdOutCmd) -> bBuffer;

					for (ijk = 0; ijk < 256; ijk++)
						diskdata [ijk] = pIdSector [ijk];

					ConvertToString (diskdata, 27, 46, hardDiskSerialNo);

					done = TRUE;

					first = false;
				}
			}

			CloseHandle (hPhysicalDriveIOCTL);
		}
	}

	return done;
}


// DoIDENTIFY
// FUNCTION: Send an IDENTIFY command to the drive
// bDriveNum = 0-3
// bIDCmd = IDE_ATA_IDENTIFY or IDE_ATAPI_IDENTIFY
BOOL DoIDENTIFY (HANDLE hPhysicalDriveIOCTL, PSENDCMDINPARAMS pSCIP,
				 PSENDCMDOUTPARAMS pSCOP, BYTE bIDCmd, BYTE bDriveNum,
				 PDWORD lpcbBytesReturned)
{
	// Set up data structures for IDENTIFY command.
	pSCIP -> cBufferSize = IDENTIFY_BUFFER_SIZE;
	pSCIP -> irDriveRegs.bFeaturesReg = 0;
	pSCIP -> irDriveRegs.bSectorCountReg = 1;
	pSCIP -> irDriveRegs.bSectorNumberReg = 1;
	pSCIP -> irDriveRegs.bCylLowReg = 0;
	pSCIP -> irDriveRegs.bCylHighReg = 0;

	// Compute the drive number.
	pSCIP -> irDriveRegs.bDriveHeadReg = 0xA0 | ((bDriveNum & 1) << 4);

	// The command can either be IDE identify or ATAPI identify.
	pSCIP -> irDriveRegs.bCommandReg = bIDCmd;
	pSCIP -> bDriveNumber = bDriveNum;
	pSCIP -> cBufferSize = IDENTIFY_BUFFER_SIZE;

	return ( DeviceIoControl (hPhysicalDriveIOCTL, DFP_RECEIVE_DRIVE_DATA,
		(LPVOID) pSCIP,
		sizeof(SENDCMDINPARAMS) - 1,
		(LPVOID) pSCOP,
		sizeof(SENDCMDOUTPARAMS) + IDENTIFY_BUFFER_SIZE - 1,
		lpcbBytesReturned, NULL) );
}


#define  SENDIDLENGTH  sizeof (SENDCMDOUTPARAMS) + IDENTIFY_BUFFER_SIZE


int ReadIdeDriveAsScsiDriveInNT (std::wstring& hardDiskSerialNo)
{
	int done = FALSE;
	int controller = 0;
	bool first = true;

	for (controller = 0; controller < 2; controller++)
	{
		if ( !first ) { break; }

		HANDLE hScsiDriveIOCTL = 0;
		WCHAR  driveName [256];

		//  Try to get a handle to PhysicalDrive IOCTL, report failure
		//  and exit if can't.
		swprintf_s(driveName,L"\\\\.\\Scsi%d:", controller);

		//  Windows NT, Windows 2000, any rights should do
		hScsiDriveIOCTL = CreateFile (driveName,
			GENERIC_READ | GENERIC_WRITE, 
			FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
			OPEN_EXISTING, 0, NULL);

		if (hScsiDriveIOCTL != INVALID_HANDLE_VALUE)
		{
			int drive = 0;

			for (drive = 0; drive < 2; drive++)
			{
				char buffer [sizeof (SRB_IO_CONTROL) + SENDIDLENGTH];
				SRB_IO_CONTROL *p = (SRB_IO_CONTROL *) buffer;
				SENDCMDINPARAMS *pin =
					(SENDCMDINPARAMS *) (buffer + sizeof (SRB_IO_CONTROL));
				DWORD dummy;

				memset (buffer, 0, sizeof (buffer));
				p -> HeaderLength = sizeof (SRB_IO_CONTROL);
				p -> Timeout = 10000;
				p -> Length = SENDIDLENGTH;
				p -> ControlCode = IOCTL_SCSI_MINIPORT_IDENTIFY;
				//strncpy_s((char *) p -> Signature, 8, "SCSIDISK", 8);
				memcpy((char *) p->Signature, "SCSIDISK", 8);

				//strncpy_s()

				pin -> irDriveRegs.bCommandReg = IDE_ATA_IDENTIFY;
				pin -> bDriveNumber = drive;

				if (DeviceIoControl (hScsiDriveIOCTL, IOCTL_SCSI_MINIPORT, 
					buffer,
					sizeof (SRB_IO_CONTROL) +
					sizeof (SENDCMDINPARAMS) - 1,
					buffer,
					sizeof (SRB_IO_CONTROL) + SENDIDLENGTH,
					&dummy, NULL))
				{
					SENDCMDOUTPARAMS *pOut =
						(SENDCMDOUTPARAMS *) (buffer + sizeof (SRB_IO_CONTROL));
					IDSECTOR *pId = (IDSECTOR *) (pOut -> bBuffer);
					if (pId -> sModelNumber [0])
					{
						DWORD diskdata [256];
						int ijk = 0;
						USHORT *pIdSector = (USHORT *) pId;

						for (ijk = 0; ijk < 256; ijk++)
							diskdata [ijk] = pIdSector [ijk];

						ConvertToString (diskdata, 27, 46, hardDiskSerialNo);

						first = false;
						done = TRUE;
					}
				}
			}
			CloseHandle (hScsiDriveIOCTL);
		}
	}

	return done;
}


void ConvertToString (DWORD diskdata [256], int firstIndex, int lastIndex, std::wstring& hardDiskSerialNo)
{
	static char string [1024];
	int index = 0;
	int position = 0;

	//  each integer has two characters stored in it backwards
	for (index = firstIndex; index <= lastIndex; index++)
	{
		//  get high byte for 1st character
		string [position] = (char) (diskdata [index] / 256);
		position++;

		//  get low byte for 2nd character
		string [position] = (char) (diskdata [index] % 256);
		position++;
	}

	//  end the string 
	string [position] = '\0';

	//  cut off the trailing blanks
	for (index = position - 1; index > 0 && ' ' == string [index]; index--)
		string [index] = '\0';

	int nRet = 0;
	WCHAR serialNum[MAX_PATH] = {0};
	nRet = MultiByteToWideChar(CP_ACP,
		0, 
		string,
		-1,
		serialNum,
		MAX_PATH);
	if (0 == nRet) { return; }

	std::wstring tmpStr(serialNum);
	hardDiskSerialNo = tmpStr;

	return;
}


void GetHardDiskSerialNumber (std::wstring& hardDiskSerialNo)
{
	int done = FALSE;
	//  this works under WinNT4 or Win2K if you have admin rights
	done = ReadPhysicalDriveInNT (hardDiskSerialNo);

	//  this should work in WinNT or Win2K if previous did not work
	//  this is kind of a backdoor via the SCSI mini port driver into
	//  the IDE drives
	if ( ! done) done = ReadIdeDriveAsScsiDriveInNT (hardDiskSerialNo);
	if (!done)
	{
		hardDiskSerialNo = L"UNKNOWN";
	}
	return;
}

int AstringToWstring(const std::string &aStringA, std::wstring &aStringW)
{
	const int BUFFER_SIZE = 512;
	wchar_t basememory[BUFFER_SIZE] = {0};
	wchar_t *widechar = basememory;

	int chars = ::MultiByteToWideChar(CP_ACP, 0, aStringA.c_str(), -1, NULL, 0);
	if (0 < chars)
	{
		if (chars >= BUFFER_SIZE)
		{
			wchar_t *widechar = new wchar_t[chars+1];
			ZeroMemory(widechar, sizeof(wchar_t)*(chars+1));
		}

		if (NULL != widechar)
		{
			chars = ::MultiByteToWideChar(CP_ACP, 0, aStringA.c_str(), -1, widechar, chars);
			if (0 < chars)
			{
				aStringW.assign(widechar);
			}

			if (chars >= BUFFER_SIZE)
			{
				delete[] widechar;
				widechar = NULL;
			}

			return chars;
		}
	}

	return 0;
}

//获取IP地址
bool GetComputerIp(std::wstring &aIp)
{
	bool Ret = false;
	char ComputerName[MAX_PATH] = {0};
	wchar_t IpAddress[MAX_PATH] = {0};
	std::string Ip; 
	struct hostent* HostInfo = NULL;
	struct in_addr addr;

	if(gethostname(ComputerName,sizeof(ComputerName)) == 0) 
	{
		//如果能获取计算机主机信息的话，则获取本机IP地址
		HostInfo = gethostbyname(ComputerName);
		if(HostInfo != NULL) 
		{
			addr.s_addr = *(unsigned long *)HostInfo->h_addr_list[0];
			char *IpChar = inet_ntoa(addr);
			if (IpChar)
			{
				if (AstringToWstring(std::string(IpChar), aIp) > 0)
				{
					Ret = true;
				}
			}
		} 
	}

	return Ret;
}


//判断是哪一个适配器
bool IsLocalAdapter(const wchar_t *aAdapter)
{
	bool Ret = false;
	if (aAdapter)
	{
#define NET_CARD_KEY L"System\\CurrentControlSet\\Control\\Network\\{4D36E972-E325-11CE-BFC1-08002BE10318}"
		wchar_t DataBuf[MAX_PATH+1] = {0};
		unsigned long DataLen = MAX_PATH;
		unsigned long Type = REG_SZ;
		HKEY NetKey = NULL;
		HKEY LocalNet = NULL;

		if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, NET_CARD_KEY, 0, KEY_READ, &NetKey) == ERROR_SUCCESS)
		{
			wsprintf(DataBuf, L"%s\\Connection", aAdapter);
			if(RegOpenKeyEx(NetKey, DataBuf, 0, KEY_READ, &LocalNet) == ERROR_SUCCESS)
			{
				if (RegQueryValueExW(LocalNet, L"PnpInstanceID", 0, &Type, (BYTE *)DataBuf, &DataLen) == ERROR_SUCCESS)
				{
					if (memcmp(DataBuf, L"PCI", wcslen(L"PCI")) == 0)
					{
						Ret = true;
					}
				}
			}
		}

		if (LocalNet)
		{
			RegCloseKey(LocalNet);
			LocalNet = NULL;
		}

		if (NetKey)
		{
			RegCloseKey(NetKey);
			NetKey = NULL;
		}
	}

	return Ret;
}

std::wstring WstringFormat(const wchar_t* aFormat, ...)
{
	std::wstring iReturn = L"";
	if (aFormat)
	{
		va_list iParam = NULL;
		va_start(iParam, aFormat); // Init params
		size_t iLength = _vscwprintf(aFormat, iParam) + 1; // Get format length
		std::vector<wchar_t> iWcharArray(iLength, L'\0');
		size_t iWritten = _vsnwprintf_s(&iWcharArray[0], iWcharArray.size(), iLength, aFormat, iParam);
		if (0 < iWritten)
			iReturn = &iWcharArray[0];
		va_end(iParam);
	}

	return iReturn;
}

bool GetComputerMac(std::wstring &aMac, const std::wstring &aIp, bool aIpJudge)
{
	bool Ret = false;
	IP_ADAPTER_INFO *IpAdaptersInfo =NULL;
	IP_ADAPTER_INFO *IpAdaptersInfoHead =NULL;
	unsigned long DataSize = sizeof(IP_ADAPTER_INFO);

	IpAdaptersInfo = (IP_ADAPTER_INFO *)GlobalAlloc(GPTR, DataSize);
	if (IpAdaptersInfo != NULL)
	{
		unsigned RetVal = GetAdaptersInfo(IpAdaptersInfo, &DataSize);
		if (RetVal != ERROR_SUCCESS)
		{
			GlobalFree(IpAdaptersInfo);
			IpAdaptersInfo = NULL;
			if(RetVal == ERROR_BUFFER_OVERFLOW)
			{
				IpAdaptersInfo = (IP_ADAPTER_INFO *)GlobalAlloc(GPTR, DataSize);
				if (IpAdaptersInfo == NULL)
				{
					return Ret;
				}
				else
				{
					if (GetAdaptersInfo(IpAdaptersInfo, &DataSize) != ERROR_SUCCESS)
					{
						GlobalFree(IpAdaptersInfo);
						IpAdaptersInfo = NULL;
						return Ret;
					}
				}
			}
			else
			{
				return Ret;
			}
		}

		IpAdaptersInfoHead = IpAdaptersInfo;
		std::wstring AdapterName;
		std::wstring Ip;
		do
		{
			//依据Ip判定筛选网卡
			if (aIpJudge)
			{
				AstringToWstring(std::string(IpAdaptersInfo->IpAddressList.IpAddress.String), Ip);
				if (!aIp.empty() && _wcsicmp(aIp.c_str(), Ip.c_str()) == 0)
				{
					aMac = WstringFormat(L"%02x-%02x-%02x-%02x-%02x-%02x", 
						(int)IpAdaptersInfo->Address[0],
						(int)IpAdaptersInfo->Address[1],
						(int)IpAdaptersInfo->Address[2],
						(int)IpAdaptersInfo->Address[3],
						(int)IpAdaptersInfo->Address[4],
						(int)IpAdaptersInfo->Address[5]);

					Ret = true;
					break;
				}
			}//依据注册表筛选网卡
			else
			{
				AstringToWstring(std::string(IpAdaptersInfo->AdapterName), AdapterName);
				if (IsLocalAdapter(AdapterName.c_str()))
				{
					aMac = WstringFormat(L"%02x-%02x-%02x-%02x-%02x-%02x", 
						(int)IpAdaptersInfo->Address[0],
						(int)IpAdaptersInfo->Address[1],
						(int)IpAdaptersInfo->Address[2],
						(int)IpAdaptersInfo->Address[3],
						(int)IpAdaptersInfo->Address[4],
						(int)IpAdaptersInfo->Address[5]);

					Ret = true;
					break;
				}
			}

			IpAdaptersInfo = IpAdaptersInfo->Next;
		}while (IpAdaptersInfo);

		if (IpAdaptersInfoHead != NULL)
		{
			GlobalFree(IpAdaptersInfoHead);

			IpAdaptersInfoHead = NULL;
		}

	}

	return Ret;
}

CGetCpuAndBiosInfo::CGetCpuAndBiosInfo(void)
{
}

CGetCpuAndBiosInfo::~CGetCpuAndBiosInfo(void)
{
}
CString CGetCpuAndBiosInfo::GetBIOSId()
{
	HKEY   hKey; 
	LPCTSTR   StrKey= _T("HARDWARE\\DESCRIPTION\\System\\BIOS");
	LONG Lret=RegOpenKeyEx(HKEY_LOCAL_MACHINE,StrKey,NULL,KEY_READ,&hKey);
	if(ERROR_SUCCESS==RegOpenKeyEx(HKEY_LOCAL_MACHINE,StrKey,NULL,KEY_READ,&hKey)) 
	{ 
		DWORD   dwSize=255,dwType=REG_SZ; 
		char   bytedate[256]; 
		if   (ERROR_SUCCESS==RegQueryValueEx(hKey,_T("BIOSVendor"),0,&dwType,(BYTE   *)bytedate,&dwSize)) 
		{ 
			m_strbiosid.Format(_T("%s"),bytedate);

		} 
		RegCloseKey(hKey); 
	} 
	return m_strbiosid;

}
CString CGetCpuAndBiosInfo::GetCpuId()
{

	unsigned   long   s1,s2; 
	char   sel; 
	sel= '1'; 
	CString   VernderID; 
	CString   MyCpuID,CPUID1,CPUID2; 
	switch(sel) 
	{ 
	case   '1': 
		__asm{ 
			mov   eax,01h 
				xor   edx,edx 
				cpuid 
				mov   s1,edx 
				mov   s2,eax 
		} 
		CPUID1.Format( _T("%08X%08X"),s1,s2); 
		__asm{ 
			mov   eax,03h 
				xor   ecx,ecx 
				xor   edx,edx 
				cpuid 
				mov   s1,edx 
				mov   s2,ecx 
		} 
		CPUID2.Format( _T("%08X%08X "),s1,s2); 
		break; 
	case   '2': 
		{ 
			__asm{ 
				mov   ecx,119h 
					rdmsr 
					or   eax,00200000h 
					wrmsr 
			} 
		} 
		//AfxMessageBox( "CPU   id   is   disabled. "); 
		break; 
	} 
	m_strcpuid   =  CPUID1+CPUID2; 
	return m_strcpuid;


}

}