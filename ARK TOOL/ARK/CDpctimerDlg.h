#pragma once

typedef struct _DPCTIMERINFO
{
	ULONG	    TimerAddress;      //TIMER地址
	ULONG	    DPCAddress;        //DPC地址
	ULONG	    Period;            //周期
	ULONG       FuncAddress;       //函数地址
	WCHAR	    ImgPath[MAX_PATH]; //所属模块
}DPCTIMERINFO, *PDPCTIMERINFO;

// CDpctimerDlg 对话框
class CDpctimerDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CDpctimerDlg)

public:
	CDpctimerDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CDpctimerDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DPCTIMER };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	CListCtrl m_list;
	CMenu m_Menu;

	VOID EnumDPCTIMER(PVOID pBuff);
	virtual BOOL OnInitDialog();
	afx_msg void OnNMRClickList1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnRenewdpctimer();
};
