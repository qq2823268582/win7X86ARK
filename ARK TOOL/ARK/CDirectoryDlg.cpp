// CDirectoryDlg.cpp: 实现文件
//

#include "pch.h"
#include "ARK.h"
#include "CDirectoryDlg.h"
#include "afxdialogex.h"
#include "CPeDlg.h"

// CDirectoryDlg 对话框
IMPLEMENT_DYNAMIC(CDirectoryDlg, CDialogEx)
CDirectoryDlg::CDirectoryDlg(PIMAGE_OPTIONAL_HEADER32 pOptHeader, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIRECTORY, pParent)
	, pOptHeader(pOptHeader)
{

}

CDirectoryDlg::~CDirectoryDlg()
{
}

void CDirectoryDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CDirectoryDlg, CDialogEx)
END_MESSAGE_MAP()

// CDirectoryDlg 消息处理程序
BOOL CDirectoryDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	//获取控件初始偏移
	int nIndex = IDC_EDIT1;
	//循环写入数据
	CString strTempData;
	for (int i = 0; i < 16; i++)
	{
		//写入RVA
		strTempData = "";
		strTempData.AppendFormat(_T("%p"), pOptHeader->DataDirectory[i].VirtualAddress);
		SetDlgItemText(nIndex + i * 2, strTempData.GetBuffer());
		//写入大小
		strTempData = "";
		strTempData.AppendFormat(_T("%p"), pOptHeader->DataDirectory[i].Size);
		SetDlgItemText(nIndex + i * 2 + 1, strTempData.GetBuffer());
	}

	return TRUE;
}