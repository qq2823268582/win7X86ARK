// CPeDlg.cpp: 实现文件
//

#include "pch.h"
#include "ARK.h"
#include "CPeDlg.h"
#include "afxdialogex.h"
#include "ARKDlg.h"
#include "CSection.h"
#include "CDirectoryDlg.h"
#include "CImportDlg.h"
#include "CExportDlg.h"
#include "CReloacationDlg.h"

// CPeDlg 对话框
IMPLEMENT_DYNAMIC(CPeDlg, CDialogEx)

CPeDlg::CPeDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_PE, pParent)
{

}

CPeDlg::~CPeDlg()
{
}

void CPeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MFCEDITBROWSE1, m_1);
	DDX_Control(pDX, IDC_STATIC222, m_222);
}

BEGIN_MESSAGE_MAP(CPeDlg, CDialogEx)
	ON_EN_CHANGE(IDC_MFCEDITBROWSE1, &CPeDlg::OnEnChangeMfceditbrowse1)
	ON_WM_DROPFILES()
	ON_BN_CLICKED(IDC_BUTTON4, &CPeDlg::OnBnClickedButton4)
	ON_BN_CLICKED(IDC_BUTTON1, &CPeDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, &CPeDlg::OnBnClickedButton2)
	ON_BN_CLICKED(IDC_BUTTON3, &CPeDlg::OnBnClickedButton3)
	ON_BN_CLICKED(IDC_BUTTON5, &CPeDlg::OnBnClickedButton5)
END_MESSAGE_MAP()

//CPeDlg 消息处理程序
BOOL CPeDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}


//1.拖动文件
void CPeDlg::OnDropFiles(HDROP hDropInfo)
{
	//1.获取拖入的文件路径，放入m_szFilePath
	TCHAR szPath[MAX_PATH];
	if (DragQueryFile(hDropInfo, 0, szPath, MAX_PATH) != 0)
	{
		Filepath = "";
		Filepath += szPath;
	}

	//2.更新到窗口显示  
	m_1.SetWindowTextW(szPath);   //UpdateData(FALSE);

	//3.将文件信息读取到结构体中
	if (GetFilePEInfo(Filepath))
	{
		//4.将PE信息显示到窗口控件中
		ShowPEInfo();
		Show32PE();
	}

	CDialogEx::OnDropFiles(hDropInfo);
}

//2.多次拖动文件的动态刷新
void CPeDlg::OnEnChangeMfceditbrowse1()
{
	//1.获取编辑框的值到变量：文件路径	
	m_1.GetWindowTextW(Filepath);   //UpdateData(TRUE);

	//2.从文件路径取出文件的PE信息
	if (GetFilePEInfo(Filepath))
	{
		//将PE信息显示到控件中
		ShowPEInfo();
		Show32PE();
	}
}

//3.获取文件中的PE头信息
bool CPeDlg::GetFilePEInfo(const TCHAR * pszPathName)
{
	//1.打开文件
	HANDLE hFile = CreateFile(pszPathName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, OPEN_EXISTING, SECURITY_IMPERSONATION, NULL);
	if (!hFile)
	{
		MessageBox(_T("文件打开失败!!"));
		return FALSE;
	}
		
	//2.获得文件大小
	DWORD PeFileSize = GetFileSize(hFile, NULL);
	if (PeFileSize == 0)
	{
		CloseHandle(hFile);
		return FALSE;
	}

	//3.申请内存
	LPVOID DumpAddr = VirtualAlloc(NULL, PeFileSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (!DumpAddr)
	{
		CloseHandle(hFile);
		return FALSE;
	}
		
	//4.读取文件
	DWORD dwRet = 0;
	BOOL CanBeRead = ReadFile(hFile, DumpAddr, PeFileSize, &dwRet, NULL);
	if (!CanBeRead)
	{
		CloseHandle(hFile);
		VirtualFree(DumpAddr, PeFileSize, MEM_DECOMMIT);
		return FALSE;
	}
		
	//5.获得DOS头地址
	pDosHeader = (PIMAGE_DOS_HEADER)DumpAddr;
	if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE)
	{
		MessageBox(_T("该文件不是可解析的PE文件,请重试"), _T("ERROR"));
		return FALSE;
	}

	//6.获得NT头地址
	PIMAGE_NT_HEADERS pNtHeader = (PIMAGE_NT_HEADERS)((ULONG)pDosHeader->e_lfanew + (ULONG)DumpAddr);
	if (pNtHeader->Signature != IMAGE_NT_SIGNATURE)  return FALSE;


	//7.获取文件头地址和可选头地址
	pFileHeader = &pNtHeader->FileHeader;
	pOptHeader = &pNtHeader->OptionalHeader;

	//8.获取节表头基址
	pSectionHeader = (PIMAGE_SECTION_HEADER)((PUCHAR)pOptHeader + pFileHeader->SizeOfOptionalHeader);

	return true;
}

//4.将PE信息展示在窗口编辑框
void CPeDlg::ShowPEInfo()
{
	//编辑框控件的第一个ID值
	DWORD dwIndex = IDC_EDIT1;
	//用于保存信息
	//入口点
	CString szTemp;
	szTemp.AppendFormat(_T("%p"), pOptHeader->AddressOfEntryPoint);
	SetDlgItemText(dwIndex, szTemp.GetBuffer());
	dwIndex++;

	//镜像基址
	szTemp = "";
	szTemp.AppendFormat(_T("%p"), pOptHeader->ImageBase);
	SetDlgItemText(dwIndex, szTemp.GetBuffer());
	dwIndex++;

	//镜像大小
	szTemp = "";
	szTemp.AppendFormat(_T("%p"), pOptHeader->SizeOfImage);
	SetDlgItemText(dwIndex, szTemp.GetBuffer());
	dwIndex++;

	//代码基址
	szTemp = "";
	szTemp.AppendFormat(_T("%p"), pOptHeader->BaseOfCode);
	SetDlgItemText(dwIndex, szTemp.GetBuffer());
	dwIndex++;

	//数据基址
	szTemp = "";
	szTemp.AppendFormat(_T("%p"), pOptHeader->BaseOfData);
	SetDlgItemText(dwIndex, szTemp.GetBuffer());
	dwIndex++;

	//块对齐
	szTemp = "";
	szTemp.AppendFormat(_T("%p"), pOptHeader->SectionAlignment);
	SetDlgItemText(dwIndex, szTemp.GetBuffer());
	dwIndex++;

	//文件块对齐
	szTemp = "";
	szTemp.AppendFormat(_T("%p"), pOptHeader->FileAlignment);
	SetDlgItemText(dwIndex, szTemp.GetBuffer());
	dwIndex++;

	//标志字
	szTemp = "";
	szTemp.AppendFormat(_T("%04X"), pOptHeader->Magic);
	SetDlgItemText(dwIndex, szTemp.GetBuffer());
	dwIndex++;

	//子系统
	szTemp = "";
	szTemp.AppendFormat(_T("%04X"), pOptHeader->Subsystem);
	SetDlgItemText(dwIndex, szTemp.GetBuffer());
	dwIndex++;

	//区段数目
	szTemp = "";
	szTemp.AppendFormat(_T("%04X"), pFileHeader->NumberOfSections);
	SetDlgItemText(dwIndex, szTemp.GetBuffer());
	dwIndex++;

	//日期时间标志
	szTemp = "";
	szTemp.AppendFormat(_T("%X"), pFileHeader->TimeDateStamp);
	SetDlgItemText(dwIndex, szTemp.GetBuffer());
	dwIndex++;

	//部首大小
	szTemp = "";
	szTemp.AppendFormat(_T("%p"), pOptHeader->SizeOfHeaders);
	SetDlgItemText(dwIndex, szTemp.GetBuffer());
	dwIndex++;


	//特征值
	szTemp = "";
	szTemp.AppendFormat(_T("%04X"), pFileHeader->Characteristics);
	SetDlgItemText(dwIndex, szTemp.GetBuffer());
	dwIndex++;

	//校验和
	szTemp = "";
	szTemp.AppendFormat(_T("%p"), pOptHeader->CheckSum);
	SetDlgItemText(dwIndex, szTemp.GetBuffer());
	dwIndex++;

	//可选头的大小
	szTemp = "";
	szTemp.AppendFormat(_T("%04X"), pFileHeader->SizeOfOptionalHeader);
	SetDlgItemText(dwIndex, szTemp.GetBuffer());
	dwIndex++;

	//数据目录表的个数
	szTemp = "";
	szTemp.AppendFormat(_T("%04X"), pOptHeader->NumberOfRvaAndSizes);
	SetDlgItemText(dwIndex, szTemp.GetBuffer());
	dwIndex++;
}

//5.显示是否是32位PE文件
void CPeDlg::Show32PE()
{
	if (pOptHeader->Magic == 0x10b)  //0x10b  32   0x20b 64
	{
		m_222.SetWindowTextW(L"32位PE文件");
	}
	else if (pOptHeader->Magic == 0x20b)
	{
		m_222.SetWindowTextW(L"64位PE文件");
	}
	else
	{		
		m_222.SetWindowTextW(L"未知PE文件");
	}

}

//6.注释掉OK按钮
void CPeDlg::OnOK()
{	
	//CDialogEx::OnOK();
}



//1.节表按钮
void CPeDlg::OnBnClickedButton4()
{
	//创建模块窗口类
	CSection SectionDlg(pSectionHeader, pFileHeader);
	//显示模态窗口
	SectionDlg.DoModal();
}

//2.目录表按钮
void CPeDlg::OnBnClickedButton5()
{
	//创建模块窗口类
	CDirectoryDlg DirectoryDlg(pOptHeader);
	//显示模态窗口
	DirectoryDlg.DoModal();
}

//3.导入表按钮
void CPeDlg::OnBnClickedButton1()
{
	//创建模块窗口类
	CImportDlg ImportDlg(pDosHeader, pOptHeader);
	//显示模态窗口
	ImportDlg.DoModal();
	
}

//4.导出表按钮
void CPeDlg::OnBnClickedButton2()
{
	//创建模块窗口类
	CExportDlg ExportDlg(pDosHeader, pOptHeader);
	//显示模态窗口
	ExportDlg.DoModal();
}


//5.重定位表按钮
void CPeDlg::OnBnClickedButton3()
{
	//创建模块窗口类
	CReloacationDlg RelocationDlg(pDosHeader, pOptHeader);
	//显示模态窗口
	RelocationDlg.DoModal();
}


