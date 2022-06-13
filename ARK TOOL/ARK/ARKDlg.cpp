
// ARKDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "ARK.h"
#include "ARKDlg.h"
#include "afxdialogex.h"
//-----------------------------------------
#include "CDriverDlg.h"
#include "CProcessDlg.h"
#include "CGdtDlg.h"
#include "CIdtDlg.h"
#include "CSsdtDlg.h"
#include "CPeDlg.h"
#include "CNotifyDlg.h"
#include "CObcallbackDlg.h"
#include "CDpctimerDlg.h"
#include "CIotimerDlg.h"

HANDLE m_hDev;   //定义一个全局变量

//-------------------------------------------
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CARKDlg 对话框
CARKDlg::CARKDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_ARK_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

//增加析构函数的实现
CARKDlg::~CARKDlg()        
{
	CloseMyService();
}

void CARKDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TAB1, m_tab);
}

BEGIN_MESSAGE_MAP(CARKDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB1, &CARKDlg::OnTcnSelchangeTab1)
END_MESSAGE_MAP()

// CARKDlg 消息处理程序
BOOL CARKDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	//固定窗口大小，不能拖动变大变小
	::SetWindowLong(m_hWnd, GWL_STYLE, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX);

	//1.加载驱动
	LoadDriver();
	OpenDevice();

	//2.向0环发送自身PID，请求进程保护 （目的是保护自身进程不被HOOK）
	ULONG MyPID = _getpid();  //_getpid API函数获取自身PID
	SendSelfPid(&MyPID);

	//3.画tab标签页
	AddTabWnd(L"驱动", new CDriverDlg, IDD_DRIVER);
	AddTabWnd(L"进程", new CProcessDlg, IDD_PROCESS);
	AddTabWnd(L"GDT", new CGdtDlg, IDD_GDT);
	AddTabWnd(L"IDT", new CIdtDlg, IDD_IDT);
	AddTabWnd(L"SSDT", new CSsdtDlg, IDD_SSDT);
	AddTabWnd(L"系统回调", new CNotifyDlg, IDD_NOTIFY);
	AddTabWnd(L"对象回调", new CObcallbackDlg, IDD_OBCALLBACK);
	AddTabWnd(L"PE", new CPeDlg, IDD_PE);
	AddTabWnd(L"DPC定时器", new CDpctimerDlg, IDD_DPCTIMER);
	AddTabWnd(L"IO定时器", new CIotimerDlg, IDD_IOTIMER);

	//4.将tab标签页显示出来
	m_tab.SetCurSel(0);
	OnTcnSelchangeTab1(0, 0);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。
void CARKDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CARKDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


//---------------------------------------------标签页--------------------------------
//自定义函数：增加标签页
void CARKDlg::AddTabWnd(const CString& title, CDialogEx* pSubWnd, UINT uId)
{
	//1.插入一页标签
	m_tab.InsertItem(m_tab.GetItemCount(), title); //GetItemCount

	//2.创建子窗口，设置父窗口
	pSubWnd->Create(uId, &m_tab);  //Create

	//3.调整窗口的位置
	CRect rect;
	m_tab.GetClientRect(rect);     //GetClientRect
	rect.DeflateRect(1, 23, 1, 1); //DeflateRect
	pSubWnd->MoveWindow(rect);     //MoveWindow

	//4.要增加的对话框放到末尾（尾插法）
	m_tabSubWnd.push_back(pSubWnd); //push_back

	//5.将新插入的选项卡变为选中状态
	m_tab.SetCurSel(m_tabSubWnd.size() - 1); //SetCurSel
}

//切换标签页时
void CARKDlg::OnTcnSelchangeTab1(NMHDR *pNMHDR, LRESULT *pResult)
{
	//1.for循环将所有标签页都隐藏
	for (auto&i : m_tabSubWnd)
	{
		i->ShowWindow(SW_HIDE);
	}

	//2.将当前标签页窗口显示出来
	m_tabSubWnd[m_tab.GetCurSel()]->ShowWindow(SW_SHOW);  //GetCurSel

	//3.将当前标签页窗口更新
	m_tabSubWnd[m_tab.GetCurSel()]->UpdateWindow();

	//*pResult = 0;     //这个必须注释掉
}

//------------------------------------------------服务、驱动--------------------------
//1.自己创建服务，加载驱动
BOOL CARKDlg::LoadDriver()
{
	//1.打开服务管理器
	m_hServiceMgr = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

	//获取当前程序所在路径
	WCHAR pszFileName[MAX_PATH] = { 0 };
	GetModuleFileName(NULL, pszFileName, MAX_PATH);
	//获取当前程序所在目录
	(wcsrchr(pszFileName, '\\'))[0] = 0;
	//拼接驱动文件路径
	WCHAR pszDriverName[MAX_PATH] = { 0 };
	swprintf_s(pszDriverName, L"%s\\%s", pszFileName, L"MyDriver.sys");

	//2.创建服务
	m_hService = CreateService(
		m_hServiceMgr,		//SMC句柄
		L"MyService",		//驱动服务名称
		L"MyDriver",		//驱动服务显示名称
		SERVICE_ALL_ACCESS,		//权限（所有访问权限）
		SERVICE_KERNEL_DRIVER,	//服务类型（驱动程序）
		SERVICE_DEMAND_START,	//启动方式
		SERVICE_ERROR_IGNORE,	//错误控制
		pszDriverName,		//驱动文件路径
		NULL,	//加载组命令
		NULL,	//TagId(指向一个加载顺序的标签值)
		NULL,	//依存关系
		NULL,	//服务启动名
		NULL	//密码
	);
	//2.1判断服务是否存在
	if (GetLastError() == ERROR_SERVICE_EXISTS)
	{
		//如果服务存在，只要打开
		m_hService = OpenService(m_hServiceMgr, L"MyService", SERVICE_ALL_ACCESS);
	}

	//3.启动服务
	StartService(m_hService, NULL, NULL);
	return TRUE;
}
//2.打开驱动设备，开始I/O通信
BOOL CARKDlg::OpenDevice()
{
	//打开设备，获取句柄
	m_hDev = CreateFile(
		L"\\\\.\\ArkToMe666",
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	if (m_hDev == INVALID_HANDLE_VALUE)
	{
		::MessageBox(NULL, L"[3环程序]打开驱动设备失败", L"提示", NULL);
		return FALSE;
	}

	//方便调试
	//::MessageBox(NULL, L"[3环程序]打开驱动设备成功", L"提示", NULL);
	return TRUE;
}
//3.关闭驱动设备，卸载驱动，  
BOOL CARKDlg::CloseMyService()
{
	//关闭驱动设备
	CloseHandle(m_hDev);
	//删除服务
	DeleteService(m_hService);
	//关闭服务句柄
	CloseServiceHandle(m_hService);
	//关闭服务管理器句柄
	CloseServiceHandle(m_hServiceMgr);

	return TRUE;
}
//4.向0环发送自己的PID
VOID CARKDlg::SendSelfPid(PULONG pPid)
{
	DWORD size = 0;

	DeviceIoControl(
		m_hDev,		//打开的设备句柄
		selfPid,	//控制码
		pPid,		//输入缓冲区
		sizeof(ULONG),			//输入缓冲区大小
		NULL,		//输出缓冲区
		0,			//输出缓冲区的大小
		&size,		//实际传输的字节数
		NULL		//是否是OVERLAPPED操作
	);
	return VOID();
}