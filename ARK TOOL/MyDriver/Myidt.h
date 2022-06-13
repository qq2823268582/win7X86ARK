#pragma once
#include "ntifs.h"

//IDT信息结构体
typedef struct _IDTINFO
{
	ULONG pFunction;		//处理函数的地址
	ULONG Selector;			//段选择子
	ULONG ParamCount;		//参数个数
	ULONG Dpl;				//段特权级
	ULONG GateType;			//类型
}IDTINFO, *PIDTINFO;

#pragma pack(1)
//IDT描述符的结构体
typedef struct _IDT_ENTRY
{
	UINT32 uOffsetLow : 16;		//处理程序低地址偏移 
	UINT32 uSelector : 16;		//段选择子
	UINT32 paramCount : 5;		//参数个数
	UINT32 none : 3;			//无，保留
	UINT32 GateType : 4;		//中断类型
	UINT32 StorageSegment : 1;	//为0是系统段，为1是数据段或代码段
	UINT32 DPL : 2;			//特权级
	UINT32 Present : 1;		//段是否有效
	UINT32 uOffsetHigh : 16;		//处理程序高地址偏移
}IDT_ENTER, *PIDT_ENTRY;
#pragma pack()