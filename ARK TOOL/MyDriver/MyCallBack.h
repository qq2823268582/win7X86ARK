#pragma once
#include "ntifs.h"
#include "ntstrsafe.h"

typedef struct _NOTIFYINFO
{
	ULONG	CallbackType;
	ULONG	CallbacksAddr;
	ULONGLONG	Cookie;           // just work to cmpcallback
	WCHAR	ImgPath[260];
}NOTIFYINFO, *PNOTIFYINFO;

typedef struct _OBCALLBACKINFO
{
	ULONG	    ObjectAddress;     //回调对象地址
	ULONG	    ObjectType;        //回调类型
	ULONG	    CallType;          //函数类型
	WCHAR	    Altitude[260];     //回调高度
	ULONG       FuncAddress;       //函数地址
	WCHAR	    ImgPath[260]; //所属模块
}OBCALLBACKINFO, *POBCALLBACKINFO;

//-----------------------------------------------------------------------------------------------------------
//1.OBJECT_TYPE_INITIALIZER没啥用，就是为了填充OBJECT_TYPE中的成员OBJECT_TYPE_INITIALIZER TypeInfo
typedef struct _OBJECT_TYPE_INITIALIZER
{
	USHORT Length;					  // Uint2B
	UCHAR ObjectTypeFlags;			  // UChar
	ULONG ObjectTypeCode;			  // Uint4B
	ULONG InvalidAttributes;		  // Uint4B
	GENERIC_MAPPING GenericMapping;	  // _GENERIC_MAPPING
	ULONG ValidAccessMask;			 // Uint4B
	ULONG RetainAccess;				  // Uint4B
	POOL_TYPE PoolType;				 // _POOL_TYPE
	ULONG DefaultPagedPoolCharge;	 // Uint4B
	ULONG DefaultNonPagedPoolCharge; // Uint4B
	PVOID DumpProcedure;			 // Ptr64     void
	PVOID OpenProcedure;			// Ptr64     long
	PVOID CloseProcedure;			// Ptr64     void
	PVOID DeleteProcedure;				// Ptr64     void
	PVOID ParseProcedure;			// Ptr64     long
	PVOID SecurityProcedure;			// Ptr64     long
	PVOID QueryNameProcedure;			// Ptr64     long
	PVOID OkayToCloseProcedure;			// Ptr64     unsigned char
#if (NTDDI_VERSION >= NTDDI_WINBLUE)    // Win8.1
	ULONG WaitObjectFlagMask;			// Uint4B
	USHORT WaitObjectFlagOffset;		// Uint2B
	USHORT WaitObjectPointerOffset;		// Uint2B
#endif
}OBJECT_TYPE_INITIALIZER, *POBJECT_TYPE_INITIALIZER;

//2.OBJECT_TYPE中的最后一个成员：LIST_ENTRY CallbackList就是对象回调链表
typedef struct _OBJECT_TYPE
{
	LIST_ENTRY TypeList;			     // _LIST_ENTRY
	UNICODE_STRING Name;				 // _UNICODE_STRING
	PVOID DefaultObject;				 // Ptr64 Void
	UCHAR Index;						 // UChar
	ULONG TotalNumberOfObjects;			 // Uint4B
	ULONG TotalNumberOfHandles;			 // Uint4B
	ULONG HighWaterNumberOfObjects;		 // Uint4B
	ULONG HighWaterNumberOfHandles;		 // Uint4B
	OBJECT_TYPE_INITIALIZER TypeInfo;	 // _OBJECT_TYPE_INITIALIZER
	EX_PUSH_LOCK TypeLock;				 // _EX_PUSH_LOCK
	ULONG Key;						     // Uint4B
	LIST_ENTRY CallbackList;			 // _LIST_ENTRY   ------------------------->OBCALLBACK链表
}OBJECT_TYPE, *POBJECT_TYPE;

//3.CALL_BACK_INFO填充的是OBCALLBACK中的成员：PCALL_BACK_INFO  ObHandle;
//CALL_BACK_INFO中的成员Altitude非常重要
typedef struct _CALL_BACK_INFO
{
	ULONG Unknow;
	ULONG Unknow1;
	UNICODE_STRING Altitude; //------------------------>重要！！！！
	LIST_ENTRY NextEntryItemList; //(callbacklist) 跟上面开头的那个一样 存储下一个callbacklist
	ULONG Operations;
	PVOID ObHandle; //存储详细的数据 版本号 POB_OPERATION_REGISTRATION AltitudeString 也就是本身节点CALL_BACK_INFO 注销时也使用这个 注意是指针 //CALL_BACK_INFO
	PVOID ObjectType;
	ULONG PreCallbackAddr;
	ULONG PostCallbackAddr;
}CALL_BACK_INFO, *PCALL_BACK_INFO;

#pragma pack(1)
//OBCALLBACK结构
typedef struct _OB_CALLBACK
{
	LIST_ENTRY ListEntry;
	ULONGLONG Unknown;
	PCALL_BACK_INFO  ObHandle;
	PVOID   ObTypeAddr;
	PVOID	PreCall;
	PVOID   PostCall;
}OB_CALLBACK, *POB_CALLBACK;
#pragma pack()

PUNICODE_STRING  FindKernelModule(PDRIVER_OBJECT pDriverObj, ULONG funcAddr);
void EnumCreateProcessNotify(PDRIVER_OBJECT pDriverObj, PNOTIFYINFO* pnotify);
void EnumCreateThreadNotify(PDRIVER_OBJECT pDriverObj, PNOTIFYINFO* pnotify);
VOID EnumImageNotify(PDRIVER_OBJECT pDriverObj, PNOTIFYINFO* pnotify);
VOID EnumCallBackList(PDRIVER_OBJECT pDriverObj, PNOTIFYINFO* pnotify);

VOID EnumProcessObCallback(PDRIVER_OBJECT pDriverObj, POBCALLBACKINFO* pobcallback);
VOID EnumThreadObCallback(PDRIVER_OBJECT pDriverObj, POBCALLBACKINFO* pobcallback);