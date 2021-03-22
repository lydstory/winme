#include "stdafx.h"

CString GetCpuId()
{
 	CString m_strcpuid; 

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
#include <iostream>
using namespace std;
int _tmain(int argc, _TCHAR* argv[])
{
	CString str = GetCpuId();

	std::wcout<<str.GetBuffer()<<std::endl;
	return 0;
}
