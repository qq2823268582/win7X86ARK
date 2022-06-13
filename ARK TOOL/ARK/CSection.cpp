// CSection.cpp: 实现文件
//

#include "pch.h"
#include "ARK.h"
#include "CSection.h"
#include "afxdialogex.h"
#include "CPeDlg.h"
#include "ARKDlg.h"

// CSection 对话框
IMPLEMENT_DYNAMIC(CSection, CDialogEx)
CSection::CSection(PIMAGE_SECTION_HEADER pSectionHeader, PIMAGE_FILE_HEADER pFileHeader,CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_SECTION, pParent)
	, pSectionHeader(pSectionHeader)
	, pFileHeader(pFileHeader)
{

}

CSection::~CSection()
{
}

void CSection::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST1, m_list);
}

BEGIN_MESSAGE_MAP(CSection, CDialogEx)
END_MESSAGE_MAP()

// CSection 消息处理程序
BOOL CSection::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	
	//1.插入6列		
	CRect rect;
	m_list.GetClientRect(rect);
	m_list.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	m_list.InsertColumn(0, _T("节名"), 0, rect.right / 6);
	m_list.InsertColumn(1, _T("VirtualAddress"), 0, rect.right / 6);
	m_list.InsertColumn(2, _T("VirtualSize"), 0, rect.right / 6);
	m_list.InsertColumn(3, _T("PointerToRawData"), 0, rect.right / 6);
	m_list.InsertColumn(4, _T("SizeOfRawData"), 0, rect.right / 6);
	m_list.InsertColumn(5, _T("Characteristics:"), 0, rect.right / 6);


	//2.获得节表数
	int SecNum = pFileHeader->NumberOfSections;

	//3.循环添加区段信息
	CString strTempInfo;
	for (int i = 0; i < SecNum; i++)
	{
		//区段名
		strTempInfo = "";
		strTempInfo.AppendFormat(_T("%.8S"), pSectionHeader->Name);  //因为名字长度为8，不以0结尾，所以只能%.8S
		m_list.InsertItem(i, strTempInfo.GetBuffer());

		//区段内存偏移
		strTempInfo = "";
		strTempInfo.AppendFormat(_T("%p"), pSectionHeader->VirtualAddress);
		m_list.SetItemText(i, 1, strTempInfo.GetBuffer());

		//区段在内存中的大小
		strTempInfo = "";
		strTempInfo.AppendFormat(_T("%p"), pSectionHeader->Misc);
		m_list.SetItemText(i, 2, strTempInfo.GetBuffer());

		//区段文件偏移
		strTempInfo = "";
		strTempInfo.AppendFormat(_T("%p"), pSectionHeader->PointerToRawData);
		m_list.SetItemText(i, 3, strTempInfo.GetBuffer());

		//区段在文件中的大小
		strTempInfo = "";
		strTempInfo.AppendFormat(_T("%p"), pSectionHeader->SizeOfRawData);
		m_list.SetItemText(i, 4, strTempInfo.GetBuffer());

		//区段标志
		strTempInfo = "";
		strTempInfo.AppendFormat(_T("%p"), pSectionHeader->Characteristics);
		m_list.SetItemText(i, 5, strTempInfo.GetBuffer());

		pSectionHeader++;
	}

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}