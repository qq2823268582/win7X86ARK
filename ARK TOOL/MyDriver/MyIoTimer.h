#pragma once
#include <ntifs.h>

typedef struct _IO_TIMER
{
	SHORT Type;
	SHORT TimerFlag;
	LIST_ENTRY TimerList;
	PVOID TimerRoutine;
	PVOID Context;
	PDEVICE_OBJECT DeviceObject;
} IO_TIMER, *PIO_TIMER;

typedef struct _IOTIMERINFO
{
	ULONG	    TimerAddress;      //TIMER地址
	ULONG       Type;
	ULONG       Flag;
	ULONG	    Device;             //设备
	ULONG	    Context;            //上下文
	ULONG       FuncAddress;       //函数地址
	WCHAR	    ImgPath[260];      //所属模块
}IOTIMERINFO, *PIOTIMERINFO;

VOID EnumIoTimer(PDRIVER_OBJECT driverObject, PIOTIMERINFO* piotimerinfo);