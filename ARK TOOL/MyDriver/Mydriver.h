#pragma once
#include "ntifs.h"


//驱动信息结构体
typedef struct _DRIVERINFO
{
	WCHAR wcDriverBasePath[260];	//驱动名
	WCHAR wcDriverFullPath[260];	//驱动路径
	ULONG DllBase;		//加载基址
	ULONG SizeOfImage;	//大小
}DRIVERINFO, *PDRIVERINFO;