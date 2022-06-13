#pragma once
#include "ntifs.h"

typedef struct _ServiceDesriptorEntry
{
	ULONG* ServiceTableBase;		//服务表基址
	ULONG* ServiceCounterTableBase;	//函数表中每个函数被调用的次数
	ULONG NumberOfService;			//服务函数的个数,
	ULONG ParamTableBase;			//参数表基址
}SSDTEntry, *PSSDTEntry;

// 导入SSDT全局变量
NTSYSAPI SSDTEntry KeServiceDescriptorTable;

//SSDT信息结构体
typedef struct _SSDTINFO
{
	ULONG uIndex;	//回调号
	ULONG uFuntionAddr;	//函数地址
}SSDTINFO, *PSSDTINFO;