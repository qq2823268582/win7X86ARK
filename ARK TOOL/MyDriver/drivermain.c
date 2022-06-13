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

ULONG  g_uPid;	                 //要保护的本ARK进程的PID
PDRIVER_OBJECT driver1=NULL;
//--------------------------------------------------------------------------------------------------------------//

extern PETHREAD pThreadObj1;
extern BOOLEAN bTerminated1;

extern PETHREAD pThreadObj2;
extern BOOLEAN bTerminated2;

//------------------------------------------未导出函数的声明-----------------------------------------------//

NTSTATUS PsReferenceProcessFilePointer(IN  PEPROCESS Process, OUT PVOID *OutFileObject);
NTKERNELAPI HANDLE PsGetProcessInheritedFromUniqueProcessId(IN PEPROCESS Process);//未公开进行导出
NTKERNELAPI struct _PEB* PsGetProcessPeb(PEPROCESS proc);
typedef VOID(NTAPI* fpTypePspExitThread)(IN NTSTATUS ExitStatus);
fpTypePspExitThread g_fpPspExitThreadAddr = NULL;
//----------------------------------------------------------------------------------------------------------------//


//--------------------------------------------通信设备-------------------------------------//
//1.设备名
#define NAME_DEVICE L"\\Device\\ArkToMe"
//2.链接符号名
#define  NAME_SYMBOL L"\\??\\ArkToMe666"
//3.控制码宏
#define MYCTLCODE(code) CTL_CODE(FILE_DEVICE_UNKNOWN,0x800+(code),METHOD_OUT_DIRECT,FILE_ANY_ACCESS)
//4.控制码枚举
enum MyCtlCode
{
	getDriverCount = MYCTLCODE(1),		//获取驱动数量
	enumDriver = MYCTLCODE(2),		//遍历驱动
	hideDriver = MYCTLCODE(3),		//隐藏驱动
	getProcessCount = MYCTLCODE(4),	//获取进程数量
	enumProcess = MYCTLCODE(5),		//遍历进程
	hideProcess = MYCTLCODE(6),		//隐藏进程
	killProcess = MYCTLCODE(7),		//结束进程
	getModuleCount = MYCTLCODE(8),	//获取指定进程的模块数量
	enumModule = MYCTLCODE(9),		//遍历指定进程的模块
	getThreadCount = MYCTLCODE(10),	//获取指定进程的线程数量
	enumThread = MYCTLCODE(11),		//遍历指定进程的线程
	getFileCount = MYCTLCODE(12),	//获取文件数量
	enumFile = MYCTLCODE(13),		//遍历文件
	deleteFile = MYCTLCODE(14),		//删除文件
	getRegKeyCount = MYCTLCODE(15),	//获取注册表子项数量
	enumReg = MYCTLCODE(16),		//遍历注册表
	//newKey = MYCTLCODE(17),			//创建新键
	deleteKey = MYCTLCODE(18),		//删除新键
	enumIdt = MYCTLCODE(19),		//遍历IDT表
	getGdtCount = MYCTLCODE(20),	//获取GDT表中段描述符的数量
	enumGdt = MYCTLCODE(21),		//遍历GDT表
	getSsdtCount = MYCTLCODE(22),	//获取SSDT表中系统服务描述符的数量
	enumSsdt = MYCTLCODE(23),		//遍历SSDT表
	selfPid = MYCTLCODE(24),		//将自身的进程ID传到0环
	hideThread = MYCTLCODE(25),       //隐藏线程
	hideModule = MYCTLCODE(26),       //隐藏模块
	DriveInject = MYCTLCODE(27),      //驱动注入
	ProtectProcess = MYCTLCODE(28),   //进程保护
	getVadCount = MYCTLCODE(29),      //获得VAD的数量
	enumProcessVad = MYCTLCODE(30),   //进程VAD遍历
	enumNotify = MYCTLCODE(17),       //遍历系统回调
	enumObcallback = MYCTLCODE(31),  //遍历对象回调
	enumDpctimer = MYCTLCODE(32),    //遍历DPC定时器
	enumIotimer = MYCTLCODE(33),     //遍历IO定时器
};
//5.打开设备
NTSTATUS OnCreate(DEVICE_OBJECT *pDevice, IRP *pIrp)
{
	//避免编译器报未使用的错误
	UNREFERENCED_PARAMETER(pDevice);

	//设置IRP完成状态
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	//设置IRP操作了多少个字节
	pIrp->IoStatus.Information = 0;
	//处理IRP
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}
//6.关闭设备
NTSTATUS OnClose(DEVICE_OBJECT* pDevice, IRP* pIrp)
{
	//避免编译器报未使用的错误
	UNREFERENCED_PARAMETER(pDevice);
	KdPrint(("设备被关闭\n"));
	//1.设置IRP完成状态
	pIrp->IoStatus.Status = STATUS_SUCCESS;

	//2.设置IRP操作了多少个字节
	pIrp->IoStatus.Information = 0;

	//3.处理IRP （不继续向下传递）
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}
//7.设备I/O
NTSTATUS OnDeviceIoControl(DEVICE_OBJECT* pDevice, IRP* pIrp)
{
	//--------------------------------------I/O通信第一步：获取各种必备信息-------------------------------------
	//1.获取当前IRP栈
	PIO_STACK_LOCATION pIoStack = IoGetCurrentIrpStackLocation(pIrp);

	//2.获取控制码
	ULONG uCtrlCode = pIoStack->Parameters.DeviceIoControl.IoControlCode;

	//3.获取I/O缓存
	PVOID pInputBuff = NULL;
	PVOID pOutBuff = NULL;
	//3.1获取输入缓冲区(如果存在)
	if (pIrp->AssociatedIrp.SystemBuffer != NULL)
	{
		pInputBuff = pIrp->AssociatedIrp.SystemBuffer;
	}
	//3.2获取输出缓冲区(如果存在)
	if (pIrp->MdlAddress != NULL)
	{
		//获取MDL缓冲区在内核中的映射
		pOutBuff = MmGetSystemAddressForMdlSafe(pIrp->MdlAddress, NormalPagePriority);
	}

	//4.获取输入I/O缓存的大小
	ULONG inputSize = pIoStack->Parameters.DeviceIoControl.InputBufferLength;

	//5.获取输出I/O缓存的大小
	ULONG outputSize = pIoStack->Parameters.DeviceIoControl.OutputBufferLength;


	//-----------------------------------------I/O通信第二步：分发控制码，进行不同的行为操作------------------------
	//根据相应的控制码进行相应操作
	switch (uCtrlCode)
	{
	case getProcessCount:
	{
		PEPROCESS proc = NULL;
		//计数用的
		ULONG ProcessCount = 0;
		// 设定PID范围,循环遍历
		for (int i = 4; i < 100000; i += 4)
		{
			// 若通过PID能找到EPROCESS，存放到proc中
			if (NT_SUCCESS(PsLookupProcessByProcessId((HANDLE)i, &proc)))
			{
				// 进一步判断进程是否有效
				ULONG TableCode = *(ULONG*)((ULONG)proc + 0xF4);
				if (TableCode)
				{
					ProcessCount++;// 个数+1 
				}
				ObDereferenceObject(proc);// 递减引用计数
			}
		}

		//6.遍历结束将数量写入输出缓冲区
		*(ULONG*)pOutBuff = ProcessCount;

		//------------------------------------------因为返回的字节有变化，所以自己处理，直接return---------
		 //1.设置IRP完成状态
		pIrp->IoStatus.Status = STATUS_SUCCESS;    //状态为成功
		//2.设置IRP操作了多少个字节
		pIrp->IoStatus.Information = sizeof(ProcessCount);
		//3.处理IRP
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
		return STATUS_SUCCESS;
	}

	case enumProcess:
	{
		//这句话有2个作用：可以备份一个pOutBuff作为基址，还可以强制转换pOutBuff为PPROCESSINFO类型
		//将输出缓冲区全置0
		RtlFillMemory(pOutBuff, outputSize, 0);
		PPROCESSINFO pOutProcessInfo = (PPROCESSINFO)pOutBuff;

		NTSTATUS status = STATUS_SUCCESS;
		PEPROCESS pEProcess = NULL;

		// 开始遍历
		for (ULONG i = 4; i < 0x10000; i = i + 4)
		{
			status = PsLookupProcessByProcessId((HANDLE)i, &pEProcess);
			if (NT_SUCCESS(status))  //一定要判断status是否成功，否则会蓝屏
			{
				// 进一步判断进程是否有效
				ULONG TableCode = *(ULONG*)((ULONG)pEProcess + 0xF4);
				if (TableCode)
				{
					// 从进程结构中获取进程信息
					HANDLE hParentProcessId = NULL;
					HANDLE hProcessId = NULL;
					PCHAR pszProcessName = NULL;
					PVOID pFilePoint = NULL;
					POBJECT_NAME_INFORMATION pObjectNameInfo = NULL;

					// 获取进程程序路径
					status = PsReferenceProcessFilePointer(pEProcess, &pFilePoint);
					if (NT_SUCCESS(status))
					{
						status = IoQueryFileDosDeviceName(pFilePoint, &pObjectNameInfo);
						if (NT_SUCCESS(status))
						{
							//DbgPrint("----------------%d------------------------", i);
							//DbgPrint("PEPROCESS=0x%p\n", pEProcess);

							//获取进程路径
						   // DbgPrint("PATH=%ws\n", pObjectNameInfo->Name.Buffer);
							RtlCopyMemory(pOutProcessInfo->szpath, pObjectNameInfo->Name.Buffer, pObjectNameInfo->Name.Length);

							//获取父进程 PID
							//hParentProcessId = PsGetProcessInheritedFromUniqueProcessId(pEProcess);
							//DbgPrint("PPID=%d\n", hParentProcessId);
							pOutProcessInfo->PPID = (ULONG)PsGetProcessInheritedFromUniqueProcessId(pEProcess);

							// 获取进程 PID
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

		//------------------------------------------因为返回的字节有变化，所以自己处理，直接return---------
		//1.设置IRP完成状态
		pIrp->IoStatus.Status = STATUS_SUCCESS;    //状态为成功
		//2.设置IRP操作了多少个字节
		pIrp->IoStatus.Information = (ULONG)pOutProcessInfo - (ULONG)pOutBuff;   //返回字节数怎么处理，教训
		//3.处理IRP
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
		return STATUS_SUCCESS;
	}

	case killProcess:
	{
		//1.搜索获得PspTerminateThreadByPointer的函数地址
		ULONG uPspTerminateThreadByPointerAddr = GetPspTerminateThreadByPointer();
		if (0 == uPspTerminateThreadByPointerAddr)
		{
			DbgPrint("查找PspTerminateThreadByPointerAddr地址出错\n");
			return STATUS_SUCCESS;
		}

		//2.搜索获得PspExitThread的函数地址
		g_fpPspExitThreadAddr = (fpTypePspExitThread)GetPspExitThread(uPspTerminateThreadByPointerAddr);
		if (NULL == g_fpPspExitThreadAddr)
		{
			DbgPrint("查找PspExitThread地址出错\n");
			return STATUS_SUCCESS;
		}

		//3.从输入缓冲区拿到进程的PID
		HANDLE processid = (HANDLE)(*(ULONG*)pInputBuff);

		//4.暴力搜索全部线程，找到指定EPROCESS所属的全部线程并关闭
		KillProcess(processid);

		break;
	}

	case hideProcess:
	{
		//1.从输入缓冲区拿到进程的PID
		HANDLE processid = (HANDLE)(*(ULONG*)pInputBuff);

		//2.断链隐藏指定PID的进程
		HideProcess(processid);

		break;
	}

	case getModuleCount:
	{
		//1.从输入缓冲区拿到进程的PID
		HANDLE processid = (HANDLE)(*(ULONG*)pInputBuff);

		//2.查找PID对应的进程EPROCESS
		PEPROCESS pEProcess = NULL;
		NTSTATUS status = PsLookupProcessByProcessId(processid, &pEProcess);
		if (!NT_SUCCESS(status))
		{
			DbgPrint("没有找到进程,尼玛不会隐藏了吧\n");
			return STATUS_SUCCESS;
		}
		//递减引用计数
		ObDereferenceObject(pEProcess);

		//3.找到PEB(由于PEB在用户层空间,因此需要进程挂靠）
		KAPC_STATE pAPC = { 0 };
		KeStackAttachProcess(pEProcess, &pAPC);
		//函数PsGetProcessPeb未导出，需要声明
		PPEB peb = PsGetProcessPeb(pEProcess);
		if (peb == NULL)
		{
			DbgPrint("没有找到peb\n");
			return STATUS_SUCCESS;
		}

		//4.通过PEB遍历LDR中的LIST_ENTRY
		PLIST_ENTRY pList_First = (PLIST_ENTRY)(*(ULONG*)((ULONG)peb + 0xc) + 0xc);
		PLIST_ENTRY pList_Temp = pList_First;
		ULONG ModuleCount = 0;
		do
		{
			pList_Temp = pList_Temp->Blink;
			ModuleCount++;
		} while (pList_Temp != pList_First);

		//5.将模块数量写入输出缓冲区
		*(ULONG*)pOutBuff = ModuleCount;

		//6.进程解除挂靠
		KeUnstackDetachProcess(&pAPC);

		//------------------------------------------因为返回的字节有变化，所以自己处理，直接return---------
	   //1.设置IRP完成状态
		pIrp->IoStatus.Status = STATUS_SUCCESS;    //状态为成功
		//2.设置IRP操作了多少个字节
		pIrp->IoStatus.Information = sizeof(ModuleCount);
		//3.处理IRP
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
		return STATUS_SUCCESS;
	}

	case enumModule:
	{
		//1.从输入缓冲区拿到进程的PID
		HANDLE processid = (HANDLE)(*(ULONG*)pInputBuff);

		//2.查找PID对应的进程EPROCESS
		PEPROCESS pEProcess = NULL;
		NTSTATUS status = PsLookupProcessByProcessId(processid, &pEProcess);
		if (!NT_SUCCESS(status))
		{
			DbgPrint("没有找到进程,尼玛不会隐藏了吧\n");
			return STATUS_SUCCESS;
		}
		//递减引用计数
		ObDereferenceObject(pEProcess);

		//3.找到PEB(由于PEB在用户层空间,因此需要进程挂靠）
		KAPC_STATE pAPC = { 0 };
		KeStackAttachProcess(pEProcess, &pAPC);
		//函数PsGetProcessPeb未导出，需要声明
		PPEB peb = PsGetProcessPeb(pEProcess);
		if (peb == NULL)
		{
			DbgPrint("没有找到peb\n");
			return STATUS_SUCCESS;
		}

		//4.通过PEB遍历LDR中的LIST_ENTRY
		PLIST_ENTRY pList_First = (PLIST_ENTRY)(*(ULONG*)((ULONG)peb + 0xc) + 0xc);
		PLIST_ENTRY pList_Temp = pList_First;

		//5.遍历LISTENTRY偏移获得模块大小、基址、偏移
		RtlFillMemory(pOutBuff, outputSize, 0);
		PMODULEINFO pOutModuleInfo = (PMODULEINFO)pOutBuff;
		do
		{
			//强制转换类型，因为LISTENTRY的基址刚好与PLDR_DATA_TABLE_ENTRY的基址重合
			PLDR_DATA_TABLE_ENTRY pModuleInfo = (PLDR_DATA_TABLE_ENTRY)pList_Temp;
			pList_Temp = pList_Temp->Blink;

			if (pModuleInfo->FullDllName.Buffer != 0)
			{
				//拷贝模块路径
				RtlCopyMemory(pOutModuleInfo->wcModuleFullPath, pModuleInfo->FullDllName.Buffer, pModuleInfo->FullDllName.Length);
				//拷贝模块基址
				pOutModuleInfo->DllBase = (ULONG)pModuleInfo->DllBase;
				//拷贝模块大小
				pOutModuleInfo->SizeOfImage = pModuleInfo->SizeOfImage;
			}
			pOutModuleInfo++;

		} while (pList_Temp != pList_First);

		//6.解除进程挂靠
		KeUnstackDetachProcess(&pAPC);

		//------------------------------------------因为返回的字节有变化，所以自己处理，直接return---------
	   //1.设置IRP完成状态
		pIrp->IoStatus.Status = STATUS_SUCCESS;    //状态为成功
		//2.设置IRP操作了多少个字节
		pIrp->IoStatus.Information = (ULONG)pOutModuleInfo - (ULONG)pOutBuff;   //返回字节数怎么处理，教训
		//3.处理IRP
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
		return STATUS_SUCCESS;
	}

	case getThreadCount:
	{
		//1.从输入缓冲区拿到进程的PID
		HANDLE processid = (HANDLE)(*(ULONG*)pInputBuff);

		//2.查找PID对应的进程EPROCESS
		PEPROCESS pEProcess = NULL;
		NTSTATUS status = PsLookupProcessByProcessId(processid, &pEProcess);
		if (!NT_SUCCESS(status))
		{
			DbgPrint("没有找到进程,尼玛不会隐藏了吧\n");
			return STATUS_SUCCESS;
		}
		//递减引用计数
		ObDereferenceObject(pEProcess);

		//3.获取线程链表
		PLIST_ENTRY pList_First = (PLIST_ENTRY)((ULONG)pEProcess + 0x188);
		PLIST_ENTRY pList_Temp = pList_First;
		ULONG ThreadCount = 0;
		do
		{
			pList_Temp = pList_Temp->Blink;
			ThreadCount++;
		} while (pList_Temp != pList_First);

		//4.将线程数量写入输出缓冲区
		*(ULONG*)pOutBuff = ThreadCount;

		//------------------------------------------因为返回的字节有变化，所以自己处理，直接return---------
		//1.设置IRP完成状态
		pIrp->IoStatus.Status = STATUS_SUCCESS;    //状态为成功
		//2.设置IRP操作了多少个字节
		pIrp->IoStatus.Information = sizeof(ThreadCount);
		//3.处理IRP
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
		return STATUS_SUCCESS;
	}

	case enumThread:
	{
		//1.从输入缓冲区拿到进程的PID
		HANDLE processid = (HANDLE)(*(ULONG*)pInputBuff);

		//2.查找PID对应的进程EPROCESS
		PEPROCESS pEProcess = NULL;
		NTSTATUS status = PsLookupProcessByProcessId(processid, &pEProcess);
		if (!NT_SUCCESS(status))
		{
			DbgPrint("没有找到进程,尼玛不会隐藏了吧\n");
			return STATUS_SUCCESS;
		}
		//递减引用计数
		ObDereferenceObject(pEProcess);

		//3.获取线程链表
		PLIST_ENTRY pList_First = (PLIST_ENTRY)((ULONG)pEProcess + 0x188);
		PLIST_ENTRY pList_Temp = pList_First;

		//4.将输出缓冲区全置0,并强制转换类型
		RtlFillMemory(pOutBuff, outputSize, 0);
		PTHREADINFO pOutThreadInfo = (PTHREADINFO)pOutBuff;
		do
		{
			//1.获取LISTENTRY所属的ETHREAD基址
			PETHREAD pThreadInfo = (PETHREAD)((ULONG)pList_Temp - 0x268);

			//2.获取线程ID
			pOutThreadInfo->Tid = *(PULONG)((ULONG)pThreadInfo + 0x230);

			//3.获取线程执行块地址
			pOutThreadInfo->pEthread = (ULONG)pThreadInfo;

			//4.获取Teb结构地址
			pOutThreadInfo->pTeb = *(PULONG)((ULONG)pThreadInfo + 0x88);

			//5.获取静态优先级
			pOutThreadInfo->BasePriority = *(CHAR*)((ULONG)pThreadInfo + 0x135);

			//6.获取切换次数
			pOutThreadInfo->ContextSwitches = *(PULONG)((ULONG)pThreadInfo + 0x64);

			//7.链表节点向后移动
			pList_Temp = pList_Temp->Blink;

			//8.输出缓冲区指针后移
			pOutThreadInfo++;
		} while (pList_Temp != pList_First);

		//------------------------------------------因为返回的字节有变化，所以自己处理，直接return---------
		//1.设置IRP完成状态
		pIrp->IoStatus.Status = STATUS_SUCCESS;    //状态为成功
		//2.设置IRP操作了多少个字节
		pIrp->IoStatus.Information = (ULONG)pOutThreadInfo - (ULONG)pOutBuff;   //返回字节数怎么处理，教训
		//3.处理IRP
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
		return STATUS_SUCCESS;
	}

	case getDriverCount:
	{
		//1.获取驱动链
		PLDR_DATA_TABLE_ENTRY pLdr = (PLDR_DATA_TABLE_ENTRY)pDevice->DriverObject->DriverSection;
		//2.获取第一个链表节点
		PLIST_ENTRY pList_First = &pLdr->InLoadOrderLinks;
		PLIST_ENTRY pList_Temp = pList_First;
		//3.遍历链表
		ULONG driverCount = 0;
		do
		{
			pList_Temp = pList_Temp->Blink;
			driverCount++;
		} while (pList_Temp != pList_First);
		//4.数据传输-写入3环
		*(ULONG*)pOutBuff = driverCount;

		//---------------------------自己处理--------------------------------------		
		pIrp->IoStatus.Status = STATUS_SUCCESS;          // 完成状态
		pIrp->IoStatus.Information = sizeof(driverCount);// 总共传输字节数
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);       //不继续往下传递
		return STATUS_SUCCESS;
	}

	case enumDriver:
	{
		//1.获取驱动链
		PLDR_DATA_TABLE_ENTRY pLdr = (PLDR_DATA_TABLE_ENTRY)pDevice->DriverObject->DriverSection;

		//2.获取第一个链表节点
		PLIST_ENTRY pList_First = &pLdr->InLoadOrderLinks;
		PLIST_ENTRY pList_Temp = pList_First;

		//3.将输出缓冲区全置0，强制转换类型
		RtlFillMemory(pOutBuff, outputSize, 0);
		PDRIVERINFO pOutDriverInfo = (PDRIVERINFO)pOutBuff;

		//4.循环遍历驱动链表
		do
		{
			PLDR_DATA_TABLE_ENTRY pDriverInfo = (PLDR_DATA_TABLE_ENTRY)pList_Temp;			
			if (pDriverInfo->DllBase != 0)
			{
				//获取驱动名
				RtlCopyMemory(pOutDriverInfo->wcDriverBasePath, pDriverInfo->BaseDllName.Buffer, pDriverInfo->BaseDllName.Length);
				//获取驱动路径
				RtlCopyMemory(pOutDriverInfo->wcDriverFullPath, pDriverInfo->FullDllName.Buffer, pDriverInfo->FullDllName.Length);
				//获取驱动基址
				pOutDriverInfo->DllBase = (ULONG)pDriverInfo->DllBase;
				//获取驱动大小
				pOutDriverInfo->SizeOfImage = pDriverInfo->SizeOfImage;
			}

			//链表后移
			pList_Temp = pList_Temp->Blink;
			//输出缓冲区指针后移
			pOutDriverInfo++;

		} while (pList_Temp != pList_First);


		pIrp->IoStatus.Status = STATUS_SUCCESS;          // 完成状态
		pIrp->IoStatus.Information = (ULONG)pOutDriverInfo - (ULONG)pOutBuff;
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);       //不继续往下传递
		return STATUS_SUCCESS;
	}

	case hideDriver:
	{
		//1.获取驱动链
		PLDR_DATA_TABLE_ENTRY pLdr = (PLDR_DATA_TABLE_ENTRY)pDevice->DriverObject->DriverSection;

		//2.获取第一个链表节点
		PLIST_ENTRY pList_First = &pLdr->InLoadOrderLinks;
		PLIST_ENTRY pList_Temp = pList_First;

		//3.从输入缓冲区取出目标驱动名，并进行格式化（UNICODE_STRING）
		UNICODE_STRING pHideDriverName = { 0 };
		RtlInitUnicodeString(&pHideDriverName, pInputBuff);

		//4.遍历驱动链表，对比名字，找到目标后将目标驱动断链
		do
		{
			//强制转换类型，方便获得成员
			PLDR_DATA_TABLE_ENTRY pDriverInfo = (PLDR_DATA_TABLE_ENTRY)pList_Temp;
			//对比名字
			if (RtlCompareUnicodeString(&pDriverInfo->BaseDllName, &pHideDriverName, FALSE) == 0)
			{
				//改变链表的前后连接点
				pList_Temp->Blink->Flink = pList_Temp->Flink;				
				pList_Temp->Flink->Blink = pList_Temp->Blink;
				
				//改变自身的前后连接点
				pList_Temp->Flink = (PLIST_ENTRY)&pList_Temp->Flink;
				pList_Temp->Blink = (PLIST_ENTRY)&pList_Temp->Blink;

				break;
			}
			//如果没找到，链表后移
			pList_Temp = pList_Temp->Blink;

		} while (pList_Temp != pList_First);
		
		pIrp->IoStatus.Status = STATUS_SUCCESS;          // 完成状态
		pIrp->IoStatus.Information = 0;// 总共传输字节数
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);       //不继续往下传递
		return STATUS_SUCCESS;
	}

	case getGdtCount:
	{
		//1.获取GDTR寄存器的值,存放到sgdtInfo中     //指令：sgdt xxxx
		XDT_INFO sgdtInfo = { 0,0,0 };
		_asm sgdt sgdtInfo;

		//2.获取GDT表数组首地址
		PGDT_ENTER pGdtEntry = NULL;
		pGdtEntry = (PGDT_ENTER)MAKE_LONG(sgdtInfo.xLowXdtBase, sgdtInfo.xHighXdtBase);
				
		//3.获取GDT表数组个数   //包含有效与无效的gdt项， p=0无效
		ULONG gdtCount = sgdtInfo.uXdtLimit / 8;

		//4.获取GDT信息
		ULONG nCount = 0;
		for (ULONG i = 0; i < gdtCount; i++)
		{
			//如果段无效，则不遍历
			if (pGdtEntry[i].P == 0)
			{
				continue;
			}
			nCount++;
		}

		//5.将获得的gdt表的项数写入输出缓冲区
		*(ULONG*)pOutBuff = nCount;

		//---------------------------自己处理--------------------------------------		
		pIrp->IoStatus.Status = STATUS_SUCCESS;          // 完成状态
		pIrp->IoStatus.Information = sizeof(nCount);// 总共传输字节数
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);       //不继续往下传递
		return STATUS_SUCCESS;		
	}

	case enumGdt:
	{
		//1.获取GDTR寄存器的值,存放到sgdtInfo中     //指令：sgdt xxxx
		XDT_INFO sgdtInfo = { 0,0,0 };
		_asm sgdt sgdtInfo;

		//2.获取GDT表数组首地址
		PGDT_ENTER pGdtEntry = NULL;
		pGdtEntry = (PGDT_ENTER)MAKE_LONG(sgdtInfo.xLowXdtBase, sgdtInfo.xHighXdtBase);

		//3.获取GDT表数组个数   //包含有效与无效的gdt项， p=0无效
		ULONG gdtCount = sgdtInfo.uXdtLimit / 8;

		//4.强制转换类型
		RtlFillMemory(pOutBuff, outputSize, 0);
		PGDTINFO pOutGdtInfo = (PGDTINFO)pOutBuff;

		//5.获取GDT信息
		for (ULONG i = 0; i < gdtCount; i++)
		{
			//如果段无效，则不遍历
			if (pGdtEntry[i].P == 0)
			{
				continue;
			}
			//段基址
			pOutGdtInfo->BaseAddr = MAKE_LONG2(pGdtEntry[i].base0_23, pGdtEntry[i].base24_31);
			//段特权等级
			pOutGdtInfo->Dpl = (ULONG)pGdtEntry[i].DPL;
			//段类型
			pOutGdtInfo->GateType = (ULONG)pGdtEntry[i].Type;
			//段粒度
			pOutGdtInfo->Grain = (ULONG)pGdtEntry[i].G;
			//段限长
			pOutGdtInfo->Limit = MAKE_LONG(pGdtEntry[i].Limit0_15, pGdtEntry[i].Limit16_19);
			pOutGdtInfo++;
		}


		pIrp->IoStatus.Status = STATUS_SUCCESS;          // 完成状态
		pIrp->IoStatus.Information = (ULONG)pOutGdtInfo - (ULONG)pOutBuff;
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);       //不继续往下传递
		return STATUS_SUCCESS;
	}

	case enumIdt:
	{
		//1.利用指令获取idtr的值
		XDT_INFO sidtInfo = { 0,0,0 };
		__asm sidt sidtInfo;

		//2.获取IDT表基址
		PIDT_ENTRY pIDTEntry = NULL;
		pIDTEntry = (PIDT_ENTRY)MAKE_LONG(sidtInfo.xLowXdtBase, sidtInfo.xHighXdtBase);

		//3.强制转换类型
		RtlFillMemory(pOutBuff, outputSize, 0);
		PIDTINFO pOutIdtInfo = (PIDTINFO)pOutBuff;

		//4.获取IDT信息
		for (ULONG i = 0; i < 0x100; i++)
		{
			//处理函数地址
			pOutIdtInfo->pFunction = MAKE_LONG(pIDTEntry[i].uOffsetLow, pIDTEntry[i].uOffsetHigh);

			//段选择子
			pOutIdtInfo->Selector = pIDTEntry[i].uSelector;

			//类型
			pOutIdtInfo->GateType = pIDTEntry[i].GateType;

			//特权等级
			pOutIdtInfo->Dpl = pIDTEntry[i].DPL;

			//参数个数
			pOutIdtInfo->ParamCount = pIDTEntry[i].paramCount;

			pOutIdtInfo++;
		}

		
		pIrp->IoStatus.Status = STATUS_SUCCESS;          // 完成状态
		pIrp->IoStatus.Information = (ULONG)pOutIdtInfo - (ULONG)pOutBuff;
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);       //不继续往下传递
		return STATUS_SUCCESS;
	}

	case getSsdtCount:
	{
		//获取SSDT表中服务函数的个数
		ULONG SSDTCount = 0;
		*(ULONG*)pOutBuff = SSDTCount= KeServiceDescriptorTable.NumberOfService;
		
		//---------------------------自己处理--------------------------------------		
		pIrp->IoStatus.Status = STATUS_SUCCESS;          // 完成状态
		pIrp->IoStatus.Information = sizeof(SSDTCount);// 总共传输字节数
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);       //不继续往下传递
		return STATUS_SUCCESS;
	}

	case enumSsdt:
	{
		//1.强制转换类型
		RtlFillMemory(pOutBuff, outputSize, 0);
		PSSDTINFO pOutSsdtInfo = (PSSDTINFO)pOutBuff;
		//2.循环遍历
		for (ULONG i = 0; i < KeServiceDescriptorTable.NumberOfService; i++)
		{
			//函数地址
			pOutSsdtInfo->uFuntionAddr = (ULONG)KeServiceDescriptorTable.ServiceTableBase[i];
			//调用号
			pOutSsdtInfo->uIndex = i;
			//指针后移
			pOutSsdtInfo++;
		}
		
		pIrp->IoStatus.Status = STATUS_SUCCESS;          // 完成状态
		pIrp->IoStatus.Information = (ULONG)pOutSsdtInfo - (ULONG)pOutBuff;
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);       //不继续往下传递
		return STATUS_SUCCESS;
	}

	case selfPid:
	{
		//从输入缓冲区取出进程的PID放入全局变量中
		g_uPid = *(ULONG*)pInputBuff;
		break;
	}

	case hideThread:
	{
		//取出输入缓冲区的进程PID
		HANDLE processId = (HANDLE)(*(ULONG*)pInputBuff);

		//PID转化为EProcess
		PEPROCESS pEProcess = NULL;
		NTSTATUS status = PsLookupProcessByProcessId(processId, &pEProcess);
		if (!NT_SUCCESS(status))
		{
			DbgPrint("没有找到进程,尼玛不会隐藏了吧\n");
			return STATUS_SUCCESS;
		}
		//递减引用计数
		ObDereferenceObject(pEProcess);


		//RemoveEntryList((PLIST_ENTRY)(*(PULONG)((PUCHAR)pEProcess + 0x2C)));	//KTHREAD -> ThreadListEntry
		//RemoveEntryList((PLIST_ENTRY)(*(PULONG)((PUCHAR)pEProcess + 0x188)));   //ETHREAD -> ThreadListEntry
			
		PLIST_ENTRY pList1 = (PLIST_ENTRY)(*(PULONG)((PUCHAR)pEProcess + 0x2c));
		PLIST_ENTRY pList2 = (PLIST_ENTRY)(*(PULONG)((PUCHAR)pEProcess + 0x188));
		
		//改变链表的前后连接点
		pList1->Blink->Flink = pList1->Flink;
		pList1->Flink->Blink = pList1->Blink;

		//改变自身的前后连接点
		pList1->Flink = (PLIST_ENTRY)&pList1->Flink;
		pList1->Blink = (PLIST_ENTRY)&pList1->Blink;

		//改变链表的前后连接点
		pList2->Blink->Flink = pList2->Flink;
		pList2->Flink->Blink = pList2->Blink;

		//改变自身的前后连接点
		pList2->Flink = (PLIST_ENTRY)&pList2->Flink;
		pList2->Blink = (PLIST_ENTRY)&pList2->Blink;
		

		pIrp->IoStatus.Status = STATUS_SUCCESS;          // 完成状态
		pIrp->IoStatus.Information = 0;// 总共传输字节数
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);       //不继续往下传递
		return STATUS_SUCCESS;
	}

	case hideModule:
	{
		//1.从输入缓冲区拿到目标模块的DLLBase和所属PID
		ULONG ModuleBase[2] = { 0 };
		RtlCopyMemory(ModuleBase, pInputBuff, 2 * sizeof(ULONG));				
		ULONG DLLBase = ModuleBase[0];
		HANDLE processid = (HANDLE)ModuleBase[1];
		DbgPrint("DLLBase=%p\n", DLLBase);
		DbgPrint("processid=%d\n", processid);

		//2.查找PID对应的进程EPROCESS
		PEPROCESS pEProcess = NULL;		
		NTSTATUS status = PsLookupProcessByProcessId(processid, &pEProcess);
		if (!NT_SUCCESS(status))
		{
			DbgPrint("没有找到进程,尼玛不会隐藏了吧\n");
			return STATUS_SUCCESS;
		}
		//递减引用计数
		ObDereferenceObject(pEProcess);

		//3.找到PEB	
		KAPC_STATE pAPC = { 0 };
		KeStackAttachProcess(pEProcess, &pAPC);
		PPEB peb = PsGetProcessPeb(pEProcess);
		if (peb == NULL)
		{
			DbgPrint("没有找到peb\n");
			return STATUS_SUCCESS;
		}

		//4.通过PEB遍历LDR中的LIST_ENTRY
		PLIST_ENTRY pList_First = (PLIST_ENTRY)(*(ULONG*)((ULONG)peb + 0xc) + 0xc);
		PLIST_ENTRY pList_Temp = pList_First;

		//5.断链
		do
		{
			//强制转换类型，因为LISTENTRY的基址刚好与PLDR_DATA_TABLE_ENTRY的基址重合
			PLDR_DATA_TABLE_ENTRY pModuleInfo = (PLDR_DATA_TABLE_ENTRY)pList_Temp;
			
			if (pModuleInfo->DllBase == (PVOID)DLLBase)
			{
				// 三个链表同时给断掉
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

		pIrp->IoStatus.Status = STATUS_SUCCESS;          // 完成状态
		pIrp->IoStatus.Information = 0;// 总共传输字节数
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);       //不继续往下传递
		return STATUS_SUCCESS;
	}

	case DriveInject:
	{
		//1.从输入缓冲区取出PID和DLL路径
		INJECT_INFO InjectInfo = { 0 };
		RtlCopyMemory(&InjectInfo, pInputBuff, sizeof(INJECT_INFO));
			
		NTSTATUS status;
		if (!&InjectInfo)
		{
			DbgPrint("InjectInfo是空的\n");
			return STATUS_SUCCESS;
		}

		if (!InjectDll(&InjectInfo))
		{
			DbgPrint("驱动注入失败\n");
			return STATUS_SUCCESS;
		}

		pIrp->IoStatus.Status = STATUS_SUCCESS;          // 完成状态
		pIrp->IoStatus.Information = 0;// 总共传输字节数
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);       //不继续往下传递
		return STATUS_SUCCESS;
	}

	case ProtectProcess:
	{
		//1.从输入缓冲区拿到进程的PID
		HANDLE processid = (HANDLE)(*(ULONG*)pInputBuff);

		//DbgBreakPoint();

		//2.断链隐藏指定PID的进程
		ProtectTheProcess(processid);

		pIrp->IoStatus.Status = STATUS_SUCCESS;          // 完成状态
		pIrp->IoStatus.Information = 0;// 总共传输字节数
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);       //不继续往下传递
		return STATUS_SUCCESS;
	}

	case getVadCount:
	{
		//1.从输入缓冲区拿到进程的PID
		HANDLE processid = (HANDLE)(*(ULONG*)pInputBuff);

		//2.获得VAD的数目
		ULONG VADCount = 0;
		VADCount = GetVADCount(processid);
		
		//3.将线程数量写入输出缓冲区
		*(ULONG*)pOutBuff = VADCount;

		//------------------------------------------因为返回的字节有变化，所以自己处理，直接return---------
		//1.设置IRP完成状态
		pIrp->IoStatus.Status = STATUS_SUCCESS;    //状态为成功
		//2.设置IRP操作了多少个字节
		pIrp->IoStatus.Information = sizeof(VADCount);
		//3.处理IRP
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
		return STATUS_SUCCESS;
	}

	case enumProcessVad:
	{
		//1.从输入缓冲区拿到进程的PID
		HANDLE processid = (HANDLE)(*(ULONG*)pInputBuff);

		//2.将输出缓冲区全置0,并强制转换类型
		RtlFillMemory(pOutBuff, outputSize, 0);
		PVADINFO pOutVADInfo = (PVADINFO)pOutBuff;

		//3.遍历VAD
		_EnumProcessVAD(processid, pOutVADInfo);
				
		//------------------------------------------因为返回的字节有变化，所以自己处理，直接return---------
		//1.设置IRP完成状态
		pIrp->IoStatus.Status = STATUS_SUCCESS;    //状态为成功
		//2.设置IRP操作了多少个字节
		pIrp->IoStatus.Information = (ULONG)pOutVADInfo - (ULONG)pOutBuff;   //返回字节数怎么处理，教训
		//3.处理IRP
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
		return STATUS_SUCCESS;
	}

	case enumNotify:
	{
		//3.将输出缓冲区全置0，强制转换类型
		RtlFillMemory(pOutBuff, outputSize, 0);
		PNOTIFYINFO pOutNotifyInfo = (PNOTIFYINFO)pOutBuff;
		EnumCreateProcessNotify(driver1, &pOutNotifyInfo);	 //没有用二级指针，调了1天，才发现问题	
		EnumCreateThreadNotify(driver1, &pOutNotifyInfo);
		EnumImageNotify(driver1, &pOutNotifyInfo);
		EnumCallBackList(driver1, &pOutNotifyInfo);

		pIrp->IoStatus.Status = STATUS_SUCCESS;          // 完成状态
		pIrp->IoStatus.Information = (ULONG)pOutNotifyInfo - (ULONG)pOutBuff;
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);       //不继续往下传递
		return STATUS_SUCCESS;
	}

	case enumObcallback:
	{
		//1.将输出缓冲区全置0，强制转换类型
		RtlFillMemory(pOutBuff, outputSize, 0);
		POBCALLBACKINFO pOutobcallback = (POBCALLBACKINFO)pOutBuff;

		EnumProcessObCallback(driver1, &pOutobcallback);
		EnumThreadObCallback(driver1, &pOutobcallback);

		pIrp->IoStatus.Status = STATUS_SUCCESS;          // 完成状态
		pIrp->IoStatus.Information = (ULONG)pOutobcallback - (ULONG)pOutBuff;
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);       //不继续往下传递
		return STATUS_SUCCESS;
	}

	case enumDpctimer:
	{
		//1.将输出缓冲区全置0，强制转换类型
		RtlFillMemory(pOutBuff, outputSize, 0);
		PDPCTIMERINFO pOutDpcInfo = (PDPCTIMERINFO)pOutBuff;

		EnumDpcTimer(driver1, &pOutDpcInfo);

		pIrp->IoStatus.Status = STATUS_SUCCESS;          // 完成状态
		pIrp->IoStatus.Information = (ULONG)pOutDpcInfo - (ULONG)pOutBuff;
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);       //不继续往下传递
		return STATUS_SUCCESS;
	}

	case enumIotimer:
	{
		//1.将输出缓冲区全置0，强制转换类型
		RtlFillMemory(pOutBuff, outputSize, 0);
		PIOTIMERINFO pOutIOTIMERInfo = (PIOTIMERINFO)pOutBuff;

		EnumIoTimer(driver1, &pOutIOTIMERInfo);

		pIrp->IoStatus.Status = STATUS_SUCCESS;          // 完成状态
		pIrp->IoStatus.Information = (ULONG)pOutIOTIMERInfo - (ULONG)pOutBuff;
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);       //不继续往下传递
		return STATUS_SUCCESS;
	}

	default:
		break;
	}

	//-----------------------------------------------------I/O通信第三步：设置状态、返回字节数、是否继续往下传递----------
	//1.设置IRP完成状态
	pIrp->IoStatus.Status = STATUS_SUCCESS;    //状态为成功
	//2.设置IRP操作了多少个字节
	pIrp->IoStatus.Information = outputSize ? outputSize : inputSize;   //如果存在outputSize，那么就选outputSize，否则选inputSize
	//3.处理IRP
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);   //不继续往下传递
	//-----------------------------------------------------------------------------------------------------------------------

	return STATUS_SUCCESS;
}
//------------------------------------------------------------------------------------------------------------------//



//-------------------------------驱动函数-------------------------------//
//1.卸载驱动
VOID OnUnload(DRIVER_OBJECT* driver)
{
	if (pThreadObj1!=NULL)
	{
		bTerminated1 = TRUE;
		KeWaitForSingleObject(pThreadObj1, Executive, KernelMode, FALSE, NULL);
		ObDereferenceObject(pThreadObj1);
		DbgPrint("线程1被关闭\n");
	}

	if (pThreadObj2 != NULL)
	{
		bTerminated2 = TRUE;
		KeWaitForSingleObject(pThreadObj2, Executive, KernelMode, FALSE, NULL);
		ObDereferenceObject(pThreadObj2);
		DbgPrint("线程2被关闭\n");
	}


	//1.卸载 SYSENTER-HOOK
	UnstallSysenterHook();

	//2.删除符号链接
	UNICODE_STRING symbolName = RTL_CONSTANT_STRING(NAME_SYMBOL);
	IoDeleteSymbolicLink(&symbolName);

	//3.销毁设备对象
	IoDeleteDevice(driver->DeviceObject);

	//4.卸载KiFastCallEntryHOOK
	unstallKiFastCallEntryHook();

	DbgPrint("驱动被卸载\n");
}
//2.驱动入口函数
NTSTATUS DriverEntry(PDRIVER_OBJECT driver, PUNICODE_STRING path)
{
	//避免编译器报未使用的错误
	UNREFERENCED_PARAMETER(path);
	NTSTATUS status = STATUS_SUCCESS;	
	driver1 = driver;
	
	//1.重载内核
	reloadNT(driver);

	//2.安装KiFastCallEntryHook
	installKiFastCallEntryHook();
	
	//3.绑定卸载函数
	driver->DriverUnload = OnUnload;

	//4.创建设备	
	UNICODE_STRING devName = RTL_CONSTANT_STRING(NAME_DEVICE);
	PDEVICE_OBJECT pDevice = NULL;
	status = IoCreateDevice(
		driver,			//驱动对象(新创建的设备对象所属驱动对象)
		0,				//设备扩展大小
		&devName,		//设备名称
		FILE_DEVICE_UNKNOWN,	//设备类型(未知类型)
		0,				//设备特征信息
		FALSE,			//设备是否为独占的
		&pDevice		//创建完成的设备对象指针
	);
	if (!NT_SUCCESS(status))
	{
		DbgPrint("创建设备失败，错误码：0x%08X\n", status);
		return status;
	}

	//5.读写方式为直接读写方式
	pDevice->Flags = DO_DIRECT_IO;

	//6.绑定符号链接
	UNICODE_STRING symbolName = RTL_CONSTANT_STRING(NAME_SYMBOL);
	IoCreateSymbolicLink(&symbolName, &devName);

	//7.绑定派遣函数(在驱动对象中)
	driver->MajorFunction[IRP_MJ_CREATE] = OnCreate;
	driver->MajorFunction[IRP_MJ_CLOSE] = OnClose;
	driver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = OnDeviceIoControl;

	//8.安装SYSENTER-HOOK
	InstallSysenterHook();

	return status;
}

