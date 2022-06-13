#pragma once

typedef struct _OBCALLBACKINFO
{
	ULONG	    ObjectAddress;     //回调对象地址
	ULONG	    ObjectType;        //回调类型
	ULONG	    CallType;          //函数类型
	WCHAR	    Altitude[260];     //回调高度
	ULONG       FuncAddress;       //函数地址
	WCHAR	    ImgPath[MAX_PATH]; //所属模块
}OBCALLBACKINFO, *POBCALLBACKINFO;


// CObcallbackDlg 对话框
class CObcallbackDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CObcallbackDlg)

public:
	CObcallbackDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CObcallbackDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_OBCALLBACK };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()

public:
	CMenu m_Menu;
	CListCtrl m_list;

	VOID EnumOBCallBack(PVOID pBuff);
	virtual BOOL OnInitDialog();
	afx_msg void OnRenewobcallback();	
	afx_msg void OnNMRClickList1(NMHDR *pNMHDR, LRESULT *pResult);
};
