#pragma once
#include "ntifs.h"

typedef struct _DPCTIMERINFO
{
	ULONG	    TimerAddress;      //TIMER��ַ
	ULONG	    DPCAddress;        //DPC��ַ
	ULONG	    Period;            //����
	ULONG       FuncAddress;       //������ַ
	WCHAR	    ImgPath[260]; //����ģ��
}DPCTIMERINFO, *PDPCTIMERINFO;

//�ܹ�0x18�ֽڴ�С    x64�ܹ�0x20�ֽڴ�С
typedef struct _KTIMER_TABLE_ENTRY
{
	ULONG			Lock;    //4�ֽ�
	LIST_ENTRY		Entry;   //12�ֽ�=8�ֽ�����+4�ֽڿհ�
	ULARGE_INTEGER	Time;    //8�ֽ�
} KTIMER_TABLE_ENTRY, *PKTIMER_TABLE_ENTRY;

VOID EnumDpcTimer(PDRIVER_OBJECT pDriverObj, PDPCTIMERINFO* pdpcinfo);