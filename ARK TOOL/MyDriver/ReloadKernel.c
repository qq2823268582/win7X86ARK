#include "ReloadKernel.h"
#include "Myssdt.h"
#include "Myprocess.h"

//-------------------------------------------------内核重载---------------------------------------------------//
ULONG_PTR g_OldSysenter;	            //保存原本的SYSTENTER_EIP_MSR中记录的函数地址
extern ULONG g_uPid;	                //要保护的进程的PID
static char*		g_pNewNtKernel;		// 新内核
static ULONG		g_ntKernelSize;		// 内核的映像大小
static SSDTEntry*	g_pNewSSDTEntry;	// 新ssdt的入口地址
static ULONG		g_hookAddr;			// 被hook位置的首地址
static ULONG		g_hookAddr_next_ins;// 被hook的指令的下一条指令的首地址.


//---------------------------------------------工具函数-------------------------------------------------
//自定义函数：打开文件
NTSTATUS createFile(wchar_t * filepath,
	ULONG access,
	ULONG share,
	ULONG openModel,
	BOOLEAN isDir,
	HANDLE * hFile)
{

	NTSTATUS status = STATUS_SUCCESS;

	IO_STATUS_BLOCK StatusBlock = { 0 };
	ULONG           ulShareAccess = share;
	ULONG ulCreateOpt = FILE_SYNCHRONOUS_IO_NONALERT;

	UNICODE_STRING path;
	RtlInitUnicodeString(&path, filepath);

	// 1. 初始化OBJECT_ATTRIBUTES的内容
	OBJECT_ATTRIBUTES objAttrib = { 0 };
	ULONG ulAttributes = OBJ_CASE_INSENSITIVE/*不区分大小写*/ | OBJ_KERNEL_HANDLE/*内核句柄*/;
	InitializeObjectAttributes(&objAttrib,    // 返回初始化完毕的结构体
		&path,      // 文件对象名称
		ulAttributes,  // 对象属性
		NULL, NULL);   // 一般为NULL

	// 2. 创建文件对象
	ulCreateOpt |= isDir ? FILE_DIRECTORY_FILE : FILE_NON_DIRECTORY_FILE;

	status = ZwCreateFile(hFile,                 // 返回文件句柄
		access,				 // 文件操作描述
		&objAttrib,            // OBJECT_ATTRIBUTES
		&StatusBlock,          // 接受函数的操作结果
		0,                     // 初始文件大小
		FILE_ATTRIBUTE_NORMAL, // 新建文件的属性
		ulShareAccess,         // 文件共享方式
		openModel,			 // 文件存在则打开不存在则创建
		ulCreateOpt,           // 打开操作的附加标志位
		NULL,                  // 扩展属性区
		0);                    // 扩展属性区长度
	return status;
}

//自定义函数：读取文件
NTSTATUS readFile(HANDLE hFile,
	ULONG offsetLow,
	ULONG offsetHig,
	ULONG sizeToRead,
	PVOID pBuff,
	ULONG* read)
{
	NTSTATUS status;
	IO_STATUS_BLOCK isb = { 0 };
	LARGE_INTEGER offset;
	// 设置要读取的文件偏移
	offset.HighPart = offsetHig;
	offset.LowPart = offsetLow;

	status = ZwReadFile(hFile,/*文件句柄*/
		NULL,/*事件对象,用于异步IO*/
		NULL,/*APC的完成通知例程:用于异步IO*/
		NULL,/*完成通知例程序的附加参数*/
		&isb,/*IO状态*/
		pBuff,/*保存文件数据的缓冲区*/
		sizeToRead,/*要读取的字节数*/
		&offset,/*要读取的文件位置*/
		NULL);
	if (status == STATUS_SUCCESS)
		*read = isb.Information;
	return status;
}
// 关闭内存页写入保护
void _declspec(naked) disablePageWriteProtect()
{
	_asm
	{
		push eax;
		mov eax, cr0;
		and eax, ~0x10000;
		mov cr0, eax;
		pop eax;
		ret;
	}
}

// 开启内存页写入保护
void _declspec(naked) enablePageWriteProtect()
{
	_asm
	{
		push eax;
		mov eax, cr0;
		or eax, 0x10000;
		mov cr0, eax;
		pop eax;
		ret;
	}
}



//-----------------------------------------------重载内核-------------------------------------------
//【自定义函数1】loadNtKernelModule：重新加载一个NT内核模块到内存中
//【参数1】UNICODE_STRING* ntkernelPath ：NT内核模块的路径（通过LDR链表可以获取） //一级指针
//【参数2】char** pOut ：指向“新内核的基址”的指针  //二级指针
NTSTATUS loadNtKernelModule(IN UNICODE_STRING* ntkernelPath, OUT char** pOut)
{
	//1.将内核模块作为文件来打开.
	NTSTATUS status = STATUS_SUCCESS;
	HANDLE hFile = NULL;
	PCHAR pBuff = NULL;
	status = createFile(ntkernelPath->Buffer,
		GENERIC_READ,
		FILE_SHARE_READ,
		FILE_OPEN_IF,
		FALSE,
		&hFile);
	if (status != STATUS_SUCCESS)
	{
		KdPrint(("打开文件失败\n"));
		goto _DONE;
	}

	//2.将PE文件头部读取到内存
	char pKernelBuff[0x1000];
	ULONG read = 0;
	status = readFile(hFile, 0, 0, 0x1000, pKernelBuff, &read);
	if (STATUS_SUCCESS != status)
	{
		KdPrint(("读取文件内容失败\n"));
		goto _DONE;
	}

	//3.申请内存
	IMAGE_DOS_HEADER* pDos = (IMAGE_DOS_HEADER*)pKernelBuff;
	IMAGE_NT_HEADERS* pnt = (IMAGE_NT_HEADERS*)((ULONG)pDos + pDos->e_lfanew);
	ULONG imgSize = pnt->OptionalHeader.SizeOfImage;
		
	

	pBuff = ExAllocatePool(NonPagedPool, imgSize);

	if (pBuff == NULL)
	{
		KdPrint(("内存申请失败失败\n"));
		status = STATUS_BUFFER_ALL_ZEROS;//随便返回个错误码
		goto _DONE;
	}

	//4.拷贝头部
	RtlCopyMemory(pBuff, pKernelBuff, pnt->OptionalHeader.SizeOfHeaders);

	//5.拷贝节区
	IMAGE_SECTION_HEADER* pScnHdr = IMAGE_FIRST_SECTION(pnt);
	ULONG scnCount = pnt->FileHeader.NumberOfSections;
	for (ULONG i = 0; i < scnCount; ++i)
	{
		status = readFile(hFile,
			pScnHdr[i].PointerToRawData,
			0,
			pScnHdr[i].SizeOfRawData,
			pScnHdr[i].VirtualAddress + pBuff,
			&read);
		if (status != STATUS_SUCCESS)
			goto _DONE;
	}

_DONE:
	//关闭打开的文件句柄
	ZwClose(hFile);
	//将申请的内存的基址传递出去
	*pOut = pBuff;
	//如果操作失败，将指针置为NULL
	if (status != STATUS_SUCCESS)
	{
		if (pBuff != NULL)
		{
			ExFreePool(pBuff);
			*pOut = pBuff = NULL;
		}
	}
	return status;
}

//【自定义函数2】fixRelocation：修复重定位表
//【参数1】char* pDosHdr：加载的新内核的基址
//【参数2】ULONG curNtKernelBase：旧内核的基址
void fixRelocation(char* pDosHdr, ULONG curNtKernelBase)
{
	IMAGE_DOS_HEADER* pDos = (IMAGE_DOS_HEADER*)pDosHdr;
	IMAGE_NT_HEADERS* pNt = (IMAGE_NT_HEADERS*)((ULONG)pDos + pDos->e_lfanew);
	ULONG uRelRva = pNt->OptionalHeader.DataDirectory[5].VirtualAddress;
	IMAGE_BASE_RELOCATION* pRel =
		(IMAGE_BASE_RELOCATION*)(uRelRva + (ULONG)pDos);

	while (pRel->SizeOfBlock)
	{
		typedef struct
		{
			USHORT offset : 12;
			USHORT type : 4;
		}TypeOffset;

		ULONG count = (pRel->SizeOfBlock - 8) / 2;
		TypeOffset* pTypeOffset = (TypeOffset*)(pRel + 1);
		for (ULONG i = 0; i < count; ++i)
		{
			if (pTypeOffset[i].type != 3)
			{
				continue;
			}

			ULONG* pFixAddr = (ULONG*)(pTypeOffset[i].offset + pRel->VirtualAddress + (ULONG)pDos);
			//
			// 减去默认加载基址
			//
			*pFixAddr -= pNt->OptionalHeader.ImageBase;

			//
			// 加上新的加载基址(使用的是当前内核的加载基址,这样做
			// 是为了让新内核使用当前内核的数据(全局变量,未初始化变量等数据).)
			//
			*pFixAddr += (ULONG)curNtKernelBase;
		}

		pRel = (IMAGE_BASE_RELOCATION*)((ULONG)pRel + pRel->SizeOfBlock);
	}
}

//【自定义函数3】initSSDT：填充SSDT表
//【参数1】char* pNewBase ：新加载的内核的基址
//【参数2】char* pCurKernelBase ：当前正在使用的内核的基址
void initSSDT(IN char* pNewBase, IN char* pCurKernelBase)
{
	// 1. 分别获取当前内核,新加载的内核的`KeServiceDescriptorTable`
	//    的地址
	SSDTEntry* pCurSSDTEnt = &KeServiceDescriptorTable;
	g_pNewSSDTEntry = (SSDTEntry*)((ULONG)pCurSSDTEnt - (ULONG)pCurKernelBase + (ULONG)pNewBase);

	// 2. 获取新加载的内核以下三张表的地址:
	// 2.1 服务函数表基址
	g_pNewSSDTEntry->ServiceTableBase = (ULONG*)
		((ULONG)pCurSSDTEnt->ServiceTableBase - (ULONG)pCurKernelBase + (ULONG)pNewBase);

	// 2.3 服务函数参数字节数表基址
	g_pNewSSDTEntry->ParamTableBase = (ULONG)
		((ULONG)pCurSSDTEnt->ParamTableBase - (ULONG)pCurKernelBase + (ULONG)pNewBase);

	// 2.2 服务函数调用次数表基址(有时候这个表并不存在.)
	if (pCurSSDTEnt->ServiceCounterTableBase)
	{
		g_pNewSSDTEntry->ServiceCounterTableBase = (ULONG*)
			((ULONG)pCurSSDTEnt->ServiceCounterTableBase - (ULONG)pCurKernelBase + (ULONG)pNewBase);
	}

	// 2.3 设置新SSDT表的服务个数
	g_pNewSSDTEntry->NumberOfService = pCurSSDTEnt->NumberOfService;

	//3. 将服务函数的地址填充到新SSDT表(重定位时其实已经修复好了,
	//   但是,在修复重定位的时候,是使用当前内核的加载基址的, 修复重定位
	//   为之后, 新内核的SSDT表保存的服务函数的地址都是当前内核的地址,
	//   在这里要将这些服务函数的地址改回新内核中的函数地址.)
	disablePageWriteProtect();
	for (ULONG i = 0; i < g_pNewSSDTEntry->NumberOfService; ++i)
	{
		// 减去当前内核的加载基址
		g_pNewSSDTEntry->ServiceTableBase[i] -= (ULONG)pCurKernelBase;
		// 换上新内核的加载基址.
		g_pNewSSDTEntry->ServiceTableBase[i] += (ULONG)pNewBase;
	}
	enablePageWriteProtect();
}

//总函数：重载内核
NTSTATUS reloadNT(PDRIVER_OBJECT driver)
{
	NTSTATUS status = STATUS_SUCCESS;

	//1.通过遍历内核链表的方式来找到内核主模块
	LDR_DATA_TABLE_ENTRY* pLdr = ((LDR_DATA_TABLE_ENTRY*)driver->DriverSection);
	for (int i = 0; i < 2; ++i)    //内核主模块在链表中的第2项.
	{
		pLdr = (LDR_DATA_TABLE_ENTRY*)pLdr->InLoadOrderLinks.Flink;
	}

	//2.保存当前内核的镜像大小/基址
	g_ntKernelSize = pLdr->SizeOfImage;
	char* pCurKernelBase = (char*)pLdr->DllBase;

	//3.读取nt模块的文件内容到堆空间.  
	status = loadNtKernelModule(IN &pLdr->FullDllName, OUT &g_pNewNtKernel);
	if (STATUS_SUCCESS != status)
	{
		return status;
	 }

	//4.修复新nt模块的重定位.  
	fixRelocation(IN g_pNewNtKernel, IN(ULONG)pCurKernelBase);

	//5.使用当前正在使用的内核的数据来填充新内核的SSDT表.   
	initSSDT(g_pNewNtKernel, pCurKernelBase);

	return status;
}



//--------------------------------------KiFastCallEntry-HOOK--------------------------------------
//如果调用函数是ZwOpenProcess，则将新内核的ZwOpenProcess函数地址返回
//【自定义函数】SSDTFilter：SSDT过滤函数.  
//【参数1】ULONG index          /*索引号,也是调用号*/
//【参数2】ULONG tableAddress   /*表的地址,可能是SSDT表的地址,也可能是Shadow SSDT表的地址*/
//【参数3】ULONG funAddr        /*从表中取出的函数地址*/
ULONG SSDTFilter(ULONG index,ULONG tableAddress,ULONG funAddr)
{
	// 如果是SSDT表的话
	if (tableAddress == (ULONG)KeServiceDescriptorTable.ServiceTableBase)
	{
		// 判断调用号(190是ZwOpenProcess函数的调用号)
		if (index == 190)
		{
			// 返回新SSDT表的函数地址,也就是新内核的函数地址
			// 感觉这里也可以换成其它函数地址，不一定得是新的内核的函数地址
			return g_pNewSSDTEntry->ServiceTableBase[190];
		}
	}
	// 返回旧的函数地址
	return funAddr;
}

//通过汇编更改了edx的值（SSDT表中取出的函数地址）--->执行inline-hook时被覆盖的5字节---->跳回原先的路线
void _declspec(naked) myKiFastCallEntry()
{
	_asm
	{
		pushad;
		pushfd;

		push edx; //从表中取出的函数地址
		push edi; // 表的地址
		push eax; // 调用号
		call SSDTFilter; // 调用过滤函数

		;// 函数调用完毕之后栈空间布局,指令pushad将
		;// 32位的通用寄存器保存在栈中,栈空间布局为:
		;// [esp + 00] <=> eflag
		;// [esp + 04] <=> edi
		;// [esp + 08] <=> esi
		;// [esp + 0C] <=> ebp
		;// [esp + 10] <=> esp
		;// [esp + 14] <=> ebx
		;// [esp + 18] <=> edx <<-- 使用函数返回值来修改这个位置
		;// [esp + 1C] <=> ecx
		;// [esp + 20] <=> eax
		mov dword ptr ds : [esp + 0x18], eax;
		popfd; // popfd时,实际上edx的值就回被修改
		popad;

		; //执行被hook覆盖的两条指令
		sub esp, ecx;
		shr ecx, 2;
		jmp g_hookAddr_next_ins;
	}
}

//卸载KiFastCallEntry-HOOK    //卸载放在上面是因为安装的时候如果遇到已经被HOOK过了，那么要先卸载已安装的
void unstallKiFastCallEntryHook()
{
	if (g_hookAddr)
	{
		//原始数据
		UCHAR srcCode[5] = { 0x2b,0xe1,0xc1,0xe9,0x02 };

		disablePageWriteProtect();
		_asm sti
		RtlCopyMemory((PUCHAR)g_hookAddr, srcCode, 5);
		_asm cli

		g_hookAddr = 0;

		enablePageWriteProtect();
	}

	if (g_pNewNtKernel)
	{
		ExFreePool(g_pNewNtKernel);
		g_pNewNtKernel = NULL;
	}
}

//安装KiFastCallEntry-HOOK   //MSR取得KiFastCallEntry函数首地址--->搜索HOOK位置--->替换成myKiFastEntry
void installKiFastCallEntryHook()
{
	g_hookAddr = 0;

	// 1. 找到KiFastCallEntry函数首地址
	ULONG uKiFastCallEntry = 0;
	_asm
	{
		;// KiFastCallEntry函数地址保存
		;// 在特殊模组寄存器的0x176号寄存器中
		push ecx;
		push eax;
		push edx;
		mov ecx, 0x176; // 设置编号
		rdmsr; ;// 读取到edx:eax
		mov uKiFastCallEntry, eax;// 保存到变量
		pop edx;
		pop eax;
		pop ecx;
	}

	// 2. 找到HOOK的位置, 并保存5字节的数据
	// 2.1 HOOK的位置选定为(此处正好5字节,):
	//  2be1            sub     esp, ecx ;
	// 	c1e902          shr     ecx, 2   ;
	UCHAR hookCode[5] = { 0x2b,0xe1,0xc1,0xe9,0x02 }; //保存inline hook覆盖的5字节
	ULONG i = 0;
	for (; i < 0x1FF; ++i)
	{
		if (RtlCompareMemory((UCHAR*)uKiFastCallEntry + i,hookCode,5) == 5)
		{
			break;
		}
	}
	if (i >= 0x1FF)
	{
		KdPrint(("在KiFastCallEntry函数中没有找到HOOK位置,可能KiFastCallEntry已经被HOOK过了\n"));
		unstallKiFastCallEntryHook();
		return;
	}

	g_hookAddr = uKiFastCallEntry + i;
	g_hookAddr_next_ins = g_hookAddr + 5;

	// 3. 开始inline hook
	UCHAR jmpCode[5] = { 0xe9 };// jmp xxxx
	disablePageWriteProtect();

	// 3.1 计算跳转偏移
	// 跳转偏移 = 目标地址 - 当前地址 - 5
	*(ULONG*)(jmpCode + 1) = (ULONG)myKiFastCallEntry - g_hookAddr - 5;

	// 3.2 将跳转指令写入
	RtlCopyMemory((PUCHAR)(uKiFastCallEntry + i),jmpCode,5);

	enablePageWriteProtect();
}



//----------------------------------------------Sysenter-Hook反调试、反结束-----------------------------------//
//自定义函数：保护自身进程不被ZwOpenProcess打开   //首先判断调用号是不是0xBE、其次判断PID是不是等于自身
VOID _declspec(naked) MySysenter()
{
	__asm
	{
		// 检查调用号
		cmp eax, 0xBE;	         //0xBE(ZwOpenProcess的调用号)
		jne _DONE;		         // 调用号不是0xBE,执行调用原来的 Sysenter 函数

		push eax;				//保存寄存器

		mov eax, [edx + 0x14];	// eax保存了PCLIENT_ID
		mov eax, [eax];			// eax保存了PID

		cmp eax, [g_uPid];		//判断是否是要保护的进程的ID
		pop eax;				// 恢复寄存器
		jne _DONE;				// 不是要保护的进程就跳转

		mov[edx + 0xc], 0;		// 是要保护的进程就将访问权限设置为0，让后续函数调用失败

	_DONE:
		// 调用原来的Sysenter
		jmp g_OldSysenter;
	}
}

//安装SysenterHook   //备份SYSTENTER_EIP_MSR寄存器里的旧函数地址，然后将自定义函数地址写入SYSTENTER_EIP_MSR寄存器
VOID InstallSysenterHook()
{
	__asm
	{
		push edx;						//保存寄存器
		push eax;
		push ecx;

		//备份原始函数
		mov ecx, 0x176;					// SYSTENTER_EIP_MSR寄存器的编号
		rdmsr;							// 将ECX指定的MSR加载到EDX：EAX
		mov[g_OldSysenter], eax;	   // 将旧的地址保存到全局变量中

		//将新的函数设置进去
		mov eax, MySysenter;
		xor edx, edx;
		wrmsr;							// 将edx:eax 写入ECX指定的MSR寄存器中

		pop ecx;						//恢复寄存器
		pop eax;
		pop edx;

		ret;
	}
}

//卸载SysenterHook  //将备份的旧函数地址重新写入SYSTENTER_EIP_MSR寄存器
VOID UnstallSysenterHook()
{
	__asm
	{
		push edx;
		push eax;
		push ecx;

		mov eax, [g_OldSysenter];
		xor edx, edx;
		mov ecx, 0x176;
		wrmsr;

		pop ecx;
		pop eax;
		pop edx;
	}
}


//感觉应该是双重保险
//第一重保险是Sysenter-Hook，将SYSTENTER_EIP_MSR中记录的EIP地址替换成我们的自定义函数地址，我们的函数里面过滤一下
//相当于将进内核的路给断了

//第二重保险是KiFastCallEntry-HOOK，将SSDT表中取出的函数地址替换成了新的内核中的SSDT表的函数地址


//SSDT-HOOK,是更改SSDT表中的函数地址，应该是最靠后的了吧？