#include "Myinject.h"


extern PSYSTEM_SERVICE_DESCRIPTOR_TABLE KeServiceDescriptorTable;

//总函数：步骤1.过滤无效PID-->步骤2.通过PID获取EProcess-->步骤3.进程挂靠-->步骤4.获取ntdll模块基地址-->步骤5.获取LdrLoadDll函数地址
//-->步骤6.修正shellcode-->步骤7.创建用户线程-->步骤8.执行线程函数-->步骤9.启动shellcode--->步骤10.LdrLoadDll加载DLL-->步骤11.解除进程挂靠
NTSTATUS InjectDll(IN PINJECT_INFO InjectInfo)
{
	//1.对传进来的参数进行过滤，为空则返回不成功
	HANDLE  ProcessID = NULL;
	ProcessID = InjectInfo->ProcessId;	
	NTSTATUS Status = STATUS_SUCCESS;
	if (ProcessID == NULL)
	{
		Status = STATUS_UNSUCCESSFUL;
		return Status;
	}

	//2.通过PID获取EProcess
	PEPROCESS EProcess = NULL;
	Status = PsLookupProcessByProcessId(ProcessID, &EProcess);
	if (Status != STATUS_SUCCESS)
	{
		DbgPrint(("PsLookupProcessByProcessId函数失败\n"));
		return Status;
	}
	ObDereferenceObject(EProcess);

	//3.进程挂靠，进入目标进程空间
	KAPC_STATE ApcState;
	KeStackAttachProcess((PRKPROCESS)EProcess, &ApcState);
	__try
	{
		//4.获取ntdll模块基地址
		UNICODE_STRING NtdllUnicodeString = { 0 };
		RtlInitUnicodeString(&NtdllUnicodeString, L"Ntdll.dll");
		PVOID NtdllAddress = NULL;
		NtdllAddress = GetModuleBase(EProcess, &NtdllUnicodeString);
		if (!NtdllAddress)
		{
			DbgPrint("%s: Failed to get Ntdll base\n", __FUNCTION__);
			Status = STATUS_NOT_FOUND;
		}

		//5.获取LdrLoadDll函数地址
		PVOID LdrLoadDll = NULL;
		if (NT_SUCCESS(Status))
		{
			LdrLoadDll = GetFuncAddr(NtdllAddress, "LdrLoadDll", EProcess);

			if (!LdrLoadDll)
			{
				DbgPrint("%s: Failed to get LdrLoadDll address\n", __FUNCTION__);
				Status = STATUS_NOT_FOUND;
			}
		}

		//6.修正shellcode（将LdrLoadDll的函数地址、要注入的那个DLL的路径写入shellcode）
		UNICODE_STRING DllFullPath = { 0 };		
		RtlInitUnicodeString(&DllFullPath, InjectInfo->DllName); //L"C:\\Users\\TH000\\Desktop\\Dll.dll"
		PINJECT_BUFFER InjectBuffer = GetShellCode(LdrLoadDll, &DllFullPath);

		//7.创建用户线程-->执行线程函数-->启动shellcode--->LdrLoadDll加载DLL
		ExecuteInNewThread(InjectBuffer, NULL, THREAD_CREATE_FLAGS_HIDE_FROM_DEBUGGER, TRUE, &Status);
		if (!NT_SUCCESS(Status))
		{
			DbgPrint(("ExecuteInNewThread函数失败\n"));
		}

	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{

		Status = STATUS_UNSUCCESSFUL;
	}

	//8.解除进程挂靠
	KeUnstackDetachProcess(&ApcState);

	return Status;
}

//步骤4.获取指定进程空间中加载的NTDLL.dll的模块基址
PVOID GetModuleBase(IN PEPROCESS EProcess, IN PUNICODE_STRING ModuleName)
{
	//1.过滤无效的EProcess
	if (EProcess == NULL)
		return NULL;

	__try
	{
		//2.获得PEB
		PPEB Peb = PsGetProcessPeb(EProcess);
		if (!Peb)
		{
			DbgPrint("%s: No PEB present. Aborting\n", __FUNCTION__);
			return NULL;
		}

		//3.延时等待并进行探测初始化是否完成
		LARGE_INTEGER Time = { 0 };
		Time.QuadPart = -250ll * 10 * 1000;			//250 ms.
		for (INT i = 0; !Peb->Ldr && i < 10; i++)
		{
			DbgPrint("%s: Loader not intialiezd, waiting\n", __FUNCTION__);
			KeDelayExecutionThread(KernelMode, TRUE, &Time);
		}
		//如果在2.5秒内还是没有初始化完成，返回null
		if (!Peb->Ldr)
		{
			DbgPrint("%s: Loader was not intialiezd in time. Aborting\n", __FUNCTION__);
			return NULL;
		}

		//4.遍历链表
		for (PLIST_ENTRY ListEntry = Peb->Ldr->InLoadOrderModuleList.Flink;
			ListEntry != &Peb->Ldr->InLoadOrderModuleList;
			ListEntry = ListEntry->Flink)
		{
			PLDR_DATA_TABLE_ENTRY_INJECT LdrDataTableEntry = CONTAINING_RECORD(ListEntry, LDR_DATA_TABLE_ENTRY_INJECT, InLoadOrderLinks);
			if (RtlCompareUnicodeString(&LdrDataTableEntry->BaseDllName, ModuleName, TRUE) == 0)
				return LdrDataTableEntry->DllBase;
		}

	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		DbgPrint("%s: Exception, Code: 0x%X\n", __FUNCTION__, GetExceptionCode());
	}

	return NULL;
}

//步骤5.获取NTDLL.dll中的导出函数LdrLoadDll的函数地址
PVOID GetFuncAddr(IN PVOID ModuleBase, IN PCCHAR FunctionName, IN PEPROCESS EProcess)
{
	//1.过滤无效的模块基址
	if (ModuleBase == NULL)
		return NULL;

	//2.判断MZ标记
	PIMAGE_DOS_HEADER_INJECT ImageDosHeader = (PIMAGE_DOS_HEADER_INJECT)ModuleBase;
	if (ImageDosHeader->e_magic != IMAGE_DOS_SIGNATURE)
	{
		return NULL;
	}
	//3.获得导出表的基址
	PIMAGE_NT_HEADERS32_INJECT ImageNtHeaders32 = NULL;
	ImageNtHeaders32 = (PIMAGE_NT_HEADERS32_INJECT)((PUCHAR)ModuleBase + ImageDosHeader->e_lfanew);
	PIMAGE_EXPORT_DIRECTORY_INJECT ImageExportDirectory = NULL;
	ImageExportDirectory = (PIMAGE_EXPORT_DIRECTORY_INJECT)(ImageNtHeaders32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress + (ULONG_PTR)ModuleBase);
	//ULONG ExportDirectorySize = 0;
	//ExportDirectorySize = ImageNtHeaders32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;

	//4.获得导出表的3张表
	PUSHORT pAddressOfOrds = (PUSHORT)(ImageExportDirectory->AddressOfNameOrdinals + (ULONG_PTR)ModuleBase);
	PULONG  pAddressOfNames = (PULONG)(ImageExportDirectory->AddressOfNames + (ULONG_PTR)ModuleBase);
	PULONG  pAddressOfFuncs = (PULONG)(ImageExportDirectory->AddressOfFunctions + (ULONG_PTR)ModuleBase);

	//5.遍历导出表找到函数地址
	ULONG_PTR FunctionAddress = 0;
	for (ULONG i = 0; i < ImageExportDirectory->NumberOfFunctions; ++i)
	{
		USHORT OrdIndex = 0xFFFF;
		PCHAR  pName = NULL;

		// Find by index
		if ((ULONG_PTR)FunctionName <= 0xFFFF)
		{
			OrdIndex = (USHORT)i;
		}
		// Find by name
		else if ((ULONG_PTR)FunctionName > 0xFFFF && i < ImageExportDirectory->NumberOfNames)
		{
			pName = (PCHAR)(pAddressOfNames[i] + (ULONG_PTR)ModuleBase);
			OrdIndex = pAddressOfOrds[i];
		}
		// Weird params
		else
			return NULL;


		if (((ULONG_PTR)FunctionName <= 0xFFFF && (USHORT)((ULONG_PTR)FunctionName) == OrdIndex + ImageExportDirectory->Base) ||
			((ULONG_PTR)FunctionName > 0xFFFF && strcmp(pName, FunctionName) == 0))
		{
			FunctionAddress = pAddressOfFuncs[OrdIndex] + (ULONG_PTR)ModuleBase;
			break;
		}
	}

	return (PVOID)FunctionAddress;
}

//步骤6.修正shellcode
PINJECT_BUFFER GetShellCode(IN PVOID LdrLoadDll, IN PUNICODE_STRING DllFullPath)
{
	NTSTATUS Status = STATUS_SUCCESS;
	PINJECT_BUFFER InjectBuffer = NULL;
	SIZE_T Size = PAGE_SIZE;

	// Code
	UCHAR Code[] =
	{
		0x68, 0, 0, 0, 0,                       // push ModuleHandle            offset +1 
		0x68, 0, 0, 0, 0,                       // push ModuleFileName          offset +6
		0x6A, 0,                                // push Flags  
		0x6A, 0,                                // push PathToFile
		0xE8, 0, 0, 0, 0,                       // call LdrLoadDll              offset +15
		0xBA, 0, 0, 0, 0,                       // mov edx, COMPLETE_OFFSET     offset +20
		0xC7, 0x02, 0x7E, 0x1E, 0x37, 0xC0,     // mov [edx], CALL_COMPLETE     
		0xBA, 0, 0, 0, 0,                       // mov edx, STATUS_OFFSET       offset +31
		0x89, 0x02,                             // mov [edx], eax
		0xC2, 0x04, 0x00                        // ret 4
	};
	//1.申请内存
	Status = ZwAllocateVirtualMemory(ZwCurrentProcess(), (PVOID*)&InjectBuffer, 0, &Size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (NT_SUCCESS(Status))
	{
		//
		PUNICODE_STRING32 pUserPath = &InjectBuffer->Path32;
		pUserPath->Length = DllFullPath->Length;
		pUserPath->MaximumLength = DllFullPath->MaximumLength;
		pUserPath->Buffer = (ULONG)(ULONG_PTR)InjectBuffer->Buffer;

		// Copy path
		memcpy((PVOID)pUserPath->Buffer, DllFullPath->Buffer, DllFullPath->Length);

		// Copy code
		memcpy(InjectBuffer, Code, sizeof(Code));

		// Fill stubs
		*(ULONG*)((PUCHAR)InjectBuffer + 1) = (ULONG)(ULONG_PTR)&InjectBuffer->ModuleHandle;
		*(ULONG*)((PUCHAR)InjectBuffer + 6) = (ULONG)(ULONG_PTR)pUserPath;
		*(ULONG*)((PUCHAR)InjectBuffer + 15) = (ULONG)((ULONG_PTR)LdrLoadDll - ((ULONG_PTR)InjectBuffer + 15) - 5 + 1);
		*(ULONG*)((PUCHAR)InjectBuffer + 20) = (ULONG)(ULONG_PTR)&InjectBuffer->Complete;
		*(ULONG*)((PUCHAR)InjectBuffer + 31) = (ULONG)(ULONG_PTR)&InjectBuffer->Status;

		return InjectBuffer;
	}

	UNREFERENCED_PARAMETER(DllFullPath);
	return NULL;
}


//封装函数：内部调用顺序为：ExecuteInNewThread--->SeCreateThreadEx--->NtCreateThreadEx
NTSTATUS ExecuteInNewThread(IN PVOID BaseAddress, IN PVOID Parameter, IN ULONG Flags, IN BOOLEAN Wait, OUT PNTSTATUS ExitStatus)
{
	//1.创建线程
	HANDLE ThreadHandle = NULL;
	OBJECT_ATTRIBUTES ObjectAttributes = { 0 };
	//指定一个对象句柄的属性  句柄只能在内核模式访问
	InitializeObjectAttributes(&ObjectAttributes, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
	NTSTATUS Status = SeCreateThreadEx(
		&ThreadHandle, THREAD_QUERY_LIMITED_INFORMATION, &ObjectAttributes,
		ZwCurrentProcess(), BaseAddress, Parameter, Flags,
		0, 0x1000, 0x100000, NULL
	);

	//2.等待线程完成
	if (NT_SUCCESS(Status) && Wait != FALSE)
	{
		//60s
		LARGE_INTEGER Timeout = { 0 };
		Timeout.QuadPart = -(60ll * 10 * 1000 * 1000);
		Status = ZwWaitForSingleObject(ThreadHandle, TRUE, &Timeout);
		if (NT_SUCCESS(Status))
		{
			//查询线程退出码
			THREAD_BASIC_INFORMATION ThreadBasicInfo = { 0 };
			ULONG ReturnLength = 0;

			Status = ZwQueryInformationThread(ThreadHandle, ThreadBasicInformation, &ThreadBasicInfo, sizeof(ThreadBasicInfo), &ReturnLength);
			if (NT_SUCCESS(Status) && ExitStatus)
			{
				*ExitStatus = ThreadBasicInfo.ExitStatus;
			}
			else if (!NT_SUCCESS(Status))
			{
				DbgPrint("%s: ZwQueryInformationThread failed with status 0x%X\n", __FUNCTION__, Status);
			}
		}
		else
			DbgPrint("%s: ZwWaitForSingleObject failed with status 0x%X\n", __FUNCTION__, Status);
	}
	else
	{
		DbgPrint("%s: ZwCreateThreadEx failed with status 0x%X\n", __FUNCTION__, Status);
	}
	if (ThreadHandle)
	{
		ZwClose(ThreadHandle);
	}
	return Status;
}


//封装函数：获得NtCreateThreadEx的函数地址--->NtCreateThreadEx创建用户线程
NTSTATUS NTAPI SeCreateThreadEx(
	OUT PHANDLE ThreadHandle,
	IN ACCESS_MASK DesiredAccess,
	IN PVOID ObjectAttributes,
	IN HANDLE ProcessHandle,	//目标进程句柄
	IN PVOID StartAddress,	//线程起始地址
	IN PVOID Parameter,		//线程参数
	IN ULONG Flags,
	IN SIZE_T StackZeroBits,
	IN SIZE_T SizeOfStackCommit,
	IN SIZE_T SizeOfStackReserve,
	IN PNT_PROC_THREAD_ATTRIBUTE_LIST AttributeList
)
{
	NTSTATUS Status = STATUS_SUCCESS;

	//1.获得NtCreateThreadEx的函数地址	
	PULONG_PTR STBase = KeServiceDescriptorTable->ServiceTableBase;
	LPFN_NTCREATETHREADEX NtCreateThreadEx = (LPFN_NTCREATETHREADEX)(STBase[0x58]);

	//2.NtCreateThreadEx创建用户线程
	if (NtCreateThreadEx)
	{
		//1.在ETHREAD偏移0x13a处找到先前模式
		PUCHAR pPrevMode = (PUCHAR)PsGetCurrentThread() + 0x13a;
		//2.取出先前模式的值，保存在TEMP中
		UCHAR TEMP = *pPrevMode;
		//3.修改先前模式的值为内核模式
		*pPrevMode = KernelMode;
		//4.创建线程
		Status = NtCreateThreadEx(
			ThreadHandle, DesiredAccess, ObjectAttributes,
			ProcessHandle, StartAddress, Parameter,
			Flags, StackZeroBits, SizeOfStackCommit,
			SizeOfStackReserve, AttributeList
		);
		//5.将之前保存在TEMP中的先前模式的值写回去
		*pPrevMode = TEMP;
	}
	else
		Status = STATUS_NOT_FOUND;

	return Status;
}