#include "ReloadKernel.h"
#include "Myssdt.h"
#include "Myprocess.h"

//-------------------------------------------------�ں�����---------------------------------------------------//
ULONG_PTR g_OldSysenter;	            //����ԭ����SYSTENTER_EIP_MSR�м�¼�ĺ�����ַ
extern ULONG g_uPid;	                //Ҫ�����Ľ��̵�PID
static char*		g_pNewNtKernel;		// ���ں�
static ULONG		g_ntKernelSize;		// �ں˵�ӳ���С
static SSDTEntry*	g_pNewSSDTEntry;	// ��ssdt����ڵ�ַ
static ULONG		g_hookAddr;			// ��hookλ�õ��׵�ַ
static ULONG		g_hookAddr_next_ins;// ��hook��ָ�����һ��ָ����׵�ַ.


//---------------------------------------------���ߺ���-------------------------------------------------
//�Զ��庯�������ļ�
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

	// 1. ��ʼ��OBJECT_ATTRIBUTES������
	OBJECT_ATTRIBUTES objAttrib = { 0 };
	ULONG ulAttributes = OBJ_CASE_INSENSITIVE/*�����ִ�Сд*/ | OBJ_KERNEL_HANDLE/*�ں˾��*/;
	InitializeObjectAttributes(&objAttrib,    // ���س�ʼ����ϵĽṹ��
		&path,      // �ļ���������
		ulAttributes,  // ��������
		NULL, NULL);   // һ��ΪNULL

	// 2. �����ļ�����
	ulCreateOpt |= isDir ? FILE_DIRECTORY_FILE : FILE_NON_DIRECTORY_FILE;

	status = ZwCreateFile(hFile,                 // �����ļ����
		access,				 // �ļ���������
		&objAttrib,            // OBJECT_ATTRIBUTES
		&StatusBlock,          // ���ܺ����Ĳ������
		0,                     // ��ʼ�ļ���С
		FILE_ATTRIBUTE_NORMAL, // �½��ļ�������
		ulShareAccess,         // �ļ�����ʽ
		openModel,			 // �ļ�������򿪲������򴴽�
		ulCreateOpt,           // �򿪲����ĸ��ӱ�־λ
		NULL,                  // ��չ������
		0);                    // ��չ����������
	return status;
}

//�Զ��庯������ȡ�ļ�
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
	// ����Ҫ��ȡ���ļ�ƫ��
	offset.HighPart = offsetHig;
	offset.LowPart = offsetLow;

	status = ZwReadFile(hFile,/*�ļ����*/
		NULL,/*�¼�����,�����첽IO*/
		NULL,/*APC�����֪ͨ����:�����첽IO*/
		NULL,/*���֪ͨ������ĸ��Ӳ���*/
		&isb,/*IO״̬*/
		pBuff,/*�����ļ����ݵĻ�����*/
		sizeToRead,/*Ҫ��ȡ���ֽ���*/
		&offset,/*Ҫ��ȡ���ļ�λ��*/
		NULL);
	if (status == STATUS_SUCCESS)
		*read = isb.Information;
	return status;
}
// �ر��ڴ�ҳд�뱣��
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

// �����ڴ�ҳд�뱣��
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



//-----------------------------------------------�����ں�-------------------------------------------
//���Զ��庯��1��loadNtKernelModule�����¼���һ��NT�ں�ģ�鵽�ڴ���
//������1��UNICODE_STRING* ntkernelPath ��NT�ں�ģ���·����ͨ��LDR������Ի�ȡ�� //һ��ָ��
//������2��char** pOut ��ָ�����ں˵Ļ�ַ����ָ��  //����ָ��
NTSTATUS loadNtKernelModule(IN UNICODE_STRING* ntkernelPath, OUT char** pOut)
{
	//1.���ں�ģ����Ϊ�ļ�����.
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
		KdPrint(("���ļ�ʧ��\n"));
		goto _DONE;
	}

	//2.��PE�ļ�ͷ����ȡ���ڴ�
	char pKernelBuff[0x1000];
	ULONG read = 0;
	status = readFile(hFile, 0, 0, 0x1000, pKernelBuff, &read);
	if (STATUS_SUCCESS != status)
	{
		KdPrint(("��ȡ�ļ�����ʧ��\n"));
		goto _DONE;
	}

	//3.�����ڴ�
	IMAGE_DOS_HEADER* pDos = (IMAGE_DOS_HEADER*)pKernelBuff;
	IMAGE_NT_HEADERS* pnt = (IMAGE_NT_HEADERS*)((ULONG)pDos + pDos->e_lfanew);
	ULONG imgSize = pnt->OptionalHeader.SizeOfImage;
		
	

	pBuff = ExAllocatePool(NonPagedPool, imgSize);

	if (pBuff == NULL)
	{
		KdPrint(("�ڴ�����ʧ��ʧ��\n"));
		status = STATUS_BUFFER_ALL_ZEROS;//��㷵�ظ�������
		goto _DONE;
	}

	//4.����ͷ��
	RtlCopyMemory(pBuff, pKernelBuff, pnt->OptionalHeader.SizeOfHeaders);

	//5.��������
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
	//�رմ򿪵��ļ����
	ZwClose(hFile);
	//��������ڴ�Ļ�ַ���ݳ�ȥ
	*pOut = pBuff;
	//�������ʧ�ܣ���ָ����ΪNULL
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

//���Զ��庯��2��fixRelocation���޸��ض�λ��
//������1��char* pDosHdr�����ص����ں˵Ļ�ַ
//������2��ULONG curNtKernelBase�����ں˵Ļ�ַ
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
			// ��ȥĬ�ϼ��ػ�ַ
			//
			*pFixAddr -= pNt->OptionalHeader.ImageBase;

			//
			// �����µļ��ػ�ַ(ʹ�õ��ǵ�ǰ�ں˵ļ��ػ�ַ,������
			// ��Ϊ�������ں�ʹ�õ�ǰ�ں˵�����(ȫ�ֱ���,δ��ʼ������������).)
			//
			*pFixAddr += (ULONG)curNtKernelBase;
		}

		pRel = (IMAGE_BASE_RELOCATION*)((ULONG)pRel + pRel->SizeOfBlock);
	}
}

//���Զ��庯��3��initSSDT�����SSDT��
//������1��char* pNewBase ���¼��ص��ں˵Ļ�ַ
//������2��char* pCurKernelBase ����ǰ����ʹ�õ��ں˵Ļ�ַ
void initSSDT(IN char* pNewBase, IN char* pCurKernelBase)
{
	// 1. �ֱ��ȡ��ǰ�ں�,�¼��ص��ں˵�`KeServiceDescriptorTable`
	//    �ĵ�ַ
	SSDTEntry* pCurSSDTEnt = &KeServiceDescriptorTable;
	g_pNewSSDTEntry = (SSDTEntry*)((ULONG)pCurSSDTEnt - (ULONG)pCurKernelBase + (ULONG)pNewBase);

	// 2. ��ȡ�¼��ص��ں��������ű�ĵ�ַ:
	// 2.1 ���������ַ
	g_pNewSSDTEntry->ServiceTableBase = (ULONG*)
		((ULONG)pCurSSDTEnt->ServiceTableBase - (ULONG)pCurKernelBase + (ULONG)pNewBase);

	// 2.3 �����������ֽ������ַ
	g_pNewSSDTEntry->ParamTableBase = (ULONG)
		((ULONG)pCurSSDTEnt->ParamTableBase - (ULONG)pCurKernelBase + (ULONG)pNewBase);

	// 2.2 ���������ô������ַ(��ʱ�������������.)
	if (pCurSSDTEnt->ServiceCounterTableBase)
	{
		g_pNewSSDTEntry->ServiceCounterTableBase = (ULONG*)
			((ULONG)pCurSSDTEnt->ServiceCounterTableBase - (ULONG)pCurKernelBase + (ULONG)pNewBase);
	}

	// 2.3 ������SSDT��ķ������
	g_pNewSSDTEntry->NumberOfService = pCurSSDTEnt->NumberOfService;

	//3. ���������ĵ�ַ��䵽��SSDT��(�ض�λʱ��ʵ�Ѿ��޸�����,
	//   ����,���޸��ض�λ��ʱ��,��ʹ�õ�ǰ�ں˵ļ��ػ�ַ��, �޸��ض�λ
	//   Ϊ֮��, ���ں˵�SSDT����ķ������ĵ�ַ���ǵ�ǰ�ں˵ĵ�ַ,
	//   ������Ҫ����Щ�������ĵ�ַ�Ļ����ں��еĺ�����ַ.)
	disablePageWriteProtect();
	for (ULONG i = 0; i < g_pNewSSDTEntry->NumberOfService; ++i)
	{
		// ��ȥ��ǰ�ں˵ļ��ػ�ַ
		g_pNewSSDTEntry->ServiceTableBase[i] -= (ULONG)pCurKernelBase;
		// �������ں˵ļ��ػ�ַ.
		g_pNewSSDTEntry->ServiceTableBase[i] += (ULONG)pNewBase;
	}
	enablePageWriteProtect();
}

//�ܺ����������ں�
NTSTATUS reloadNT(PDRIVER_OBJECT driver)
{
	NTSTATUS status = STATUS_SUCCESS;

	//1.ͨ�������ں�����ķ�ʽ���ҵ��ں���ģ��
	LDR_DATA_TABLE_ENTRY* pLdr = ((LDR_DATA_TABLE_ENTRY*)driver->DriverSection);
	for (int i = 0; i < 2; ++i)    //�ں���ģ���������еĵ�2��.
	{
		pLdr = (LDR_DATA_TABLE_ENTRY*)pLdr->InLoadOrderLinks.Flink;
	}

	//2.���浱ǰ�ں˵ľ����С/��ַ
	g_ntKernelSize = pLdr->SizeOfImage;
	char* pCurKernelBase = (char*)pLdr->DllBase;

	//3.��ȡntģ����ļ����ݵ��ѿռ�.  
	status = loadNtKernelModule(IN &pLdr->FullDllName, OUT &g_pNewNtKernel);
	if (STATUS_SUCCESS != status)
	{
		return status;
	 }

	//4.�޸���ntģ����ض�λ.  
	fixRelocation(IN g_pNewNtKernel, IN(ULONG)pCurKernelBase);

	//5.ʹ�õ�ǰ����ʹ�õ��ں˵�������������ں˵�SSDT��.   
	initSSDT(g_pNewNtKernel, pCurKernelBase);

	return status;
}



//--------------------------------------KiFastCallEntry-HOOK--------------------------------------
//������ú�����ZwOpenProcess�������ں˵�ZwOpenProcess������ַ����
//���Զ��庯����SSDTFilter��SSDT���˺���.  
//������1��ULONG index          /*������,Ҳ�ǵ��ú�*/
//������2��ULONG tableAddress   /*��ĵ�ַ,������SSDT��ĵ�ַ,Ҳ������Shadow SSDT��ĵ�ַ*/
//������3��ULONG funAddr        /*�ӱ���ȡ���ĺ�����ַ*/
ULONG SSDTFilter(ULONG index,ULONG tableAddress,ULONG funAddr)
{
	// �����SSDT��Ļ�
	if (tableAddress == (ULONG)KeServiceDescriptorTable.ServiceTableBase)
	{
		// �жϵ��ú�(190��ZwOpenProcess�����ĵ��ú�)
		if (index == 190)
		{
			// ������SSDT��ĺ�����ַ,Ҳ�������ں˵ĺ�����ַ
			// �о�����Ҳ���Ի�������������ַ����һ�������µ��ں˵ĺ�����ַ
			return g_pNewSSDTEntry->ServiceTableBase[190];
		}
	}
	// ���ؾɵĺ�����ַ
	return funAddr;
}

//ͨ����������edx��ֵ��SSDT����ȡ���ĺ�����ַ��--->ִ��inline-hookʱ�����ǵ�5�ֽ�---->����ԭ�ȵ�·��
void _declspec(naked) myKiFastCallEntry()
{
	_asm
	{
		pushad;
		pushfd;

		push edx; //�ӱ���ȡ���ĺ�����ַ
		push edi; // ��ĵ�ַ
		push eax; // ���ú�
		call SSDTFilter; // ���ù��˺���

		;// �����������֮��ջ�ռ䲼��,ָ��pushad��
		;// 32λ��ͨ�üĴ���������ջ��,ջ�ռ䲼��Ϊ:
		;// [esp + 00] <=> eflag
		;// [esp + 04] <=> edi
		;// [esp + 08] <=> esi
		;// [esp + 0C] <=> ebp
		;// [esp + 10] <=> esp
		;// [esp + 14] <=> ebx
		;// [esp + 18] <=> edx <<-- ʹ�ú�������ֵ���޸����λ��
		;// [esp + 1C] <=> ecx
		;// [esp + 20] <=> eax
		mov dword ptr ds : [esp + 0x18], eax;
		popfd; // popfdʱ,ʵ����edx��ֵ�ͻر��޸�
		popad;

		; //ִ�б�hook���ǵ�����ָ��
		sub esp, ecx;
		shr ecx, 2;
		jmp g_hookAddr_next_ins;
	}
}

//ж��KiFastCallEntry-HOOK    //ж�ط�����������Ϊ��װ��ʱ����������Ѿ���HOOK���ˣ���ôҪ��ж���Ѱ�װ��
void unstallKiFastCallEntryHook()
{
	if (g_hookAddr)
	{
		//ԭʼ����
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

//��װKiFastCallEntry-HOOK   //MSRȡ��KiFastCallEntry�����׵�ַ--->����HOOKλ��--->�滻��myKiFastEntry
void installKiFastCallEntryHook()
{
	g_hookAddr = 0;

	// 1. �ҵ�KiFastCallEntry�����׵�ַ
	ULONG uKiFastCallEntry = 0;
	_asm
	{
		;// KiFastCallEntry������ַ����
		;// ������ģ��Ĵ�����0x176�żĴ�����
		push ecx;
		push eax;
		push edx;
		mov ecx, 0x176; // ���ñ��
		rdmsr; ;// ��ȡ��edx:eax
		mov uKiFastCallEntry, eax;// ���浽����
		pop edx;
		pop eax;
		pop ecx;
	}

	// 2. �ҵ�HOOK��λ��, ������5�ֽڵ�����
	// 2.1 HOOK��λ��ѡ��Ϊ(�˴�����5�ֽ�,):
	//  2be1            sub     esp, ecx ;
	// 	c1e902          shr     ecx, 2   ;
	UCHAR hookCode[5] = { 0x2b,0xe1,0xc1,0xe9,0x02 }; //����inline hook���ǵ�5�ֽ�
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
		KdPrint(("��KiFastCallEntry������û���ҵ�HOOKλ��,����KiFastCallEntry�Ѿ���HOOK����\n"));
		unstallKiFastCallEntryHook();
		return;
	}

	g_hookAddr = uKiFastCallEntry + i;
	g_hookAddr_next_ins = g_hookAddr + 5;

	// 3. ��ʼinline hook
	UCHAR jmpCode[5] = { 0xe9 };// jmp xxxx
	disablePageWriteProtect();

	// 3.1 ������תƫ��
	// ��תƫ�� = Ŀ���ַ - ��ǰ��ַ - 5
	*(ULONG*)(jmpCode + 1) = (ULONG)myKiFastCallEntry - g_hookAddr - 5;

	// 3.2 ����תָ��д��
	RtlCopyMemory((PUCHAR)(uKiFastCallEntry + i),jmpCode,5);

	enablePageWriteProtect();
}



//----------------------------------------------Sysenter-Hook�����ԡ�������-----------------------------------//
//�Զ��庯��������������̲���ZwOpenProcess��   //�����жϵ��ú��ǲ���0xBE������ж�PID�ǲ��ǵ�������
VOID _declspec(naked) MySysenter()
{
	__asm
	{
		// �����ú�
		cmp eax, 0xBE;	         //0xBE(ZwOpenProcess�ĵ��ú�)
		jne _DONE;		         // ���úŲ���0xBE,ִ�е���ԭ���� Sysenter ����

		push eax;				//����Ĵ���

		mov eax, [edx + 0x14];	// eax������PCLIENT_ID
		mov eax, [eax];			// eax������PID

		cmp eax, [g_uPid];		//�ж��Ƿ���Ҫ�����Ľ��̵�ID
		pop eax;				// �ָ��Ĵ���
		jne _DONE;				// ����Ҫ�����Ľ��̾���ת

		mov[edx + 0xc], 0;		// ��Ҫ�����Ľ��̾ͽ�����Ȩ������Ϊ0���ú�����������ʧ��

	_DONE:
		// ����ԭ����Sysenter
		jmp g_OldSysenter;
	}
}

//��װSysenterHook   //����SYSTENTER_EIP_MSR�Ĵ�����ľɺ�����ַ��Ȼ���Զ��庯����ַд��SYSTENTER_EIP_MSR�Ĵ���
VOID InstallSysenterHook()
{
	__asm
	{
		push edx;						//����Ĵ���
		push eax;
		push ecx;

		//����ԭʼ����
		mov ecx, 0x176;					// SYSTENTER_EIP_MSR�Ĵ����ı��
		rdmsr;							// ��ECXָ����MSR���ص�EDX��EAX
		mov[g_OldSysenter], eax;	   // ���ɵĵ�ַ���浽ȫ�ֱ�����

		//���µĺ������ý�ȥ
		mov eax, MySysenter;
		xor edx, edx;
		wrmsr;							// ��edx:eax д��ECXָ����MSR�Ĵ�����

		pop ecx;						//�ָ��Ĵ���
		pop eax;
		pop edx;

		ret;
	}
}

//ж��SysenterHook  //�����ݵľɺ�����ַ����д��SYSTENTER_EIP_MSR�Ĵ���
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


//�о�Ӧ����˫�ر���
//��һ�ر�����Sysenter-Hook����SYSTENTER_EIP_MSR�м�¼��EIP��ַ�滻�����ǵ��Զ��庯����ַ�����ǵĺ����������һ��
//�൱�ڽ����ں˵�·������

//�ڶ��ر�����KiFastCallEntry-HOOK����SSDT����ȡ���ĺ�����ַ�滻�����µ��ں��е�SSDT��ĺ�����ַ


//SSDT-HOOK,�Ǹ���SSDT���еĺ�����ַ��Ӧ���������˰ɣ�