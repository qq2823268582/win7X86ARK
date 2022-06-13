#include "MyCallBack.h"
#include "Myprocess.h"

extern PDRIVER_OBJECT driver1;

//����һ����ַ���ӱ�����ģ�鿪ʼ�������ж��������ģ���PUNICODE_STRING��ʽ������ 
PUNICODE_STRING  FindKernelModule(PDRIVER_OBJECT pDriverObj, ULONG funcAddr)
{
	PLDR_DATA_TABLE_ENTRY p = (PLDR_DATA_TABLE_ENTRY)pDriverObj->DriverSection;
	PLIST_ENTRY pList, pStartList;
	pList = pStartList = (PLIST_ENTRY)p;

	do
	{
		p = (PLDR_DATA_TABLE_ENTRY)pList;
		if (p->DllBase != 0)
		{
			if ((ULONG)p->DllBase <= funcAddr && ((ULONG)p->DllBase + p->SizeOfImage) > funcAddr)
			{
				return  &p->BaseDllName;
			}
		}
		pList = pList->Flink;
	} while (pList != pStartList);
	return NULL;
}

//��ȡPspCreateProcessNotifyRoutine�ĵ�ַ
ULONG GetPspCreateProcessNotifyRoutine()
{
	//1.���PsSetCreateProcessNotifyRoutine�ĵ�ַ
	UNICODE_STRING	unstrFunc;
	RtlInitUnicodeString(&unstrFunc, L"PsSetCreateProcessNotifyRoutine");
	ULONG FuncAddr1 = 0;
	FuncAddr1 = (ULONG)MmGetSystemRoutineAddress(&unstrFunc);
	//DbgPrint("��һ�������PsSetCreateProcessNotifyRoutine�ĵ�ַ�� %x \n", FuncAddr1);

	//2.���������ַ��Ч�����ߵ�ַ���һ�ξ���Ҳ��Ч���򷵻�ʧ��
	if (!FuncAddr1)
	{
		//DbgPrint("��һ�������PsSetCreateProcessNotifyRoutine��ַʧ��\n");
		return 0;
	}

	//3.��PsSetCreateProcessNotifyRoutine�ĵ�ַΪ��ַ���Χ0x2f��С��������
	ULONG FuncAddr2 = 0;
	for (ULONG i = FuncAddr1; i < FuncAddr1 + 0x2f; i++)
	{
		//3.1 �����룺0xe8
		if (*(PUCHAR)i == 0x08 && *(PUCHAR)(i + 1) == 0xe8)
		{
			LONG OffsetAddr1 = 0;
			memcpy(&OffsetAddr1, (PUCHAR)(i + 2), 4);
			FuncAddr2 = i + OffsetAddr1 + 6;
			//DbgPrint("�ڶ��������PspSetCreateProcessNotifyRoutine�ĵ�ַ %x", FuncAddr2);
			break;
		}
	}

	//4.���û�������������룬���ߵ�ַ���һ�ξ���Ҳ��Ч������ʧ�ܵĽ��
	if (!FuncAddr2)
	{
		//DbgPrint("�ڶ��������PspSetCreateProcessNotifyRoutine��ַʧ��\n");
		return 0;
	}

	//5.��PspSetCreateProcessNotifyRoutineΪ��ַ���0xff��Χ������������
	for (ULONG j = FuncAddr2; j < FuncAddr2 + 0xff; j++)
	{
		if (*(PUCHAR)j == 0xc7 && *(PUCHAR)(j + 1) == 0x45 && *(PUCHAR)(j + 2) == 0x0c)
		{
			LONG OffsetAddr2 = 0;
			memcpy(&OffsetAddr2, (PUCHAR)(j + 3), 4);
			//DbgPrint("�����������PspCreateProcessNotifyRoutine�ĵ�ַ:%x", OffsetAddr2);
			return OffsetAddr2;
		}
	}

	//6.���û�������������룬����ʧ�ܵĽ��
	//DbgPrint("�����������PspCreateProcessNotifyRoutine��ַʧ��\n");
	return 0;
}
//�������̻ص�
void EnumCreateProcessNotify(PDRIVER_OBJECT pDriverObj, PNOTIFYINFO* pnotify)
{
	//1.���PspCreateProcessNotifyRoutine�ĵ�ַ
	ULONG PspCreateProcessNotifyRoutine = 0;
	PspCreateProcessNotifyRoutine = GetPspCreateProcessNotifyRoutine();
	if (!PspCreateProcessNotifyRoutine)
		return;

	//2.����
	ULONG NotifyAddr = 0;
	PUNICODE_STRING puni = NULL;
	for (ULONG i = 0; i < 64; i++)
	{
		//1.ȡ�������е�ÿһ�������ֵ   //ÿһ��ĳ���Ϊ4��X64��ÿһ���Ϊ8
		NotifyAddr = *(PULONG)(PspCreateProcessNotifyRoutine + i * 4);
		//2.�ж�ֵ�Ƿ���Ч��ֵ��Ӧ�ĵ�ַ�Ƿ���Ч
		if (MmIsAddressValid((PVOID)NotifyAddr) && NotifyAddr != 0)
		{
			//DbgPrint("NotifyAddr= %x \n", NotifyAddr);
			NotifyAddr = *(PULONG)((NotifyAddr & 0xfffffff8) + 4);

			(*pnotify)->CallbacksAddr = NotifyAddr;
			(*pnotify)->CallbackType = 1;
			puni = FindKernelModule(pDriverObj, NotifyAddr);
			if (puni)
			{
				RtlCopyMemory((*pnotify)->ImgPath, puni->Buffer, puni->Length);
				//DbgPrint("ImgPath= %ws \n", (*pnotify)->ImgPath);
			}

			(*pnotify)++;
		}
	}

}

//�����̻߳ص�����ĵ�ַ
ULONG SearchPspCreateThreadNotifyRoutine()
{	
	//1.��ȡPsSetCreateThreadNotifyRoutine������ַ
	UNICODE_STRING uStrFuncName = RTL_CONSTANT_STRING(L"PsSetCreateThreadNotifyRoutine");
	ULONG FuncAddr = 0;
	FuncAddr = (ULONG)MmGetSystemRoutineAddress(&uStrFuncName);
	//DbgPrint("��һ������ȡPsSetCreateThreadNotifyRoutine�ĵ�ַΪ: %x\n", FuncAddr);

	//2.��ȡPspCreateThreadNotifyRoutine���������̻߳ص�������
	LONG ThreadNotifyAddr = 0;
	for (ULONG i = FuncAddr; i < FuncAddr + 0x2f; i++)
	{
		__try
		{
			if ((*(PUCHAR)i == 0xbe))
			{
				RtlCopyMemory(&ThreadNotifyAddr, (PUCHAR)(i + 1), 4);
				//DbgPrint("�ڶ��������PspCreateThreadNotifyRoutine�ĵ�ַ %x\n", ThreadNotifyAddr);

				return ThreadNotifyAddr;
			}
		}
		__except (1)
		{
			ThreadNotifyAddr = 0;
			break;
		}
	}

	//3.���û��������������ʧ�ܵĽ��
	return ThreadNotifyAddr;
}
//�����̻߳ص�
void EnumCreateThreadNotify(PDRIVER_OBJECT pDriverObj, PNOTIFYINFO* pnotify)
{
	ULONG PspCreateThreadNotifyRoutine = SearchPspCreateThreadNotifyRoutine();

	if (!PspCreateThreadNotifyRoutine)
	{
		return;
	}

	//2.����
	ULONG NotifyAddr = 0;
	PUNICODE_STRING puni = NULL;
	for (ULONG i = 0; i < 64; i++)
	{
		//1.ȡ�������е�ÿһ�������ֵ   //ÿһ��ĳ���Ϊ4��X64��ÿһ���Ϊ8
		NotifyAddr = *(PULONG)(PspCreateThreadNotifyRoutine + i * 4);
		//2.�ж�ֵ�Ƿ���Ч��ֵ��Ӧ�ĵ�ַ�Ƿ���Ч
		if (MmIsAddressValid((PVOID)NotifyAddr) && NotifyAddr != 0)
		{
			//DbgPrint("NotifyAddr= %x \n", NotifyAddr);
			NotifyAddr = *(PULONG)((NotifyAddr & 0xfffffff8) + 4);	

			(*pnotify)->CallbacksAddr = NotifyAddr;
			(*pnotify)->CallbackType = 2;
			puni = FindKernelModule(pDriverObj, NotifyAddr);
			if (puni)
			{
				RtlCopyMemory((*pnotify)->ImgPath, puni->Buffer, puni->Length);
			}

			(*pnotify)++;
		}
	}
}


//���PspLoadImageNotifyRoutine�ĵ�ַ
ULONG GetPspLoadImageNotifyRoutine()
{
	//1.��ȡPsSetLoadImageNotifyRoutine��ַ
	UNICODE_STRING uStrFuncName = RTL_CONSTANT_STRING(L"PsSetLoadImageNotifyRoutine");
	ULONG FuncAddr = 0;
	FuncAddr = (ULONG)MmGetSystemRoutineAddress(&uStrFuncName);
	//DbgPrint("��һ������ȡPsSetLoadImageNotifyRoutine�ĵ�ַΪ: %x\n", FuncAddr);

	//2.����������PspLoadImageNotifyRoutine
	ULONG ImageNotify = 0;
	for (ULONG i = FuncAddr; i < FuncAddr + 0x2f; i++)
	{
		__try
		{
			if ((*(PUCHAR)i == 0x74) && (*(PUCHAR)(i + 1) == 0x25) && (*(PUCHAR)(i + 2) == 0xBE))
			{
				RtlCopyMemory(&ImageNotify, (PUCHAR)(i + 3), 4);
				//DbgPrint("�ڶ��������PspLoadImageNotifyRoutine�ĵ�ַ %x\n", ImageNotify);
				return ImageNotify;
			}
		}
		__except (1)
		{
			ImageNotify = 0;
			break;
		}
	}

	return ImageNotify;
}
//����ImageNotify
VOID EnumImageNotify(PDRIVER_OBJECT pDriverObj, PNOTIFYINFO* pnotify)
{
	ULONG PspLoadImageNotifyRoutine = GetPspLoadImageNotifyRoutine();

	if (!PspLoadImageNotifyRoutine)
	{
		return;
	}

	ULONG NotifyAddr = 0, MagicPtr = 0;
	PUNICODE_STRING puni = NULL;
	for (ULONG i = 0; i < 64; i++)
	{
		MagicPtr = PspLoadImageNotifyRoutine + i * 4;

		NotifyAddr = *(PULONG)(MagicPtr);

		if (MmIsAddressValid((PVOID)NotifyAddr) && NotifyAddr != 0)
		{
			NotifyAddr = *(PULONG)((NotifyAddr & 0xfffffff8) + 4);			
			(*pnotify)->CallbacksAddr = NotifyAddr;
			(*pnotify)->CallbackType = 3;
			puni = FindKernelModule(pDriverObj, NotifyAddr);
			if (puni)
			{
				RtlCopyMemory((*pnotify)->ImgPath, puni->Buffer, puni->Length);
				//DbgPrint("ImgPath= %ws \n", (*pnotify)->ImgPath);
			}
			(*pnotify)++;
		}
	}
}

//����ע���ص����������ͷ
ULONG SearchCallbackListHead()
{
	//1.���CmRegisterCallback�ĺ�����ַ
	UNICODE_STRING uStrFuncName = RTL_CONSTANT_STRING(L"CmRegisterCallback");
	ULONG FuncAddr1 = 0;
	FuncAddr1 = (ULONG)MmGetSystemRoutineAddress(&uStrFuncName);
	//DbgPrint("step1:CmRegisterCallback�ĵ�ַΪ��%x\r\n", FuncAddr1);
	if (!FuncAddr1)
	{
		//DbgPrint("MmGetSystemRoutineAddress Error\r\n");
		return 0;
	}

	//2.�������CmpRegisterCallbackInternal�ĺ�����ַ
	ULONG FuncAddr2 = 0;
	for (ULONG i = FuncAddr1; i < FuncAddr1 + 0x2f; i++)
	{
		__try
		{
			if ((*(PUCHAR)i == 0x08) && (*(PUCHAR)(i + 1) == 0xe8))
			{
				LONG OffsetAddr = 0;
				RtlCopyMemory(&OffsetAddr, (PUCHAR)(i + 2), 4);
				FuncAddr2 = OffsetAddr + 6 + i;
				//DbgPrint("step2:CmpRegisterCallbackInternal�ĵ�ַΪ��%x\r\n", FuncAddr2);
				break;
			}
		}
		__except (1)
		{
			FuncAddr2 = 0;
			break;
		}
	}

	if (!FuncAddr2)
	{
		//DbgPrint("CmpRegisterCallbackInternal Error\r\n");
		return 0;
	}

	//3.�������CmpInsertCallbackInListByAltitude�ĺ�����ַ
	ULONG FuncAddr3 = 0;
	for (ULONG j = FuncAddr2; j < FuncAddr2 + 0xff; j++)
	{
		__try
		{
			if ((*(PUCHAR)j == 0x8b) && (*(PUCHAR)(j + 1) == 0xc6) && (*(PUCHAR)(j + 2) == 0xe8))
			{
				LONG OffsetAddr = 0;
				RtlCopyMemory(&OffsetAddr, (PUCHAR)(j + 3), 4);
				FuncAddr3 = OffsetAddr + 7 + j;
				//DbgPrint("step3:CmpInsertCallbackInListByAltitude�ĵ�ַΪ��%x\r\n", FuncAddr3);
				break;
			}
		}
		__except (1)
		{
			FuncAddr3 = 0;
			break;
		}
	}

	if (!FuncAddr3)
	{
		//DbgPrint("CmpInsertCallbackInListByAltitude Error\r\n");
		return 0;
	}

	//4.����CallbackListHead
	ULONG ListHead = 0;
	for (ULONG k = FuncAddr3; k < FuncAddr3 + 0xff; k++)
	{
		__try
		{
			if (*(PUCHAR)k == 0xBB)
			{
				RtlCopyMemory(&ListHead, (PUCHAR)(k + 1), 4);
				//DbgPrint("step4:CallbackListHead�ĵ�ַΪ��%x\r\n", ListHead);
				break;
			}
		}
		__except (1)
		{
			ListHead = 0;
			break;
		}
	}

	if (!ListHead)
	{
		//DbgPrint("CallbackListHead Error\r\n");
		return 0;
	}

	return ListHead;
}

//����ע���ص�����
VOID EnumCallBackList(PDRIVER_OBJECT pDriverObj, PNOTIFYINFO* pnotify)
{
	//1.���ע���ص�����ͷ
	ULONG pHead = 0;
	pHead = SearchCallbackListHead();
	if (!pHead)
	{
		return;
	}

	//2.ȡ������ͷ�еĽڵ�pList_First
	PLIST_ENTRY pList_Temp = NULL;
	pList_Temp = (PLIST_ENTRY)(*(PULONG)pHead);

	//4.ѭ����������
	while ((ULONG)pList_Temp != pHead)
	{
		//��������ַ��Ч��˵���ǿ�����
		if (!MmIsAddressValid(pList_Temp))
		{
			break;
		}

		//��ȡpCookie(�����ŵ���Cookie��
		PLARGE_INTEGER  pLiRegCookie = NULL;
		pLiRegCookie = (PLARGE_INTEGER)((ULONG)pList_Temp + 0x10);

		//��ȡpFuncAddr�������ŵ���FuncAddr��
		PULONG pFuncAddr = 0;
		pFuncAddr = (PULONG)((ULONG)pList_Temp + 0x1C);
		PUNICODE_STRING puni = NULL;
		//�жϵ�ַ�Ƿ���Ч
		if (MmIsAddressValid(pFuncAddr) && MmIsAddressValid(pLiRegCookie))
		{
			(*pnotify)->CallbacksAddr = *pFuncAddr;
			(*pnotify)->CallbackType = 4;
			(*pnotify)->Cookie = (*pLiRegCookie).QuadPart;
			puni = FindKernelModule(pDriverObj, *pFuncAddr);
			if (puni)
			{
				RtlCopyMemory((*pnotify)->ImgPath, puni->Buffer, puni->Length);
				//DbgPrint("ImgPath= %ws \n", (*pnotify)->ImgPath);
			}
			(*pnotify)++;
			//DbgPrint("FuncAddrΪ��%x \n", *pFuncAddr);
			//DbgPrint("CookieΪ��%x \n", *pLiRegCookie);

		}
		//����ָ����һ���ڵ�
		pList_Temp = pList_Temp->Flink;
	}
}

//------------------------------------------------------------------------------------
//��������ص�
// ��ȡ���̶������ͻص�
VOID EnumProcessObCallback(PDRIVER_OBJECT pDriverObj, POBCALLBACKINFO* pobcallback)
{
	POB_CALLBACK pObCallback = NULL;

	// ֱ�ӻ�ȡ CallbackList ����
	LIST_ENTRY CallbackList = ((POBJECT_TYPE)(*PsProcessType))->CallbackList;

	// ��ʼ����
	pObCallback = (POB_CALLBACK)CallbackList.Flink;

	do
	{
		//���ȹ��˵�pObCallback��Ч��
		if (!MmIsAddressValid(pObCallback))
		{
			break;
		}
		//���Ź��˵�ObHandleΪ�ջ���Ч��
		if (pObCallback->ObHandle == NULL || !MmIsAddressValid(pObCallback->ObHandle))
		{
			break;
		}

		//���˵�Object��ַ��Ч��
		if (pObCallback->ObTypeAddr == NULL || !MmIsAddressValid(pObCallback->ObTypeAddr))
		{
			break;
		}

		if (pObCallback->PreCall&&MmIsAddressValid(pObCallback->PreCall))
		{		
			//1.Object��ַ
			DbgPrint("[����]Object��ַ = 0x%p\n", pObCallback->ObTypeAddr);
			(*pobcallback)->ObjectAddress = pObCallback->ObTypeAddr;

			//2.����
			(*pobcallback)->ObjectType = 1;

			//3.ǰ/��
			(*pobcallback)->CallType = 1;

			//4.�߶�
			PUNICODE_STRING puni3 = NULL;
			puni3 = &pObCallback->ObHandle->Altitude;
			DbgPrint("[����]Altitude = %wZ\n", puni3);
			RtlCopyMemory((*pobcallback)->Altitude, puni3->Buffer, puni3->Length);
			
			//5.ǰ�ص�������ַ
			DbgPrint("[����]PreCall = 0x%p\n", pObCallback->PreCall);
			(*pobcallback)->FuncAddress = pObCallback->PreCall;

			//6.ǰ�ص���������ģ��
			PUNICODE_STRING puni1 = NULL;
			puni1 = FindKernelModule(pDriverObj, pObCallback->PreCall);
			if (puni1)
			{
				DbgPrint("[����]PreCall·�� = %wZ\n", puni1);
				RtlCopyMemory((*pobcallback)->ImgPath, puni1->Buffer, puni1->Length);				
			}

			(*pobcallback)++;
		}


		if (pObCallback->PostCall&&MmIsAddressValid(pObCallback->PostCall))
		{
			//1.Object��ַ
			DbgPrint("[����]Object��ַ = 0x%p\n", pObCallback->ObTypeAddr);
			(*pobcallback)->ObjectAddress = pObCallback->ObTypeAddr;

			//2.����
			(*pobcallback)->ObjectType = 1;

			//3.ǰ/��
			(*pobcallback)->CallType = 2;

			//4.�߶�
			PUNICODE_STRING puni4 = NULL;
			puni4 = &pObCallback->ObHandle->Altitude;
			DbgPrint("[����]Altitude = %wZ\n", puni4);
			RtlCopyMemory((*pobcallback)->Altitude, puni4->Buffer, puni4->Length);

			//5.��ص�������ַ
			DbgPrint("[����]PostCall = 0x%p\n", pObCallback->PostCall);
			(*pobcallback)->FuncAddress = pObCallback->PostCall;

			//6.��ص���������ģ��
			PUNICODE_STRING puni2 = NULL;
			puni2 = FindKernelModule(pDriverObj, pObCallback->PostCall);
			if (puni2)
			{
				DbgPrint("[����]PostCall·�� = %wZ\n", puni2);
				RtlCopyMemory((*pobcallback)->ImgPath, puni2->Buffer, puni2->Length);
			}

			(*pobcallback)++;
		}


		// ��ȡ��һ������Ϣ
		pObCallback = (POB_CALLBACK)pObCallback->ListEntry.Flink;

	} while (CallbackList.Flink != (PLIST_ENTRY)pObCallback);
	
}


// ��ȡ�̶߳������ͻص�
VOID EnumThreadObCallback(PDRIVER_OBJECT pDriverObj, POBCALLBACKINFO* pobcallback)
{
	POB_CALLBACK pObCallback = NULL;

	// ֱ�ӻ�ȡ CallbackList ����
	LIST_ENTRY CallbackList = ((POBJECT_TYPE)(*PsThreadType))->CallbackList;

	// ��ʼ����
	pObCallback = (POB_CALLBACK)CallbackList.Flink;

	do
	{
		//���ȹ��˵�pObCallback��Ч��
		if (!MmIsAddressValid(pObCallback))
		{
			break;
		}
		//���Ź��˵�ObHandleΪ�ջ���Ч��
		if (pObCallback->ObHandle == NULL || !MmIsAddressValid(pObCallback->ObHandle))
		{
			break;
		}

		//���˵�Object��ַ��Ч��
		if (pObCallback->ObTypeAddr == NULL || !MmIsAddressValid(pObCallback->ObTypeAddr))
		{
			break;
		}

		if (pObCallback->PreCall&&MmIsAddressValid(pObCallback->PreCall))
		{
			//1.Object��ַ
			(*pobcallback)->ObjectAddress = pObCallback->ObTypeAddr;

			//2.����
			(*pobcallback)->ObjectType = 2;

			//3.ǰ/��
			(*pobcallback)->CallType = 1;

			//4.�߶�
			PUNICODE_STRING puni3 = NULL;
			puni3 = &pObCallback->ObHandle->Altitude;
			DbgPrint("[�߳�]Altitude = %wZ\n", puni3);
			RtlCopyMemory((*pobcallback)->Altitude, puni3->Buffer, puni3->Length);

			//5.ǰ�ص�������ַ
			(*pobcallback)->FuncAddress = pObCallback->PreCall;

			//6.ǰ�ص���������ģ��
			PUNICODE_STRING puni1 = NULL;
			puni1 = FindKernelModule(pDriverObj, pObCallback->PreCall);
			if (puni1)
			{				
				RtlCopyMemory((*pobcallback)->ImgPath, puni1->Buffer, puni1->Length);
			}

			(*pobcallback)++;
		}


		if (pObCallback->PostCall&&MmIsAddressValid(pObCallback->PostCall))
		{
			//1.Object��ַ
			(*pobcallback)->ObjectAddress = pObCallback->ObTypeAddr;

			//2.����
			(*pobcallback)->ObjectType = 2;

			//3.ǰ/��
			(*pobcallback)->CallType = 2;

			//4.�߶�
			PUNICODE_STRING puni4 = NULL;
			puni4 = &pObCallback->ObHandle->Altitude;
			DbgPrint("[�߳�]Altitude = %wZ\n", puni4);
			RtlCopyMemory((*pobcallback)->Altitude, puni4->Buffer, puni4->Length);

			//5.ǰ�ص�������ַ
			(*pobcallback)->FuncAddress = pObCallback->PostCall;

			//6.ǰ�ص���������ģ��
			PUNICODE_STRING puni2 = NULL;
			puni2 = FindKernelModule(pDriverObj, pObCallback->PostCall);
			if (puni2)
			{
				RtlCopyMemory((*pobcallback)->ImgPath, puni2->Buffer, puni2->Length);
			}

			(*pobcallback)++;
		}

		// ��ȡ��һ������Ϣ
		pObCallback = (POB_CALLBACK)pObCallback->ListEntry.Flink;

	} while (CallbackList.Flink != (PLIST_ENTRY)pObCallback);
	
}
