// CDpctimerDlg.cpp: 实现文件
//

#include "pch.h"
#include "ARK.h"
#include "CDpctimerDlg.h"
#include "afxdialogex.h"
#include "ARKDlg.h"

extern HANDLE m_hDev;  //引用其它文件的全局变量

// CDpctimerDlg 对话框

IMPLEMENT_DYNAMIC(CDpctimerDlg, CDialogEx)

CDpctimerDlg::CDpctimerDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DPCTIMER, pParent)
{

}

CDpctimerDlg::~CDpctimerDlg()
{
}

void CDpctimerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST1, m_list);
}


BEGIN_MESSAGE_MAP(CDpctimerDlg, CDialogEx)
	ON_NOTIFY(NM_RCLICK, IDC_LIST1, &CDpctimerDlg::OnNMRClickList1)
	ON_COMMAND(ID_RENEWDPCTIMER, &CDpctimerDlg::OnRenewdpctimer)
END_MESSAGE_MAP()


// CDpctimerDlg 消息处理程序
BOOL CDpctimerDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	//1.加载右键菜单
	m_Menu.LoadMenu(IDR_MENU11);

	//2.给列表添加列
	CRect rect;
	m_list.GetClientRect(rect);
	m_list.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	m_list.InsertColumn(0, L"TIMER地址", 0, rect.right / 5);
	m_list.InsertColumn(1, L"DPC地址", 0, rect.right / 5);
	m_list.InsertColumn(2, L"周期(ms)", 0, rect.right / 5);
	m_list.InsertColumn(3, L"函数入口", 0, rect.right / 5);	
	m_list.InsertColumn(4, L"函数所属模块", 0, rect.right / 5);
	
	//3.申请内存
	ULONG nCount = 500;
	PVOID DpcTimerInfo = malloc(sizeof(DPCTIMERINFO)*nCount);
	memset(DpcTimerInfo, 0, sizeof(DPCTIMERINFO)*nCount);

	//4.向0环发送请求遍历
	EnumDPCTIMER(DpcTimerInfo);

	//5.获取个数		
	PDPCTIMERINFO buffTemp1 = (PDPCTIMERINFO)DpcTimerInfo;
	ULONG num = 0;
	while (buffTemp1->TimerAddress)
	{
		num++;
		buffTemp1++;
	}

	//6.插入信息到List
	PDPCTIMERINFO buffTemp2 = (PDPCTIMERINFO)DpcTimerInfo;
	int j = 0;
	for (ULONG i = 0; i < num; i++, j++)
	{
		CString buffer;
		m_list.InsertItem(j, _T(""));

		//1.TIMER地址
		buffer.Format(_T("0x%08X"), buffTemp2[i].TimerAddress);
		m_list.SetItemText(j, 0, buffer);

		//2.DPC地址
		buffer.Format(_T("0x%08X"), buffTemp2[i].DPCAddress);
		m_list.SetItemText(j, 1, buffer);

		//3.周期
		buffer.Format(_T("%.8d"), buffTemp2[i].Period);
		m_list.SetItemText(j, 2, buffer);

		//4.函数入口
		buffer.Format(_T("0x%08x"), buffTemp2[i].FuncAddress);
		m_list.SetItemText(j, 3, buffer);

		//5.函数所属模块
		if (buffTemp2[i].ImgPath)
		{
			buffer.Format(_T("%ws"), buffTemp2[i].ImgPath);
			m_list.SetItemText(j, 4, buffer);
		}
		else
		{
			m_list.SetItemText(j, 4, L" ");
		}
	}

	//释放空间
	free(DpcTimerInfo);

	return TRUE;
}

//向0环发送请求遍历系统回调
VOID CDpctimerDlg::EnumDPCTIMER(PVOID pBuff)
{
	DWORD size = 0;
	DeviceIoControl(
		m_hDev,		//打开的设备句柄
		enumDpctimer,	//控制码
		NULL,		//输入缓冲区
		0,			//输入缓冲区大小
		pBuff,		//输出缓冲区
		sizeof(DPCTIMERINFO) * 500,	//输出缓冲区的大小
		&size,		//实际传输的字节数
		NULL		//是否是OVERLAPPED操作
	);
	return VOID();
}

void CDpctimerDlg::OnNMRClickList1(NMHDR *pNMHDR, LRESULT *pResult)
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

void CDpctimerDlg::OnRenewdpctimer()
{
	//1.删除所有的列表内容
	m_list.DeleteAllItems();

	//3.申请内存
	ULONG nCount = 500;
	PVOID DpcTimerInfo = malloc(sizeof(DPCTIMERINFO)*nCount);
	memset(DpcTimerInfo, 0, sizeof(DPCTIMERINFO)*nCount);

	//4.向0环发送请求遍历
	EnumDPCTIMER(DpcTimerInfo);

	//5.获取个数		
	PDPCTIMERINFO buffTemp1 = (PDPCTIMERINFO)DpcTimerInfo;
	ULONG num = 0;
	while (buffTemp1->TimerAddress)
	{
		num++;
		buffTemp1++;
	}

	//6.插入信息到List
	PDPCTIMERINFO buffTemp2 = (PDPCTIMERINFO)DpcTimerInfo;
	int j = 0;
	for (ULONG i = 0; i < num; i++, j++)
	{
		CString buffer;
		m_list.InsertItem(j, _T(""));

		//1.TIMER地址
		buffer.Format(_T("0x%08X"), buffTemp2[i].TimerAddress);
		m_list.SetItemText(j, 0, buffer);

		//2.DPC地址
		buffer.Format(_T("0x%08X"), buffTemp2[i].DPCAddress);
		m_list.SetItemText(j, 1, buffer);

		//3.周期
		buffer.Format(_T("%.8d"), buffTemp2[i].Period);
		m_list.SetItemText(j, 2, buffer);

		//4.函数入口
		buffer.Format(_T("0x%08x"), buffTemp2[i].FuncAddress);
		m_list.SetItemText(j, 3, buffer);

		//5.函数所属模块
		if (buffTemp2[i].ImgPath)
		{
			buffer.Format(_T("%ws"), buffTemp2[i].ImgPath);
			m_list.SetItemText(j, 4, buffer);
		}
		else
		{
			m_list.SetItemText(j, 4, L" ");
		}
	}

	//释放空间
	free(DpcTimerInfo);
}
