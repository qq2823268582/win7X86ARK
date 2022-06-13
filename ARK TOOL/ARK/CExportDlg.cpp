// CExportDlg.cpp: 实现文件
//

#include "pch.h"
#include "ARK.h"
#include "CExportDlg.h"
#include "afxdialogex.h"
#include "ARKDlg.h"
#include "CPeDlg.h"

// CExportDlg 对话框
IMPLEMENT_DYNAMIC(CExportDlg, CDialogEx)
CExportDlg::CExportDlg(PIMAGE_DOS_HEADER pDosHeader, PIMAGE_OPTIONAL_HEADER32 pOptHeader, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_EXPORT, pParent)
	, pDosHeader(pDosHeader)
	, pOptHeader(pOptHeader)
{

}

CExportDlg::~CExportDlg()
{
}

void CExportDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST1, m_list);
}

BEGIN_MESSAGE_MAP(CExportDlg, CDialogEx)
END_MESSAGE_MAP()

// CExportDlg 消息处理程序
BOOL CExportDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	DWORD pExport_Offset = pOptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
	PIMAGE_EXPORT_DIRECTORY pExport_Base = (PIMAGE_EXPORT_DIRECTORY)((PUCHAR)pDosHeader + RvaToFoa(pDosHeader, pExport_Offset));

	//编辑框控件的第一个ID值
	DWORD dwIndex = IDC_EDIT1;

	//1.属性
	CString szTemp;
	szTemp.AppendFormat(_T("%p"), pExport_Base->Characteristics);
	SetDlgItemText(dwIndex, szTemp.GetBuffer());
	dwIndex++;

	//2.时间戳
	szTemp = "";
	szTemp.AppendFormat(_T("%p"), pExport_Base->TimeDateStamp);
	SetDlgItemText(dwIndex, szTemp.GetBuffer());
	dwIndex++;

	//3.基址
	szTemp = "";
	szTemp.AppendFormat(_T("%x"), pExport_Base->Base);
	SetDlgItemText(dwIndex, szTemp.GetBuffer());
	dwIndex++;

	//4.name rva
	szTemp = "";
	szTemp.AppendFormat(_T("0x%x"), pExport_Base->Name);
	SetDlgItemText(dwIndex, szTemp.GetBuffer());
	dwIndex++;

	//5.name
	szTemp = "";
	PCHAR pName = PCHAR((PUCHAR)pDosHeader + RvaToFoa(pDosHeader, pExport_Base->Name));
	szTemp.AppendFormat(_T("%S"), pName);
	SetDlgItemText(dwIndex, szTemp.GetBuffer());
	dwIndex++;


	//6.NumberOfFunctions
	szTemp = "";
	szTemp.AppendFormat(_T("%x"), pExport_Base->NumberOfFunctions);
	SetDlgItemText(dwIndex, szTemp.GetBuffer());
	dwIndex++;

	//7.NumberOfNames
	szTemp = "";
	szTemp.AppendFormat(_T("%x"), pExport_Base->NumberOfNames);
	SetDlgItemText(dwIndex, szTemp.GetBuffer());
	dwIndex++;

	//8.AddressOfFunctions
	szTemp = "";
	szTemp.AppendFormat(_T("0x%x"), pExport_Base->AddressOfFunctions);
	SetDlgItemText(dwIndex, szTemp.GetBuffer());
	dwIndex++;

	//9.AddressOfNames
	szTemp = "";
	szTemp.AppendFormat(_T("0x%x"), pExport_Base->AddressOfNames);
	SetDlgItemText(dwIndex, szTemp.GetBuffer());
	dwIndex++;

	//10.AddressOfNameOrdinals
	szTemp = "";
	szTemp.AppendFormat(_T("0x%x"), pExport_Base->AddressOfNameOrdinals);
	SetDlgItemText(dwIndex, szTemp.GetBuffer());
	dwIndex++;

	UpdateData(FALSE); //妈的，缺了这个东西，下面的列表出不来
	
	CRect rect;
	m_list.GetClientRect(rect);
	m_list.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	m_list.InsertColumn(0, _T("Ordinal"), 0, rect.right / 3);
	m_list.InsertColumn(1, _T("RVA"), 0, rect.right / 3);
	m_list.InsertColumn(2, _T("Function Name"), 0, rect.right / 3);

	//获得地址表的基址
	LPDWORD pFuncAdd_Base = (LPDWORD)((PUCHAR)pDosHeader + RvaToFoa(pDosHeader, pExport_Base->AddressOfFunctions));
	//获得名称表的基址
	LPDWORD pNameAdd_Base = (LPDWORD)((PUCHAR)pDosHeader + RvaToFoa(pDosHeader, pExport_Base->AddressOfNames));
	//获得序号表的基址
	LPWORD pOrdiAdd_Base = (LPWORD)((PUCHAR)pDosHeader + RvaToFoa(pDosHeader, pExport_Base->AddressOfNameOrdinals));

	CString strTemp;
	int j = 0;
	for (DWORD i = 0; i < pExport_Base->NumberOfNames; i++, j++)
	{
		m_list.InsertItem(j, _T(""));

		//取出序号表中的序号
		DWORD Ord = pOrdiAdd_Base[i];
		strTemp.Format(_T("%d"), Ord);
		m_list.SetItemText(j, 0,strTemp);

		//序号加上基址作为数组下标去地址表找到函数地址（RVA）
		DWORD funRVA = pFuncAdd_Base[Ord+pExport_Base->Base];
		strTemp.Format(_T("0x%x"), funRVA);
		m_list.SetItemText(j, 1, strTemp);

		//索引作为数组下标去名称表找到函数名称地址（RVA）
		DWORD NameRVA = pNameAdd_Base[i];
		PCHAR NameAdd = PCHAR((PUCHAR)pDosHeader + RvaToFoa(pDosHeader, NameRVA));
		strTemp.Format(_T("%S"), NameAdd);
		m_list.SetItemText(j, 2, strTemp);		
	}

	return TRUE;
}


DWORD CExportDlg::RvaToFoa(IN PIMAGE_DOS_HEADER pDosHeader_Base, IN DWORD RVA)
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

