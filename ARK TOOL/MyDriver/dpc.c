#include "MyDpcTimer.h"
#include "MyCallBack.h"

//����������KiProcessorBlock
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

//����DPC��ʱ��
VOID EnumDpcTimer(PDRIVER_OBJECT pDriverObj, PDPCTIMERINFO* pdpcinfo)
{	
	//1.���KiProcessorBlock�ĵ�ַ
	PULONG   pKiProcessorBlock = NULL;
	pKiProcessorBlock = (PULONG)GetKiProcessorBlock();

	//2.���CPU�ĺ���
	ULONG CPUNumber = KeNumberProcessors;

	//3.����ÿ��CPU
	ULONG j = 0;
	for (j = 0; j < CPUNumber; j++, pKiProcessorBlock++)
	{
		//1.ʹ��ǰ�߳�������ĳ����������
		KeSetSystemAffinityThread(j + 1);   
		
		//2.���KPRCB�Ļ�ַ
		PUCHAR CurrentKPRCBAddress = NULL;
		CurrentKPRCBAddress = pKiProcessorBlock;

		//3.�ָ���ԭ�������ܵ��߳�
		KeRevertToUserAffinityThread();

		//4.���TimerTableEntry��ַ
		PUCHAR CurrentTimerTableEntry = NULL;
		CurrentTimerTableEntry = (PUCHAR)(*(ULONG64*)CurrentKPRCBAddress + 0x1960 + 0x40);

		//5.����TimerTableEntry�����ÿһ��
		ULONG i = 0;
		for (i = 0; i < 0x100; i++)
		{
			//��õ�ǰ���ַ
			PLIST_ENTRY CurrentEntry = NULL;
			CurrentEntry = (PLIST_ENTRY)(CurrentTimerTableEntry + sizeof(KTIMER_TABLE_ENTRY) * i + 4); //��ΪlistEntry����ƫ��0x4
			
			//�����һ���ַ
			PLIST_ENTRY NextEntry = NULL;
			NextEntry = CurrentEntry->Blink;
			
			//�жϵ�ַ�Ƿ���Ч
			if (MmIsAddressValid(CurrentEntry) && MmIsAddressValid(NextEntry))
			{
				//��ʼ����
				while (NextEntry != CurrentEntry)
				{
					//1.���Timer�ṹ����׵�ַ    //LIST_ENTRY����Ƕ��KTIMER�ṹ���е�
					PKTIMER Timer = NULL;
					Timer = CONTAINING_RECORD(NextEntry, KTIMER, TimerListEntry);
		
					//���Timer��RealDpc��RealDpc->DeferredRoutine����Ч����ô���DPC������Ч��
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
							"��ʱ����ַ:[0x%08x]\t  "
							"Dpc��ַ:[0x%08x]\t   "
							"��ʱ������:[%.8d]\t   "
							"DPC������ַ:[0x%08x]\n  "
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
