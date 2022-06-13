// CProcessDlg.cpp: 实现文件
//

#include "pch.h"
#include "ARK.h"
#include "ARKDlg.h"
#include "CProcessDlg.h"
#include "afxdialogex.h"
#include "CModuleDlg.h"
#include "CThreadDlg.h"
#include "CInjectDlg.h"
#include "CVadDlg.h"


extern HANDLE m_hDev;


// CProcessDlg 对话框
IMPLEMENT_DYNAMIC(CProcessDlg, CDialogEx)

CProcessDlg::CProcessDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_PROCESS, pParent)
{

}

CProcessDlg::~CProcessDlg()
{
	delete InjectDlg;
}

void CProcessDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST1, m_list);
}


BEGIN_MESSAGE_MAP(CProcessDlg, CDialogEx)
	ON_COMMAND(ID_ReNew, &CProcessDlg::OnRenew)
	ON_COMMAND(ID_killprocess, &CProcessDlg::Onkillprocess)
	ON_COMMAND(ID_HIDEPROCESS, &CProcessDlg::OnHideprocess)
	ON_COMMAND(ID_LOOKMODULE, &CProcessDlg::OnLookmodule)
	ON_COMMAND(ID_LOOKTHREAD, &CProcessDlg::OnLookthread)
	ON_NOTIFY(NM_RCLICK, IDC_LIST1, &CProcessDlg::OnNMRClickList1)
	ON_COMMAND(ID_HIDEALLTHREAD, &CProcessDlg::OnHideallthread)
	ON_COMMAND(ID_INJECTPROCESS, &CProcessDlg::OnInjectprocess)
	ON_COMMAND(ID_PROTECTPROCESS, &CProcessDlg::OnProtectprocess)
	ON_COMMAND(ID_PROCESSVAD, &CProcessDlg::OnProcessvad)
END_MESSAGE_MAP()


// CProcessDlg 消息处理程序
BOOL CProcessDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	
	//--------------------------------画好列表标题--------------------------------------//
	//1.加载列表的右键菜单
	m_menu.LoadMenu(IDR_MENU1);
	//2.设置列表控件的样式,列表标题
	m_list.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	CRect rect;
	m_list.GetClientRect(rect);
	m_list.InsertColumn(0, L"序号", 0, rect.right / 9);
	m_list.InsertColumn(1, L"进程名", 0, rect.right / 6);
	m_list.InsertColumn(2, L"进程PID", 0, rect.right / 9);
	m_list.InsertColumn(3, L"父进程PID", 0, rect.right / 9);
	m_list.InsertColumn(4, L"进程路径", 0, rect.right / 2);
	
	//3.获取进程数量
	ULONG nCount = GetProcessCount();
	
	//4.根据进程数量申请对应大小的内存，存放从0环传回来的进程信息    
	PVOID ProcessInfo = malloc(sizeof(PROCESSINFO)*nCount);
	memset(ProcessInfo, 0, sizeof(PROCESSINFO)*nCount);        //如果不清零，会有乱码，教训

	//5.遍历进程 
	EnumProcess(IN OUT ProcessInfo, IN nCount);

	//--------------------------------------将进程信息插入列表内容之中--------------------//
	//1.将申请的内存强制转换类型，方便取出里面我们自定义的结构体
	PPROCESSINFO buffTemp = (PPROCESSINFO)ProcessInfo;
	//2.循环插入每一行
	int j = 0;
	for (ULONG i = 0; i < nCount; i++, j++)
	{
		//如果PID为0，说明是系统进程		
		if (buffTemp[i].PID == 0)
		{
			j--;
			continue;
		}

		//定义一个CString类型的临时缓冲区，方便对各种数据格式化显示
		CString buffer;

		//插入虚表头
		m_list.InsertItem(j, _T(""));

		//1.---->插入序号
		buffer.Format(_T("%d"), i);
		m_list.SetItemText(j, 0, buffer);

		//2.1保存进程路径 （因为要从进程路径中截取出进程名）
		WCHAR tempBuffer[MAX_PATH] = { 0 };
		wcscpy_s(tempBuffer, MAX_PATH, buffTemp[i].szpath);
		//2.2 获取进程名
		PathStripPath(buffTemp[i].szpath);
		//2.3---- > 插入进程名
		m_list.SetItemText(j, 1, buffTemp[i].szpath);

		//3.---->插入进程PID
		buffer.Format(_T("%d"), buffTemp[i].PID);
		m_list.SetItemText(j, 2, buffer);

		//4.---->插入父进程PID
		buffer.Format(_T("%d"), buffTemp[i].PPID);
		m_list.SetItemText(j, 3, buffer);

		//5.---->插入进程路径
		m_list.SetItemText(j, 4, tempBuffer);

	}
	//释放空间
	free(ProcessInfo);


	return TRUE;
}

//1.获得进程的数量，以便决定列表的行数
ULONG CProcessDlg::GetProcessCount()
{
	//驱动实际从0环传过来的字节数 （一般没什么用）
	DWORD size = 0;

	//用于接收从0环传输回来的进程的数量
	ULONG ProcessCount = 0;

	//获取驱动的数量
	DeviceIoControl(
		m_hDev,		//打开的设备句柄
		getProcessCount,	//控制码
		NULL,		//输入缓冲区
		0,			//输入缓冲区大小
		&ProcessCount,		//输出缓冲区
		sizeof(ProcessCount),	//输出缓冲区的大小
		&size,		//实际传输的字节数
		NULL		//是否是OVERLAPPED操作
	);

	return ProcessCount;
}
//2.获得进程的具体信息
VOID CProcessDlg::EnumProcess(IN OUT PVOID pBuff, IN ULONG ProcessCount)
{
	DWORD size = 0;

	DeviceIoControl(
		m_hDev,		//打开的设备句柄
		enumProcess,	//控制码
		NULL,		//输入缓冲区
		0,			//输入缓冲区大小
		pBuff,		//输出缓冲区
		sizeof(PROCESSINFO)*ProcessCount,	//输出缓冲区的大小
		&size,		//实际传输的字节数
		NULL		//是否是OVERLAPPED操作
	);

	return VOID();
}
//3.杀死进程
VOID CProcessDlg::KillProcess(IN ULONG PID)
{
	DWORD size = 0;
	ULONG processid = PID;
	DeviceIoControl(
		m_hDev,		//打开的设备句柄
		killProcess,//控制码
		&processid,		//输入缓冲区
		sizeof(ULONG),	//输入缓冲区大小
		NULL,		//输出缓冲区
		0,	        //输出缓冲区的大小
		&size,		//实际传输的字节数
		NULL		//是否是OVERLAPPED操作
	);

	return VOID();
}
//4.隐藏进程
VOID CProcessDlg::HideProcess(IN ULONG PID)
{
	DWORD size = 0;
	ULONG processid = PID;
	DeviceIoControl(
		m_hDev,		//打开的设备句柄
		hideProcess,//控制码
		&processid,		//输入缓冲区
		sizeof(ULONG),	//输入缓冲区大小
		NULL,		//输出缓冲区
		0,	        //输出缓冲区的大小
		&size,		//实际传输的字节数
		NULL		//是否是OVERLAPPED操作
	);

	return VOID();
}
//5.隐藏线程
VOID CProcessDlg::HideAllthread(IN ULONG PID)
{
	DWORD size = 0;

	DeviceIoControl(
		m_hDev,		//打开的设备句柄
		hideThread,//控制码
		&PID,		//输入缓冲区
		sizeof(ULONG),	//输入缓冲区大小
		NULL,		//输出缓冲区
		0,	        //输出缓冲区的大小
		&size,		//实际传输的字节数
		NULL		//是否是OVERLAPPED操作
	);

	return VOID();
}
//6.进程保护
VOID CProcessDlg::ProtectTheProcess(IN ULONG PID)
{
	DWORD size = 0;
	ULONG processid = PID;
	DeviceIoControl(
		m_hDev,		//打开的设备句柄
		ProtectProcess,//控制码
		&processid,		//输入缓冲区
		sizeof(ULONG),	//输入缓冲区大小
		NULL,		//输出缓冲区
		0,	        //输出缓冲区的大小
		&size,		//实际传输的字节数
		NULL		//是否是OVERLAPPED操作
	);

	return VOID();
}





//右键点击事件
void CProcessDlg::OnNMRClickList1(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	//1.获取选中行的行号
	int nIndex = m_list.GetSelectionMark();

	if (nIndex != -1)
	{
		//弹出右键菜单
		CMenu* pSubMenu = m_menu.GetSubMenu(0);
		CPoint pos;
		GetCursorPos(&pos);
		pSubMenu->TrackPopupMenu(0, pos.x, pos.y, this);

		//获得选中的行所属的进程的PID
		CString str = m_list.GetItemText(nIndex, 2);
		PID = _tstoi(LPCTSTR(str));
	}


	*pResult = 0;
}

//右键菜单1:刷新
void CProcessDlg::OnRenew()
{
	//1.删除所有的列表内容
	m_list.DeleteAllItems();

	//2.获取进程数量
	ULONG nCount = GetProcessCount();

	//3.根据进程数量申请对应大小的内存，存放从0环传回来的进程信息    
	PVOID ProcessInfo = malloc(sizeof(PROCESSINFO)*nCount);
	memset(ProcessInfo, 0, sizeof(PROCESSINFO)*nCount);        //如果不清零，会有乱码，教训

	//4.遍历进程 
	EnumProcess(IN OUT ProcessInfo, IN nCount);

	//--------------------------------------将进程信息插入列表内容之中--------------------//
	//1.将申请的内存强制转换类型，方便取出里面我们自定义的结构体
	PPROCESSINFO buffTemp = (PPROCESSINFO)ProcessInfo;
	//2.循环插入每一行
	int j = 0;
	for (ULONG i = 0; i < nCount; i++, j++)
	{
		//如果PID为0，说明是一个无效的进程
		if (buffTemp[i].PID == 0)
		{
			j--;
			continue;
		}

		//定义一个CString类型的临时缓冲区，方便对各种数据格式化显示
		CString buffer;

		//插入虚表头
		m_list.InsertItem(j, _T(""));

		//1.---->插入序号
		buffer.Format(_T("%d"), i);
		m_list.SetItemText(j, 0, buffer);

		//2.1保存进程路径 （因为要从进程路径中截取出进程名）
		WCHAR tempBuffer[MAX_PATH] = { 0 };
		wcscpy_s(tempBuffer, MAX_PATH, buffTemp[i].szpath);
		//2.2 获取进程名
		PathStripPath(buffTemp[i].szpath);
		//2.3---- > 插入进程名
		m_list.SetItemText(j, 1, buffTemp[i].szpath);

		//3.---->插入进程PID
		buffer.Format(_T("%d"), buffTemp[i].PID);
		m_list.SetItemText(j, 2, buffer);

		//4.---->插入父进程PID
		buffer.Format(_T("%d"), buffTemp[i].PPID);
		m_list.SetItemText(j, 3, buffer);

		//5.---->插入进程路径
		m_list.SetItemText(j, 4, tempBuffer);

	}
	//释放空间
	free(ProcessInfo);
}

//右键菜单2：强制结束进程
void CProcessDlg::Onkillprocess()
{
	KillProcess(PID);
}

//右键菜单3：隐藏进程
void CProcessDlg::OnHideprocess()
{
	HideProcess(PID);
}

//右键菜单4：遍历模块
void CProcessDlg::OnLookmodule()
{
	//创建模块窗口类
	CModuleDlg moduleDlg(PID);
	//显示模态窗口
	moduleDlg.DoModal();
}

//右键菜单5：遍历线程
void CProcessDlg::OnLookthread()
{
	//创建模块窗口类
	CThreadDlg moduleDlg(PID);
	//显示模态窗口
	moduleDlg.DoModal();
}

//右键菜单6：隐藏线程
void CProcessDlg::OnHideallthread()
{
	HideAllthread(PID);
}

//右键菜单7：驱动注入
void CProcessDlg::OnInjectprocess()
{
	//创建模块窗口类
	//CInjectDlg InjectDlg(PID);
	InjectDlg = new CInjectDlg(PID);
	InjectDlg->Create(IDD_INJECT);
	InjectDlg->ShowWindow(SW_SHOWNORMAL);
	//显示模态窗口
	//InjectDlg.DoModal();
}

//右键菜单8：进程保护
void CProcessDlg::OnProtectprocess()
{
	ProtectTheProcess(PID);
}

//右键菜单9：进程VAD
void CProcessDlg::OnProcessvad()
{
	//创建模块窗口类
	//CVadDlg VadDlg(PID);
	//显示模态窗口
	//VadDlg.DoModal();

	VadDlg = new CVadDlg(PID);
	VadDlg->Create(IDD_VAD);
	VadDlg->ShowWindow(SW_SHOWNORMAL);
}
