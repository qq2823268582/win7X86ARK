#include <Ntifs.h>
#include "Myprocess.h"
#include "Mydriver.h"
#include "Mygdt.h"
#include "Myidt.h"
#include "Myssdt.h"
#include "ReloadKernel.h"
#include "Myinject.h"
#include "MyCallBack.h"
#include "MyDpcTimer.h"
#include "MyIoTimer.h"

ULONG  g_uPid;	                 //Ҫ�����ı�ARK���̵�PID
PDRIVER_OBJECT driver1=NULL;
//--------------------------------------------------------------------------------------------------------------//

extern PETHREAD pThreadObj1;
extern BOOLEAN bTerminated1;

extern PETHREAD pThreadObj2;
extern BOOLEAN bTerminated2;

//------------------------------------------δ��������������-----------------------------------------------//

NTSTATUS PsReferenceProcessFilePointer(IN  PEPROCESS Process, OUT PVOID *OutFileObject);
NTKERNELAPI HANDLE PsGetProcessInheritedFromUniqueProcessId(IN PEPROCESS Process);//δ�������е���
NTKERNELAPI struct _PEB* PsGetProcessPeb(PEPROCESS proc);
typedef VOID(NTAPI* fpTypePspExitThread)(IN NTSTATUS ExitStatus);
fpTypePspExitThread g_fpPspExitThreadAddr = NULL;
//----------------------------------------------------------------------------------------------------------------//


//--------------------------------------------ͨ���豸-------------------------------------//
//1.�豸��
#define NAME_DEVICE L"\\Device\\ArkToMe"
//2.���ӷ�����
#define  NAME_SYMBOL L"\\??\\ArkToMe666"
//3.�������
#define MYCTLCODE(code) CTL_CODE(FILE_DEVICE_UNKNOWN,0x800+(code),METHOD_OUT_DIRECT,FILE_ANY_ACCESS)
//4.������ö��
enum MyCtlCode
{
	getDriverCount = MYCTLCODE(1),		//��ȡ��������
	enumDriver = MYCTLCODE(2),		//��������
	hideDriver = MYCTLCODE(3),		//��������
	getProcessCount = MYCTLCODE(4),	//��ȡ��������
	enumProcess = MYCTLCODE(5),		//��������
	hideProcess = MYCTLCODE(6),		//���ؽ���
	killProcess = MYCTLCODE(7),		//��������
	getModuleCount = MYCTLCODE(8),	//��ȡָ�����̵�ģ������
	enumModule = MYCTLCODE(9),		//����ָ�����̵�ģ��
	getThreadCount = MYCTLCODE(10),	//��ȡָ�����̵��߳�����
	enumThread = MYCTLCODE(11),		//����ָ�����̵��߳�
	getFileCount = MYCTLCODE(12),	//��ȡ�ļ�����
	enumFile = MYCTLCODE(13),		//�����ļ�
	deleteFile = MYCTLCODE(14),		//ɾ���ļ�
	getRegKeyCount = MYCTLCODE(15),	//��ȡע�����������
	enumReg = MYCTLCODE(16),		//����ע���
	//newKey = MYCTLCODE(17),			//�����¼�
	deleteKey = MYCTLCODE(18),		//ɾ���¼�
	enumIdt = MYCTLCODE(19),		//����IDT��
	getGdtCount = MYCTLCODE(20),	//��ȡGDT���ж�������������
	enumGdt = MYCTLCODE(21),		//����GDT��
	getSsdtCount = MYCTLCODE(22),	//��ȡSSDT����ϵͳ����������������
	enumSsdt = MYCTLCODE(23),		//����SSDT��
	selfPid = MYCTLCODE(24),		//������Ľ���ID����0��
	hideThread = MYCTLCODE(25),       //�����߳�
	hideModule = MYCTLCODE(26),       //����ģ��
	DriveInject = MYCTLCODE(27),      //����ע��
	ProtectProcess = MYCTLCODE(28),   //���̱���
	getVadCount = MYCTLCODE(29),      //���VAD������
	enumProcessVad = MYCTLCODE(30),   //����VAD����
	enumNotify = MYCTLCODE(17),       //����ϵͳ�ص�
	enumObcallback = MYCTLCODE(31),  //��������ص�
	enumDpctimer = MYCTLCODE(32),    //����DPC��ʱ��
	enumIotimer = MYCTLCODE(33),     //����IO��ʱ��
};
//5.���豸
NTSTATUS OnCreate(DEVICE_OBJECT *pDevice, IRP *pIrp)
{
	//�����������δʹ�õĴ���
	UNREFERENCED_PARAMETER(pDevice);

	//����IRP���״̬
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	//����IRP�����˶��ٸ��ֽ�
	pIrp->IoStatus.Information = 0;
	//����IRP
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}
//6.�ر��豸
NTSTATUS OnClose(DEVICE_OBJECT* pDevice, IRP* pIrp)
{
	//�����������δʹ�õĴ���
	UNREFERENCED_PARAMETER(pDevice);
	KdPrint(("�豸���ر�\n"));
	//1.����IRP���״̬
	pIrp->IoStatus.Status = STATUS_SUCCESS;

	//2.����IRP�����˶��ٸ��ֽ�
	pIrp->IoStatus.Information = 0;

	//3.����IRP �����������´��ݣ�
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}
//7.�豸I/O
NTSTATUS OnDeviceIoControl(DEVICE_OBJECT* pDevice, IRP* pIrp)
{
	//--------------------------------------I/Oͨ�ŵ�һ������ȡ���ֱر���Ϣ-------------------------------------
	//1.��ȡ��ǰIRPջ
	PIO_STACK_LOCATION pIoStack = IoGetCurrentIrpStackLocation(pIrp);

	//2.��ȡ������
	ULONG uCtrlCode = pIoStack->Parameters.DeviceIoControl.IoControlCode;

	//3.��ȡI/O����
	PVOID pInputBuff = NULL;
	PVOID pOutBuff = NULL;
	//3.1��ȡ���뻺����(�������)
	if (pIrp->AssociatedIrp.SystemBuffer != NULL)
	{
		pInputBuff = pIrp->AssociatedIrp.SystemBuffer;
	}
	//3.2��ȡ���������(�������)
	if (pIrp->MdlAddress != NULL)
	{
		//��ȡMDL���������ں��е�ӳ��
		pOutBuff = MmGetSystemAddressForMdlSafe(pIrp->MdlAddress, NormalPagePriority);
	}

	//4.��ȡ����I/O����Ĵ�С
	ULONG inputSize = pIoStack->Parameters.DeviceIoControl.InputBufferLength;

	//5.��ȡ���I/O����Ĵ�С
	ULONG outputSize = pIoStack->Parameters.DeviceIoControl.OutputBufferLength;


	//-----------------------------------------I/Oͨ�ŵڶ������ַ������룬���в�ͬ����Ϊ����------------------------
	//������Ӧ�Ŀ����������Ӧ����
	switch (uCtrlCode)
	{
	case getProcessCount:
	{
		PEPROCESS proc = NULL;
		//�����õ�
		ULONG ProcessCount = 0;
		// �趨PID��Χ,ѭ������
		for (int i = 4; i < 100000; i += 4)
		{
			// ��ͨ��PID���ҵ�EPROCESS����ŵ�proc��
			if (NT_SUCCESS(PsLookupProcessByProcessId((HANDLE)i, &proc)))
			{
				// ��һ���жϽ����Ƿ���Ч
				ULONG TableCode = *(ULONG*)((ULONG)proc + 0xF4);
				if (TableCode)
				{
					ProcessCount++;// ����+1 
				}
				ObDereferenceObject(proc);// �ݼ����ü���
			}
		}

		//6.��������������д�����������
		*(ULONG*)pOutBuff = ProcessCount;

		//------------------------------------------��Ϊ���ص��ֽ��б仯�������Լ�����ֱ��return---------
		 //1.����IRP���״̬
		pIrp->IoStatus.Status = STATUS_SUCCESS;    //״̬Ϊ�ɹ�
		//2.����IRP�����˶��ٸ��ֽ�
		pIrp->IoStatus.Information = sizeof(ProcessCount);
		//3.����IRP
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
		return STATUS_SUCCESS;
	}

	case enumProcess:
	{
		//��仰��2�����ã����Ա���һ��pOutBuff��Ϊ��ַ��������ǿ��ת��pOutBuffΪPPROCESSINFO����
		//�����������ȫ��0
		RtlFillMemory(pOutBuff, outputSize, 0);
		PPROCESSINFO pOutProcessInfo = (PPROCESSINFO)pOutBuff;

		NTSTATUS status = STATUS_SUCCESS;
		PEPROCESS pEProcess = NULL;

		// ��ʼ����
		for (ULONG i = 4; i < 0x10000; i = i + 4)
		{
			status = PsLookupProcessByProcessId((HANDLE)i, &pEProcess);
			if (NT_SUCCESS(status))  //һ��Ҫ�ж�status�Ƿ�ɹ������������
			{
				// ��һ���жϽ����Ƿ���Ч
				ULONG TableCode = *(ULONG*)((ULONG)pEProcess + 0xF4);
				if (TableCode)
				{
					// �ӽ��̽ṹ�л�ȡ������Ϣ
					HANDLE hParentProcessId = NULL;
					HANDLE hProcessId = NULL;
					PCHAR pszProcessName = NULL;
					PVOID pFilePoint = NULL;
					POBJECT_NAME_INFORMATION pObjectNameInfo = NULL;

					// ��ȡ���̳���·��
					status = PsReferenceProcessFilePointer(pEProcess, &pFilePoint);
					if (NT_SUCCESS(status))
					{
						status = IoQueryFileDosDeviceName(pFilePoint, &pObjectNameInfo);
						if (NT_SUCCESS(status))
						{
							//DbgPrint("----------------%d------------------------", i);
							//DbgPrint("PEPROCESS=0x%p\n", pEProcess);

							//��ȡ����·��
						   // DbgPrint("PATH=%ws\n", pObjectNameInfo->Name.Buffer);
							RtlCopyMemory(pOutProcessInfo->szpath, pObjectNameInfo->Name.Buffer, pObjectNameInfo->Name.Length);

							//��ȡ������ PID
							//hParentProcessId = PsGetProcessInheritedFromUniqueProcessId(pEProcess);
							//DbgPrint("PPID=%d\n", hParentProcessId);
							pOutProcessInfo->PPID = (ULONG)PsGetProcessInheritedFromUniqueProcessId(pEProcess);

							// ��ȡ���� PID
							//hProcessId = PsGetProcessId(pEProcess);
							//DbgPrint("PID=%d\n", hProcessId);
							pOutProcessInfo->PID = (ULONG)PsGetProcessId(pEProcess);

							pOutProcessInfo++;
						}
						ObDereferenceObject(pFilePoint);
					}
				}
				ObDereferenceObject(pEProcess);
			}
		}

		//------------------------------------------��Ϊ���ص��ֽ��б仯�������Լ�����ֱ��return---------
		//1.����IRP���״̬
		pIrp->IoStatus.Status = STATUS_SUCCESS;    //״̬Ϊ�ɹ�
		//2.����IRP�����˶��ٸ��ֽ�
		pIrp->IoStatus.Information = (ULONG)pOutProcessInfo - (ULONG)pOutBuff;   //�����ֽ�����ô������ѵ
		//3.����IRP
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
		return STATUS_SUCCESS;
	}

	case killProcess:
	{
		//1.�������PspTerminateThreadByPointer�ĺ�����ַ
		ULONG uPspTerminateThreadByPointerAddr = GetPspTerminateThreadByPointer();
		if (0 == uPspTerminateThreadByPointerAddr)
		{
			DbgPrint("����PspTerminateThreadByPointerAddr��ַ����\n");
			return STATUS_SUCCESS;
		}

		//2.�������PspExitThread�ĺ�����ַ
		g_fpPspExitThreadAddr = (fpTypePspExitThread)GetPspExitThread(uPspTerminateThreadByPointerAddr);
		if (NULL == g_fpPspExitThreadAddr)
		{
			DbgPrint("����PspExitThread��ַ����\n");
			return STATUS_SUCCESS;
		}

		//3.�����뻺�����õ����̵�PID
		HANDLE processid = (HANDLE)(*(ULONG*)pInputBuff);

		//4.��������ȫ���̣߳��ҵ�ָ��EPROCESS������ȫ���̲߳��ر�
		KillProcess(processid);

		break;
	}

	case hideProcess:
	{
		//1.�����뻺�����õ����̵�PID
		HANDLE processid = (HANDLE)(*(ULONG*)pInputBuff);

		//2.��������ָ��PID�Ľ���
		HideProcess(processid);

		break;
	}

	case getModuleCount:
	{
		//1.�����뻺�����õ����̵�PID
		HANDLE processid = (HANDLE)(*(ULONG*)pInputBuff);

		//2.����PID��Ӧ�Ľ���EPROCESS
		PEPROCESS pEProcess = NULL;
		NTSTATUS status = PsLookupProcessByProcessId(processid, &pEProcess);
		if (!NT_SUCCESS(status))
		{
			DbgPrint("û���ҵ�����,���겻�������˰�\n");
			return STATUS_SUCCESS;
		}
		//�ݼ����ü���
		ObDereferenceObject(pEProcess);

		//3.�ҵ�PEB(����PEB���û���ռ�,�����Ҫ���̹ҿ���
		KAPC_STATE pAPC = { 0 };
		KeStackAttachProcess(pEProcess, &pAPC);
		//����PsGetProcessPebδ��������Ҫ����
		PPEB peb = PsGetProcessPeb(pEProcess);
		if (peb == NULL)
		{
			DbgPrint("û���ҵ�peb\n");
			return STATUS_SUCCESS;
		}

		//4.ͨ��PEB����LDR�е�LIST_ENTRY
		PLIST_ENTRY pList_First = (PLIST_ENTRY)(*(ULONG*)((ULONG)peb + 0xc) + 0xc);
		PLIST_ENTRY pList_Temp = pList_First;
		ULONG ModuleCount = 0;
		do
		{
			pList_Temp = pList_Temp->Blink;
			ModuleCount++;
		} while (pList_Temp != pList_First);

		//5.��ģ������д�����������
		*(ULONG*)pOutBuff = ModuleCount;

		//6.���̽���ҿ�
		KeUnstackDetachProcess(&pAPC);

		//------------------------------------------��Ϊ���ص��ֽ��б仯�������Լ�����ֱ��return---------
	   //1.����IRP���״̬
		pIrp->IoStatus.Status = STATUS_SUCCESS;    //״̬Ϊ�ɹ�
		//2.����IRP�����˶��ٸ��ֽ�
		pIrp->IoStatus.Information = sizeof(ModuleCount);
		//3.����IRP
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
		return STATUS_SUCCESS;
	}

	case enumModule:
	{
		//1.�����뻺�����õ����̵�PID
		HANDLE processid = (HANDLE)(*(ULONG*)pInputBuff);

		//2.����PID��Ӧ�Ľ���EPROCESS
		PEPROCESS pEProcess = NULL;
		NTSTATUS status = PsLookupProcessByProcessId(processid, &pEProcess);
		if (!NT_SUCCESS(status))
		{
			DbgPrint("û���ҵ�����,���겻�������˰�\n");
			return STATUS_SUCCESS;
		}
		//�ݼ����ü���
		ObDereferenceObject(pEProcess);

		//3.�ҵ�PEB(����PEB���û���ռ�,�����Ҫ���̹ҿ���
		KAPC_STATE pAPC = { 0 };
		KeStackAttachProcess(pEProcess, &pAPC);
		//����PsGetProcessPebδ��������Ҫ����
		PPEB peb = PsGetProcessPeb(pEProcess);
		if (peb == NULL)
		{
			DbgPrint("û���ҵ�peb\n");
			return STATUS_SUCCESS;
		}

		//4.ͨ��PEB����LDR�е�LIST_ENTRY
		PLIST_ENTRY pList_First = (PLIST_ENTRY)(*(ULONG*)((ULONG)peb + 0xc) + 0xc);
		PLIST_ENTRY pList_Temp = pList_First;

		//5.����LISTENTRYƫ�ƻ��ģ���С����ַ��ƫ��
		RtlFillMemory(pOutBuff, outputSize, 0);
		PMODULEINFO pOutModuleInfo = (PMODULEINFO)pOutBuff;
		do
		{
			//ǿ��ת�����ͣ���ΪLISTENTRY�Ļ�ַ�պ���PLDR_DATA_TABLE_ENTRY�Ļ�ַ�غ�
			PLDR_DATA_TABLE_ENTRY pModuleInfo = (PLDR_DATA_TABLE_ENTRY)pList_Temp;
			pList_Temp = pList_Temp->Blink;

			if (pModuleInfo->FullDllName.Buffer != 0)
			{
				//����ģ��·��
				RtlCopyMemory(pOutModuleInfo->wcModuleFullPath, pModuleInfo->FullDllName.Buffer, pModuleInfo->FullDllName.Length);
				//����ģ���ַ
				pOutModuleInfo->DllBase = (ULONG)pModuleInfo->DllBase;
				//����ģ���С
				pOutModuleInfo->SizeOfImage = pModuleInfo->SizeOfImage;
			}
			pOutModuleInfo++;

		} while (pList_Temp != pList_First);

		//6.������̹ҿ�
		KeUnstackDetachProcess(&pAPC);

		//------------------------------------------��Ϊ���ص��ֽ��б仯�������Լ�����ֱ��return---------
	   //1.����IRP���״̬
		pIrp->IoStatus.Status = STATUS_SUCCESS;    //״̬Ϊ�ɹ�
		//2.����IRP�����˶��ٸ��ֽ�
		pIrp->IoStatus.Information = (ULONG)pOutModuleInfo - (ULONG)pOutBuff;   //�����ֽ�����ô������ѵ
		//3.����IRP
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
		return STATUS_SUCCESS;
	}

	case getThreadCount:
	{
		//1.�����뻺�����õ����̵�PID
		HANDLE processid = (HANDLE)(*(ULONG*)pInputBuff);

		//2.����PID��Ӧ�Ľ���EPROCESS
		PEPROCESS pEProcess = NULL;
		NTSTATUS status = PsLookupProcessByProcessId(processid, &pEProcess);
		if (!NT_SUCCESS(status))
		{
			DbgPrint("û���ҵ�����,���겻�������˰�\n");
			return STATUS_SUCCESS;
		}
		//�ݼ����ü���
		ObDereferenceObject(pEProcess);

		//3.��ȡ�߳�����
		PLIST_ENTRY pList_First = (PLIST_ENTRY)((ULONG)pEProcess + 0x188);
		PLIST_ENTRY pList_Temp = pList_First;
		ULONG ThreadCount = 0;
		do
		{
			pList_Temp = pList_Temp->Blink;
			ThreadCount++;
		} while (pList_Temp != pList_First);

		//4.���߳�����д�����������
		*(ULONG*)pOutBuff = ThreadCount;

		//------------------------------------------��Ϊ���ص��ֽ��б仯�������Լ�����ֱ��return---------
		//1.����IRP���״̬
		pIrp->IoStatus.Status = STATUS_SUCCESS;    //״̬Ϊ�ɹ�
		//2.����IRP�����˶��ٸ��ֽ�
		pIrp->IoStatus.Information = sizeof(ThreadCount);
		//3.����IRP
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
		return STATUS_SUCCESS;
	}

	case enumThread:
	{
		//1.�����뻺�����õ����̵�PID
		HANDLE processid = (HANDLE)(*(ULONG*)pInputBuff);

		//2.����PID��Ӧ�Ľ���EPROCESS
		PEPROCESS pEProcess = NULL;
		NTSTATUS status = PsLookupProcessByProcessId(processid, &pEProcess);
		if (!NT_SUCCESS(status))
		{
			DbgPrint("û���ҵ�����,���겻�������˰�\n");
			return STATUS_SUCCESS;
		}
		//�ݼ����ü���
		ObDereferenceObject(pEProcess);

		//3.��ȡ�߳�����
		PLIST_ENTRY pList_First = (PLIST_ENTRY)((ULONG)pEProcess + 0x188);
		PLIST_ENTRY pList_Temp = pList_First;

		//4.�����������ȫ��0,��ǿ��ת������
		RtlFillMemory(pOutBuff, outputSize, 0);
		PTHREADINFO pOutThreadInfo = (PTHREADINFO)pOutBuff;
		do
		{
			//1.��ȡLISTENTRY������ETHREAD��ַ
			PETHREAD pThreadInfo = (PETHREAD)((ULONG)pList_Temp - 0x268);

			//2.��ȡ�߳�ID
			pOutThreadInfo->Tid = *(PULONG)((ULONG)pThreadInfo + 0x230);

			//3.��ȡ�߳�ִ�п��ַ
			pOutThreadInfo->pEthread = (ULONG)pThreadInfo;

			//4.��ȡTeb�ṹ��ַ
			pOutThreadInfo->pTeb = *(PULONG)((ULONG)pThreadInfo + 0x88);

			//5.��ȡ��̬���ȼ�
			pOutThreadInfo->BasePriority = *(CHAR*)((ULONG)pThreadInfo + 0x135);

			//6.��ȡ�л�����
			pOutThreadInfo->ContextSwitches = *(PULONG)((ULONG)pThreadInfo + 0x64);

			//7.����ڵ�����ƶ�
			pList_Temp = pList_Temp->Blink;

			//8.���������ָ�����
			pOutThreadInfo++;
		} while (pList_Temp != pList_First);

		//------------------------------------------��Ϊ���ص��ֽ��б仯�������Լ�����ֱ��return---------
		//1.����IRP���״̬
		pIrp->IoStatus.Status = STATUS_SUCCESS;    //״̬Ϊ�ɹ�
		//2.����IRP�����˶��ٸ��ֽ�
		pIrp->IoStatus.Information = (ULONG)pOutThreadInfo - (ULONG)pOutBuff;   //�����ֽ�����ô������ѵ
		//3.����IRP
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
		return STATUS_SUCCESS;
	}

	case getDriverCount:
	{
		//1.��ȡ������
		PLDR_DATA_TABLE_ENTRY pLdr = (PLDR_DATA_TABLE_ENTRY)pDevice->DriverObject->DriverSection;
		//2.��ȡ��һ������ڵ�
		PLIST_ENTRY pList_First = &pLdr->InLoadOrderLinks;
		PLIST_ENTRY pList_Temp = pList_First;
		//3.��������
		ULONG driverCount = 0;
		do
		{
			pList_Temp = pList_Temp->Blink;
			driverCount++;
		} while (pList_Temp != pList_First);
		//4.���ݴ���-д��3��
		*(ULONG*)pOutBuff = driverCount;

		//---------------------------�Լ�����--------------------------------------		
		pIrp->IoStatus.Status = STATUS_SUCCESS;          // ���״̬
		pIrp->IoStatus.Information = sizeof(driverCount);// �ܹ������ֽ���
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);       //���������´���
		return STATUS_SUCCESS;
	}

	case enumDriver:
	{
		//1.��ȡ������
		PLDR_DATA_TABLE_ENTRY pLdr = (PLDR_DATA_TABLE_ENTRY)pDevice->DriverObject->DriverSection;

		//2.��ȡ��һ������ڵ�
		PLIST_ENTRY pList_First = &pLdr->InLoadOrderLinks;
		PLIST_ENTRY pList_Temp = pList_First;

		//3.�����������ȫ��0��ǿ��ת������
		RtlFillMemory(pOutBuff, outputSize, 0);
		PDRIVERINFO pOutDriverInfo = (PDRIVERINFO)pOutBuff;

		//4.ѭ��������������
		do
		{
			PLDR_DATA_TABLE_ENTRY pDriverInfo = (PLDR_DATA_TABLE_ENTRY)pList_Temp;			
			if (pDriverInfo->DllBase != 0)
			{
				//��ȡ������
				RtlCopyMemory(pOutDriverInfo->wcDriverBasePath, pDriverInfo->BaseDllName.Buffer, pDriverInfo->BaseDllName.Length);
				//��ȡ����·��
				RtlCopyMemory(pOutDriverInfo->wcDriverFullPath, pDriverInfo->FullDllName.Buffer, pDriverInfo->FullDllName.Length);
				//��ȡ������ַ
				pOutDriverInfo->DllBase = (ULONG)pDriverInfo->DllBase;
				//��ȡ������С
				pOutDriverInfo->SizeOfImage = pDriverInfo->SizeOfImage;
			}

			//�������
			pList_Temp = pList_Temp->Blink;
			//���������ָ�����
			pOutDriverInfo++;

		} while (pList_Temp != pList_First);


		pIrp->IoStatus.Status = STATUS_SUCCESS;          // ���״̬
		pIrp->IoStatus.Information = (ULONG)pOutDriverInfo - (ULONG)pOutBuff;
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);       //���������´���
		return STATUS_SUCCESS;
	}

	case hideDriver:
	{
		//1.��ȡ������
		PLDR_DATA_TABLE_ENTRY pLdr = (PLDR_DATA_TABLE_ENTRY)pDevice->DriverObject->DriverSection;

		//2.��ȡ��һ������ڵ�
		PLIST_ENTRY pList_First = &pLdr->InLoadOrderLinks;
		PLIST_ENTRY pList_Temp = pList_First;

		//3.�����뻺����ȡ��Ŀ���������������и�ʽ����UNICODE_STRING��
		UNICODE_STRING pHideDriverName = { 0 };
		RtlInitUnicodeString(&pHideDriverName, pInputBuff);

		//4.�������������Ա����֣��ҵ�Ŀ���Ŀ����������
		do
		{
			//ǿ��ת�����ͣ������ó�Ա
			PLDR_DATA_TABLE_ENTRY pDriverInfo = (PLDR_DATA_TABLE_ENTRY)pList_Temp;
			//�Ա�����
			if (RtlCompareUnicodeString(&pDriverInfo->BaseDllName, &pHideDriverName, FALSE) == 0)
			{
				//�ı������ǰ�����ӵ�
				pList_Temp->Blink->Flink = pList_Temp->Flink;				
				pList_Temp->Flink->Blink = pList_Temp->Blink;
				
				//�ı������ǰ�����ӵ�
				pList_Temp->Flink = (PLIST_ENTRY)&pList_Temp->Flink;
				pList_Temp->Blink = (PLIST_ENTRY)&pList_Temp->Blink;

				break;
			}
			//���û�ҵ����������
			pList_Temp = pList_Temp->Blink;

		} while (pList_Temp != pList_First);
		
		pIrp->IoStatus.Status = STATUS_SUCCESS;          // ���״̬
		pIrp->IoStatus.Information = 0;// �ܹ������ֽ���
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);       //���������´���
		return STATUS_SUCCESS;
	}

	case getGdtCount:
	{
		//1.��ȡGDTR�Ĵ�����ֵ,��ŵ�sgdtInfo��     //ָ�sgdt xxxx
		XDT_INFO sgdtInfo = { 0,0,0 };
		_asm sgdt sgdtInfo;

		//2.��ȡGDT�������׵�ַ
		PGDT_ENTER pGdtEntry = NULL;
		pGdtEntry = (PGDT_ENTER)MAKE_LONG(sgdtInfo.xLowXdtBase, sgdtInfo.xHighXdtBase);
				
		//3.��ȡGDT���������   //������Ч����Ч��gdt� p=0��Ч
		ULONG gdtCount = sgdtInfo.uXdtLimit / 8;

		//4.��ȡGDT��Ϣ
		ULONG nCount = 0;
		for (ULONG i = 0; i < gdtCount; i++)
		{
			//�������Ч���򲻱���
			if (pGdtEntry[i].P == 0)
			{
				continue;
			}
			nCount++;
		}

		//5.����õ�gdt�������д�����������
		*(ULONG*)pOutBuff = nCount;

		//---------------------------�Լ�����--------------------------------------		
		pIrp->IoStatus.Status = STATUS_SUCCESS;          // ���״̬
		pIrp->IoStatus.Information = sizeof(nCount);// �ܹ������ֽ���
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);       //���������´���
		return STATUS_SUCCESS;		
	}

	case enumGdt:
	{
		//1.��ȡGDTR�Ĵ�����ֵ,��ŵ�sgdtInfo��     //ָ�sgdt xxxx
		XDT_INFO sgdtInfo = { 0,0,0 };
		_asm sgdt sgdtInfo;

		//2.��ȡGDT�������׵�ַ
		PGDT_ENTER pGdtEntry = NULL;
		pGdtEntry = (PGDT_ENTER)MAKE_LONG(sgdtInfo.xLowXdtBase, sgdtInfo.xHighXdtBase);

		//3.��ȡGDT���������   //������Ч����Ч��gdt� p=0��Ч
		ULONG gdtCount = sgdtInfo.uXdtLimit / 8;

		//4.ǿ��ת������
		RtlFillMemory(pOutBuff, outputSize, 0);
		PGDTINFO pOutGdtInfo = (PGDTINFO)pOutBuff;

		//5.��ȡGDT��Ϣ
		for (ULONG i = 0; i < gdtCount; i++)
		{
			//�������Ч���򲻱���
			if (pGdtEntry[i].P == 0)
			{
				continue;
			}
			//�λ�ַ
			pOutGdtInfo->BaseAddr = MAKE_LONG2(pGdtEntry[i].base0_23, pGdtEntry[i].base24_31);
			//����Ȩ�ȼ�
			pOutGdtInfo->Dpl = (ULONG)pGdtEntry[i].DPL;
			//������
			pOutGdtInfo->GateType = (ULONG)pGdtEntry[i].Type;
			//������
			pOutGdtInfo->Grain = (ULONG)pGdtEntry[i].G;
			//���޳�
			pOutGdtInfo->Limit = MAKE_LONG(pGdtEntry[i].Limit0_15, pGdtEntry[i].Limit16_19);
			pOutGdtInfo++;
		}


		pIrp->IoStatus.Status = STATUS_SUCCESS;          // ���״̬
		pIrp->IoStatus.Information = (ULONG)pOutGdtInfo - (ULONG)pOutBuff;
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);       //���������´���
		return STATUS_SUCCESS;
	}

	case enumIdt:
	{
		//1.����ָ���ȡidtr��ֵ
		XDT_INFO sidtInfo = { 0,0,0 };
		__asm sidt sidtInfo;

		//2.��ȡIDT���ַ
		PIDT_ENTRY pIDTEntry = NULL;
		pIDTEntry = (PIDT_ENTRY)MAKE_LONG(sidtInfo.xLowXdtBase, sidtInfo.xHighXdtBase);

		//3.ǿ��ת������
		RtlFillMemory(pOutBuff, outputSize, 0);
		PIDTINFO pOutIdtInfo = (PIDTINFO)pOutBuff;

		//4.��ȡIDT��Ϣ
		for (ULONG i = 0; i < 0x100; i++)
		{
			//��������ַ
			pOutIdtInfo->pFunction = MAKE_LONG(pIDTEntry[i].uOffsetLow, pIDTEntry[i].uOffsetHigh);

			//��ѡ����
			pOutIdtInfo->Selector = pIDTEntry[i].uSelector;

			//����
			pOutIdtInfo->GateType = pIDTEntry[i].GateType;

			//��Ȩ�ȼ�
			pOutIdtInfo->Dpl = pIDTEntry[i].DPL;

			//��������
			pOutIdtInfo->ParamCount = pIDTEntry[i].paramCount;

			pOutIdtInfo++;
		}

		
		pIrp->IoStatus.Status = STATUS_SUCCESS;          // ���״̬
		pIrp->IoStatus.Information = (ULONG)pOutIdtInfo - (ULONG)pOutBuff;
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);       //���������´���
		return STATUS_SUCCESS;
	}

	case getSsdtCount:
	{
		//��ȡSSDT���з������ĸ���
		ULONG SSDTCount = 0;
		*(ULONG*)pOutBuff = SSDTCount= KeServiceDescriptorTable.NumberOfService;
		
		//---------------------------�Լ�����--------------------------------------		
		pIrp->IoStatus.Status = STATUS_SUCCESS;          // ���״̬
		pIrp->IoStatus.Information = sizeof(SSDTCount);// �ܹ������ֽ���
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);       //���������´���
		return STATUS_SUCCESS;
	}

	case enumSsdt:
	{
		//1.ǿ��ת������
		RtlFillMemory(pOutBuff, outputSize, 0);
		PSSDTINFO pOutSsdtInfo = (PSSDTINFO)pOutBuff;
		//2.ѭ������
		for (ULONG i = 0; i < KeServiceDescriptorTable.NumberOfService; i++)
		{
			//������ַ
			pOutSsdtInfo->uFuntionAddr = (ULONG)KeServiceDescriptorTable.ServiceTableBase[i];
			//���ú�
			pOutSsdtInfo->uIndex = i;
			//ָ�����
			pOutSsdtInfo++;
		}
		
		pIrp->IoStatus.Status = STATUS_SUCCESS;          // ���״̬
		pIrp->IoStatus.Information = (ULONG)pOutSsdtInfo - (ULONG)pOutBuff;
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);       //���������´���
		return STATUS_SUCCESS;
	}

	case selfPid:
	{
		//�����뻺����ȡ�����̵�PID����ȫ�ֱ�����
		g_uPid = *(ULONG*)pInputBuff;
		break;
	}

	case hideThread:
	{
		//ȡ�����뻺�����Ľ���PID
		HANDLE processId = (HANDLE)(*(ULONG*)pInputBuff);

		//PIDת��ΪEProcess
		PEPROCESS pEProcess = NULL;
		NTSTATUS status = PsLookupProcessByProcessId(processId, &pEProcess);
		if (!NT_SUCCESS(status))
		{
			DbgPrint("û���ҵ�����,���겻�������˰�\n");
			return STATUS_SUCCESS;
		}
		//�ݼ����ü���
		ObDereferenceObject(pEProcess);


		//RemoveEntryList((PLIST_ENTRY)(*(PULONG)((PUCHAR)pEProcess + 0x2C)));	//KTHREAD -> ThreadListEntry
		//RemoveEntryList((PLIST_ENTRY)(*(PULONG)((PUCHAR)pEProcess + 0x188)));   //ETHREAD -> ThreadListEntry
			
		PLIST_ENTRY pList1 = (PLIST_ENTRY)(*(PULONG)((PUCHAR)pEProcess + 0x2c));
		PLIST_ENTRY pList2 = (PLIST_ENTRY)(*(PULONG)((PUCHAR)pEProcess + 0x188));
		
		//�ı������ǰ�����ӵ�
		pList1->Blink->Flink = pList1->Flink;
		pList1->Flink->Blink = pList1->Blink;

		//�ı������ǰ�����ӵ�
		pList1->Flink = (PLIST_ENTRY)&pList1->Flink;
		pList1->Blink = (PLIST_ENTRY)&pList1->Blink;

		//�ı������ǰ�����ӵ�
		pList2->Blink->Flink = pList2->Flink;
		pList2->Flink->Blink = pList2->Blink;

		//�ı������ǰ�����ӵ�
		pList2->Flink = (PLIST_ENTRY)&pList2->Flink;
		pList2->Blink = (PLIST_ENTRY)&pList2->Blink;
		

		pIrp->IoStatus.Status = STATUS_SUCCESS;          // ���״̬
		pIrp->IoStatus.Information = 0;// �ܹ������ֽ���
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);       //���������´���
		return STATUS_SUCCESS;
	}

	case hideModule:
	{
		//1.�����뻺�����õ�Ŀ��ģ���DLLBase������PID
		ULONG ModuleBase[2] = { 0 };
		RtlCopyMemory(ModuleBase, pInputBuff, 2 * sizeof(ULONG));				
		ULONG DLLBase = ModuleBase[0];
		HANDLE processid = (HANDLE)ModuleBase[1];
		DbgPrint("DLLBase=%p\n", DLLBase);
		DbgPrint("processid=%d\n", processid);

		//2.����PID��Ӧ�Ľ���EPROCESS
		PEPROCESS pEProcess = NULL;		
		NTSTATUS status = PsLookupProcessByProcessId(processid, &pEProcess);
		if (!NT_SUCCESS(status))
		{
			DbgPrint("û���ҵ�����,���겻�������˰�\n");
			return STATUS_SUCCESS;
		}
		//�ݼ����ü���
		ObDereferenceObject(pEProcess);

		//3.�ҵ�PEB	
		KAPC_STATE pAPC = { 0 };
		KeStackAttachProcess(pEProcess, &pAPC);
		PPEB peb = PsGetProcessPeb(pEProcess);
		if (peb == NULL)
		{
			DbgPrint("û���ҵ�peb\n");
			return STATUS_SUCCESS;
		}

		//4.ͨ��PEB����LDR�е�LIST_ENTRY
		PLIST_ENTRY pList_First = (PLIST_ENTRY)(*(ULONG*)((ULONG)peb + 0xc) + 0xc);
		PLIST_ENTRY pList_Temp = pList_First;

		//5.����
		do
		{
			//ǿ��ת�����ͣ���ΪLISTENTRY�Ļ�ַ�պ���PLDR_DATA_TABLE_ENTRY�Ļ�ַ�غ�
			PLDR_DATA_TABLE_ENTRY pModuleInfo = (PLDR_DATA_TABLE_ENTRY)pList_Temp;
			
			if (pModuleInfo->DllBase == (PVOID)DLLBase)
			{
				// ��������ͬʱ���ϵ�
				pModuleInfo->InLoadOrderLinks.Blink->Flink =pModuleInfo->InLoadOrderLinks.Flink;
				pModuleInfo->InLoadOrderLinks.Flink->Blink =pModuleInfo->InLoadOrderLinks.Blink;
				//
				pModuleInfo->InInitializationOrderLinks.Blink->Flink =pModuleInfo->InInitializationOrderLinks.Flink;
				pModuleInfo->InInitializationOrderLinks.Flink->Blink =pModuleInfo->InInitializationOrderLinks.Blink;
				//
				pModuleInfo->InMemoryOrderLinks.Blink->Flink =pModuleInfo->InMemoryOrderLinks.Flink;
				pModuleInfo->InMemoryOrderLinks.Flink->Blink =pModuleInfo->InMemoryOrderLinks.Blink;
				
				break;
			}
			pList_Temp = pList_Temp->Blink;

		} while (pList_Temp != pList_First);

		KeUnstackDetachProcess(&pAPC);

		pIrp->IoStatus.Status = STATUS_SUCCESS;          // ���״̬
		pIrp->IoStatus.Information = 0;// �ܹ������ֽ���
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);       //���������´���
		return STATUS_SUCCESS;
	}

	case DriveInject:
	{
		//1.�����뻺����ȡ��PID��DLL·��
		INJECT_INFO InjectInfo = { 0 };
		RtlCopyMemory(&InjectInfo, pInputBuff, sizeof(INJECT_INFO));
			
		NTSTATUS status;
		if (!&InjectInfo)
		{
			DbgPrint("InjectInfo�ǿյ�\n");
			return STATUS_SUCCESS;
		}

		if (!InjectDll(&InjectInfo))
		{
			DbgPrint("����ע��ʧ��\n");
			return STATUS_SUCCESS;
		}

		pIrp->IoStatus.Status = STATUS_SUCCESS;          // ���״̬
		pIrp->IoStatus.Information = 0;// �ܹ������ֽ���
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);       //���������´���
		return STATUS_SUCCESS;
	}

	case ProtectProcess:
	{
		//1.�����뻺�����õ����̵�PID
		HANDLE processid = (HANDLE)(*(ULONG*)pInputBuff);

		//DbgBreakPoint();

		//2.��������ָ��PID�Ľ���
		ProtectTheProcess(processid);

		pIrp->IoStatus.Status = STATUS_SUCCESS;          // ���״̬
		pIrp->IoStatus.Information = 0;// �ܹ������ֽ���
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);       //���������´���
		return STATUS_SUCCESS;
	}

	case getVadCount:
	{
		//1.�����뻺�����õ����̵�PID
		HANDLE processid = (HANDLE)(*(ULONG*)pInputBuff);

		//2.���VAD����Ŀ
		ULONG VADCount = 0;
		VADCount = GetVADCount(processid);
		
		//3.���߳�����д�����������
		*(ULONG*)pOutBuff = VADCount;

		//------------------------------------------��Ϊ���ص��ֽ��б仯�������Լ�����ֱ��return---------
		//1.����IRP���״̬
		pIrp->IoStatus.Status = STATUS_SUCCESS;    //״̬Ϊ�ɹ�
		//2.����IRP�����˶��ٸ��ֽ�
		pIrp->IoStatus.Information = sizeof(VADCount);
		//3.����IRP
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
		return STATUS_SUCCESS;
	}

	case enumProcessVad:
	{
		//1.�����뻺�����õ����̵�PID
		HANDLE processid = (HANDLE)(*(ULONG*)pInputBuff);

		//2.�����������ȫ��0,��ǿ��ת������
		RtlFillMemory(pOutBuff, outputSize, 0);
		PVADINFO pOutVADInfo = (PVADINFO)pOutBuff;

		//3.����VAD
		_EnumProcessVAD(processid, pOutVADInfo);
				
		//------------------------------------------��Ϊ���ص��ֽ��б仯�������Լ�����ֱ��return---------
		//1.����IRP���״̬
		pIrp->IoStatus.Status = STATUS_SUCCESS;    //״̬Ϊ�ɹ�
		//2.����IRP�����˶��ٸ��ֽ�
		pIrp->IoStatus.Information = (ULONG)pOutVADInfo - (ULONG)pOutBuff;   //�����ֽ�����ô������ѵ
		//3.����IRP
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
		return STATUS_SUCCESS;
	}

	case enumNotify:
	{
		//3.�����������ȫ��0��ǿ��ת������
		RtlFillMemory(pOutBuff, outputSize, 0);
		PNOTIFYINFO pOutNotifyInfo = (PNOTIFYINFO)pOutBuff;
		EnumCreateProcessNotify(driver1, &pOutNotifyInfo);	 //û���ö���ָ�룬����1�죬�ŷ�������	
		EnumCreateThreadNotify(driver1, &pOutNotifyInfo);
		EnumImageNotify(driver1, &pOutNotifyInfo);
		EnumCallBackList(driver1, &pOutNotifyInfo);

		pIrp->IoStatus.Status = STATUS_SUCCESS;          // ���״̬
		pIrp->IoStatus.Information = (ULONG)pOutNotifyInfo - (ULONG)pOutBuff;
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);       //���������´���
		return STATUS_SUCCESS;
	}

	case enumObcallback:
	{
		//1.�����������ȫ��0��ǿ��ת������
		RtlFillMemory(pOutBuff, outputSize, 0);
		POBCALLBACKINFO pOutobcallback = (POBCALLBACKINFO)pOutBuff;

		EnumProcessObCallback(driver1, &pOutobcallback);
		EnumThreadObCallback(driver1, &pOutobcallback);

		pIrp->IoStatus.Status = STATUS_SUCCESS;          // ���״̬
		pIrp->IoStatus.Information = (ULONG)pOutobcallback - (ULONG)pOutBuff;
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);       //���������´���
		return STATUS_SUCCESS;
	}

	case enumDpctimer:
	{
		//1.�����������ȫ��0��ǿ��ת������
		RtlFillMemory(pOutBuff, outputSize, 0);
		PDPCTIMERINFO pOutDpcInfo = (PDPCTIMERINFO)pOutBuff;

		EnumDpcTimer(driver1, &pOutDpcInfo);

		pIrp->IoStatus.Status = STATUS_SUCCESS;          // ���״̬
		pIrp->IoStatus.Information = (ULONG)pOutDpcInfo - (ULONG)pOutBuff;
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);       //���������´���
		return STATUS_SUCCESS;
	}

	case enumIotimer:
	{
		//1.�����������ȫ��0��ǿ��ת������
		RtlFillMemory(pOutBuff, outputSize, 0);
		PIOTIMERINFO pOutIOTIMERInfo = (PIOTIMERINFO)pOutBuff;

		EnumIoTimer(driver1, &pOutIOTIMERInfo);

		pIrp->IoStatus.Status = STATUS_SUCCESS;          // ���״̬
		pIrp->IoStatus.Information = (ULONG)pOutIOTIMERInfo - (ULONG)pOutBuff;
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);       //���������´���
		return STATUS_SUCCESS;
	}

	default:
		break;
	}

	//-----------------------------------------------------I/Oͨ�ŵ�����������״̬�������ֽ������Ƿ�������´���----------
	//1.����IRP���״̬
	pIrp->IoStatus.Status = STATUS_SUCCESS;    //״̬Ϊ�ɹ�
	//2.����IRP�����˶��ٸ��ֽ�
	pIrp->IoStatus.Information = outputSize ? outputSize : inputSize;   //�������outputSize����ô��ѡoutputSize������ѡinputSize
	//3.����IRP
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);   //���������´���
	//-----------------------------------------------------------------------------------------------------------------------

	return STATUS_SUCCESS;
}
//------------------------------------------------------------------------------------------------------------------//



//-------------------------------��������-------------------------------//
//1.ж������
VOID OnUnload(DRIVER_OBJECT* driver)
{
	if (pThreadObj1!=NULL)
	{
		bTerminated1 = TRUE;
		KeWaitForSingleObject(pThreadObj1, Executive, KernelMode, FALSE, NULL);
		ObDereferenceObject(pThreadObj1);
		DbgPrint("�߳�1���ر�\n");
	}

	if (pThreadObj2 != NULL)
	{
		bTerminated2 = TRUE;
		KeWaitForSingleObject(pThreadObj2, Executive, KernelMode, FALSE, NULL);
		ObDereferenceObject(pThreadObj2);
		DbgPrint("�߳�2���ر�\n");
	}


	//1.ж�� SYSENTER-HOOK
	UnstallSysenterHook();

	//2.ɾ����������
	UNICODE_STRING symbolName = RTL_CONSTANT_STRING(NAME_SYMBOL);
	IoDeleteSymbolicLink(&symbolName);

	//3.�����豸����
	IoDeleteDevice(driver->DeviceObject);

	//4.ж��KiFastCallEntryHOOK
	unstallKiFastCallEntryHook();

	DbgPrint("������ж��\n");
}
//2.������ں���
NTSTATUS DriverEntry(PDRIVER_OBJECT driver, PUNICODE_STRING path)
{
	//�����������δʹ�õĴ���
	UNREFERENCED_PARAMETER(path);
	NTSTATUS status = STATUS_SUCCESS;	
	driver1 = driver;
	
	//1.�����ں�
	reloadNT(driver);

	//2.��װKiFastCallEntryHook
	installKiFastCallEntryHook();
	
	//3.��ж�غ���
	driver->DriverUnload = OnUnload;

	//4.�����豸	
	UNICODE_STRING devName = RTL_CONSTANT_STRING(NAME_DEVICE);
	PDEVICE_OBJECT pDevice = NULL;
	status = IoCreateDevice(
		driver,			//��������(�´������豸����������������)
		0,				//�豸��չ��С
		&devName,		//�豸����
		FILE_DEVICE_UNKNOWN,	//�豸����(δ֪����)
		0,				//�豸������Ϣ
		FALSE,			//�豸�Ƿ�Ϊ��ռ��
		&pDevice		//������ɵ��豸����ָ��
	);
	if (!NT_SUCCESS(status))
	{
		DbgPrint("�����豸ʧ�ܣ������룺0x%08X\n", status);
		return status;
	}

	//5.��д��ʽΪֱ�Ӷ�д��ʽ
	pDevice->Flags = DO_DIRECT_IO;

	//6.�󶨷�������
	UNICODE_STRING symbolName = RTL_CONSTANT_STRING(NAME_SYMBOL);
	IoCreateSymbolicLink(&symbolName, &devName);

	//7.����ǲ����(������������)
	driver->MajorFunction[IRP_MJ_CREATE] = OnCreate;
	driver->MajorFunction[IRP_MJ_CLOSE] = OnClose;
	driver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = OnDeviceIoControl;

	//8.��װSYSENTER-HOOK
	InstallSysenterHook();

	return status;
}

