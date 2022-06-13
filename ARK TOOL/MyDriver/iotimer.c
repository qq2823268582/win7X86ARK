#include "MyIoTimer.h"
#include "MyCallBack.h"


VOID EnumIoTimer(PDRIVER_OBJECT driverObject, PIOTIMERINFO* piotimerinfo)
{
	//1.���IoInitializeTimer�ĺ�����ַ
	UNICODE_STRING uniVariableName;
	RtlInitUnicodeString(&uniVariableName, L"IoInitializeTimer");
	ULONG FuncAddr = 0;
	FuncAddr = (ULONG)MmGetSystemRoutineAddress(&uniVariableName);
	if (FuncAddr == NULL)
	{
		return ;
	}

	//2.����IoTimerQueueHead
	PLIST_ENTRY IoTimerQueueHead = NULL;
	for (ULONG i = FuncAddr; i < FuncAddr + 0x500; i++)
	{
		if (*(PUCHAR)i == 0xb9)
		{
			IoTimerQueueHead = (PLIST_ENTRY)*(PULONG)(i + 1);
			DbgPrint("IoTimerQueueHeadΪ%x\n", IoTimerQueueHead);
			break;
		}
	}
	if (IoTimerQueueHead == NULL)
	{
		return ;
	}

	//3.����ɵ��жϵȼ�
	KIRQL OldIrql;
	OldIrql = KeRaiseIrqlToDpcLevel();

	//4.�жϵ�ַ�Ƿ���Ч
	if (IoTimerQueueHead && MmIsAddressValid((PVOID)IoTimerQueueHead))
	{
		PLIST_ENTRY NextEntry = IoTimerQueueHead->Flink;
		//����LIST_ENTRY
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

	//5.�ָ��ɵ��жϵȼ�
	KeLowerIrql(OldIrql);
}