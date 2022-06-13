#pragma once
#include "ntifs.h"

//IDT��Ϣ�ṹ��
typedef struct _IDTINFO
{
	ULONG pFunction;		//�������ĵ�ַ
	ULONG Selector;			//��ѡ����
	ULONG ParamCount;		//��������
	ULONG Dpl;				//����Ȩ��
	ULONG GateType;			//����
}IDTINFO, *PIDTINFO;

#pragma pack(1)
//IDT�������Ľṹ��
typedef struct _IDT_ENTRY
{
	UINT32 uOffsetLow : 16;		//�������͵�ַƫ�� 
	UINT32 uSelector : 16;		//��ѡ����
	UINT32 paramCount : 5;		//��������
	UINT32 none : 3;			//�ޣ�����
	UINT32 GateType : 4;		//�ж�����
	UINT32 StorageSegment : 1;	//Ϊ0��ϵͳ�Σ�Ϊ1�����ݶλ�����
	UINT32 DPL : 2;			//��Ȩ��
	UINT32 Present : 1;		//���Ƿ���Ч
	UINT32 uOffsetHigh : 16;		//�������ߵ�ַƫ��
}IDT_ENTER, *PIDT_ENTRY;
#pragma pack()