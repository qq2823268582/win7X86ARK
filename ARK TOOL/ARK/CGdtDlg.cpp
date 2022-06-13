// CGdtDlg.cpp: 实现文件
//

#include "pch.h"
#include "ARK.h"
#include "CGdtDlg.h"
#include "afxdialogex.h"
#include "ARKDlg.h"

extern HANDLE m_hDev;  //引用其它文件的全局变量

// CGdtDlg 对话框
IMPLEMENT_DYNAMIC(CGdtDlg, CDialogEx)

CGdtDlg::CGdtDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_GDT, pParent)
{

}

CGdtDlg::~CGdtDlg()
{
}

void CGdtDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST1, m_list);
}

BEGIN_MESSAGE_MAP(CGdtDlg, CDialogEx)
	ON_NOTIFY(NM_RCLICK, IDC_LIST1, &CGdtDlg::OnNMRClickList1)
	ON_COMMAND(ID_RENEWGDT, &CGdtDlg::OnRenewgdt)
END_MESSAGE_MAP()

// CGdtDlg 消息处理程序
BOOL CGdtDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	//1.加载右键菜单
	m_Menu.LoadMenu(IDR_MENU3);

	//2.给列表添加列
	CRect rect;
	m_list.GetClientRect(rect);
	m_list.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);	
	m_list.InsertColumn(0, L"段基址", 0, rect.right / 5);
	m_list.InsertColumn(1, L"段限长", 0, rect.right / 5);
	m_list.InsertColumn(2, L"段粒度", 0, rect.right / 5);
	m_list.InsertColumn(3, L"特权级", 0, rect.right / 5);
	m_list.InsertColumn(4, L"类型", 0, rect.right / 5);
	
	//3.获取GDT数量
	ULONG nCount = GetGdtCount();

	//4.根据返回的大小申请空间
	PVOID pGdtInfo = malloc(sizeof(GDTINFO)*nCount);
	memset(pGdtInfo, 0, sizeof(GDTINFO)*nCount);

	//5.遍历GDT
	EnumGdt(pGdtInfo, nCount);

	//6.填充列表
	PGDTINFO buffTemp = (PGDTINFO)pGdtInfo;
	for (ULONG i = 0; i < nCount; i++)
	{
		CString buffer;
		m_list.InsertItem(i, _T(""));

		//段基址
		buffer.Format(_T("0x%08X"), buffTemp[i].BaseAddr);
		m_list.SetItemText(i, 0, buffer);
		//段限长
		buffer.Format(_T("0x%08X"), buffTemp[i].Limit);
		m_list.SetItemText(i, 1, buffer);
		//段粒度
		buffer.Format(_T("%d"), buffTemp[i].Grain);
		m_list.SetItemText(i, 2, buffer);
		//特权级
		buffer.Format(_T("%d"), buffTemp[i].Dpl);
		m_list.SetItemText(i, 3, buffer);
		//类型
		buffer.Format(_T("%d"), buffTemp[i].GateType);
		m_list.SetItemText(i, 4, buffer);
	}

	//释放空间
	free(pGdtInfo);

	return TRUE;  
}

//向0环发送请求获得GDT表的项数
ULONG CGdtDlg::GetGdtCount()
{
	DWORD size = 0;
	//用于接收从0环传输回来的段描述符的数量
	ULONG GdtCount = 0;
	//获取驱动的数量
	DeviceIoControl(
		m_hDev,		//打开的设备句柄
		getGdtCount,	//控制码
		NULL,		//输入缓冲区
		0,			//输入缓冲区大小
		&GdtCount,		//输出缓冲区
		sizeof(GdtCount),	//输出缓冲区的大小
		&size,		//实际传输的字节数
		NULL		//是否是OVERLAPPED操作
	);

	return GdtCount;
}

//向0环发送请求遍历GDT表
VOID CGdtDlg::EnumGdt(PVOID pBuff, ULONG GdtCount)
{
	DWORD size = 0;

	DeviceIoControl(
		m_hDev,		//打开的设备句柄
		enumGdt,	//控制码
		NULL,		//输入缓冲区
		0,			//输入缓冲区大小
		pBuff,		//输出缓冲区
		sizeof(GDTINFO) * GdtCount,	//输出缓冲区的大小
		&size,		//实际传输的字节数
		NULL		//是否是OVERLAPPED操作
	);

	return VOID();
}

//右键点击事件
void CGdtDlg::OnNMRClickList1(NMHDR *pNMHDR, LRESULT *pResult)
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

//刷新GDT表
void CGdtDlg::OnRenewgdt()
{
	//1.清空列表
	m_list.DeleteAllItems();

	//2.获取GDT数量
	ULONG nCount = GetGdtCount();

	//3.根据返回的大小申请空间
	PVOID pGdtInfo = malloc(sizeof(GDTINFO)*nCount);
	memset(pGdtInfo, 0, sizeof(GDTINFO)*nCount);

	//4.遍历GDT
	EnumGdt(pGdtInfo, nCount);

	//5.填充列表
	PGDTINFO buffTemp = (PGDTINFO)pGdtInfo;
	for (ULONG i = 0; i < nCount; i++)
	{
		CString buffer;
		m_list.InsertItem(i, _T(""));

		//段基址
		buffer.Format(_T("0x%08X"), buffTemp[i].BaseAddr);
		m_list.SetItemText(i, 0, buffer);
		//段限长
		buffer.Format(_T("0x%08X"), buffTemp[i].Limit);
		m_list.SetItemText(i, 1, buffer);
		//段粒度
		buffer.Format(_T("%d"), buffTemp[i].Grain);
		m_list.SetItemText(i, 2, buffer);
		//特权级
		buffer.Format(_T("%d"), buffTemp[i].Dpl);
		m_list.SetItemText(i, 3, buffer);
		//类型
		buffer.Format(_T("%d"), buffTemp[i].GateType);
		m_list.SetItemText(i, 4, buffer);
	}

	//6.释放空间
	free(pGdtInfo);	
}
