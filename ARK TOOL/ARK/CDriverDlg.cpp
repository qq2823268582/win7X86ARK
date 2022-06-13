// CDriverDlg.cpp: 实现文件
//

#include "pch.h"
#include "ARK.h"
#include "CDriverDlg.h"
#include "afxdialogex.h"
#include "ARKDlg.h"

extern HANDLE m_hDev;

// CDriverDlg 对话框
IMPLEMENT_DYNAMIC(CDriverDlg, CDialogEx)
CDriverDlg::CDriverDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DRIVER, pParent)
{

}

CDriverDlg::~CDriverDlg()
{
}

void CDriverDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST1, m_list);
}


BEGIN_MESSAGE_MAP(CDriverDlg, CDialogEx)
	ON_COMMAND(ID_RENEW2, &CDriverDlg::OnRenew2)
	ON_COMMAND(ID_HIDEDRIVER, &CDriverDlg::OnHidedriver)
	ON_NOTIFY(NM_RCLICK, IDC_LIST1, &CDriverDlg::OnNMRClickList1)
END_MESSAGE_MAP()


// CDriverDlg 消息处理程序
BOOL CDriverDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	//1.加载右键菜单
	m_Menu.LoadMenu(IDR_MENU2);

	//2.给列表添加列
	CRect rect;
	m_list.GetClientRect(rect);
	m_list.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	m_list.InsertColumn(0, L"驱动名", 0, rect.right / 6);
	m_list.InsertColumn(1, L"基址", 0, rect.right / 6);
	m_list.InsertColumn(2, L"大小", 0, rect.right / 6);
	m_list.InsertColumn(3, L"驱动路径", 0, rect.right / 2);

	//3.向0环发送请求获取驱动数量
	ULONG nCount = GetDriverCount();

	//char szbuffer[0x260] = { 0 };
	//sprintf_s(szbuffer, "nCount=%d", nCount);
	//MessageBoxA(0,szbuffer,0,0);


	//4.根据返回的数量申请空间
	PVOID DriverInfo = malloc(sizeof(DRIVERINFO)*nCount);
	memset(DriverInfo, 0, sizeof(DRIVERINFO)*nCount);

	//5.向0环发送请求遍历驱动
	EnumDriver(DriverInfo, nCount);

	//6.插入信息到List
	PDRIVERINFO buffTemp = (PDRIVERINFO)DriverInfo;
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

		//驱动名
		m_list.SetItemText(j, 0, buffTemp[i].wcDriverBasePath);

		//格式化基址
		buffer.Format(_T("0x%08X"), buffTemp[i].DllBase);
		m_list.SetItemText(j, 1, buffer);

		//格式化大小
		buffer.Format(_T("0x%08X"), buffTemp[i].SizeOfImage);
		m_list.SetItemText(j, 2, buffer);

		//驱动路径
		m_list.SetItemText(j, 3, buffTemp[i].wcDriverFullPath);
	}
	//释放空间
	free(DriverInfo);

	return TRUE;  
}

//右键菜单:刷新驱动
void CDriverDlg::OnRenew2()
{
	//1.清空列表
	m_list.DeleteAllItems();
	
	//2.向0环发送请求获取驱动数量
	ULONG nCount = GetDriverCount();

	//3.根据返回的数量申请空间
	PVOID DriverInfo = malloc(sizeof(DRIVERINFO)*nCount);

	//4.向0环发送请求遍历驱动
	EnumDriver(DriverInfo, nCount);

	//5.插入信息到List
	PDRIVERINFO buffTemp = (PDRIVERINFO)DriverInfo;
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

		//驱动名
		m_list.SetItemText(j, 0, buffTemp[i].wcDriverBasePath);

		//格式化基址
		buffer.Format(_T("0x%08X"), buffTemp[i].DllBase);
		m_list.SetItemText(j, 1, buffer);

		//格式化大小
		buffer.Format(_T("0x%08X"), buffTemp[i].SizeOfImage);
		m_list.SetItemText(j, 2, buffer);

		//驱动路径
		m_list.SetItemText(j, 3, buffTemp[i].wcDriverFullPath);
	}
	//6.释放空间
	free(DriverInfo);
}

//右键菜单：隐藏驱动
void CDriverDlg::OnHidedriver()
{
	//1.获取要隐藏的驱动名
	DWORD nNow = m_list.GetSelectionMark();
	CString BaseName = m_list.GetItemText(nNow, 0);

	//2.向0环发送隐藏驱动请求	
	HideDriverInfo(BaseName.GetBuffer(0));

	//3.刷新驱动
	OnRenew2();
}

//右键弹出菜单
void CDriverDlg::OnNMRClickList1(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);	
	if (m_list.GetSelectionMark() != -1)
	{
		CMenu *nMenu = m_Menu.GetSubMenu(0);
		POINT pos;
		GetCursorPos(&pos);
		nMenu->TrackPopupMenu(TPM_LEFTALIGN, pos.x, pos.y, this);
	}
	*pResult = 0;
}

//向0环发送请求获得驱动的数量
ULONG CDriverDlg::GetDriverCount()
{
	DWORD size = 0;
	//用于接收从0环传输回来的驱动的数量
	ULONG DriverCount = 0;
	//获取驱动的数量
	DeviceIoControl(
		m_hDev,		//打开的设备句柄
		getDriverCount,	//控制码
		NULL,		//输入缓冲区
		0,			//输入缓冲区大小
		&DriverCount,	//输出缓冲区
		sizeof(ULONG),//输出缓冲区的大小
		&size,		//实际传输的字节数
		NULL		//是否是OVERLAPPED操作
	);

	return DriverCount;
}
//向0环发送请求遍历驱动
VOID CDriverDlg::EnumDriver(PVOID pBuff, ULONG DriverCount)
{
	DWORD size = 0;

	DeviceIoControl(
		m_hDev,		//打开的设备句柄
		enumDriver,	//控制码
		NULL,		//输入缓冲区
		0,			//输入缓冲区大小
		pBuff,		//输出缓冲区
		sizeof(DRIVERINFO)*DriverCount,	//输出缓冲区的大小
		&size,		//实际传输的字节数
		NULL		//是否是OVERLAPPED操作
	);

	return VOID();
}
//向0环发送请求隐藏驱动
VOID CDriverDlg::HideDriverInfo(WCHAR* pDriverName)
{
	DWORD size = 0;

	DeviceIoControl(
		m_hDev,		//打开的设备句柄
		hideDriver,	//控制码
		pDriverName,		//输入缓冲区
		MAX_PATH,			//输入缓冲区大小
		NULL,		//输出缓冲区
		0,			//输出缓冲区的大小
		&size,		//实际传输的字节数
		NULL		//是否是OVERLAPPED操作
	);
	return VOID();
}