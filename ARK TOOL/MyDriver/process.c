#include"Myprocess.h"

typedef VOID(NTAPI* fpTypePspExitThread)(IN NTSTATUS ExitStatus);
extern fpTypePspExitThread g_fpPspExitThreadAddr;

//-------------------------------------杀死进程------------------------------------------------------
//1.搜索获得PspTerminateThreadByPointer的函数地址
ULONG GetPspTerminateThreadByPointer()
{
	//1.获得PsTerminateSystemThread的函数地址
	UNICODE_STRING funcName;
	RtlInitUnicodeString(&funcName, L"PsTerminateSystemThread");
	ULONG baseAddr = (ULONG)MmGetSystemRoutineAddress(&funcName);

	//2.以PsTerminateSystemThread的地址为起点，在1024个字节范围内进行搜索
	ULONG FuncAddr = 0;
	for (ULONG i = baseAddr; i < (baseAddr + 1024); i++)
	{
		//搜索特征码：0x50,0xe8
		if (((*(PUCHAR)(UCHAR*)(i - 1)) == 0x50) && ((*(PUCHAR)(UCHAR*)(i)) == 0xe8))
		{
			ULONG offset = *(PULONG)(i + 1);
			FuncAddr = i + 5 + offset;
			DbgPrint("找到PspTerminateThreadByPointer的函数地址=%x", FuncAddr);
			break;
		}
	}
	return FuncAddr;
}
//2.搜索获得PspExitThread的函数地址
ULONG GetPspExitThread(ULONG BaseAddr)
{
	ULONG FuncAddr = 0;
	for (ULONG i = BaseAddr; i < (BaseAddr + 1024); i++)
	{
		//搜索特征码： 0x0c,0xe8
		if (((*(PUCHAR)(UCHAR*)(i - 1)) == 0x0c) && ((*(PUCHAR)(UCHAR*)(i)) == 0xe8))
		{
			ULONG m_offset = *(PULONG)(i + 1);
			FuncAddr = i + 5 + m_offset;
			DbgPrint("找到PspExitThread的函数地址=%x", FuncAddr);
			break;
		}
	}
	return FuncAddr;
}
//3.APC回调函数：关闭线程
VOID SelfTerminateThread(KAPC *Apc, PKNORMAL_ROUTINE *NormalRoutine, PVOID *NormalContext, PVOID *SystemArgument1, PVOID *SystemArgument2)
{
	ExFreePool(Apc);
	g_fpPspExitThreadAddr(STATUS_SUCCESS);
}
//4.自定义函数：杀死进程
VOID KillProcess(HANDLE hPid)
{
	//1.通过PID找到进程的EProcess
	PEPROCESS pEProcess = NULL;
	NTSTATUS status = PsLookupProcessByProcessId(hPid, &pEProcess);
	if (!NT_SUCCESS(status))
	{
		DbgPrint("没有在PsCidTable中找到进程,尼玛不会隐藏了吧\n");
		return;
	}

	//2.暴力搜索全部线程，找到指定EPROCESS的线程并关闭
	PEPROCESS pEProc = NULL;
	PETHREAD  pEThread = NULL;
	for (ULONG i = 4; i < 0x25600; i += 4)
	{
		//2.1找到每个TID对应的ETHREAD
		if (NT_SUCCESS(PsLookupThreadByThreadId((HANDLE)i, &pEThread)))
		{
			//2.1.1.通过ETHREAD得到对应的pEProc
			pEProc = IoThreadToProcess(pEThread);
			//2.1.2对比pEProc是否与目标进程的pEProcess相等，若相等，说明这个线程属于目标进程
			if (pEProc == pEProcess)
			{
				DbgPrint("找到");

				//1.为APC申请内存
				PKAPC pApc = NULL;
				pApc = (PKAPC)ExAllocatePool(NonPagedPool, sizeof(KAPC));
				if (NULL == pApc) return;

				//2.插入内核apc
				KeInitializeApc(pApc, (PKTHREAD)pEThread, OriginalApcEnvironment, (PKKERNEL_ROUTINE)&SelfTerminateThread, NULL, NULL, 0, NULL);
				KeInsertQueueApc(pApc, NULL, 0, 2);
			}

			//2.1.3.解除对线程的引用
			ObDereferenceObject(pEThread);
		}
	}

	//3.解除对进程的引用
	ObDereferenceObject(pEProcess);
}
//----------------------------------------------------------------------------------------------------


//-------------------------------------隐藏进程---------------------------------------------------
//1.逆向PsGetProcessId获得UniqueProcessId在EPROCESS结构体中的偏移--->获得ActiveProcessLinks在EPROCESS结构体中的偏移 （版本通用）
ULONG GetActiveListOffset()
{
	//获得PsGetProcessId函数的函数地址
	UNICODE_STRING FunName = { 0 };
	RtlInitUnicodeString(&FunName, L"PsGetProcessId");
	PUCHAR pFun = (PUCHAR)MmGetSystemRoutineAddress(&FunName);

	if (pFun)
	{
		for (size_t i = 0; i < 0x100; i++)
		{
			//反汇编8B80 XXXX中，XXXX即为UniqueProcessId在EPROCESS结构体中的偏移
			if (pFun[i] == 0x8B && pFun[i + 1] == 0x80)
			{
				//在EPROCESS结构体中，UniqueProcessId + 4为ActiveProcessLinks
				return *(PULONG)(pFun + i + 2) + 4;
			}
		}
	}

	//如果没找到，说明失败了，返回0
	return 0;

}
//2.自定义函数：隐藏进程
VOID HideProcess(HANDLE hPid)
{
	//1.查找进程EPROCESS
	PEPROCESS pEProcess = NULL;
	NTSTATUS status = PsLookupProcessByProcessId(hPid, &pEProcess);
	if (!NT_SUCCESS(status))
	{
		DbgPrint("没有在PsCidTable中找到进程,尼玛不会隐藏了吧\n");
		return;
	}
	ObDereferenceObject(pEProcess);


	//2.获得ActiveProcessLinks在EPROCESS结构体中的偏移
	ULONG uOffset = GetActiveListOffset();
	if (!uOffset)
	{
		DbgPrint("没有获得ActiveProcessLinks在EPROCESS结构体中的偏移\n");
		return;
	}

	//3.获得当前进程链表节点、前一个节点、后一个节点
	PLIST_ENTRY MyNode = (PLIST_ENTRY)((PUCHAR)pEProcess + uOffset);
	PLIST_ENTRY nextNode = MyNode->Flink;
	PLIST_ENTRY preNode = MyNode->Blink;

	//4.断链
	//4.1上一个节点的下一个节点指向我的下一个节点
	preNode->Flink = MyNode->Flink;
	//4.2下一个节点的上一个节点指向我的上一个节点
	nextNode->Blink = MyNode->Blink;
	//4.3我的Flink和Blink都指向我自己，否则蓝屏
	MyNode->Flink = MyNode->Blink = MyNode;
}


//-----------------------------------------进程保护------------------------------------------------
#define  _HANDLE_TABLE_OFFSET_EPROCESS  0x0f4
#define  _SPECIAL_PURPOSE                8
#define  _MAX                           511
#define  _BODY_OFFSET_OBJECT_HEADER     0x18

NTKERNELAPI PVOID NTAPI ObGetObjectType(IN PVOID pObject);
PUCHAR PsGetProcessImageFileName(__in PEPROCESS Process);



PETHREAD pThreadObj1 = NULL;
BOOLEAN bTerminated1 = FALSE;

PETHREAD pThreadObj2 = NULL;
BOOLEAN bTerminated2 = FALSE;


//遍历当前进程的私有句柄表
NTSTATUS SeScanProcessHandleTable(PEPROCESS CurEProcess, PEPROCESS TargetProcess)
{
	//1.过滤为NULL的EProcess
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	if (CurEProcess == NULL)
	{
		return Status;
	}

	//2.由EProcess--->HandleTable   //win7x86中EProcess偏移0x0f4
	PHANDLE_TABLE  HandleTable = NULL;
	HandleTable = (PHANDLE_TABLE)(*((ULONG*)((UINT8*)CurEProcess + _HANDLE_TABLE_OFFSET_EPROCESS)));
	if (HandleTable == NULL)
	{
		return Status;
	}

	//3.HandleTable--->TableCode
	PVOID  FakeTableCode = NULL;
	FakeTableCode = HandleTable->TableCode; //TableCode与HandleTable地址重叠 //HandleTable->TableCode 或者 *HandleTable都可以取到

	//4.TableCode低2位&3，判断是几级表
	ULONG  Flag = 0;
	Flag = (ULONG)(FakeTableCode) & 0x03;    //00 01 10

	//5.去掉低两位，才是真正的TableCode
	ULONG TrueTableCode = 0;
	TrueTableCode = (ULONG)FakeTableCode & 0xFFFFFFFC;

	//6.按照3种情况讨论
	switch (Flag)
	{
	case 0:
	{
		//遍历1级句柄表  //TrueTableCode是句柄表的基址 //TrueTableCode此时类似一级指针
		EnumTable0(TrueTableCode, CurEProcess, TargetProcess);
		break;
	}
	case 1:
	{
		//遍历2级句柄表  //*TrueTableCode 才是句柄表的基址  //TrueTableCode此时类似二级指针
		EnumTable1(TrueTableCode, CurEProcess, TargetProcess);
		break;
	}
	case 2:
	{
		//遍历3级句柄表  //**TrueTableCode 才是句柄表的基址  //TrueTableCode此时类似三级指针
		EnumTable2(TrueTableCode, CurEProcess, TargetProcess);
		break;
	}

	}
	return Status;
}

NTSTATUS EnumTable0(ULONG TableCode, PEPROCESS CurEProcess, PEPROCESS TargetProcess)
{
	//1.越过特殊用途8字节到第一个HandleTableEntry    //为什么要越过第1项？
	PHANDLE_TABLE_ENTRY HandleTableEntry = NULL;
	HandleTableEntry = (PHANDLE_TABLE_ENTRY)((ULONG*)(TableCode + _SPECIAL_PURPOSE));

	//2.遍历1级表
	for (ULONG i = 0; i < _MAX; i++)
	{
		//1.过滤掉无效的项
		if (!MmIsAddressValid((PVOID)HandleTableEntry))
		{
			HandleTableEntry++;
			continue;
		}

		//2.去掉低3位掩码标志,转换为对象头指针
		PVOID ObjectHeader = (PVOID)((ULONG)(HandleTableEntry->Object) & 0xFFFFFFF8);

		//3.过滤掉无效的对象头
		if (!MmIsAddressValid(ObjectHeader))
		{
			HandleTableEntry++;
			continue;
		}

		//4.获得对象类型
		UCHAR uIndex = *(PUCHAR)((ULONG)ObjectHeader + 0xC);

		//5.过滤掉不是进程的对象
		if (uIndex != 7)
		{
			HandleTableEntry++;
			continue;
		}

		//6.获得EPROCESS
		PEPROCESS eProcess = (PEPROCESS)((UINT8*)ObjectHeader + _BODY_OFFSET_OBJECT_HEADER);

		//7.过滤掉无效的EPROCESS			
		if (!MmIsAddressValid(eProcess))
		{
			HandleTableEntry++;
			continue;
		}

		//8.过滤掉不是目标EPROCESS的进程
		if (eProcess != TargetProcess)
		{
			HandleTableEntry++;
			continue;
		}

		//9.打印当前进程名，PID	
		ULONG uPid = 0;
		PUCHAR szProcessName = NULL;
		uPid = *(PULONG)((PUCHAR)CurEProcess + 0xb4);
		szProcessName = PsGetProcessImageFileName(CurEProcess);
		DbgPrint("检测到：[%s]打开了受保护进程 \n", szProcessName);

		//10.将当前进程对应的这个句柄项的权限更改
		DbgPrint("句柄初始权限:[%x]\n", HandleTableEntry->GrantedAccess);
		if ((HandleTableEntry->GrantedAccess & PROCESS_TERMINATE) == PROCESS_TERMINATE)
		{
			HandleTableEntry->GrantedAccess &= ~PROCESS_TERMINATE;
		}
		if ((HandleTableEntry->GrantedAccess & PROCESS_VM_OPERATION) == PROCESS_VM_OPERATION)
		{
			HandleTableEntry->GrantedAccess &= ~PROCESS_VM_OPERATION;
		}
		if ((HandleTableEntry->GrantedAccess & PROCESS_VM_READ) == PROCESS_VM_READ)
		{
			HandleTableEntry->GrantedAccess &= ~PROCESS_VM_READ;
		}
		if ((HandleTableEntry->GrantedAccess & PROCESS_VM_WRITE) == PROCESS_VM_WRITE)
		{
			HandleTableEntry->GrantedAccess &= ~PROCESS_VM_WRITE;
		}
		DbgPrint("句柄降权后的权限:[%x]\n", HandleTableEntry->GrantedAccess);
		//DbgPrint("----------------------------------------------\n\n");
		//9.结构体指针后移
		HandleTableEntry++;
	}
	return STATUS_SUCCESS;
}

NTSTATUS EnumTable1(ULONG TableCode, PEPROCESS CurEProcess, PEPROCESS TargetProcess)
{
	do
	{
		//取出TableCode里面的值，才是PHANDLE_TABLE_ENTRY   //有点像二级指针
		EnumTable0((*(ULONG*)TableCode), CurEProcess, TargetProcess);
		//TableCode后移1项
		(UINT8*)TableCode += sizeof(ULONG);

	} while (*(ULONG*)TableCode != 0 && MmIsAddressValid((PVOID)(*(ULONG*)TableCode))); //当地址有效，地址中的值有效？

	return STATUS_SUCCESS;
}

NTSTATUS EnumTable2(ULONG TableCode, PEPROCESS CurEProcess, PEPROCESS TargetProcess)
{
	do
	{
		EnumTable1(*(ULONG*)TableCode, CurEProcess, TargetProcess);
		(UINT8*)TableCode += sizeof(ULONG);

	} while (*(ULONG*)TableCode != 0 && MmIsAddressValid((PVOID)(*(ULONG*)TableCode)));

	return STATUS_SUCCESS;
}


//内核线程1
NTSTATUS ProcessProtect(PEPROCESS TargetProcess)
{
	//1.过滤为NULL的EProcess
	NTSTATUS Status = STATUS_SUCCESS;
	if (TargetProcess == NULL)
	{
		return STATUS_UNSUCCESSFUL;
	}

	while (1)
	{
		//DbgPrint("----------------开始检测--------------------\n");
		//1.暴力搜索所有的进程，获得EPROCESS
		PEPROCESS CurEProcess = NULL;
		for (ULONG i = 4; i < 0x10000; i = i + 4)
		{
			Status = PsLookupProcessByProcessId((HANDLE)i, &CurEProcess);
			if (NT_SUCCESS(Status))
			{
				ObDereferenceObject(CurEProcess);

				//2.在当前进程的私有句柄表中，查看是否打开了目标进程，若是则修改句柄项进行降权
				SeScanProcessHandleTable(CurEProcess, TargetProcess);
			}
		}
		//DbgPrint("----------------检测结束--------------------\n");

		//3.当标志bTerminated为TRUE时跳出死循环
		if (bTerminated1)
		{
			break;
		}

		//4.休眠5秒
		LARGE_INTEGER inteval;
		inteval.QuadPart = -50000000;
		KeDelayExecutionThread(KernelMode, FALSE, &inteval);
	}

	//5.终止线程
	PsTerminateSystemThread(STATUS_SUCCESS);	//PsTerminateSystemThread只针对当前内核线程，且必须用在内核线程的内部
	//return STATUS_SUCCESS;
}

//封装的创建内核线程函数,内部运行内核线程1
NTSTATUS CreateKeThread1(PEPROCESS TargetProcess)
{
	OBJECT_ATTRIBUTES objAddr = { 0 };
	HANDLE threadHandle = 0;
	NTSTATUS status = STATUS_SUCCESS;
	// 初始化一个OBJECT_ATTRIBUTES 对象
	InitializeObjectAttributes(&objAddr, NULL, OBJ_KERNEL_HANDLE, 0, NULL);
	// 创建线程
	status = PsCreateSystemThread(&threadHandle, THREAD_ALL_ACCESS, &objAddr, NULL, NULL, ProcessProtect, TargetProcess);
	if (NT_SUCCESS(status))
	{
		// 通过句柄获得线程的对象
		status = ObReferenceObjectByHandle(threadHandle, THREAD_ALL_ACCESS, *PsThreadType, KernelMode, &pThreadObj1, NULL);

		// 释放句柄
		ZwClose(threadHandle);

		if (!NT_SUCCESS(status))
		{
			// 若获取对象失败，也设置标志为TRUE
			bTerminated1 = TRUE;
		}
	}
	return status;
}


//内核线程2
NTSTATUS ZeroDebugPort(PEPROCESS TargetProcess)
{
	//1.过滤为NULL的EProcess
	NTSTATUS Status = STATUS_SUCCESS;
	if (TargetProcess == NULL)
	{
		return STATUS_UNSUCCESSFUL;
	}

	while (1)
	{
		//1.如果收到信号，跳出死循环
		if (bTerminated2)
		{
			break;
		}

		//DbgPrint("----开始对DebugPort进行清0----\n\n");
			__try
			{
				DWORD64 debugport = *(DWORD64*)((DWORD64)TargetProcess + 0xec);
				DbgPrint("当前debugport的值--->[%d]\n", debugport);

				if (debugport != 0)
				{
					*(DWORD64*)((DWORD64)TargetProcess + 0xec) = 0;
					debugport = *(DWORD64*)((DWORD64)TargetProcess + 0xec);
					DbgPrint("操作后debugport的值--->[%d]\n", debugport);
				}
			}
			__except (1)
			{
				DbgPrint("%s: Exception\n", __FUNCTION__);
			}		

		//3.休眠5秒
		LARGE_INTEGER inteval;
		inteval.QuadPart = -50000000;
		KeDelayExecutionThread(KernelMode, FALSE, &inteval);
	}

	//4.终止线程
	PsTerminateSystemThread(STATUS_SUCCESS);
	//return STATUS_SUCCESS;
}

//封装的创建内核线程函数,内部运行内核线程2
NTSTATUS CreateKeThread2(PEPROCESS TargetProcess)
{
	OBJECT_ATTRIBUTES objAddr = { 0 };
	HANDLE threadHandle = 0;
	NTSTATUS status = STATUS_SUCCESS;
	// 初始化一个OBJECT_ATTRIBUTES 对象
	InitializeObjectAttributes(&objAddr, NULL, OBJ_KERNEL_HANDLE, 0, NULL);
	// 创建线程
	status = PsCreateSystemThread(&threadHandle, THREAD_ALL_ACCESS, &objAddr, NULL, NULL, ZeroDebugPort, TargetProcess);
	if (NT_SUCCESS(status))
	{
		// 通过句柄获得线程的对象
		status = ObReferenceObjectByHandle(threadHandle, THREAD_ALL_ACCESS, *PsThreadType, KernelMode, &pThreadObj2, NULL);

		// 释放句柄
		ZwClose(threadHandle);

		if (!NT_SUCCESS(status))
		{
			// 若获取对象失败，也设置标志为TRUE
			bTerminated2 = TRUE;
		}
	}
	return status;
}


VOID ProtectTheProcess(HANDLE hPid)
{
	//1.查找进程EPROCESS
	PEPROCESS TargetProcess = NULL;
	NTSTATUS status = PsLookupProcessByProcessId(hPid, &TargetProcess);
	if (!NT_SUCCESS(status))
	{
		DbgPrint("没有找到进程,尼玛不会隐藏了吧\n");
		return ;
	}
	ObDereferenceObject(TargetProcess);

	//2.创建内核线程，进行进程保护
	CreateKeThread1(TargetProcess);
	CreateKeThread2(TargetProcess);
		
}


//-----------------------------------------------进程的VAD相关---------------------------------
//遍历获取VAD的数目
VOID EnumVADCount(IN PMMADDRESS_NODE pVad,OUT PULONG pVadCount)
{
	//1.过滤掉无效的vad
	if (pVad == NULL)
	{
		return;
	}

	//2.遍历左子树
	EnumVADCount(pVad->LeftChild, pVadCount);

	//3.节点+1
	(*pVadCount)++;

	//4.遍历右子树
	EnumVADCount(pVad->RightChild, pVadCount);

	return;
}

//GetVADCount--->EnumVADCount
ULONG GetVADCount(HANDLE hPid)
{
	//1.通过PID获得EPROCESS
	PEPROCESS EProcess = NULL;
	NTSTATUS status = PsLookupProcessByProcessId(hPid, &EProcess);
	if (!NT_SUCCESS(status))
	{
		DbgPrint("没有找到进程,尼玛不会隐藏了吧\n");
		return 0;
	}
	ObDereferenceObject(EProcess);

	//2.获得进程的VAD树根结点的地址
	PMMADDRESS_NODE VadRoot = NULL;
	VadRoot = (PMMADDRESS_NODE)((ULONG)EProcess + 0x278);

	//3.开始遍历VAD
	ULONG VadCount = 0;
	EnumVADCount(VadRoot->RightChild,&VadCount);

	return VadCount;
}


//中序遍历VAD的AVL树
VOID _EnumVAD(PMMADDRESS_NODE pVad, PVADINFO* ppOutVADInfo)
{
	//1.过滤掉无效的vad
	if (pVad == NULL)
	{
		return;
	}

	//2.遍历左子树
	_EnumVAD(pVad->LeftChild, ppOutVADInfo);

	//3.回到父节点
	MMVAD* Root = (MMVAD*)pVad;
	
	(*ppOutVADInfo)->pVad = (ULONG)Root;

	(*ppOutVADInfo)->Start = (ULONG)Root->StartingVpn;

	(*ppOutVADInfo)->End = (ULONG)Root->EndingVpn;

	(*ppOutVADInfo)->Commit = (ULONG)Root->u.VadFlags.CommitCharge;

	(*ppOutVADInfo)->Type = (ULONG)Root->u.VadFlags.PrivateMemory;

	(*ppOutVADInfo)->Protection = (ULONG)Root->u.VadFlags.Protection;

	
	//4.申请内存  
	POBJECT_NAME_INFORMATION  stFileBuffer = (POBJECT_NAME_INFORMATION)ExAllocatePool(PagedPool, 0x100);

	//5.在父节点查询内存块
	ULONG uRetLen = 0;
	if (MmIsAddressValid(Root->Subsection) && MmIsAddressValid(Root->Subsection->ControlArea) && stFileBuffer != NULL)
	{
		if (MmIsAddressValid((PVOID)Root->Subsection->ControlArea->FilePointer.Value))
		{
			//低3位清0，得到内存块对应的FILE_OBJECT对象基址
			PFILE_OBJECT pFileObject = (PFILE_OBJECT)((Root->Subsection->ControlArea->FilePointer.Value >> 3) << 3);

			//如果FILE_OBJECT对象基址有效
			if (MmIsAddressValid(pFileObject))
			{				
				NTSTATUS Status = ObQueryNameString(pFileObject, stFileBuffer, 0x100, &uRetLen);
				if (NT_SUCCESS(Status))
				{					
					RtlCopyMemory((*ppOutVADInfo)->FileObjectName, stFileBuffer->Name.Buffer, stFileBuffer->Name.Length);									
				}				
			}			
		}		
	}
	
	
	//6.释放申请的内存
	if (stFileBuffer)
	{
		ExFreePool(stFileBuffer);
	}
	
	//7.指针后移
	(*ppOutVADInfo)++;  //可能有问题

	//8.遍历右子树
	_EnumVAD(pVad->RightChild, ppOutVADInfo);

	return;
}

//_EnumProcessVAD-->_EnumVAD
NTSTATUS _EnumProcessVAD(HANDLE hPid, PVADINFO pOutVADInfo)
{
	//1.通过PID获得EPROCESS
	PEPROCESS EProcess = NULL;
	NTSTATUS status = PsLookupProcessByProcessId(hPid, &EProcess);
	if (!NT_SUCCESS(status))
	{
		DbgPrint("没有找到进程,尼玛不会隐藏了吧\n");
		return STATUS_SUCCESS;
	}
	ObDereferenceObject(EProcess);

	//2.获得进程的VAD树根结点的地址
	PMMADDRESS_NODE VadRoot = NULL;
	VadRoot = (PMMADDRESS_NODE)((ULONG)EProcess + 0x278);

	//3.开始遍历VAD
	_EnumVAD(VadRoot->RightChild, &pOutVADInfo);

	return STATUS_SUCCESS;
}