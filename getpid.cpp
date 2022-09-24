#include <Tlhelp32.h>
#include <atlstr.h>
/***************************GetSpecifiedPID*******************************
功能描述：获取指定进程名的进程ID
输入参数：pszProcessName：进程名
输出参数：dwPID：进程ID
返 回 值：TRUE:查找到对应的进程名，获取进程ID成功；FALSE：获取进程ID失败
**************************************************************************/
BOOL  GetSpecifiedPID(DWORD *dwPID,const TCHAR *pszProcessName)
{
	TCHAR tempInfo[MAX_PATH]={0};
	BOOL  bResult=FALSE;
	PROCESSENTRY32 pe;
	CString  strExeFile,strProcessName;
	strProcessName.Format(_T("%s"),pszProcessName);
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);  
	Process32First(hSnapshot, &pe); 
	pe.dwSize=sizeof(pe);
	do{  
		strExeFile.Format(_T("%s"),pe.szExeFile);
		if(0==strProcessName.CompareNoCase(strExeFile))
		{ 
			*dwPID =pe.th32ProcessID; 
			bResult=TRUE;
			break; 
		}
	}while (Process32Next(hSnapshot, &pe)!=FALSE);
	CloseHandle(hSnapshot); 
	return bResult;
}

int _tmain(int argc, _TCHAR* argv[])
{
 
	DWORD pid;
	GetSpecifiedPID(&pid,_T("notepad.exe"));
	cout<<pid<<endl;
	system("PAUSE");
	return 0;
}
