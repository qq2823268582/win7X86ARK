#include "MyCallBack.h"
#include "Myprocess.h"

extern PDRIVER_OBJECT driver1;

//给定一个地址，从本驱动模块开始搜索，判断输出所在模块的PUNICODE_STRING形式的名称 
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

//获取PspCreateProcessNotifyRoutine的地址
ULONG GetPspCreateProcessNotifyRoutine()
{
	//1.获得PsSetCreateProcessNotifyRoutine的地址
	UNICODE_STRING	unstrFunc;
	RtlInitUnicodeString(&unstrFunc, L"PsSetCreateProcessNotifyRoutine");
	ULONG FuncAddr1 = 0;
	FuncAddr1 = (ULONG)MmGetSystemRoutineAddress(&unstrFunc);
	//DbgPrint("第一步：获得PsSetCreateProcessNotifyRoutine的地址： %x \n", FuncAddr1);

	//2.如果函数地址无效，或者地址向后一段距离也无效，则返回失败
	if (!FuncAddr1)
	{
		//DbgPrint("第一步：获得PsSetCreateProcessNotifyRoutine地址失败\n");
		return 0;
	}

	//3.以PsSetCreateProcessNotifyRoutine的地址为基址向后范围0x2f大小进行搜索
	ULONG FuncAddr2 = 0;
	for (ULONG i = FuncAddr1; i < FuncAddr1 + 0x2f; i++)
	{
		//3.1 特征码：0xe8
		if (*(PUCHAR)i == 0x08 && *(PUCHAR)(i + 1) == 0xe8)
		{
			LONG OffsetAddr1 = 0;
			memcpy(&OffsetAddr1, (PUCHAR)(i + 2), 4);
			FuncAddr2 = i + OffsetAddr1 + 6;
			//DbgPrint("第二步：获得PspSetCreateProcessNotifyRoutine的地址 %x", FuncAddr2);
			break;
		}
	}

	//4.如果没有搜索到特征码，或者地址向后一段距离也无效，返回失败的结果
	if (!FuncAddr2)
	{
		//DbgPrint("第二步：获得PspSetCreateProcessNotifyRoutine地址失败\n");
		return 0;
	}

	//5.以PspSetCreateProcessNotifyRoutine为基址向后0xff范围内搜索特征码
	for (ULONG j = FuncAddr2; j < FuncAddr2 + 0xff; j++)
	{
		if (*(PUCHAR)j == 0xc7 && *(PUCHAR)(j + 1) == 0x45 && *(PUCHAR)(j + 2) == 0x0c)
		{
			LONG OffsetAddr2 = 0;
			memcpy(&OffsetAddr2, (PUCHAR)(j + 3), 4);
			//DbgPrint("第三步：获得PspCreateProcessNotifyRoutine的地址:%x", OffsetAddr2);
			return OffsetAddr2;
		}
	}

	//6.如果没有搜索到特征码，返回失败的结果
	//DbgPrint("第三步：获得PspCreateProcessNotifyRoutine地址失败\n");
	return 0;
}
//遍历进程回调
void EnumCreateProcessNotify(PDRIVER_OBJECT pDriverObj, PNOTIFYINFO* pnotify)
{
	//1.获得PspCreateProcessNotifyRoutine的地址
	ULONG PspCreateProcessNotifyRoutine = 0;
	PspCreateProcessNotifyRoutine = GetPspCreateProcessNotifyRoutine();
	if (!PspCreateProcessNotifyRoutine)
		return;

	//2.遍历
	ULONG NotifyAddr = 0;
	PUNICODE_STRING puni = NULL;
	for (ULONG i = 0; i < 64; i++)
	{
		//1.取出数组中的每一项里面的值   //每一项的长度为4，X64里每一项长度为8
		NotifyAddr = *(PULONG)(PspCreateProcessNotifyRoutine + i * 4);
		//2.判断值是否有效，值对应的地址是否有效
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

//搜索线程回调数组的地址
ULONG SearchPspCreateThreadNotifyRoutine()
{	
	//1.获取PsSetCreateThreadNotifyRoutine函数地址
	UNICODE_STRING uStrFuncName = RTL_CONSTANT_STRING(L"PsSetCreateThreadNotifyRoutine");
	ULONG FuncAddr = 0;
	FuncAddr = (ULONG)MmGetSystemRoutineAddress(&uStrFuncName);
	//DbgPrint("第一步：获取PsSetCreateThreadNotifyRoutine的地址为: %x\n", FuncAddr);

	//2.获取PspCreateThreadNotifyRoutine，即保存线程回调的数组
	LONG ThreadNotifyAddr = 0;
	for (ULONG i = FuncAddr; i < FuncAddr + 0x2f; i++)
	{
		__try
		{
			if ((*(PUCHAR)i == 0xbe))
			{
				RtlCopyMemory(&ThreadNotifyAddr, (PUCHAR)(i + 1), 4);
				//DbgPrint("第二步：获得PspCreateThreadNotifyRoutine的地址 %x\n", ThreadNotifyAddr);

				return ThreadNotifyAddr;
			}
		}
		__except (1)
		{
			ThreadNotifyAddr = 0;
			break;
		}
	}

	//3.如果没有搜索到，返回失败的结果
	return ThreadNotifyAddr;
}
//遍历线程回调
void EnumCreateThreadNotify(PDRIVER_OBJECT pDriverObj, PNOTIFYINFO* pnotify)
{
	ULONG PspCreateThreadNotifyRoutine = SearchPspCreateThreadNotifyRoutine();

	if (!PspCreateThreadNotifyRoutine)
	{
		return;
	}

	//2.遍历
	ULONG NotifyAddr = 0;
	PUNICODE_STRING puni = NULL;
	for (ULONG i = 0; i < 64; i++)
	{
		//1.取出数组中的每一项里面的值   //每一项的长度为4，X64里每一项长度为8
		NotifyAddr = *(PULONG)(PspCreateThreadNotifyRoutine + i * 4);
		//2.判断值是否有效，值对应的地址是否有效
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


//获得PspLoadImageNotifyRoutine的地址
ULONG GetPspLoadImageNotifyRoutine()
{
	//1.获取PsSetLoadImageNotifyRoutine地址
	UNICODE_STRING uStrFuncName = RTL_CONSTANT_STRING(L"PsSetLoadImageNotifyRoutine");
	ULONG FuncAddr = 0;
	FuncAddr = (ULONG)MmGetSystemRoutineAddress(&uStrFuncName);
	//DbgPrint("第一步：获取PsSetLoadImageNotifyRoutine的地址为: %x\n", FuncAddr);

	//2.特征码搜索PspLoadImageNotifyRoutine
	ULONG ImageNotify = 0;
	for (ULONG i = FuncAddr; i < FuncAddr + 0x2f; i++)
	{
		__try
		{
			if ((*(PUCHAR)i == 0x74) && (*(PUCHAR)(i + 1) == 0x25) && (*(PUCHAR)(i + 2) == 0xBE))
			{
				RtlCopyMemory(&ImageNotify, (PUCHAR)(i + 3), 4);
				//DbgPrint("第二步：获得PspLoadImageNotifyRoutine的地址 %x\n", ImageNotify);
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
//遍历ImageNotify
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

//搜索注册表回调链表的链表头
ULONG SearchCallbackListHead()
{
	//1.获得CmRegisterCallback的函数地址
	UNICODE_STRING uStrFuncName = RTL_CONSTANT_STRING(L"CmRegisterCallback");
	ULONG FuncAddr1 = 0;
	FuncAddr1 = (ULONG)MmGetSystemRoutineAddress(&uStrFuncName);
	//DbgPrint("step1:CmRegisterCallback的地址为：%x\r\n", FuncAddr1);
	if (!FuncAddr1)
	{
		//DbgPrint("MmGetSystemRoutineAddress Error\r\n");
		return 0;
	}

	//2.搜索获得CmpRegisterCallbackInternal的函数地址
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
				//DbgPrint("step2:CmpRegisterCallbackInternal的地址为：%x\r\n", FuncAddr2);
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

	//3.搜索获得CmpInsertCallbackInListByAltitude的函数地址
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
				//DbgPrint("step3:CmpInsertCallbackInListByAltitude的地址为：%x\r\n", FuncAddr3);
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

	//4.搜索CallbackListHead
	ULONG ListHead = 0;
	for (ULONG k = FuncAddr3; k < FuncAddr3 + 0xff; k++)
	{
		__try
		{
			if (*(PUCHAR)k == 0xBB)
			{
				RtlCopyMemory(&ListHead, (PUCHAR)(k + 1), 4);
				//DbgPrint("step4:CallbackListHead的地址为：%x\r\n", ListHead);
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

//遍历注册表回调链表
VOID EnumCallBackList(PDRIVER_OBJECT pDriverObj, PNOTIFYINFO* pnotify)
{
	//1.获得注册表回调链表头
	ULONG pHead = 0;
	pHead = SearchCallbackListHead();
	if (!pHead)
	{
		return;
	}

	//2.取出链表头中的节点pList_First
	PLIST_ENTRY pList_Temp = NULL;
	pList_Temp = (PLIST_ENTRY)(*(PULONG)pHead);

	//4.循环遍历链表
	while ((ULONG)pList_Temp != pHead)
	{
		//如果链表地址无效，说明是空链表
		if (!MmIsAddressValid(pList_Temp))
		{
			break;
		}

		//获取pCookie(里面存放的是Cookie）
		PLARGE_INTEGER  pLiRegCookie = NULL;
		pLiRegCookie = (PLARGE_INTEGER)((ULONG)pList_Temp + 0x10);

		//获取pFuncAddr（里面存放的是FuncAddr）
		PULONG pFuncAddr = 0;
		pFuncAddr = (PULONG)((ULONG)pList_Temp + 0x1C);
		PUNICODE_STRING puni = NULL;
		//判断地址是否有效
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
			//DbgPrint("FuncAddr为：%x \n", *pFuncAddr);
			//DbgPrint("Cookie为：%x \n", *pLiRegCookie);

		}
		//链表指向下一个节点
		pList_Temp = pList_Temp->Flink;
	}
}

//------------------------------------------------------------------------------------
//遍历对象回调
// 获取进程对象类型回调
VOID EnumProcessObCallback(PDRIVER_OBJECT pDriverObj, POBCALLBACKINFO* pobcallback)
{
	POB_CALLBACK pObCallback = NULL;

	// 直接获取 CallbackList 链表
	LIST_ENTRY CallbackList = ((POBJECT_TYPE)(*PsProcessType))->CallbackList;

	// 开始遍历
	pObCallback = (POB_CALLBACK)CallbackList.Flink;

	do
	{
		//首先过滤掉pObCallback无效的
		if (!MmIsAddressValid(pObCallback))
		{
			break;
		}
		//接着过滤掉ObHandle为空或无效的
		if (pObCallback->ObHandle == NULL || !MmIsAddressValid(pObCallback->ObHandle))
		{
			break;
		}

		//过滤掉Object地址无效的
		if (pObCallback->ObTypeAddr == NULL || !MmIsAddressValid(pObCallback->ObTypeAddr))
		{
			break;
		}

		if (pObCallback->PreCall&&MmIsAddressValid(pObCallback->PreCall))
		{		
			//1.Object地址
			DbgPrint("[进程]Object地址 = 0x%p\n", pObCallback->ObTypeAddr);
			(*pobcallback)->ObjectAddress = pObCallback->ObTypeAddr;

			//2.类型
			(*pobcallback)->ObjectType = 1;

			//3.前/后
			(*pobcallback)->CallType = 1;

			//4.高度
			PUNICODE_STRING puni3 = NULL;
			puni3 = &pObCallback->ObHandle->Altitude;
			DbgPrint("[进程]Altitude = %wZ\n", puni3);
			RtlCopyMemory((*pobcallback)->Altitude, puni3->Buffer, puni3->Length);
			
			//5.前回调函数地址
			DbgPrint("[进程]PreCall = 0x%p\n", pObCallback->PreCall);
			(*pobcallback)->FuncAddress = pObCallback->PreCall;

			//6.前回调函数所在模块
			PUNICODE_STRING puni1 = NULL;
			puni1 = FindKernelModule(pDriverObj, pObCallback->PreCall);
			if (puni1)
			{
				DbgPrint("[进程]PreCall路径 = %wZ\n", puni1);
				RtlCopyMemory((*pobcallback)->ImgPath, puni1->Buffer, puni1->Length);				
			}

			(*pobcallback)++;
		}


		if (pObCallback->PostCall&&MmIsAddressValid(pObCallback->PostCall))
		{
			//1.Object地址
			DbgPrint("[进程]Object地址 = 0x%p\n", pObCallback->ObTypeAddr);
			(*pobcallback)->ObjectAddress = pObCallback->ObTypeAddr;

			//2.类型
			(*pobcallback)->ObjectType = 1;

			//3.前/后
			(*pobcallback)->CallType = 2;

			//4.高度
			PUNICODE_STRING puni4 = NULL;
			puni4 = &pObCallback->ObHandle->Altitude;
			DbgPrint("[进程]Altitude = %wZ\n", puni4);
			RtlCopyMemory((*pobcallback)->Altitude, puni4->Buffer, puni4->Length);

			//5.后回调函数地址
			DbgPrint("[进程]PostCall = 0x%p\n", pObCallback->PostCall);
			(*pobcallback)->FuncAddress = pObCallback->PostCall;

			//6.后回调函数所在模块
			PUNICODE_STRING puni2 = NULL;
			puni2 = FindKernelModule(pDriverObj, pObCallback->PostCall);
			if (puni2)
			{
				DbgPrint("[进程]PostCall路径 = %wZ\n", puni2);
				RtlCopyMemory((*pobcallback)->ImgPath, puni2->Buffer, puni2->Length);
			}

			(*pobcallback)++;
		}


		// 获取下一链表信息
		pObCallback = (POB_CALLBACK)pObCallback->ListEntry.Flink;

	} while (CallbackList.Flink != (PLIST_ENTRY)pObCallback);
	
}


// 获取线程对象类型回调
VOID EnumThreadObCallback(PDRIVER_OBJECT pDriverObj, POBCALLBACKINFO* pobcallback)
{
	POB_CALLBACK pObCallback = NULL;

	// 直接获取 CallbackList 链表
	LIST_ENTRY CallbackList = ((POBJECT_TYPE)(*PsThreadType))->CallbackList;

	// 开始遍历
	pObCallback = (POB_CALLBACK)CallbackList.Flink;

	do
	{
		//首先过滤掉pObCallback无效的
		if (!MmIsAddressValid(pObCallback))
		{
			break;
		}
		//接着过滤掉ObHandle为空或无效的
		if (pObCallback->ObHandle == NULL || !MmIsAddressValid(pObCallback->ObHandle))
		{
			break;
		}

		//过滤掉Object地址无效的
		if (pObCallback->ObTypeAddr == NULL || !MmIsAddressValid(pObCallback->ObTypeAddr))
		{
			break;
		}

		if (pObCallback->PreCall&&MmIsAddressValid(pObCallback->PreCall))
		{
			//1.Object地址
			(*pobcallback)->ObjectAddress = pObCallback->ObTypeAddr;

			//2.类型
			(*pobcallback)->ObjectType = 2;

			//3.前/后
			(*pobcallback)->CallType = 1;

			//4.高度
			PUNICODE_STRING puni3 = NULL;
			puni3 = &pObCallback->ObHandle->Altitude;
			DbgPrint("[线程]Altitude = %wZ\n", puni3);
			RtlCopyMemory((*pobcallback)->Altitude, puni3->Buffer, puni3->Length);

			//5.前回调函数地址
			(*pobcallback)->FuncAddress = pObCallback->PreCall;

			//6.前回调函数所在模块
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
			//1.Object地址
			(*pobcallback)->ObjectAddress = pObCallback->ObTypeAddr;

			//2.类型
			(*pobcallback)->ObjectType = 2;

			//3.前/后
			(*pobcallback)->CallType = 2;

			//4.高度
			PUNICODE_STRING puni4 = NULL;
			puni4 = &pObCallback->ObHandle->Altitude;
			DbgPrint("[线程]Altitude = %wZ\n", puni4);
			RtlCopyMemory((*pobcallback)->Altitude, puni4->Buffer, puni4->Length);

			//5.前回调函数地址
			(*pobcallback)->FuncAddress = pObCallback->PostCall;

			//6.前回调函数所在模块
			PUNICODE_STRING puni2 = NULL;
			puni2 = FindKernelModule(pDriverObj, pObCallback->PostCall);
			if (puni2)
			{
				RtlCopyMemory((*pobcallback)->ImgPath, puni2->Buffer, puni2->Length);
			}

			(*pobcallback)++;
		}

		// 获取下一链表信息
		pObCallback = (POB_CALLBACK)pObCallback->ListEntry.Flink;

	} while (CallbackList.Flink != (PLIST_ENTRY)pObCallback);
	
}
