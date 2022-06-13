#pragma once


// CModuleDlg 对话框

class CModuleDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CModuleDlg)

public:
	CModuleDlg(ULONG PID, CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CModuleDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_MODULE };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()


public:
	CListCtrl m_list;
	ULONG m_PID;	//进程PID
	INT pModuleBase;

	virtual BOOL OnInitDialog();  //增加了初始化函数	
	ULONG GetModuleCount(PULONG pm_PID);
	VOID  EnumModule(PULONG pm_PID, PVOID pBuff, ULONG ModuleCount);
	VOID  Hidemodule(IN INT pModuleBase);

	afx_msg void OnNMRClickList1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnRenewmodule();
	afx_msg void OnHidemodule();
};
