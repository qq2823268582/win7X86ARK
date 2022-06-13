#include"Myprocess.h"

typedef VOID(NTAPI* fpTypePspExitThread)(IN NTSTATUS ExitStatus);
extern fpTypePspExitThread g_fpPspExitThreadAddr;

//-------------------------------------ɱ������------------------------------------------------------
//1.�������PspTerminateThreadByPointer�ĺ�����ַ
ULONG GetPspTerminateThreadByPointer()
{
	//1.���PsTerminateSystemThread�ĺ�����ַ
	UNICODE_STRING funcName;
	RtlInitUnicodeString(&funcName, L"PsTerminateSystemThread");
	ULONG baseAddr = (ULONG)MmGetSystemRoutineAddress(&funcName);

	//2.��PsTerminateSystemThread�ĵ�ַΪ��㣬��1024���ֽڷ�Χ�ڽ�������
	ULONG FuncAddr = 0;
	for (ULONG i = baseAddr; i < (baseAddr + 1024); i++)
	{
		//���������룺0x50,0xe8
		if (((*(PUCHAR)(UCHAR*)(i - 1)) == 0x50) && ((*(PUCHAR)(UCHAR*)(i)) == 0xe8))
		{
			ULONG offset = *(PULONG)(i + 1);
			FuncAddr = i + 5 + offset;
			DbgPrint("�ҵ�PspTerminateThreadByPointer�ĺ�����ַ=%x", FuncAddr);
			break;
		}
	}
	return FuncAddr;
}
//2.�������PspExitThread�ĺ�����ַ
ULONG GetPspExitThread(ULONG BaseAddr)
{
	ULONG FuncAddr = 0;
	for (ULONG i = BaseAddr; i < (BaseAddr + 1024); i++)
	{
		//���������룺 0x0c,0xe8
		if (((*(PUCHAR)(UCHAR*)(i - 1)) == 0x0c) && ((*(PUCHAR)(UCHAR*)(i)) == 0xe8))
		{
			ULONG m_offset = *(PULONG)(i + 1);
			FuncAddr = i + 5 + m_offset;
			DbgPrint("�ҵ�PspExitThread�ĺ�����ַ=%x", FuncAddr);
			break;
		}
	}
	return FuncAddr;
}
//3.APC�ص��������ر��߳�
VOID SelfTerminateThread(KAPC *Apc, PKNORMAL_ROUTINE *NormalRoutine, PVOID *NormalContext, PVOID *SystemArgument1, PVOID *SystemArgument2)
{
	ExFreePool(Apc);
	g_fpPspExitThreadAddr(STATUS_SUCCESS);
}
//4.�Զ��庯����ɱ������
VOID KillProcess(HANDLE hPid)
{
	//1.ͨ��PID�ҵ����̵�EProcess
	PEPROCESS pEProcess = NULL;
	NTSTATUS status = PsLookupProcessByProcessId(hPid, &pEProcess);
	if (!NT_SUCCESS(status))
	{
		DbgPrint("û����PsCidTable���ҵ�����,���겻�������˰�\n");
		return;
	}

	//2.��������ȫ���̣߳��ҵ�ָ��EPROCESS���̲߳��ر�
	PEPROCESS pEProc = NULL;
	PETHREAD  pEThread = NULL;
	for (ULONG i = 4; i < 0x25600; i += 4)
	{
		//2.1�ҵ�ÿ��TID��Ӧ��ETHREAD
		if (NT_SUCCESS(PsLookupThreadByThreadId((HANDLE)i, &pEThread)))
		{
			//2.1.1.ͨ��ETHREAD�õ���Ӧ��pEProc
			pEProc = IoThreadToProcess(pEThread);
			//2.1.2�Ա�pEProc�Ƿ���Ŀ����̵�pEProcess��ȣ�����ȣ�˵������߳�����Ŀ�����
			if (pEProc == pEProcess)
			{
				DbgPrint("�ҵ�");

				//1.ΪAPC�����ڴ�
				PKAPC pApc = NULL;
				pApc = (PKAPC)ExAllocatePool(NonPagedPool, sizeof(KAPC));
				if (NULL == pApc) return;

				//2.�����ں�apc
				KeInitializeApc(pApc, (PKTHREAD)pEThread, OriginalApcEnvironment, (PKKERNEL_ROUTINE)&SelfTerminateThread, NULL, NULL, 0, NULL);
				KeInsertQueueApc(pApc, NULL, 0, 2);
			}

			//2.1.3.������̵߳�����
			ObDereferenceObject(pEThread);
		}
	}

	//3.����Խ��̵�����
	ObDereferenceObject(pEProcess);
}
//----------------------------------------------------------------------------------------------------


//-------------------------------------���ؽ���---------------------------------------------------
//1.����PsGetProcessId���UniqueProcessId��EPROCESS�ṹ���е�ƫ��--->���ActiveProcessLinks��EPROCESS�ṹ���е�ƫ�� ���汾ͨ�ã�
ULONG GetActiveListOffset()
{
	//���PsGetProcessId�����ĺ�����ַ
	UNICODE_STRING FunName = { 0 };
	RtlInitUnicodeString(&FunName, L"PsGetProcessId");
	PUCHAR pFun = (PUCHAR)MmGetSystemRoutineAddress(&FunName);

	if (pFun)
	{
		for (size_t i = 0; i < 0x100; i++)
		{
			//�����8B80 XXXX�У�XXXX��ΪUniqueProcessId��EPROCESS�ṹ���е�ƫ��
			if (pFun[i] == 0x8B && pFun[i + 1] == 0x80)
			{
				//��EPROCESS�ṹ���У�UniqueProcessId + 4ΪActiveProcessLinks
				return *(PULONG)(pFun + i + 2) + 4;
			}
		}
	}

	//���û�ҵ���˵��ʧ���ˣ�����0
	return 0;

}
//2.�Զ��庯�������ؽ���
VOID HideProcess(HANDLE hPid)
{
	//1.���ҽ���EPROCESS
	PEPROCESS pEProcess = NULL;
	NTSTATUS status = PsLookupProcessByProcessId(hPid, &pEProcess);
	if (!NT_SUCCESS(status))
	{
		DbgPrint("û����PsCidTable���ҵ�����,���겻�������˰�\n");
		return;
	}
	ObDereferenceObject(pEProcess);


	//2.���ActiveProcessLinks��EPROCESS�ṹ���е�ƫ��
	ULONG uOffset = GetActiveListOffset();
	if (!uOffset)
	{
		DbgPrint("û�л��ActiveProcessLinks��EPROCESS�ṹ���е�ƫ��\n");
		return;
	}

	//3.��õ�ǰ��������ڵ㡢ǰһ���ڵ㡢��һ���ڵ�
	PLIST_ENTRY MyNode = (PLIST_ENTRY)((PUCHAR)pEProcess + uOffset);
	PLIST_ENTRY nextNode = MyNode->Flink;
	PLIST_ENTRY preNode = MyNode->Blink;

	//4.����
	//4.1��һ���ڵ����һ���ڵ�ָ���ҵ���һ���ڵ�
	preNode->Flink = MyNode->Flink;
	//4.2��һ���ڵ����һ���ڵ�ָ���ҵ���һ���ڵ�
	nextNode->Blink = MyNode->Blink;
	//4.3�ҵ�Flink��Blink��ָ�����Լ�����������
	MyNode->Flink = MyNode->Blink = MyNode;
}


//-----------------------------------------���̱���------------------------------------------------
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


//������ǰ���̵�˽�о����
NTSTATUS SeScanProcessHandleTable(PEPROCESS CurEProcess, PEPROCESS TargetProcess)
{
	//1.����ΪNULL��EProcess
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	if (CurEProcess == NULL)
	{
		return Status;
	}

	//2.��EProcess--->HandleTable   //win7x86��EProcessƫ��0x0f4
	PHANDLE_TABLE  HandleTable = NULL;
	HandleTable = (PHANDLE_TABLE)(*((ULONG*)((UINT8*)CurEProcess + _HANDLE_TABLE_OFFSET_EPROCESS)));
	if (HandleTable == NULL)
	{
		return Status;
	}

	//3.HandleTable--->TableCode
	PVOID  FakeTableCode = NULL;
	FakeTableCode = HandleTable->TableCode; //TableCode��HandleTable��ַ�ص� //HandleTable->TableCode ���� *HandleTable������ȡ��

	//4.TableCode��2λ&3���ж��Ǽ�����
	ULONG  Flag = 0;
	Flag = (ULONG)(FakeTableCode) & 0x03;    //00 01 10

	//5.ȥ������λ������������TableCode
	ULONG TrueTableCode = 0;
	TrueTableCode = (ULONG)FakeTableCode & 0xFFFFFFFC;

	//6.����3���������
	switch (Flag)
	{
	case 0:
	{
		//����1�������  //TrueTableCode�Ǿ����Ļ�ַ //TrueTableCode��ʱ����һ��ָ��
		EnumTable0(TrueTableCode, CurEProcess, TargetProcess);
		break;
	}
	case 1:
	{
		//����2�������  //*TrueTableCode ���Ǿ����Ļ�ַ  //TrueTableCode��ʱ���ƶ���ָ��
		EnumTable1(TrueTableCode, CurEProcess, TargetProcess);
		break;
	}
	case 2:
	{
		//����3�������  //**TrueTableCode ���Ǿ����Ļ�ַ  //TrueTableCode��ʱ��������ָ��
		EnumTable2(TrueTableCode, CurEProcess, TargetProcess);
		break;
	}

	}
	return Status;
}

NTSTATUS EnumTable0(ULONG TableCode, PEPROCESS CurEProcess, PEPROCESS TargetProcess)
{
	//1.Խ��������;8�ֽڵ���һ��HandleTableEntry    //ΪʲôҪԽ����1�
	PHANDLE_TABLE_ENTRY HandleTableEntry = NULL;
	HandleTableEntry = (PHANDLE_TABLE_ENTRY)((ULONG*)(TableCode + _SPECIAL_PURPOSE));

	//2.����1����
	for (ULONG i = 0; i < _MAX; i++)
	{
		//1.���˵���Ч����
		if (!MmIsAddressValid((PVOID)HandleTableEntry))
		{
			HandleTableEntry++;
			continue;
		}

		//2.ȥ����3λ�����־,ת��Ϊ����ͷָ��
		PVOID ObjectHeader = (PVOID)((ULONG)(HandleTableEntry->Object) & 0xFFFFFFF8);

		//3.���˵���Ч�Ķ���ͷ
		if (!MmIsAddressValid(ObjectHeader))
		{
			HandleTableEntry++;
			continue;
		}

		//4.��ö�������
		UCHAR uIndex = *(PUCHAR)((ULONG)ObjectHeader + 0xC);

		//5.���˵����ǽ��̵Ķ���
		if (uIndex != 7)
		{
			HandleTableEntry++;
			continue;
		}

		//6.���EPROCESS
		PEPROCESS eProcess = (PEPROCESS)((UINT8*)ObjectHeader + _BODY_OFFSET_OBJECT_HEADER);

		//7.���˵���Ч��EPROCESS			
		if (!MmIsAddressValid(eProcess))
		{
			HandleTableEntry++;
			continue;
		}

		//8.���˵�����Ŀ��EPROCESS�Ľ���
		if (eProcess != TargetProcess)
		{
			HandleTableEntry++;
			continue;
		}

		//9.��ӡ��ǰ��������PID	
		ULONG uPid = 0;
		PUCHAR szProcessName = NULL;
		uPid = *(PULONG)((PUCHAR)CurEProcess + 0xb4);
		szProcessName = PsGetProcessImageFileName(CurEProcess);
		DbgPrint("��⵽��[%s]�����ܱ������� \n", szProcessName);

		//10.����ǰ���̶�Ӧ�����������Ȩ�޸���
		DbgPrint("�����ʼȨ��:[%x]\n", HandleTableEntry->GrantedAccess);
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
		DbgPrint("�����Ȩ���Ȩ��:[%x]\n", HandleTableEntry->GrantedAccess);
		//DbgPrint("----------------------------------------------\n\n");
		//9.�ṹ��ָ�����
		HandleTableEntry++;
	}
	return STATUS_SUCCESS;
}

NTSTATUS EnumTable1(ULONG TableCode, PEPROCESS CurEProcess, PEPROCESS TargetProcess)
{
	do
	{
		//ȡ��TableCode�����ֵ������PHANDLE_TABLE_ENTRY   //�е������ָ��
		EnumTable0((*(ULONG*)TableCode), CurEProcess, TargetProcess);
		//TableCode����1��
		(UINT8*)TableCode += sizeof(ULONG);

	} while (*(ULONG*)TableCode != 0 && MmIsAddressValid((PVOID)(*(ULONG*)TableCode))); //����ַ��Ч����ַ�е�ֵ��Ч��

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


//�ں��߳�1
NTSTATUS ProcessProtect(PEPROCESS TargetProcess)
{
	//1.����ΪNULL��EProcess
	NTSTATUS Status = STATUS_SUCCESS;
	if (TargetProcess == NULL)
	{
		return STATUS_UNSUCCESSFUL;
	}

	while (1)
	{
		//DbgPrint("----------------��ʼ���--------------------\n");
		//1.�����������еĽ��̣����EPROCESS
		PEPROCESS CurEProcess = NULL;
		for (ULONG i = 4; i < 0x10000; i = i + 4)
		{
			Status = PsLookupProcessByProcessId((HANDLE)i, &CurEProcess);
			if (NT_SUCCESS(Status))
			{
				ObDereferenceObject(CurEProcess);

				//2.�ڵ�ǰ���̵�˽�о�����У��鿴�Ƿ����Ŀ����̣��������޸ľ������н�Ȩ
				SeScanProcessHandleTable(CurEProcess, TargetProcess);
			}
		}
		//DbgPrint("----------------������--------------------\n");

		//3.����־bTerminatedΪTRUEʱ������ѭ��
		if (bTerminated1)
		{
			break;
		}

		//4.����5��
		LARGE_INTEGER inteval;
		inteval.QuadPart = -50000000;
		KeDelayExecutionThread(KernelMode, FALSE, &inteval);
	}

	//5.��ֹ�߳�
	PsTerminateSystemThread(STATUS_SUCCESS);	//PsTerminateSystemThreadֻ��Ե�ǰ�ں��̣߳��ұ��������ں��̵߳��ڲ�
	//return STATUS_SUCCESS;
}

//��װ�Ĵ����ں��̺߳���,�ڲ������ں��߳�1
NTSTATUS CreateKeThread1(PEPROCESS TargetProcess)
{
	OBJECT_ATTRIBUTES objAddr = { 0 };
	HANDLE threadHandle = 0;
	NTSTATUS status = STATUS_SUCCESS;
	// ��ʼ��һ��OBJECT_ATTRIBUTES ����
	InitializeObjectAttributes(&objAddr, NULL, OBJ_KERNEL_HANDLE, 0, NULL);
	// �����߳�
	status = PsCreateSystemThread(&threadHandle, THREAD_ALL_ACCESS, &objAddr, NULL, NULL, ProcessProtect, TargetProcess);
	if (NT_SUCCESS(status))
	{
		// ͨ���������̵߳Ķ���
		status = ObReferenceObjectByHandle(threadHandle, THREAD_ALL_ACCESS, *PsThreadType, KernelMode, &pThreadObj1, NULL);

		// �ͷž��
		ZwClose(threadHandle);

		if (!NT_SUCCESS(status))
		{
			// ����ȡ����ʧ�ܣ�Ҳ���ñ�־ΪTRUE
			bTerminated1 = TRUE;
		}
	}
	return status;
}


//�ں��߳�2
NTSTATUS ZeroDebugPort(PEPROCESS TargetProcess)
{
	//1.����ΪNULL��EProcess
	NTSTATUS Status = STATUS_SUCCESS;
	if (TargetProcess == NULL)
	{
		return STATUS_UNSUCCESSFUL;
	}

	while (1)
	{
		//1.����յ��źţ�������ѭ��
		if (bTerminated2)
		{
			break;
		}

		//DbgPrint("----��ʼ��DebugPort������0----\n\n");
			__try
			{
				DWORD64 debugport = *(DWORD64*)((DWORD64)TargetProcess + 0xec);
				DbgPrint("��ǰdebugport��ֵ--->[%d]\n", debugport);

				if (debugport != 0)
				{
					*(DWORD64*)((DWORD64)TargetProcess + 0xec) = 0;
					debugport = *(DWORD64*)((DWORD64)TargetProcess + 0xec);
					DbgPrint("������debugport��ֵ--->[%d]\n", debugport);
				}
			}
			__except (1)
			{
				DbgPrint("%s: Exception\n", __FUNCTION__);
			}		

		//3.����5��
		LARGE_INTEGER inteval;
		inteval.QuadPart = -50000000;
		KeDelayExecutionThread(KernelMode, FALSE, &inteval);
	}

	//4.��ֹ�߳�
	PsTerminateSystemThread(STATUS_SUCCESS);
	//return STATUS_SUCCESS;
}

//��װ�Ĵ����ں��̺߳���,�ڲ������ں��߳�2
NTSTATUS CreateKeThread2(PEPROCESS TargetProcess)
{
	OBJECT_ATTRIBUTES objAddr = { 0 };
	HANDLE threadHandle = 0;
	NTSTATUS status = STATUS_SUCCESS;
	// ��ʼ��һ��OBJECT_ATTRIBUTES ����
	InitializeObjectAttributes(&objAddr, NULL, OBJ_KERNEL_HANDLE, 0, NULL);
	// �����߳�
	status = PsCreateSystemThread(&threadHandle, THREAD_ALL_ACCESS, &objAddr, NULL, NULL, ZeroDebugPort, TargetProcess);
	if (NT_SUCCESS(status))
	{
		// ͨ���������̵߳Ķ���
		status = ObReferenceObjectByHandle(threadHandle, THREAD_ALL_ACCESS, *PsThreadType, KernelMode, &pThreadObj2, NULL);

		// �ͷž��
		ZwClose(threadHandle);

		if (!NT_SUCCESS(status))
		{
			// ����ȡ����ʧ�ܣ�Ҳ���ñ�־ΪTRUE
			bTerminated2 = TRUE;
		}
	}
	return status;
}


VOID ProtectTheProcess(HANDLE hPid)
{
	//1.���ҽ���EPROCESS
	PEPROCESS TargetProcess = NULL;
	NTSTATUS status = PsLookupProcessByProcessId(hPid, &TargetProcess);
	if (!NT_SUCCESS(status))
	{
		DbgPrint("û���ҵ�����,���겻�������˰�\n");
		return ;
	}
	ObDereferenceObject(TargetProcess);

	//2.�����ں��̣߳����н��̱���
	CreateKeThread1(TargetProcess);
	CreateKeThread2(TargetProcess);
		
}


//-----------------------------------------------���̵�VAD���---------------------------------
//������ȡVAD����Ŀ
VOID EnumVADCount(IN PMMADDRESS_NODE pVad,OUT PULONG pVadCount)
{
	//1.���˵���Ч��vad
	if (pVad == NULL)
	{
		return;
	}

	//2.����������
	EnumVADCount(pVad->LeftChild, pVadCount);

	//3.�ڵ�+1
	(*pVadCount)++;

	//4.����������
	EnumVADCount(pVad->RightChild, pVadCount);

	return;
}

//GetVADCount--->EnumVADCount
ULONG GetVADCount(HANDLE hPid)
{
	//1.ͨ��PID���EPROCESS
	PEPROCESS EProcess = NULL;
	NTSTATUS status = PsLookupProcessByProcessId(hPid, &EProcess);
	if (!NT_SUCCESS(status))
	{
		DbgPrint("û���ҵ�����,���겻�������˰�\n");
		return 0;
	}
	ObDereferenceObject(EProcess);

	//2.��ý��̵�VAD�������ĵ�ַ
	PMMADDRESS_NODE VadRoot = NULL;
	VadRoot = (PMMADDRESS_NODE)((ULONG)EProcess + 0x278);

	//3.��ʼ����VAD
	ULONG VadCount = 0;
	EnumVADCount(VadRoot->RightChild,&VadCount);

	return VadCount;
}


//�������VAD��AVL��
VOID _EnumVAD(PMMADDRESS_NODE pVad, PVADINFO* ppOutVADInfo)
{
	//1.���˵���Ч��vad
	if (pVad == NULL)
	{
		return;
	}

	//2.����������
	_EnumVAD(pVad->LeftChild, ppOutVADInfo);

	//3.�ص����ڵ�
	MMVAD* Root = (MMVAD*)pVad;
	
	(*ppOutVADInfo)->pVad = (ULONG)Root;

	(*ppOutVADInfo)->Start = (ULONG)Root->StartingVpn;

	(*ppOutVADInfo)->End = (ULONG)Root->EndingVpn;

	(*ppOutVADInfo)->Commit = (ULONG)Root->u.VadFlags.CommitCharge;

	(*ppOutVADInfo)->Type = (ULONG)Root->u.VadFlags.PrivateMemory;

	(*ppOutVADInfo)->Protection = (ULONG)Root->u.VadFlags.Protection;

	
	//4.�����ڴ�  
	POBJECT_NAME_INFORMATION  stFileBuffer = (POBJECT_NAME_INFORMATION)ExAllocatePool(PagedPool, 0x100);

	//5.�ڸ��ڵ��ѯ�ڴ��
	ULONG uRetLen = 0;
	if (MmIsAddressValid(Root->Subsection) && MmIsAddressValid(Root->Subsection->ControlArea) && stFileBuffer != NULL)
	{
		if (MmIsAddressValid((PVOID)Root->Subsection->ControlArea->FilePointer.Value))
		{
			//��3λ��0���õ��ڴ���Ӧ��FILE_OBJECT�����ַ
			PFILE_OBJECT pFileObject = (PFILE_OBJECT)((Root->Subsection->ControlArea->FilePointer.Value >> 3) << 3);

			//���FILE_OBJECT�����ַ��Ч
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
	
	
	//6.�ͷ�������ڴ�
	if (stFileBuffer)
	{
		ExFreePool(stFileBuffer);
	}
	
	//7.ָ�����
	(*ppOutVADInfo)++;  //����������

	//8.����������
	_EnumVAD(pVad->RightChild, ppOutVADInfo);

	return;
}

//_EnumProcessVAD-->_EnumVAD
NTSTATUS _EnumProcessVAD(HANDLE hPid, PVADINFO pOutVADInfo)
{
	//1.ͨ��PID���EPROCESS
	PEPROCESS EProcess = NULL;
	NTSTATUS status = PsLookupProcessByProcessId(hPid, &EProcess);
	if (!NT_SUCCESS(status))
	{
		DbgPrint("û���ҵ�����,���겻�������˰�\n");
		return STATUS_SUCCESS;
	}
	ObDereferenceObject(EProcess);

	//2.��ý��̵�VAD�������ĵ�ַ
	PMMADDRESS_NODE VadRoot = NULL;
	VadRoot = (PMMADDRESS_NODE)((ULONG)EProcess + 0x278);

	//3.��ʼ����VAD
	_EnumVAD(VadRoot->RightChild, &pOutVADInfo);

	return STATUS_SUCCESS;
}