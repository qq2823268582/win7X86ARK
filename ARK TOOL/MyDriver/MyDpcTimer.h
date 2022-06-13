#pragma once
#include "ntifs.h"

typedef struct _DPCTIMERINFO
{
	ULONG	    TimerAddress;      //TIMER地址
	ULONG	    DPCAddress;        //DPC地址
	ULONG	    Period;            //周期
	ULONG       FuncAddress;       //函数地址
	WCHAR	    ImgPath[260]; //所属模块
}DPCTIMERINFO, *PDPCTIMERINFO;

//总共0x18字节大小    x64总共0x20字节大小
typedef struct _KTIMER_TABLE_ENTRY
{
	ULONG			Lock;    //4字节
	LIST_ENTRY		Entry;   //12字节=8字节链表+4字节空白
	ULARGE_INTEGER	Time;    //8字节
} KTIMER_TABLE_ENTRY, *PKTIMER_TABLE_ENTRY;

VOID EnumDpcTimer(PDRIVER_OBJECT pDriverObj, PDPCTIMERINFO* pdpcinfo);