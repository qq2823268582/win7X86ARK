#pragma once

typedef struct _IOTIMERINFO
{
	ULONG	    TimerAddress;      //TIMER地址
	ULONG       Type;
	ULONG       Flag;
	ULONG	    Device;             //设备
	ULONG	    Context;            //上下文
	ULONG       FuncAddress;       //函数地址
	WCHAR	    ImgPath[MAX_PATH]; //所属模块
}IOTIMERINFO, *PIOTIMERINFO;
// CIotimerDlg 对话框

class CIotimerDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CIotimerDlg)

public:
	CIotimerDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CIotimerDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_IOTIMER };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	CListCtrl m_list;
	CMenu m_Menu;
	VOID EnumIOTIMER(PVOID pBuff);
	virtual BOOL OnInitDialog();
	afx_msg void OnNMRClickList1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnRenewiotimer();
};
