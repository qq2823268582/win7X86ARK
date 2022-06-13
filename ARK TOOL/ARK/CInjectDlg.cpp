// CInjectDlg.cpp: 实现文件
//

#include "pch.h"
#include "ARK.h"
#include "CInjectDlg.h"
#include "afxdialogex.h"
#include "CProcessDlg.h"
#include "ARKDlg.h"

extern HANDLE m_hDev;

// CInjectDlg 对话框
IMPLEMENT_DYNAMIC(CInjectDlg, CDialogEx)
CInjectDlg::CInjectDlg(ULONG PID, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_INJECT, pParent)
	, m_PID(PID)   //增加1个初始化变量
{

}

CInjectDlg::~CInjectDlg()
{
}

void CInjectDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT1, m_1);
}


BEGIN_MESSAGE_MAP(CInjectDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON1, &CInjectDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, &CInjectDlg::OnBnClickedButton2)
END_MESSAGE_MAP()


// CInjectDlg 消息处理程序
BOOL CInjectDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	return TRUE;
}

//浏览按钮
void CInjectDlg::OnBnClickedButton1()
{
	CFileDialog dlg(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, _T("DLL Files(*.dll)|*.DLL||"), AfxGetMainWnd());

	//模态对话框显示出来
	CString strPath;
	if (dlg.DoModal() == IDOK)
	{
		strPath = dlg.GetPathName();
	}

	//将控件文本设置为文件路径 
	m_1.SetWindowTextW(strPath);  
}

//注入按钮
void CInjectDlg::OnBnClickedButton2()
{
	InjectProcess();
	DestroyWindow();
	//EndDialog(0);
}

//向0环发送请求：向目标进程注入指定DLL
VOID CInjectDlg::InjectProcess()
{
	DWORD size = 0;

	CString wszCFileName;
	m_1.GetWindowTextW(wszCFileName);

	INJECT_INFO injectinfo = { 0 };
	injectinfo.ProcessId = m_PID;
	wcscpy_s(injectinfo.DllName, wszCFileName);
	
	DeviceIoControl(
		m_hDev,		//打开的设备句柄
		DriveInject,      //控制码
		&injectinfo,		//输入缓冲区
		sizeof(INJECT_INFO),//输入缓冲区大小
		NULL,		//输出缓冲区
		0,	        //输出缓冲区的大小
		&size,		//实际传输的字节数
		NULL		//是否是OVERLAPPED操作
	);

	return VOID();
}