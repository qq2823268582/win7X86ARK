#include "MyDpcTimer.h"
#include "MyCallBack.h"

//特征码搜索KiProcessorBlock
ULONG GetKiProcessorBlock()
{
	UNICODE_STRING usFuncName;
	RtlInitUnicodeString(&usFuncName, L"KeSetTimeIncrement");
	ULONG uFuncAddrKeSetTimeIncrement;
	uFuncAddrKeSetTimeIncrement = (ULONG)MmGetSystemRoutineAddress(&usFuncName);
	if (!MmIsAddressValid((PVOID)uFuncAddrKeSetTimeIncrement))
	{
		return 0;
	}

	return *(ULONG *)(uFuncAddrKeSetTimeIncrement + 44);
}

//遍历DPC定时器
VOID EnumDpcTimer(PDRIVER_OBJECT pDriverObj, PDPCTIMERINFO* pdpcinfo)
{	
	//1.获得KiProcessorBlock的地址
	PULONG   pKiProcessorBlock = NULL;
	pKiProcessorBlock = (PULONG)GetKiProcessorBlock();

	//2.获得CPU的核数
	ULONG CPUNumber = KeNumberProcessors;

	//3.遍历每个CPU
	ULONG j = 0;
	for (j = 0; j < CPUNumber; j++, pKiProcessorBlock++)
	{
		//1.使当前线程运行在某个处理器上
		KeSetSystemAffinityThread(j + 1);   
		
		//2.获得KPRCB的基址
		PUCHAR CurrentKPRCBAddress = NULL;
		CurrentKPRCBAddress = pKiProcessorBlock;

		//3.恢复到原来正在跑的线程
		KeRevertToUserAffinityThread();

		//4.获得TimerTableEntry基址
		PUCHAR CurrentTimerTableEntry = NULL;
		CurrentTimerTableEntry = (PUCHAR)(*(ULONG64*)CurrentKPRCBAddress + 0x1960 + 0x40);

		//5.遍历TimerTableEntry里面的每一项
		ULONG i = 0;
		for (i = 0; i < 0x100; i++)
		{
			//获得当前项基址
			PLIST_ENTRY CurrentEntry = NULL;
			CurrentEntry = (PLIST_ENTRY)(CurrentTimerTableEntry + sizeof(KTIMER_TABLE_ENTRY) * i + 4); //因为listEntry是在偏移0x4
			
			//获得下一项基址
			PLIST_ENTRY NextEntry = NULL;
			NextEntry = CurrentEntry->Blink;
			
			//判断地址是否有效
			if (MmIsAddressValid(CurrentEntry) && MmIsAddressValid(NextEntry))
			{
				//开始遍历
				while (NextEntry != CurrentEntry)
				{
					//1.获得Timer结构体的首地址    //LIST_ENTRY是内嵌在KTIMER结构体中的
					PKTIMER Timer = NULL;
					Timer = CONTAINING_RECORD(NextEntry, KTIMER, TimerListEntry);
		
					//如果Timer、RealDpc、RealDpc->DeferredRoutine都有效，那么这个DPC才是有效的
					if (MmIsAddressValid(Timer) && MmIsAddressValid(Timer->Dpc) && MmIsAddressValid(Timer->Dpc->DeferredRoutine))
					{
						(*pdpcinfo)->TimerAddress = Timer;
						(*pdpcinfo)->DPCAddress = Timer->Dpc;
						(*pdpcinfo)->Period = Timer->Period;
						(*pdpcinfo)->FuncAddress = Timer->Dpc->DeferredRoutine;
						PUNICODE_STRING puni = NULL;
						puni = FindKernelModule(pDriverObj, Timer->Dpc->DeferredRoutine);
						if (puni)
						{
							RtlCopyMemory((*pdpcinfo)->ImgPath, puni->Buffer, puni->Length);
						}

						(*pdpcinfo)++;

						DbgPrint(
							"定时器地址:[0x%08x]\t  "
							"Dpc地址:[0x%08x]\t   "
							"定时器周期:[%.8d]\t   "
							"DPC函数地址:[0x%08x]\n  "
							, Timer
							, Timer->Dpc
							, Timer->Period
							, Timer->Dpc->DeferredRoutine
						);
					}
					NextEntry = NextEntry->Blink;
				}
			}
		}
	}

}
