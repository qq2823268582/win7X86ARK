// CImportDlg.cpp: 实现文件
//

#include "pch.h"
#include "ARK.h"
#include "CImportDlg.h"
#include "afxdialogex.h"
#include "CPeDlg.h"


// CImportDlg 对话框
IMPLEMENT_DYNAMIC(CImportDlg, CDialogEx)

CImportDlg::CImportDlg(PIMAGE_DOS_HEADER pDosHeader, PIMAGE_OPTIONAL_HEADER32 pOptHeader, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_IMPORT, pParent)
	, pDosHeader(pDosHeader)
	, pOptHeader(pOptHeader)
{

}

CImportDlg::~CImportDlg()
{
}

void CImportDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST1, m_list1);
	DDX_Control(pDX, IDC_LIST2, m_list2);
}

BEGIN_MESSAGE_MAP(CImportDlg, CDialogEx)
//	ON_NOTIFY(NM_RCLICK, IDC_LIST1, &CImportDlg::OnNMRClickList1)
//	ON_NOTIFY(NM_CLICK, IDC_LIST1, &CImportDlg::OnNMClickList1)
ON_NOTIFY(NM_CLICK, IDC_LIST1, &CImportDlg::OnNMClickList1)
END_MESSAGE_MAP()

// CImportDlg 消息处理程序
BOOL CImportDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	CRect rect1;
	m_list1.GetClientRect(rect1);
	m_list1.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	m_list1.InsertColumn(0, _T("DllName"), 0, rect1.right / 6);
	m_list1.InsertColumn(1, _T("OriginalFirstThunk"), 0, rect1.right / 6);
	m_list1.InsertColumn(2, _T("TimeDateStamp"), 0, rect1.right / 6);
	m_list1.InsertColumn(3, _T("ForWarderChain"), 0, rect1.right / 6);
	m_list1.InsertColumn(4, _T("Name"), 0, rect1.right / 6);
	m_list1.InsertColumn(5, _T("FirstThunk:"), 0, rect1.right / 6);

	CRect rect2;
	m_list2.GetClientRect(rect2);
	m_list2.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	m_list2.InsertColumn(0, _T("ThunkFOA"), 0, rect2.right / 4);
	m_list2.InsertColumn(1, _T("ThunkValue"), 0, rect2.right / 4);
	m_list2.InsertColumn(2, _T("Hint"), 0, rect2.right / 4);
	m_list2.InsertColumn(3, _T("Api Name"), 0, rect2.right / 4);

	DWORD pImport_Base_RVA = pOptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
	PIMAGE_IMPORT_DESCRIPTOR pImport_Base = PIMAGE_IMPORT_DESCRIPTOR((PUCHAR)pDosHeader + RvaToFoa(pDosHeader, pImport_Base_RVA));

	CString importData;
	int ImportCount = GetImportCount();
	int j = 0;
	for (int i = 0; i < ImportCount; i++, j++)
	{	
		m_list1.InsertItem(j, _T(""));

		//插入DLLNAME
		PCHAR pDllName = PCHAR((PUCHAR)pDosHeader + RvaToFoa(pDosHeader, pImport_Base->Name));
		importData.Format(_T("%S"), pDllName);
		m_list1.SetItemText(j, 0, importData);

		//插入OriginalFirstThunk
		importData.Format(_T("0x%x"), pImport_Base->OriginalFirstThunk);
		m_list1.SetItemText(j, 1, importData);

		//插入TimeDateStamp
		importData.Format(_T("0x%08x"), pImport_Base->TimeDateStamp);
		m_list1.SetItemText(j, 2, importData);

		//插入ForwarderChain
		importData.Format(_T("0x%08x"), pImport_Base->ForwarderChain);
		m_list1.SetItemText(j, 3, importData);

		//插入Name
		importData.Format(_T("0x%x"), pImport_Base->Name);
		m_list1.SetItemText(j, 4, importData);

		//插入FirstThunk
		importData.Format(_T("0x%x"), pImport_Base->FirstThunk);
		m_list1.SetItemText(j, 5, importData);

		pImport_Base++;
	}

	return TRUE;
}


DWORD CImportDlg::RvaToFoa(IN PIMAGE_DOS_HEADER pDosHeader_Base, IN DWORD RVA)
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


//获得导入表的数目
int CImportDlg::GetImportCount()
{		
	DWORD pImport_Base_RVA = pOptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
	PIMAGE_IMPORT_DESCRIPTOR pImport_Base = PIMAGE_IMPORT_DESCRIPTOR((PUCHAR)pDosHeader + RvaToFoa(pDosHeader, pImport_Base_RVA));

	int count = 0;
	while (pImport_Base->OriginalFirstThunk && pImport_Base->FirstThunk)
	{
		count++;
		pImport_Base++;
	}
	
	return count;
}


//获取ThunkData个数
int CImportDlg::GetINTCount(PIMAGE_THUNK_DATA pINT)
{
	int i = 0;	
	while (pINT && *(PDWORD)pINT)
	{
		i++;
		pINT++;
	}	
	return i;
}

//展示DLL信息
VOID CImportDlg::showDLL(int Original)
{
	m_list2.DeleteAllItems();

	PIMAGE_THUNK_DATA pINT = (PIMAGE_THUNK_DATA)((PUCHAR)pDosHeader + RvaToFoa(pDosHeader, Original));

   int num = GetINTCount(pINT);

   CString  ThunkFOA, ThunkValue, Hint, Name;
   int j = 0;
   for (int i = 0; i < num; i++, j++)
   {
	//最高位为1，那么INT数组中的成员的低31位是导入函数的序号
	if (pINT->u1.Ordinal & 0x80000000)
	{
		ThunkFOA.Format(_T("0x%x"), pINT);
		ThunkValue.Format(_T("0x%x"), *(PDWORD)pINT);
		Hint = _T("--");
		Name = _T("--");
	}
	else
	{
		ThunkFOA.Format(_T("0x%x"), pINT);
		ThunkValue.Format(_T("0x%x"), *(PDWORD)pINT);
		PIMAGE_IMPORT_BY_NAME pName = (PIMAGE_IMPORT_BY_NAME)((PUCHAR)pDosHeader + RvaToFoa(pDosHeader, *(PDWORD)pINT));
		Hint.Format(_T("0x%x"), pName->Hint);
		Name.Format(_T("%S"), pName->Name);

	}
	m_list2.InsertItem(j, _T(""));
	m_list2.SetItemText(j, 0, ThunkFOA);
	m_list2.SetItemText(j, 1, ThunkValue);
	m_list2.SetItemText(j, 2, Hint);
	m_list2.SetItemText(j, 3, Name);

	pINT++;
   }
}
//左键点击
void CImportDlg::OnNMClickList1(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	
	int nIndex = m_list1.GetSelectionMark();
	CString str = m_list1.GetItemText(nIndex, 1);	
	int Original = _tcstol(str, NULL, 16);  //妈的，不能用_tstoi，卡了一个晚上
	
	showDLL(Original);
	*pResult = 0;
}
