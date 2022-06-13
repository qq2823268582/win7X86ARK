#pragma once
#include "ntifs.h"

typedef struct _ServiceDesriptorEntry
{
	ULONG* ServiceTableBase;		//������ַ
	ULONG* ServiceCounterTableBase;	//��������ÿ�����������õĴ���
	ULONG NumberOfService;			//�������ĸ���,
	ULONG ParamTableBase;			//�������ַ
}SSDTEntry, *PSSDTEntry;

// ����SSDTȫ�ֱ���
NTSYSAPI SSDTEntry KeServiceDescriptorTable;

//SSDT��Ϣ�ṹ��
typedef struct _SSDTINFO
{
	ULONG uIndex;	//�ص���
	ULONG uFuntionAddr;	//������ַ
}SSDTINFO, *PSSDTINFO;