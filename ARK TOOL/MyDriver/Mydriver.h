#pragma once
#include "ntifs.h"


//������Ϣ�ṹ��
typedef struct _DRIVERINFO
{
	WCHAR wcDriverBasePath[260];	//������
	WCHAR wcDriverFullPath[260];	//����·��
	ULONG DllBase;		//���ػ�ַ
	ULONG SizeOfImage;	//��С
}DRIVERINFO, *PDRIVERINFO;