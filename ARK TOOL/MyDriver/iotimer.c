#include "MyIoTimer.h"
#include "MyCallBack.h"


VOID EnumIoTimer(PDRIVER_OBJECT driverObject, PIOTIMERINFO* piotimerinfo)
{
	//1.获得IoInitializeTimer的函数地址
	UNICODE_STRING uniVariableName;
	RtlInitUnicodeString(&uniVariableName, L"IoInitializeTimer");
	ULONG FuncAddr = 0;
	FuncAddr = (ULONG)MmGetSystemRoutineAddress(&uniVariableName);
	if (FuncAddr == NULL)
	{
		return ;
	}

	//2.搜索IoTimerQueueHead
	PLIST_ENTRY IoTimerQueueHead = NULL;
	for (ULONG i = FuncAddr; i < FuncAddr + 0x500; i++)
	{
		if (*(PUCHAR)i == 0xb9)
		{
			IoTimerQueueHead = (PLIST_ENTRY)*(PULONG)(i + 1);
			DbgPrint("IoTimerQueueHead为%x\n", IoTimerQueueHead);
			break;
		}
	}
	if (IoTimerQueueHead == NULL)
	{
		return ;
	}

	//3.保存旧的中断等级
	KIRQL OldIrql;
	OldIrql = KeRaiseIrqlToDpcLevel();

	//4.判断地址是否有效
	if (IoTimerQueueHead && MmIsAddressValid((PVOID)IoTimerQueueHead))
	{
		PLIST_ENTRY NextEntry = IoTimerQueueHead->Flink;
		//遍历LIST_ENTRY
		while (MmIsAddressValid(NextEntry) && NextEntry != (PLIST_ENTRY)IoTimerQueueHead)
		{
			PIO_TIMER Timer = CONTAINING_RECORD(NextEntry, IO_TIMER, TimerList);
			
			if (Timer && MmIsAddressValid(Timer))
			{
				(*piotimerinfo)->TimerAddress = Timer;
				(*piotimerinfo)->Type = Timer->Type;
				(*piotimerinfo)->Flag = Timer->TimerFlag;
				(*piotimerinfo)->Device = Timer->DeviceObject;
				(*piotimerinfo)->Context = Timer->Context;
				(*piotimerinfo)->FuncAddress = Timer->TimerRoutine;

				PUNICODE_STRING puni = NULL;
				puni = FindKernelModule(driverObject, Timer->TimerRoutine);
				if (puni)
				{
					RtlCopyMemory((*piotimerinfo)->ImgPath, puni->Buffer, puni->Length);
				}

				(*piotimerinfo)++;
			}
			NextEntry = NextEntry->Flink;
		}
	}

	//5.恢复旧的中断等级
	KeLowerIrql(OldIrql);
}