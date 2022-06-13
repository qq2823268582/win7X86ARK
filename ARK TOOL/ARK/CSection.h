#pragma once
#include <vector>
using std::vector;


// CSection 对话框

class CSection : public CDialogEx
{
	DECLARE_DYNAMIC(CSection)

public:
	CSection(PIMAGE_SECTION_HEADER pSectionHeader , PIMAGE_FILE_HEADER pFileHeader,CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CSection();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SECTION };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()


public:
	virtual BOOL OnInitDialog();
	
	PIMAGE_SECTION_HEADER pSectionHeader;
	PIMAGE_FILE_HEADER pFileHeader;
	CListCtrl m_list;
};
