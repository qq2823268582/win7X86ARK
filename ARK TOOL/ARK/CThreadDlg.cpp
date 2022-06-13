// CThreadDlg.cpp: 实现文件
//

#include "pch.h"
#include "ARK.h"
#include "CThreadDlg.h"
#include "afxdialogex.h"
#include "ARKDlg.h"
#include "CProcessDlg.h"


extern HANDLE m_hDev;

// CThreadDlg 对话框
IMPLEMENT_DYNAMIC(CThreadDlg, CDialogEx)

CThreadDlg::CThreadDlg(ULONG PID, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_THREAD, pParent)
	, m_PID(PID)   //增加1个初始化变量
{

}

CThreadDlg::~CThreadDlg()
{
}

void CThreadDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST1, m_list);
}


BEGIN_MESSAGE_MAP(CThreadDlg, CDialogEx)
	ON_NOTIFY(NM_RCLICK, IDC_LIST1, &CThreadDlg::OnNMRClickList1)
	//ON_COMMAND(ID_HIDETHREAD, &CThreadDlg::OnHidethread)
	ON_COMMAND(ID_RENEWTHREAD, &CThreadDlg::OnRenewthread)
END_MESSAGE_MAP()


// CThreadDlg 消息处理程序
BOOL CThreadDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	//1.设置列表控件的样式
	m_list.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	CRect rect;
	m_list.GetClientRect(rect);
	m_list.InsertColumn(0, L"序号", 0, rect.right / 6);
	m_list.InsertColumn(1, L"TID", 0, rect.right / 6);
	m_list.InsertColumn(2, L"ETHREAD", 0, rect.right / 6);
	m_list.InsertColumn(3, L"TEB", 0, rect.right / 6);
	m_list.InsertColumn(4, L"优先级", 0, rect.right / 6);
	m_list.InsertColumn(5, L"切换次数", 0, rect.right / 6);

	//2.向0环发送请求获取进程中的线程数量
	ULONG nCount = GetThreadCount(&m_PID);

	//3.根据返回的数量申请空间
	PVOID ThreadInfo = malloc(sizeof(THREADINFO)*nCount);
	memset(ThreadInfo, 0, sizeof(THREADINFO)*nCount);

	//4.向0环发送请求遍历对应PID进程的所有线程
	EnumThread(&m_PID, ThreadInfo, nCount);

	//5.强制转换输出缓冲区的类型 
	PTHREADINFO buffTemp = (PTHREADINFO)ThreadInfo;

	//6.插入信息到List
	int j = 0;
	for (ULONG i = 0; i < nCount; i++, j++)
	{

		ULONG temp = (buffTemp[i].Tid) & 0x80000000;
		if (temp == 0x80000000)
		{
			j--;
			continue;
		}

		CString buffer;
		m_list.InsertItem(j, _T(""));

		//插入序号
		buffer.Format(_T("%d"), i);
		m_list.SetItemText(j, 0, buffer);

		//TID
		buffer.Format(_T("%d"), buffTemp[i].Tid);
		m_list.SetItemText(j, 1, buffer);

		//线程执行块地址
		buffer.Format(_T("0x%08X"), buffTemp[i].pEthread);
		m_list.SetItemText(j, 2, buffer);

		//Teb结构地址
		buffer.Format(_T("0x%08X"), buffTemp[i].pTeb);
		m_list.SetItemText(j, 3, buffer);

		//静态优先级
		buffer.Format(_T("%d"), buffTemp[i].BasePriority);
		m_list.SetItemText(j, 4, buffer);

		//切换次数
		buffer.Format(_T("%d"), buffTemp[i].ContextSwitches);
		m_list.SetItemText(j, 5, buffer);
	}

	//释放空间
	free(ThreadInfo);

	return TRUE;
}

//1.向0环发送信息获取线程数量
ULONG CThreadDlg::GetThreadCount(PULONG pPID)
{
	DWORD size = 0;
	//用于接收从0环传输回来的线程的数量
	ULONG ThreadCount = 0;
	//获取驱动的数量
	DeviceIoControl(
		m_hDev,		     //打开的设备句柄
		getThreadCount,	//控制码
		pPID,		    //输入缓冲区
		sizeof(ULONG), //输入缓冲区大小
		&ThreadCount,		   //输出缓冲区
		sizeof(ThreadCount),	//输出缓冲区的大小
		&size,		//实际传输的字节数
		NULL		//是否是OVERLAPPED操作
	);

	return ThreadCount;
}
//2.向0环发送信息获取线程信息
VOID CThreadDlg::EnumThread(PULONG pPID, PVOID pBuff, ULONG ThreadCount)
{
	DWORD size = 0;

	DeviceIoControl(
		m_hDev,		//打开的设备句柄
		enumThread,	//控制码
		pPID,		//输入缓冲区
		sizeof(ULONG),	//输入缓冲区大小
		pBuff,		//输出缓冲区
		sizeof(THREADINFO)*ThreadCount,	//输出缓冲区的大小
		&size,		//实际传输的字节数
		NULL		//是否是OVERLAPPED操作
	);

	return VOID();
}


//右键菜单
void CThreadDlg::OnNMRClickList1(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	
	//1.获取选中行的行号
	int nIndex = m_list.GetSelectionMark();
		
	if (nIndex != -1)
	{
		CMenu menu;//新建菜单实例，Cmenu就是一个类，这里创建一个子菜单对象
		POINT pt = { 0 };//用于存储鼠标位置的结构体变量，pt.x和pt.y分别为x.y值
		GetCursorPos(&pt);//得到鼠标点击位置
		menu.LoadMenu(IDR_MENU6);//菜单资源加载，ID改成创建的子菜单，获取创建的子菜单的指针
		menu.GetSubMenu(0)->TrackPopupMenu(0, pt.x, pt.y, this);
		

		//弹出右键菜单 ,这种方式会造成卡死，不知道为什么
		//CMenu* pSubMenu = m_menu6.GetSubMenu(IDR_MENU6);
	   // CPoint pos;
	    //GetCursorPos(&pos);
	   // pSubMenu->TrackPopupMenu(0, pos.x, pos.y, this);

		//获得选中的行所属的线程的ETHREAD基址
		//CString str = m_list.GetItemText(nIndex, 2);
		//pEthread = _tcstol(str, NULL, 16);
	}
	
	*pResult = 0;
}

//右键菜单1：刷新线程
void CThreadDlg::OnRenewthread()
{
	//1.删除所有的列表内容
	m_list.DeleteAllItems();

	//2.向0环发送请求获取进程中的线程数量
	ULONG nCount = GetThreadCount(&m_PID);

	//3.根据返回的数量申请空间
	PVOID ThreadInfo = malloc(sizeof(THREADINFO)*nCount);
	memset(ThreadInfo, 0, sizeof(THREADINFO)*nCount);

	//4.向0环发送请求遍历对应PID进程的所有线程
	EnumThread(&m_PID, ThreadInfo, nCount);

	//5.强制转换输出缓冲区的类型 
	PTHREADINFO buffTemp = (PTHREADINFO)ThreadInfo;

	//6.插入信息到List
	int j = 0;
	for (ULONG i = 0; i < nCount; i++, j++)
	{

		ULONG temp = (buffTemp[i].Tid) & 0x80000000;
		if (temp == 0x80000000)
		{
			j--;
			continue;
		}

		CString buffer;
		m_list.InsertItem(j, _T(""));

		//插入序号
		buffer.Format(_T("%d"), i);
		m_list.SetItemText(j, 0, buffer);

		//TID
		buffer.Format(_T("%d"), buffTemp[i].Tid);
		m_list.SetItemText(j, 1, buffer);

		//线程执行块地址
		buffer.Format(_T("0x%08X"), buffTemp[i].pEthread);
		m_list.SetItemText(j, 2, buffer);

		//Teb结构地址
		buffer.Format(_T("0x%08X"), buffTemp[i].pTeb);
		m_list.SetItemText(j, 3, buffer);

		//静态优先级
		buffer.Format(_T("%d"), buffTemp[i].BasePriority);
		m_list.SetItemText(j, 4, buffer);

		//切换次数
		buffer.Format(_T("%d"), buffTemp[i].ContextSwitches);
		m_list.SetItemText(j, 5, buffer);
	}

	//释放空间
	free(ThreadInfo);
}


/*
//右键菜单1：隐藏线程
void CThreadDlg::OnHidethread()
{
	Hidethread(&PID);
}
*/

/*
VOID CThreadDlg::Hidethread(IN PULONG pPID)
{
	DWORD size = 0;
	ULONG ETHREAD = (ULONG)pEthread;
	DeviceIoControl(
		m_hDev,		//打开的设备句柄
		hideThread,//控制码
		&ETHREAD,		//输入缓冲区
		sizeof(ULONG),	//输入缓冲区大小
		NULL,		//输出缓冲区
		0,	        //输出缓冲区的大小
		&size,		//实际传输的字节数
		NULL		//是否是OVERLAPPED操作
	);

	return VOID();
}
*/