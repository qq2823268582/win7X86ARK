#pragma once
#include "ntifs.h"

#define MAKE_LONG(a, b)  ((LONG)(((UINT16)(((DWORD_PTR)(a)) & 0xffff)) | ((ULONG)((UINT16)(((DWORD_PTR)(b)) & 0xffff))) << 16))
#define MAKE_LONG2(a, b) ((LONG)(((UINT64)(((DWORD_PTR)(a)) & 0xffffff)) | ((ULONG)((UINT64)(((DWORD_PTR)(b)) & 0xff))) << 24))

#pragma pack(1)
typedef struct _GDT_ENTRY
{
	UINT64 Limit0_15 : 16;	//���޳��͵�ַƫ��
	UINT64 base0_23 : 24;	//�λ���ַ�͵�ַƫ��
	UINT64 Type : 4;		//������
	UINT64 S : 1;			//���������ͣ�0ϵͳ�Σ�1���ݶλ����Σ�
	UINT64 DPL : 2;			//��Ȩ�ȼ�
	UINT64 P : 1;			//���Ƿ���Ч
	UINT64 Limit16_19 : 4;	//���޳��ߵ�ַƫ��
	UINT64 AVL : 1;			//�ɹ�ϵͳ���ʹ��
	UINT64 L : 1;
	UINT64 D_B : 1;			//Ĭ�ϲ�����С��0=16λ�Σ�1=32λ�Σ�
	UINT64 G : 1;			//����
	UINT64 base24_31 : 8;	//�λ���ַ�ߵ�ַƫ��
}GDT_ENTER, *PGDT_ENTER;
#pragma pack()

//GDTR��IDTR�Ĵ���ָ��Ľṹ��(X��ʾi��g)
typedef struct _XDT_INFO
{
	UINT16 uXdtLimit;		//Xdt��Χ
	UINT16 xLowXdtBase;		//Xdt�ͻ�ַ
	UINT16 xHighXdtBase;	//Xdt�߻�ַ
}XDT_INFO, *PXDT_INFO;

//GDT��Ϣ�ṹ��
typedef struct _GDTINFO
{
	ULONG BaseAddr;	//�λ�ַ
	ULONG Limit;	//���޳�
	ULONG Grain;	//������
	ULONG Dpl;		//��Ȩ�ȼ�
	ULONG GateType;	//����
}GDTINFO, *PGDTINFO;


