#pragma once

//IDT信息结构体
typedef struct _IDTINFO
{
	ULONG pFunction;		//处理函数的地址
	ULONG Selector;			//段选择子
	ULONG ParamCount;		//参数个数
	ULONG Dpl;				//段特权级
	ULONG GateType;			//类型
}IDTINFO, *PIDTINFO;


// CIdtDlg 对话框
class CIdtDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CIdtDlg)

public:
	CIdtDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CIdtDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_IDT };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()


public:
	CListCtrl m_list;
	CMenu m_Menu;

	afx_msg void OnNMRClickList1(NMHDR *pNMHDR, LRESULT *pResult);
	virtual BOOL OnInitDialog();
	afx_msg void OnRenewidt();
	afx_msg VOID EnumIdt(PVOID pBuff);
};
