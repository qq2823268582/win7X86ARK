// CSsdtDlg.cpp: 实现文件
//

#include "pch.h"
#include "ARK.h"
#include "CSsdtDlg.h"
#include "afxdialogex.h"
#include "ARKDlg.h"

extern HANDLE m_hDev;  //引用其它文件的全局变量

// CSsdtDlg 对话框
IMPLEMENT_DYNAMIC(CSsdtDlg, CDialogEx)

CSsdtDlg::CSsdtDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_SSDT, pParent)
{

}

CSsdtDlg::~CSsdtDlg()
{
}

void CSsdtDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST1, m_list);
}


BEGIN_MESSAGE_MAP(CSsdtDlg, CDialogEx)
	ON_NOTIFY(NM_RCLICK, IDC_LIST1, &CSsdtDlg::OnNMRClickList1)
	ON_COMMAND(ID_RENEWSSDT, &CSsdtDlg::OnRenewssdt)
END_MESSAGE_MAP()



// CSsdtDlg 消息处理程序
BOOL CSsdtDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	//1.加载右键菜单
	m_Menu.LoadMenu(IDR_MENU5);

	//2.给列表添加列
	CRect rect;
	m_list.GetClientRect(rect);
	m_list.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);	
	m_list.InsertColumn(0, L"回调号", 0, rect.right / 2);
	m_list.InsertColumn(1, L"函数地址", 0, rect.right / 2);
	
	//3.向0环发送请求获取SSDT数量
	INT nCount = GetSsdtCount();

	//4.根据返回的大小申请空间
	PVOID pSsdtInfo = malloc(sizeof(SSDTINFO)*nCount);
	
	//5.向0环发送请求遍历SSDT
	EnumSsdt(pSsdtInfo, nCount);

	//6.强制转换类型
	PSSDTINFO pbuffTemp = (PSSDTINFO)pSsdtInfo;

	//7.填充列表
	for (int i = 0; i < nCount; i++)
	{
		CString buffer;
		m_list.InsertItem(i, _T(""));
		//回调号
		buffer.Format(_T("%d"), pbuffTemp[i].uIndex);
		m_list.SetItemText(i, 0, buffer);
		//函数地址
		buffer.Format(_T("0x%08X"), pbuffTemp[i].uFuntionAddr);
		m_list.SetItemText(i, 1, buffer);
	}

	//释放空间
	free(pSsdtInfo);

	return TRUE;  
}

//向0环发送请求获得SSDT表的项数
ULONG CSsdtDlg::GetSsdtCount()
{
	DWORD size = 0;
	//用于接收从0环传输回来的系统服务描述符的数量
	ULONG SSDTCount = 0;
	//获取驱动的数量
	DeviceIoControl(
		m_hDev,		//打开的设备句柄
		getSsdtCount,	//控制码
		NULL,		//输入缓冲区
		0,			//输入缓冲区大小
		&SSDTCount,		//输出缓冲区
		sizeof(SSDTCount),	//输出缓冲区的大小
		&size,		//实际传输的字节数
		NULL		//是否是OVERLAPPED操作
	);

	return SSDTCount;
}
//向0环发送请求遍历SSDT表
VOID CSsdtDlg::EnumSsdt(PVOID pBuff, ULONG SsdtCount)
{
	DWORD size = 0;

	DeviceIoControl(
		m_hDev,		//打开的设备句柄
		enumSsdt,	//控制码
		NULL,		//输入缓冲区
		0,			//输入缓冲区大小
		pBuff,		//输出缓冲区
		sizeof(SSDTINFO) * SsdtCount,	//输出缓冲区的大小
		&size,		//实际传输的字节数
		NULL		//是否是OVERLAPPED操作
	);

	return VOID();
}

//右键点击事件
void CSsdtDlg::OnNMRClickList1(NMHDR *pNMHDR, LRESULT *pResult)
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

//刷新SSDT
void CSsdtDlg::OnRenewssdt()
{
	//1.清空列表
	m_list.DeleteAllItems();

	//2.向0环发送请求获取SSDT数量
	INT nCount = GetSsdtCount();

	//3.根据返回的大小申请空间
	PVOID pSsdtInfo = malloc(sizeof(SSDTINFO)*nCount);

	//4.向0环发送请求遍历SSDT
	EnumSsdt(pSsdtInfo, nCount);

	//5.强制转换类型
	PSSDTINFO pbuffTemp = (PSSDTINFO)pSsdtInfo;

	//6.填充列表
	for (int i = 0; i < nCount; i++)
	{
		CString buffer;
		m_list.InsertItem(i, _T(""));
		//回调号
		buffer.Format(_T("%d"), pbuffTemp[i].uIndex);
		m_list.SetItemText(i, 0, buffer);
		//函数地址
		buffer.Format(_T("0x%08X"), pbuffTemp[i].uFuntionAddr);
		m_list.SetItemText(i, 1, buffer);
	}

	//7.释放空间
	free(pSsdtInfo);
}
