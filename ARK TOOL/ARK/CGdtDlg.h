#pragma once


//GDT信息结构体
typedef struct _GDTINFO
{
	ULONG BaseAddr;	//段基址
	ULONG Limit;	//段限长
	ULONG Grain;	//段粒度
	ULONG Dpl;		//特权等级
	ULONG GateType;	//类型
}GDTINFO, *PGDTINFO;



// CGdtDlg 对话框
class CGdtDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CGdtDlg)

public:
	CGdtDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CGdtDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_GDT };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()

//----------------------要放到DECLARE_MESSAGE_MAP()下面，另起一个public:-------------------
public:
	CMenu m_Menu;
	CListCtrl m_list;

	virtual BOOL OnInitDialog(); //要加virtual关键字
	afx_msg void OnNMRClickList1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnRenewgdt();
	afx_msg ULONG GetGdtCount();
	afx_msg VOID EnumGdt(PVOID pBuff, ULONG GdtCount);
};
