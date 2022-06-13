// CVadDlg.cpp: 实现文件
//

#include "pch.h"
#include "ARK.h"
#include "CVadDlg.h"
#include "afxdialogex.h"
#include "CProcessDlg.h"
#include "ARKDlg.h"

extern HANDLE m_hDev;
// CVadDlg 对话框
IMPLEMENT_DYNAMIC(CVadDlg, CDialogEx)
CVadDlg::CVadDlg(ULONG PID, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_VAD, pParent)
	, m_PID(PID)   //增加1个初始化变量
{

}

CVadDlg::~CVadDlg()
{
}

void CVadDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST1, m_list);
}


BEGIN_MESSAGE_MAP(CVadDlg, CDialogEx)
	ON_NOTIFY(NM_RCLICK, IDC_LIST1, &CVadDlg::OnNMRClickList1)
	ON_COMMAND(ID_RENEWVAD, &CVadDlg::OnRenewvad)
END_MESSAGE_MAP()


// CVadDlg 消息处理程序
BOOL CVadDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	//1.设置列表控件的样式
	m_list.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	CRect rect;
	m_list.GetClientRect(rect);
	m_list.InsertColumn(0, L"序号", 0, rect.right /16);
	m_list.InsertColumn(1, L"Vad", 0, rect.right / 8);
	m_list.InsertColumn(2, L"Start", 0, rect.right / 16);
	m_list.InsertColumn(3, L"End", 0, rect.right / 16);
	m_list.InsertColumn(4, L"Commit", 0, rect.right / 16);
	m_list.InsertColumn(5, L"Type", 0, rect.right / 16); 
	m_list.InsertColumn(6, L"Protection", 0, rect.right / 8);
	m_list.InsertColumn(7, L"Name", 0, 7*rect.right / 16);

	//2.向0环发送请求获取进程中的VAD数量
	ULONG nCount = GetVADCount(&m_PID);

	//3.根据返回的数量申请空间
	PVOID VADInfo = malloc(sizeof(VADINFO)*nCount);
	memset(VADInfo, 0, sizeof(VADINFO)*nCount);

	//4.向0环发送请求遍历对应PID进程的所有VAD
	EnumVAD(&m_PID, VADInfo, nCount);

	//5.强制转换输出缓冲区的类型 
	PVADINFO buffTemp = (PVADINFO)VADInfo;

	//6.插入信息到List
	int j = 0;
	for (ULONG i = 0; i < nCount; i++, j++)
	{		
		CString buffer;
		m_list.InsertItem(j, _T(""));

		//插入序号
		buffer.Format(_T("%d"), i);
		m_list.SetItemText(j, 0, buffer);

		//Vad地址
		buffer.Format(_T("0x%X"), buffTemp[i].pVad);
		m_list.SetItemText(j, 1, buffer);

		//内存块起始地址
		buffer.Format(_T("0x%X"), buffTemp[i].Start);
		m_list.SetItemText(j, 2, buffer);

		//内存块结束地址
		buffer.Format(_T("0x%X"), buffTemp[i].End);
		m_list.SetItemText(j, 3, buffer);

		//已提交页
		buffer.Format(_T("%d"), buffTemp[i].Commit);
		m_list.SetItemText(j, 4, buffer);

		//内存类型
		if (buffTemp[i].Type == 1)
		{
			m_list.SetItemText(j, 5, L"Private");
		}
		else
		{
			m_list.SetItemText(j, 5, L"Mapped");
		}

		/*
		//内存保护   这里既可以用switch case，也可以用if  else if 
		if (buffTemp[i].Protection == 1)
		{
			m_list.SetItemText(j, 6, L"READONLY");
		}
		else if (buffTemp[i].Protection == 2)
		{
			m_list.SetItemText(j, 6, L"EXECUTE");
		}
		else if (buffTemp[i].Protection == 3)
		{
			m_list.SetItemText(j, 6, L"EXECUTE_READ");
		}
		else if (buffTemp[i].Protection == 4)
		{
			m_list.SetItemText(j, 6, L"READWITER");
		}
		else if (buffTemp[i].Protection == 5)
		{
			m_list.SetItemText(j, 6, L"WRITECOPY");
		}
		else if (buffTemp[i].Protection == 6)
		{
			m_list.SetItemText(j, 6, L"EXECUTE_READWITER");
		}
		else if (buffTemp[i].Protection == 7)
		{
			m_list.SetItemText(j, 6, L"EXECUTE_WRITECOPY");
		}
		else if (buffTemp[i].Protection == 0x18)
		{
			m_list.SetItemText(j, 6, L"NO_ACCESS");
		}
		*/
		
		switch (buffTemp[i].Protection)
		{
		case 1:
		{
			m_list.SetItemText(j, 6, L"READONLY");
			break;
		}
		case 2:
		{
			m_list.SetItemText(j, 6, L"EXECUTE");
			break;
		}
		case 3:
		{
			m_list.SetItemText(j, 6, L"EXECUTE_READ");
			break;
		}
		case 4:
		{
			m_list.SetItemText(j, 6, L"READWITER");
			break;
		}
		case 5:
		{
			m_list.SetItemText(j, 6, L"WRITECOPY");
			break;
		}
		case 6:
		{
			m_list.SetItemText(j, 6, L"EXECUTE_READWITER");
			break;
		}
		case 7:
		{
			m_list.SetItemText(j, 6, L"EXECUTE_WRITECOPY");
			break;
		}
		case 0x18:
		{
			m_list.SetItemText(j, 6, L"NO_ACCESS");
			break;
		}
		}
				
		//文件名
		buffer.Format(_T("%ws"), buffTemp[i].FileObjectName);
		m_list.SetItemText(j, 7, buffer);				
	}

	//7.释放空间
	free(VADInfo);

	return TRUE;
}


//7.获得VAD的数目
ULONG CVadDlg::GetVADCount(PULONG pPID)
{
	//驱动实际从0环传过来的字节数 （一般没什么用）
	DWORD size = 0;

	//用于接收从0环传输回来的VAD的数量
	ULONG VADCount = 0;

	//获取驱动的数量
	DeviceIoControl(
		m_hDev,		   //打开的设备句柄
		getVadCount,	//控制码
		pPID,		   //输入缓冲区
		sizeof(ULONG),  //输入缓冲区大小
		&VADCount,		//输出缓冲区
		sizeof(VADCount),	//输出缓冲区的大小
		&size,		//实际传输的字节数
		NULL		//是否是OVERLAPPED操作
	);

	return VADCount;
}

//7.VAD遍历
VOID CVadDlg::EnumVAD(PULONG pPID, PVOID pBuff, ULONG VadCount)
{
	DWORD size = 0;

	DeviceIoControl(
		m_hDev,		//打开的设备句柄
		enumProcessVad,	//控制码
		pPID,		//输入缓冲区
		sizeof(ULONG),	//输入缓冲区大小
		pBuff,		//输出缓冲区
		sizeof(VADINFO)*VadCount,	//输出缓冲区的大小
		&size,		//实际传输的字节数
		NULL		//是否是OVERLAPPED操作
	);

	return VOID();
}


//右键点击事件
void CVadDlg::OnNMRClickList1(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	
	//1.获取选中行的行号
	int nIndex = m_list.GetSelectionMark();

	if (nIndex != -1)
	{
		CMenu menu;//新建菜单实例，Cmenu就是一个类，这里创建一个子菜单对象
		POINT pt = { 0 };//用于存储鼠标位置的结构体变量，pt.x和pt.y分别为x.y值
		GetCursorPos(&pt);//得到鼠标点击位置
		menu.LoadMenu(IDR_MENU8);//菜单资源加载，ID改成创建的子菜单，获取创建的子菜单的指针
		menu.GetSubMenu(0)->TrackPopupMenu(0, pt.x, pt.y, this);
	}

	*pResult = 0;
}

//右键菜单1：刷新VAD
void CVadDlg::OnRenewvad()
{
	//1.删除所有的列表内容
	m_list.DeleteAllItems();

	//2.向0环发送请求获取进程中的VAD数量
	ULONG nCount = GetVADCount(&m_PID);

	//3.根据返回的数量申请空间
	PVOID VADInfo = malloc(sizeof(VADINFO)*nCount);
	memset(VADInfo, 0, sizeof(VADINFO)*nCount);

	//4.向0环发送请求遍历对应PID进程的所有VAD
	EnumVAD(&m_PID, VADInfo, nCount);

	//5.强制转换输出缓冲区的类型 
	PVADINFO buffTemp = (PVADINFO)VADInfo;

	//6.插入信息到List
	int j = 0;
	for (ULONG i = 0; i < nCount; i++, j++)
	{
		CString buffer;
		m_list.InsertItem(j, _T(""));

		//插入序号
		buffer.Format(_T("%d"), i);
		m_list.SetItemText(j, 0, buffer);

		//Vad地址
		buffer.Format(_T("0x%X"), buffTemp[i].pVad);
		m_list.SetItemText(j, 1, buffer);

		//内存块起始地址
		buffer.Format(_T("0x%X"), buffTemp[i].Start);
		m_list.SetItemText(j, 2, buffer);

		//内存块结束地址
		buffer.Format(_T("0x%X"), buffTemp[i].End);
		m_list.SetItemText(j, 3, buffer);

		//已提交页
		buffer.Format(_T("%d"), buffTemp[i].Commit);
		m_list.SetItemText(j, 4, buffer);

		//内存类型
		if (buffTemp[i].Type == 1)
		{
			m_list.SetItemText(j, 5, L"Private");
		}
		else
		{
			m_list.SetItemText(j, 5, L"Mapped");
		}
		
		//保护
		switch (buffTemp[i].Protection)
		{
		case 1:
		{
			m_list.SetItemText(j, 6, L"READONLY");
			break;
		}
		case 2:
		{
			m_list.SetItemText(j, 6, L"EXECUTE");
			break;
		}
		case 3:
		{
			m_list.SetItemText(j, 6, L"EXECUTE_READ");
			break;
		}
		case 4:
		{
			m_list.SetItemText(j, 6, L"READWITER");
			break;
		}
		case 5:
		{
			m_list.SetItemText(j, 6, L"WRITECOPY");
			break;
		}
		case 6:
		{
			m_list.SetItemText(j, 6, L"EXECUTE_READWITER");
			break;
		}
		case 7:
		{
			m_list.SetItemText(j, 6, L"EXECUTE_WRITECOPY");
			break;
		}
		case 0x18:
		{
			m_list.SetItemText(j, 6, L"NO_ACCESS");
			break;
		}
		}
		//名字
		buffer.Format(_T("%ws"), buffTemp[i].FileObjectName);
		m_list.SetItemText(j, 7, buffer);		
	}

	//7.释放空间
	free(VADInfo);
}
