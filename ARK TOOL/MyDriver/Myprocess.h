#pragma once
#include "ntifs.h"

//-------------------------------------------------进程窗口相关的各种结构体----------------------------------
//进程信息结构体
typedef struct _PROCESSINFO
{
	WCHAR szpath[260];//映像路径
	ULONG PID;		  //进程ID
	ULONG PPID;		  //进程ID
}PROCESSINFO, *PPROCESSINFO;

//模块信息结构体
typedef struct _MODULEINFO
{
	WCHAR wcModuleFullPath[260];	//模块路径
	ULONG DllBase;		            //基地址
	ULONG SizeOfImage;	            //大小
}MODULEINFO, *PMODULEINFO;

//LDR结构体
typedef struct _LDR_DATA_TABLE_ENTRY
{
	LIST_ENTRY InLoadOrderLinks;
	LIST_ENTRY InMemoryOrderLinks;
	LIST_ENTRY InInitializationOrderLinks;
	PVOID DllBase;
	PVOID EntryPoint;
	ULONG SizeOfImage;
	UNICODE_STRING FullDllName;
	UNICODE_STRING BaseDllName;
	ULONG Flags;
	USHORT LoadCount;
	USHORT TlsIndex;
	union
	{
		LIST_ENTRY HashLinks;
		struct {
			PVOID SectionPointer;
			ULONG CheckSum;
		}s1;
	}u1;
	union
	{
		struct {
			ULONG TimeDateStamp;
		}s2;
		struct {
			PVOID LoadedImports;
		}s3;
	}u2;
}LDR_DATA_TABLE_ENTRY, *PLDR_DATA_TABLE_ENTRY;

//线程信息结构体
typedef struct _THREADINFO
{
	ULONG Tid;		//线程ID
	ULONG pEthread;	//线程执行块地址
	ULONG pTeb;		//Teb结构地址
	ULONG BasePriority;	//静态优先级
	ULONG ContextSwitches;	//切换次数
}THREADINFO, *PTHREADINFO;

//VAD信息结构体
typedef struct _VADINFO
{
	ULONG pVad;	    //Vad地址
	ULONG Start;    //内存块起始地址
	ULONG End;      //内存块结束地址
	ULONG Commit;	//已提交页
	ULONG Type;	    //内存类型
	ULONG Protection;//内存保护方式
	WCHAR FileObjectName[260];	//文件对象名
}VADINFO, *PVADINFO;


//--------------------------------------------------杀死进程相关的---------------------------------------------
typedef enum _KAPC_ENVIRONMENT
{
	OriginalApcEnvironment,
	AttachedApcEnvironment,
	CurrentApcEnvironment,
	InsertApcEnvironment
} KAPC_ENVIRONMENT;

typedef VOID(*PKNORMAL_ROUTINE) (
	IN PVOID NormalContext,
	IN PVOID SystemArgument1,
	IN PVOID SystemArgument2
	);

typedef VOID(*PKKERNEL_ROUTINE) (
	IN struct _KAPC *Apc,
	IN OUT PKNORMAL_ROUTINE *NormalRoutine,
	IN OUT PVOID *NormalContext,
	IN OUT PVOID *SystemArgument1,
	IN OUT PVOID *SystemArgument2
	);

typedef VOID(*PKRUNDOWN_ROUTINE) (IN struct _KAPC *Apc);

VOID NTAPI KeInitializeApc
(
	__in PKAPC   Apc,
	__in PKTHREAD     Thread,
	__in KAPC_ENVIRONMENT     TargetEnvironment,
	__in PKKERNEL_ROUTINE     KernelRoutine,
	__in_opt PKRUNDOWN_ROUTINE    RundownRoutine,
	__in PKNORMAL_ROUTINE     NormalRoutine,
	__in KPROCESSOR_MODE  Mode,
	__in PVOID    Context
);

BOOLEAN  NTAPI KeInsertQueueApc
(
	IN PKAPC Apc,
	IN PVOID SystemArgument1,
	IN PVOID SystemArgument2,
	IN KPRIORITY PriorityBoost
);

ULONG GetPspTerminateThreadByPointer();
ULONG GetPspExitThread(ULONG BaseAddr);
VOID SelfTerminateThread(KAPC *Apc, PKNORMAL_ROUTINE *NormalRoutine, PVOID *NormalContext, PVOID *SystemArgument1, PVOID *SystemArgument2);
VOID KillProcess(HANDLE hPid);
ULONG GetActiveListOffset();
VOID HideProcess(HANDLE hPid);


//----------------------------------------------进程保护相关的----------------------------------------
typedef struct _HANDLE_TABLE
{
	PVOID TableCode;
	PEPROCESS QuotaProcess;
	HANDLE UniqueProcessId;
	ULONG HandleTableLock;
	LIST_ENTRY HandleTableList;
	ULONG HandleContentionEvent;
	PVOID DebugInfo;
	LONG  ExtraInfoPages;
	ULONG Flags;
	ULONG FirstFreeHandle;
	PVOID LastFreeHandleEntry;
	ULONG HandleCount;
	ULONG NextHandleNeedingPool;
	ULONG HandleCountHighWatermark;
} HANDLE_TABLE, *PHANDLE_TABLE;

typedef struct _HANDLE_TABLE_ENTRY
{
	union {
		PVOID Object;
		ULONG ObAttributes;
		PVOID InfoTable;
		PVOID Value;
	};
	union {
		union {
			ULONG GrantedAccess;
			struct {
				USHORT GrantedAccessIndex;
				USHORT CreatorBackTraceIndex;
			};
		};
		ULONG NextFreeTableEntry;
	};
} HANDLE_TABLE_ENTRY, *PHANDLE_TABLE_ENTRY;

NTSTATUS EnumTable0(ULONG TableCode, PEPROCESS CurEProcess, PEPROCESS TargetProcess);
NTSTATUS EnumTable1(ULONG TableCode, PEPROCESS CurEProcess, PEPROCESS TargetProcess);
NTSTATUS EnumTable2(ULONG TableCode, PEPROCESS CurEProcess, PEPROCESS TargetProcess);

NTSTATUS SeScanProcessHandleTable(PEPROCESS CurEProcess, PEPROCESS TargetProcess);
VOID     ProtectTheProcess(HANDLE hPid);
NTSTATUS CreateKeThread1(PEPROCESS TargetProcess);

//NTSTATUS ZeroDebugPort(PEPROCESS TargetProcess);
//NTSTATUS CreateKeThread2(PEPROCESS TargetProcess);


#define PROCESS_TERMINATE                  (0x0001)  
#define PROCESS_CREATE_THREAD              (0x0002)  
#define PROCESS_SET_SESSIONID              (0x0004)  
#define PROCESS_VM_OPERATION               (0x0008)  
#define PROCESS_VM_READ                    (0x0010)  
#define PROCESS_VM_WRITE                   (0x0020)  


//----------------------------------------------------------VAD相关-------------------------------------
#pragma pack(push,1)
typedef struct _EX_FAST_REF
{
	union
	{
		PVOID Object;
		ULONG_PTR RefCnt : 3;
		ULONG_PTR Value;
	};
} EX_FAST_REF, *PEX_FAST_REF;
#pragma pack(pop)

struct _SEGMENT
{
	struct _CONTROL_AREA* ControlArea;
	ULONG TotalNumberOfPtes;
	ULONG SegmentFlags;
	ULONG NumberOfCommittedPages;
	ULONGLONG SizeOfSegment;          //重要
	union
	{
		struct _MMEXTEND_INFO* ExtendInfo;
		void* BasedAddress;           //重要
	};
	EX_PUSH_LOCK SegmentLock;
	ULONG u1;
	ULONG u2;
	struct _MMPTE* PrototypePte;
	//_MMPTE ThePtes[0x1];
};

struct _CONTROL_AREA
{
	struct _SEGMENT* Segment;             //重要
	struct _LIST_ENTRY DereferenceList;
	ULONG NumberOfSectionReferences;
	ULONG NumberOfPfnReferences;
	ULONG NumberOfMappedViews;
	ULONG NumberOfUserReferences;
	ULONG  u;
	ULONG FlushInProgressCount;
	struct _EX_FAST_REF FilePointer;   //重要
};

struct _SUBSECTION
{
	struct _CONTROL_AREA* ControlArea;   //重要
	struct _MMPTE* SubsectionBase;
	struct _SUBSECTION* NextSubsection;
	ULONG PtesInSubsection;
	ULONG UnusedPtes;
	struct _MM_AVL_TABLE* GlobalPerSessionHead;
	ULONG u;
	ULONG StartingSector;
	ULONG NumberOfFullSectors;
};

#pragma pack(push,1)
struct _MMVAD_FLAGS
{
	ULONG CommitCharge : 19;                                                  //0x0
	ULONG NoChange : 1;                                                       //0x0
	ULONG VadType : 3;                                                        //0x0
	ULONG MemCommit : 1;                                                      //0x0
	ULONG Protection : 5;                                                     //0x0
	ULONG Spare : 2;                                                          //0x0
	ULONG PrivateMemory : 1;                                                  //0x0
};
#pragma pack(pop)

#pragma pack(push,1)
struct _MMVAD_FLAGS2
{
	ULONG FileOffset : 24;                                                    //0x0
	ULONG SecNoChange : 1;                                                    //0x0
	ULONG OneSecured : 1;                                                     //0x0
	ULONG MultipleSecured : 1;                                                //0x0
	ULONG Spare : 1;                                                          //0x0
	ULONG LongVad : 1;                                                        //0x0
	ULONG ExtendableFile : 1;                                                 //0x0
	ULONG Inherit : 1;                                                        //0x0
	ULONG CopyOnWrite : 1;                                                    //0x0
};
#pragma pack(pop)

typedef struct _MMVAD
{
	ULONG u1;                  //重要
	struct _MMVAD* LeftChild;  //重要
	struct _MMVAD* RightChild; //重要
	ULONG StartingVpn;         //重要
	ULONG EndingVpn;           //重要
	union
	{
		ULONG LongFlags;
		struct _MMVAD_FLAGS VadFlags;
	} u;                                              //重要
	EX_PUSH_LOCK PushLock;
	ULONG u5;
	union
	{
		ULONG LongFlags2;
		struct _MMVAD_FLAGS2 VadFlags2;
	} u2;                                                     //重要
	struct _SUBSECTION* Subsection;  //重要
	struct _MSUBSECTION* MappedSubsection;
	struct _MMPTE* FirstPrototypePte;
	struct _MMPTE* LastContiguousPte;
	struct _LIST_ENTRY ViewLinks;
	struct _EPROCESS* VadsProcess;
}MMVAD;

typedef struct _MMADDRESS_NODE
{
	ULONG u1;
	struct _MMADDRESS_NODE* LeftChild;
	struct _MMADDRESS_NODE* RightChild;
	ULONG StartingVpn;
	ULONG EndingVpn;
}MMADDRESS_NODE, *PMMADDRESS_NODE;

typedef struct _MM_AVL_TABLE
{
	MMADDRESS_NODE BalancedRoot;
	ULONG sTruct;
	PVOID NodeHint;
	PVOID NodeFreeHint;
}MM_AVL_TABLE, *PMM_AVL_TABLE;

VOID EnumVADCount(IN PMMADDRESS_NODE pVad, OUT PULONG pVadCount);
ULONG GetVADCount(HANDLE hPid);
VOID _EnumVAD(PMMADDRESS_NODE pVad, PVADINFO* ppOutVADInfo);
NTSTATUS _EnumProcessVAD(HANDLE hPid, PVADINFO pOutVADInfo);