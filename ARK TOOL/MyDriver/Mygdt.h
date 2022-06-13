#pragma once
#include "ntifs.h"

#define MAKE_LONG(a, b)  ((LONG)(((UINT16)(((DWORD_PTR)(a)) & 0xffff)) | ((ULONG)((UINT16)(((DWORD_PTR)(b)) & 0xffff))) << 16))
#define MAKE_LONG2(a, b) ((LONG)(((UINT64)(((DWORD_PTR)(a)) & 0xffffff)) | ((ULONG)((UINT64)(((DWORD_PTR)(b)) & 0xff))) << 24))

#pragma pack(1)
typedef struct _GDT_ENTRY
{
	UINT64 Limit0_15 : 16;	//段限长低地址偏移
	UINT64 base0_23 : 24;	//段基地址低地址偏移
	UINT64 Type : 4;		//段类型
	UINT64 S : 1;			//描述符类型（0系统段，1数据段或代码段）
	UINT64 DPL : 2;			//特权等级
	UINT64 P : 1;			//段是否有效
	UINT64 Limit16_19 : 4;	//段限长高地址偏移
	UINT64 AVL : 1;			//可供系统软件使用
	UINT64 L : 1;
	UINT64 D_B : 1;			//默认操作大小（0=16位段，1=32位段）
	UINT64 G : 1;			//粒度
	UINT64 base24_31 : 8;	//段基地址高地址偏移
}GDT_ENTER, *PGDT_ENTER;
#pragma pack()

//GDTR与IDTR寄存器指向的结构体(X表示i或g)
typedef struct _XDT_INFO
{
	UINT16 uXdtLimit;		//Xdt范围
	UINT16 xLowXdtBase;		//Xdt低基址
	UINT16 xHighXdtBase;	//Xdt高基址
}XDT_INFO, *PXDT_INFO;

//GDT信息结构体
typedef struct _GDTINFO
{
	ULONG BaseAddr;	//段基址
	ULONG Limit;	//段限长
	ULONG Grain;	//段粒度
	ULONG Dpl;		//特权等级
	ULONG GateType;	//类型
}GDTINFO, *PGDTINFO;


