// CModuleDlg.cpp: 实现文件
//

#include "pch.h"
#include "ARK.h"
#include "CModuleDlg.h"
#include "afxdialogex.h"
#include "ARKDlg.h"
#include "CProcessDlg.h"
#include "CPeDlg.h"

extern HANDLE m_hDev;  //引用其它文件的全局变量
// CModuleDlg 对话框

IMPLEMENT_DYNAMIC(CModuleDlg, CDialogEx)

CModuleDlg::CModuleDlg(ULONG PID, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_MODULE, pParent)
	, m_PID(PID)
{

}

CModuleDlg::~CModuleDlg()
{
}

void CModuleDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST1, m_list);
}

BEGIN_MESSAGE_MAP(CModuleDlg, CDialogEx)
	ON_NOTIFY(NM_RCLICK, IDC_LIST1, &CModuleDlg::OnNMRClickList1)
	ON_COMMAND(ID_RENEWMODULE, &CModuleDlg::OnRenewmodule)
	ON_COMMAND(ID_HIDEMODULE, &CModuleDlg::OnHidemodule)
END_MESSAGE_MAP()


// CModuleDlg 消息处理程序
BOOL CModuleDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	//1.设置列表控件的样式、列表头
	m_list.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	CRect rect;
	m_list.GetClientRect(rect);
	m_list.InsertColumn(0, L"序号", 0, rect.right / 12);
	m_list.InsertColumn(1, L"基址", 0, rect.right / 6);
	m_list.InsertColumn(2, L"大小", 0, rect.right / 6);
	m_list.InsertColumn(3, L"模块路径", 0, 7 * rect.right / 12);

	//2.向0环发送请求获取进程模块数量
	ULONG nCount = GetModuleCount(&m_PID);

	//3.根据返回的数量申请空间
	PVOID ModuleInfo = malloc(sizeof(MODULEINFO)*nCount);

	//4.向0环发送请求遍历进程模块
	EnumModule(&m_PID, ModuleInfo, nCount);

	//5.//插入信息到List
	PMODULEINFO buffTemp = (PMODULEINFO)ModuleInfo;
	int j = 0;
	for (ULONG i = 0; i < nCount; i++, j++)
	{
		if (buffTemp[i].SizeOfImage == 0)
		{
			j--;
			continue;
		}
		CString buffer;
		m_list.InsertItem(j, _T(""));

		//插入序号
		buffer.Format(_T("%d"), i);
		m_list.SetItemText(j, 0, buffer);

		//基址
		buffer.Format(_T("0x%08X"), buffTemp[i].DllBase);
		m_list.SetItemText(j, 1, buffer);

		//大小
		buffer.Format(_T("0x%08X"), buffTemp[i].SizeOfImage);
		m_list.SetItemText(j, 2, buffer);

		//模块路径
		m_list.SetItemText(j, 3, buffTemp[i].wcModuleFullPath);
	}

	//释放空间
	free(ModuleInfo);

	return TRUE;
}

//1.获得模块的数量，以便决定列表的行数
ULONG CModuleDlg::GetModuleCount(PULONG pm_PID)
{
	//驱动实际从0环传过来的字节数 （一般没什么用）
	DWORD size = 0;

	//用于接收从0环传输回来的模块的数量
	ULONG ModuleCount = 0;

	//获取模块的数量
	DeviceIoControl(
		m_hDev,		     //打开的设备句柄
		getModuleCount,	 //控制码
		pm_PID,		     //输入缓冲区
		sizeof(ULONG),   //输入缓冲区大小
		&ModuleCount,	 //输出缓冲区
		sizeof(ModuleCount),	//输出缓冲区的大小
		&size,		//实际传输的字节数
		NULL		//是否是OVERLAPPED操作
	);

	return ModuleCount;
}
//2.获得模块的信息
VOID CModuleDlg::EnumModule(PULONG pm_PID, PVOID pBuff, ULONG ModuleCount)
{
	DWORD size = 0;

	DeviceIoControl(
		m_hDev,		//打开的设备句柄
		enumModule,	//控制码
		pm_PID,		//输入缓冲区
		sizeof(ULONG),			//输入缓冲区大小
		pBuff,		//输出缓冲区
		sizeof(MODULEINFO)*ModuleCount,	//输出缓冲区的大小
		&size,		//实际传输的字节数
		NULL		//是否是OVERLAPPED操作
	);

	return VOID();
}

//右键菜单
void CModuleDlg::OnNMRClickList1(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	
	//1.获取选中行的行号
	int nIndex = m_list.GetSelectionMark();

	if (nIndex != -1)
	{
		CMenu menu;//新建菜单实例，Cmenu就是一个类，这里创建一个子菜单对象
		POINT pt = { 0 };//用于存储鼠标位置的结构体变量，pt.x和pt.y分别为x.y值
		GetCursorPos(&pt);//得到鼠标点击位置
		menu.LoadMenu(IDR_MENU7);//菜单资源加载，ID改成创建的子菜单，获取创建的子菜单的指针
		menu.GetSubMenu(0)->TrackPopupMenu(0, pt.x, pt.y, this);

		//获得选中的行所属的线程的ETHREAD基址
		CString str = m_list.GetItemText(nIndex, 1);
		pModuleBase = _tcstol(str, NULL, 16);
	}
	*pResult = 0;
}


//右键菜单1：刷新
void CModuleDlg::OnRenewmodule()
{
	//1.删除所有的列表内容
	m_list.DeleteAllItems();

	//2.向0环发送请求获取进程模块数量
	ULONG nCount = GetModuleCount(&m_PID);

	//3.根据返回的数量申请空间
	PVOID ModuleInfo = malloc(sizeof(MODULEINFO)*nCount);

	//4.向0环发送请求遍历进程模块
	EnumModule(&m_PID, ModuleInfo, nCount);

	//5.//插入信息到List
	PMODULEINFO buffTemp = (PMODULEINFO)ModuleInfo;
	int j = 0;
	for (ULONG i = 0; i < nCount; i++, j++)
	{
		if (buffTemp[i].SizeOfImage == 0)
		{
			j--;
			continue;
		}
		CString buffer;
		m_list.InsertItem(j, _T(""));

		//插入序号
		buffer.Format(_T("%d"), i);
		m_list.SetItemText(j, 0, buffer);

		//基址
		buffer.Format(_T("0x%08X"), buffTemp[i].DllBase);
		m_list.SetItemText(j, 1, buffer);

		//大小
		buffer.Format(_T("0x%08X"), buffTemp[i].SizeOfImage);
		m_list.SetItemText(j, 2, buffer);

		//模块路径
		m_list.SetItemText(j, 3, buffTemp[i].wcModuleFullPath);
	}

	//释放空间
	free(ModuleInfo);
}

//右键菜单2：隐藏模块
void CModuleDlg::OnHidemodule()
{
	Hidemodule(pModuleBase);
}

//隐藏模块
VOID CModuleDlg::Hidemodule(IN INT pModuleBase)
{
	DWORD size = 0;
	ULONG ModuleBase[2] = { 0 };
	ModuleBase[0] = (ULONG)pModuleBase;
	ModuleBase[1] = m_PID;

	//ULONG ModuleBase = (ULONG)pModuleBase;
	DeviceIoControl(
		m_hDev,		//打开的设备句柄
		hideModule,//控制码
		&ModuleBase,		//输入缓冲区
		sizeof(ModuleBase),	//输入缓冲区大小
		NULL,		//输出缓冲区
		0,	        //输出缓冲区的大小
		&size,		//实际传输的字节数
		NULL		//是否是OVERLAPPED操作
	);

	return VOID();
}