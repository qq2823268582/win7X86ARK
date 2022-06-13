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
	ULONG	    TimerAddress;      //TIMER��ַ
	ULONG       Type;
	ULONG       Flag;
	ULONG	    Device;             //�豸
	ULONG	    Context;            //������
	ULONG       FuncAddress;       //������ַ
	WCHAR	    ImgPath[260];      //����ģ��
}IOTIMERINFO, *PIOTIMERINFO;

VOID EnumIoTimer(PDRIVER_OBJECT driverObject, PIOTIMERINFO* piotimerinfo);