#pragma once
#include "CSection.h"
#include <vector>
using std::vector;

// CPeDlg 对话框

class CPeDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CPeDlg)

public:
	CPeDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CPeDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_PE };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnEnChangeMfceditbrowse1();
	CMFCEditBrowseCtrl m_1;
	afx_msg void OnDropFiles(HDROP hDropInfo);
	virtual BOOL OnInitDialog();
	afx_msg bool GetFilePEInfo(const TCHAR * pszPathName);
	afx_msg void ShowPEInfo();

	//定义DOS头基址
	PIMAGE_DOS_HEADER pDosHeader;
	//定义PE头基址
	PIMAGE_FILE_HEADER pFileHeader;
	//定义PE可选头基址
	PIMAGE_OPTIONAL_HEADER32 pOptHeader;
	//定义节表头基址
	PIMAGE_SECTION_HEADER pSectionHeader;
	//区段表数组
	//vector <IMAGE_SECTION_HEADER> m_vecSections;

	//CSection SectionDlg;



	CString Filepath;
	CStatic m_222;

	afx_msg void Show32PE();
	virtual void OnOK();
	afx_msg void OnBnClickedButton4();
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedButton2();
	afx_msg void OnBnClickedButton3();
	afx_msg void OnBnClickedButton5();
};
