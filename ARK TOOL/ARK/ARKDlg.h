
// ARKDlg.h: 头文件
//

#pragma once
#include <vector>                        //增加vector 头文件
#include <winioctl.h>
#include <winsvc.h>


//控制码
#define MYCTLCODE(code) CTL_CODE(FILE_DEVICE_UNKNOWN,0x800+(code),METHOD_OUT_DIRECT,FILE_ANY_ACCESS)

enum MyCtlCode
{
	getDriverCount = MYCTLCODE(1),		//获取驱动数量
	enumDriver = MYCTLCODE(2),		//遍历驱动
	hideDriver = MYCTLCODE(3),		//隐藏驱动
	getProcessCount = MYCTLCODE(4),	//获取进程数量
	enumProcess = MYCTLCODE(5),		//遍历进程
	hideProcess = MYCTLCODE(6),		//隐藏进程
	killProcess = MYCTLCODE(7),		//结束进程
	getModuleCount = MYCTLCODE(8),	//获取指定进程的模块数量
	enumModule = MYCTLCODE(9),		//遍历指定进程的模块
	getThreadCount = MYCTLCODE(10),	//获取指定进程的线程数量
	enumThread = MYCTLCODE(11),		//遍历指定进程的线程
	getFileCount = MYCTLCODE(12),	//获取文件数量
	enumFile = MYCTLCODE(13),		//遍历文件
	deleteFile = MYCTLCODE(14),		//删除文件
	getRegKeyCount = MYCTLCODE(15),	//获取注册表子项数量
	enumReg = MYCTLCODE(16),		//遍历注册表
	//newKey = MYCTLCODE(17),			//创建新键
	deleteKey = MYCTLCODE(18),		//删除新键
	enumIdt = MYCTLCODE(19),		//遍历IDT表
	getGdtCount = MYCTLCODE(20),	//获取GDT表中段描述符的数量
	enumGdt = MYCTLCODE(21),		//遍历GDT表
	getSsdtCount = MYCTLCODE(22),	//获取SSDT表中系统服务描述符的数量
	enumSsdt = MYCTLCODE(23),		//遍历SSDT表
	selfPid = MYCTLCODE(24),		//将自身的进程ID传到0环
	hideThread= MYCTLCODE(25),       //隐藏线程
	hideModule = MYCTLCODE(26),      //隐藏模块
	DriveInject = MYCTLCODE(27),     //进程注入
	ProtectProcess = MYCTLCODE(28),  //进程保护
	getVadCount = MYCTLCODE(29),     //获得VAD的数量
	enumProcessVad = MYCTLCODE(30),  //进程VAD遍历
	enumNotify = MYCTLCODE(17),      //遍历系统回调
	enumObcallback = MYCTLCODE(31),  //遍历对象回调
	enumDpctimer = MYCTLCODE(32),    //遍历DPC定时器
	enumIotimer = MYCTLCODE(33),     //遍历IO定时器
};


// CARKDlg 对话框
class CARKDlg : public CDialogEx
{
// 构造
public:
	CARKDlg(CWnd* pParent = nullptr);	// 标准构造函数
	~CARKDlg();                         //增加析构函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ARK_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()


//----------------增加的部分-------------------------------------------
public:
	//---------------------------标签页----------------------------------
	CTabCtrl m_tab;
	std::vector<CDialogEx*> m_tabSubWnd;
	void AddTabWnd(const CString& title, CDialogEx* pSubWnd, UINT uId);
	afx_msg void OnTcnSelchangeTab1(NMHDR *pNMHDR, LRESULT *pResult);

	//---------------------服务和驱动----------------------------------
	SC_HANDLE m_hService;
	SC_HANDLE m_hServiceMgr;	
	BOOL LoadDriver();
	BOOL OpenDevice();
	BOOL CloseMyService();
	VOID SendSelfPid(PULONG pPid);
};
