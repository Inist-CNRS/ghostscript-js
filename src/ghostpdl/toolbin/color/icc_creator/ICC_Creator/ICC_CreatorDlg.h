/* Copyright (C) 2001-2012 Artifex Software, Inc.
   All Rights Reserved.

   This software is provided AS-IS with no warranty, either express or
   implied.

   This software is distributed under license and may not be copied,
   modified or distributed except as expressly authorized under the terms
   of the license contained in the file LICENSE in this distribution.

   Refer to licensing information at http://www.artifex.com or contact
   Artifex Software, Inc.,  7 Mt. Lassen Drive - Suite A-134, San Rafael,
   CA  94903, U.S.A., +1(415)492-9861, for further information.
*/

#pragma once

#include "CIELAB.h"
#include "icc_create.h"
#include "afxwin.h"

// CICC_CreatorDlg dialog
class CICC_CreatorDlg : public CDialog
{
// Construction
public:
        CICC_CreatorDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
        enum { IDD = IDD_ICC_CREATOR_DIALOG };

        protected:
        virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

// Implementation
protected:
        HICON m_hIcon;

        int GetCIELAB(LPWSTR lpszPathName);
        int GetNames(LPWSTR lpszPathName);
        int ParseData(LPWSTR lpszPathName, bool is_ucr);
        int CreateICC(void);
		void CreateLink(link_t type);
        cielab_t *m_cielab;
        colornames_t *m_colorant_names;
        bool m_cpsi_mode;
        ucrbg_t *m_ucr_bg_data;
        ucrbg_t *m_effect_data;

        int m_num_colorant_names;
        int m_num_icc_colorants;
        int m_sample_rate;

        // Generated message map functions
        virtual BOOL OnInitDialog();
        afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
        afx_msg void OnPaint();
        afx_msg HCURSOR OnQueryDragIcon();
        DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnBnClickedCielab();
    afx_msg void OnBnClickedNames();
    afx_msg void OnBnClickedIccProfile();

    afx_msg void OnBnClickedCmyk2gray();
    afx_msg void OnBnClickedGray2cmyk();
    afx_msg void OnBnClickedCmyk2rgb();
    afx_msg void OnBnClickedRgb2cmyk();
    afx_msg void OnBnClickedCmyk2gray2();
    afx_msg void OnBnClickedPsicc();
    afx_msg void OnBnClickedGraythresh();
    afx_msg void OnBnClickedGraythreshInput();
    afx_msg void OnBnClickedRGBthreshInput();
    afx_msg void OnBnClickedCMYKthreshInput();
    afx_msg void OnEnChangeEditthreshInput();
    afx_msg void OnEnChangeEditthresh();
    CEdit m_graythreshold;
    float m_floatthreshold_gray;
    CEdit m_threshold_input;
    float m_floatthreshold_input;
    afx_msg void OnBnClickedPstables();
    afx_msg void OnBnClickedCheck1();
    afx_msg void OnBnClickedEffecttables2();
    afx_msg void OnBnClickedEffecticc3();
    CString m_effect_desc;
    CEdit m_desc_effect_str;
};
