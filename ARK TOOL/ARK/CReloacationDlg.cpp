// CReloacationDlg.cpp: 实现文件
//

#include "pch.h"
#include "ARK.h"
#include "CReloacationDlg.h"
#include "afxdialogex.h"
#include "CPeDlg.h"

// CReloacationDlg 对话框
IMPLEMENT_DYNAMIC(CReloacationDlg, CDialogEx)
CReloacationDlg::CReloacationDlg(PIMAGE_DOS_HEADER pDosHeader, PIMAGE_OPTIONAL_HEADER32 pOptHeader, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_RELOCATION, pParent)
	, pDosHeader(pDosHeader)
	, pOptHeader(pOptHeader)
{

}

CReloacationDlg::~CReloacationDlg()
{
}

void CReloacationDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST1, m_list1);
	DDX_Control(pDX, IDC_LIST2, m_list2);
}

BEGIN_MESSAGE_MAP(CReloacationDlg, CDialogEx)
	ON_NOTIFY(NM_CLICK, IDC_LIST1, &CReloacationDlg::OnNMClickList1)
END_MESSAGE_MAP()

//CReloacationDlg 消息处理程序
BOOL CReloacationDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	CRect rect1;
	m_list1.GetClientRect(rect1);
	m_list1.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);	
	m_list1.InsertColumn(0, _T("VirtualAddress(RVA)"), 0, rect1.right / 4);
	m_list1.InsertColumn(1, _T("Sizeofblock"), 0, rect1.right / 4);
	m_list1.InsertColumn(2, _T("Count"), 0, rect1.right / 4);
	m_list1.InsertColumn(3, _T("pRelSection"), 0, rect1.right / 4);

	CRect rect2;
	m_list2.GetClientRect(rect2);
	m_list2.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	m_list2.InsertColumn(0, _T("High_4"), 0, rect2.right / 3);
	m_list2.InsertColumn(1, _T("RVA"), 0, rect2.right / 3);
	m_list2.InsertColumn(2, _T("Data"), 0, rect2.right / 3);

	DWORD pRelocationBase_RVA = pOptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress;
	PIMAGE_BASE_RELOCATION pRel = (PIMAGE_BASE_RELOCATION)((PUCHAR)pDosHeader + RvaToFoa(pDosHeader, pRelocationBase_RVA));

	int RelocationCount = GetRelocationCount(pRel);
	CString Temp;
	int j = 0;
	for (int i = 0; i < RelocationCount; i++, j++)
	{
		m_list1.InsertItem(j, _T(""));

		//插入VirtualAddress(RVA)
		DWORD pRelocationBlockBase_RVA = pRel->VirtualAddress;
		Temp.Format(_T("0x%x"), pRelocationBlockBase_RVA);
		m_list1.SetItemText(j, 0, Temp);

		//插入Sizeofblock
		DWORD pRelocationBlock_Size = pRel->SizeOfBlock;
		Temp.Format(_T("0x%x"), pRelocationBlock_Size);
		m_list1.SetItemText(j, 1, Temp);

		//插入Count
		DWORD dwRelEntry = (pRel->SizeOfBlock - 8) / 2;
		Temp.Format(_T("%d"), dwRelEntry);
		m_list1.SetItemText(j, 2, Temp);

		//插入重定位块中的重定位数组的基址
		PWORD pRelData_Base = (PWORD)pRel + 8;
		Temp.Format(_T("0x%x"), pRelData_Base);
		m_list1.SetItemText(j, 3, Temp);

		pRel = (PIMAGE_BASE_RELOCATION)((PCHAR)pRel + pRel->SizeOfBlock);
	}

	return TRUE;
}

DWORD CReloacationDlg::RvaToFoa(IN PIMAGE_DOS_HEADER pDosHeader_Base, IN DWORD RVA)
{
	//2.获取NT头基址
	PIMAGE_NT_HEADERS pNTHeader_Base = PIMAGE_NT_HEADERS((PUCHAR)pDosHeader_Base + pDosHeader_Base->e_lfanew);
	//3.获取文件头基址
	PIMAGE_FILE_HEADER pFileHeader_Base = (PIMAGE_FILE_HEADER)((PUCHAR)pNTHeader_Base + 4);
	//4.获取可选头基址	
	PIMAGE_OPTIONAL_HEADER pOptionHeader_Base = (PIMAGE_OPTIONAL_HEADER)((PUCHAR)pFileHeader_Base + sizeof(IMAGE_FILE_HEADER));
	//5.获取节表头数组基址
	PIMAGE_SECTION_HEADER pSectionHeaderGroup_Base = (PIMAGE_SECTION_HEADER)((PUCHAR)pOptionHeader_Base + pFileHeader_Base->SizeOfOptionalHeader);

	//--------------------------2.当RVA在文件头内时----------------------------------
	if (RVA < pOptionHeader_Base->SizeOfHeaders)
	{
		return RVA;
	}

	//--------------------------3.当RVA在节区内时---------------------------------------------
	for (int i = 0; i < pFileHeader_Base->NumberOfSections; i++)
	{
		//1.获取节头偏移（RVA类型）
		DWORD SectionHead_Offset = pSectionHeaderGroup_Base[i].VirtualAddress;
		//2.获取节真实大小
		DWORD Section_Size = pSectionHeaderGroup_Base[i].Misc.VirtualSize;
		//3.获取节尾偏移（RVA类型）
		DWORD SectionTail_Offset = SectionHead_Offset + Section_Size;
		//4.判断RVA是否在节区内（SectionHead_Offset <= RVA< SectionTail_Offset）
		if (SectionHead_Offset <= RVA && RVA < SectionTail_Offset)
		{
			DWORD FOA = pSectionHeaderGroup_Base[i].PointerToRawData + (RVA - SectionHead_Offset);
			return FOA;
		}
	}
	//--------------------------4.其它情况：RVA不在有效范围----------------------------------
	printf("RVA不在有效范围，转换失败！\n");
	return 0;
}

int CReloacationDlg::GetRelocationCount(PIMAGE_BASE_RELOCATION pRel)
{
	int count = 0;
	while (*(PLONGLONG)pRel)
	{
		count++;
		pRel = (PIMAGE_BASE_RELOCATION)((PCHAR)pRel + pRel->SizeOfBlock);
	}
	return count;
}

void CReloacationDlg::OnNMClickList1(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	//获得行号
	int nIndex = m_list1.GetSelectionMark();
	//获得这一行的第4个的内容
	CString str1 = m_list1.GetItemText(nIndex, 3);	
	int pRelData_Base = _tcstol(str1, NULL, 16);  
	//获得这一行的第3个的内容
	CString str2 = m_list1.GetItemText(nIndex, 2);
	int Count = _tstoi((LPCTSTR)str2);
	//获得这一行的第1个的内容
	CString str3 = m_list1.GetItemText(nIndex, 0);
	int pRelocationBlockBase_RVA = _tcstol(str3, NULL, 16);

	//展示这一行中的重定位块
	showRelocationBlock(pRelData_Base, Count, pRelocationBlockBase_RVA);

	*pResult = 0;
}


VOID CReloacationDlg::showRelocationBlock(int pRelData_Base, int Count,int pRelocationBlockBase_RVA)
{
	m_list2.DeleteAllItems();

	PWORD Data_Base = (PWORD)pRelData_Base;

	CString Temp;
	int j = 0;
	for (int i = 0; i < Count; i++,j++)
	{
		m_list2.InsertItem(j, _T(""));

		//插入HIGH_4
		WORD dwHigh_4 = (Data_Base[i] & 0xF000) >> 12;
		Temp.Format(_T("%d"), dwHigh_4);
		m_list2.SetItemText(j, 0, Temp);

		//插入RVA
		WORD dwLow_12 = Data_Base[i] & 0xFFF;
		DWORD dwDataRVA = dwLow_12 + pRelocationBlockBase_RVA;
		Temp.Format(_T("0x%x"), dwDataRVA);
		m_list2.SetItemText(j, 1, Temp);

		//插入DATA
		DWORD dwDataFOA = RvaToFoa(pDosHeader, dwDataRVA);
		PDWORD pData = PDWORD((DWORD)pDosHeader + dwDataFOA);
		Temp.Format(_T("0x%x"), *pData);
		m_list2.SetItemText(j, 2, Temp);

	}
}