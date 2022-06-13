#pragma once
#include "CInjectDlg.h"
#include "CVadDlg.h"

//进程信息结构体
typedef struct _PROCESSINFO
{
	WCHAR szpath[260];//映像路径
	ULONG PID;		  //进程ID
	ULONG PPID;		  //进程ID
}PROCESSINFO, *PPROCESSINFO;

//模块信息结构体
typedef struct _MODULEINFO
{
	WCHAR wcModuleFullPath[MAX_PATH];	//模块路径
	ULONG DllBase;		                //基地址
	ULONG SizeOfImage;	                //大小
}MODULEINFO, *PMODULEINFO;

//线程信息结构体
typedef struct _THREADINFO
{
	ULONG Tid;		//线程ID
	ULONG pEthread;	//线程执行块地址
	ULONG pTeb;		//Teb结构地址
	ULONG BasePriority;	//静态优先级
	ULONG ContextSwitches;	//切换次数
}THREADINFO, *PTHREADINFO;

//VAD信息结构体
typedef struct _VADINFO
{
	ULONG pVad;	    //Vad地址
	ULONG Start;    //内存块起始地址
	ULONG End;      //内存块结束地址
	ULONG Commit;	//已提交页
	ULONG Type;	    //内存类型
	ULONG Protection;//内存保护方式
	WCHAR FileObjectName[MAX_PATH];	//文件对象名
}VADINFO, *PVADINFO;



// CProcessDlg 对话框

class CProcessDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CProcessDlg)

public:
	CProcessDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CProcessDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_PROCESS };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()

public:
	virtual BOOL OnInitDialog();
	//获取进程数量
	afx_msg ULONG GetProcessCount();
	//遍历进程信息
	afx_msg VOID EnumProcess(IN OUT PVOID pBuff, IN ULONG ProcessCount);

	ULONG PID;
	afx_msg VOID KillProcess(IN ULONG PID);
	afx_msg VOID HideProcess(IN ULONG PID);
	afx_msg VOID HideAllthread(IN ULONG PID);
	afx_msg VOID ProtectTheProcess(IN ULONG PID);
	

	CMenu m_menu;
	CListCtrl m_list;
	afx_msg void OnRenew();
	afx_msg void Onkillprocess();
	afx_msg void OnHideprocess();
	afx_msg void OnLookmodule();
	afx_msg void OnLookthread();
	afx_msg void OnNMRClickList1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnHideallthread();
	
	afx_msg void OnInjectprocess();

	CInjectDlg* InjectDlg;
	afx_msg void OnProtectprocess();
	afx_msg void OnProcessvad();

	CVadDlg* VadDlg;
};
