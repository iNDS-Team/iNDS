// ExtractDialog.cpp

#include "StdAfx.h"

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"
 
#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif 

#include "Windows/Control/DialogImpl.h"

#include "ExtractRes.h"
#include "ExtractDialogRes.h"

/*
IDD_DIALOG_EXTRACT DIALOG DISCARDABLE  0, 0, xSize, ySize MY_MODAL_DIALOG_STYLE
CAPTION "Extract"
MY_FONT
BEGIN
  LTEXT    "E&xtract to:", IDC_STATIC_EXTRACT_EXTRACT_TO, marg, marg, xSize2, 8
  
  COMBOBOX   IDC_EXTRACT_COMBO_PATH, marg, 21, xSize2 - bDotsSize - 13, 126, CBS_DROPDOWN | CBS_AUTOHSCROLL | WS_VSCROLL | WS_TABSTOP

  PUSHBUTTON  "...", IDC_EXTRACT_BUTTON_SET_PATH, xSize - marg - bDotsSize, 20, bDotsSize, bYSize, WS_GROUP

  GROUPBOX  "Path mode",IDC_EXTRACT_PATH_MODE, marg, 44, g1XSize, 57
  CONTROL  "Full pathnames", IDC_EXTRACT_RADIO_FULL_PATHNAMES,"Button", BS_AUTORADIOBUTTON | WS_GROUP,
           g1XPos2, 57, g1XSize2, 10
  CONTROL  "Current pathnames",IDC_EXTRACT_RADIO_CURRENT_PATHNAMES, "Button", BS_AUTORADIOBUTTON,
           g1XPos2, 71, g1XSize2, 10
  CONTROL  "No pathnames", IDC_EXTRACT_RADIO_NO_PATHNAMES, "Button", BS_AUTORADIOBUTTON,
           g1XPos2, 85, g1XSize2, 10

  GROUPBOX "Overwrite mode",IDC_EXTRACT_OVERWRITE_MODE, g2XPos, 44, g2XSize, 88, WS_GROUP
  CONTROL  "Ask before overwrite", IDC_EXTRACT_RADIO_ASK_BEFORE_OVERWRITE, "Button", BS_AUTORADIOBUTTON | WS_GROUP,
           g2XPos2, 57, g2XSize2, 10
  CONTROL  "Overwrite without prompt", IDC_EXTRACT_RADIO_OVERWRITE_WITHOUT_PROMPT, "Button", BS_AUTORADIOBUTTON,
           g2XPos2, 71, g2XSize2, 10
  CONTROL  "Skip existing files", IDC_EXTRACT_RADIO_SKIP_EXISTING_FILES, "Button", BS_AUTORADIOBUTTON,
           g2XPos2, 85, g2XSize2, 10
  CONTROL  "Auto rename", IDC_EXTRACT_RADIO_AUTO_RENAME, "Button", BS_AUTORADIOBUTTON,
           g2XPos2, 99, g2XSize2, 10
  CONTROL  "Auto rename existing files", IDC_EXTRACT_RADIO_AUTO_RENAME_EXISTING, "Button", BS_AUTORADIOBUTTON,
           g2XPos2,113, g2XSize2, 10

  GROUPBOX "Files",IDC_EXTRACT_FILES, marg, 140, 127, 48, NOT WS_VISIBLE | WS_DISABLED | WS_GROUP
  CONTROL  "&Selected files",IDC_EXTRACT_RADIO_SELECTED_FILES, "Button", BS_AUTORADIOBUTTON | NOT WS_VISIBLE | WS_DISABLED | WS_GROUP,
           g1XPos2, 153, g1XSize2, 10
  CONTROL  "&All files",IDC_EXTRACT_RADIO_ALL_FILES, "Button", BS_AUTORADIOBUTTON | NOT WS_VISIBLE | WS_DISABLED,
           g1XPos2, 166, g1XSize2, 10

  GROUPBOX "Password",IDC_EXTRACT_PASSWORD, g2XPos, 142, g2XSize, 46
  EDITTEXT IDC_EXTRACT_EDIT_PASSWORD,154,153,130,14, ES_PASSWORD | ES_AUTOHSCROLL
  CONTROL         "Show Password",IDC_EXTRACT_CHECK_SHOW_PASSWORD,"Button", BS_AUTOCHECKBOX | WS_TABSTOP,
           g2XPos2, 172, g2XSize2, 10
  
  DEFPUSHBUTTON  "OK",         IDOK, bXPos3, bYPos, bXSize, bYSize, WS_GROUP
  PUSHBUTTON     "Cancel", IDCANCEL, bXPos2, bYPos, bXSize, bYSize
  PUSHBUTTON     "Help",     IDHELP, bXPos1, bYPos, bXSize, bYSize
END
*/


class CExtractDialogImpl : public NWindows::NControl::CModalDialogImpl
{
 public:
   CExtractDialogImpl(NWindows::NControl::CModalDialog *dialog,wxWindow * parent , int id) : CModalDialogImpl(dialog,parent, id, wxT("Extract"))
  {
	wxStaticText *m_pStaticTextExtractTo;
	wxTextCtrl *m_pTextCtrlPassword;
	wxButton *m_pButtonBrowse;
	wxComboBox *m_pComboBoxExtractTo;
	wxCheckBox *m_pCheckBoxShowPassword;


	///Sizer for adding the controls created by users
	wxBoxSizer* topsizer = new wxBoxSizer(wxVERTICAL);

	wxArrayString pathArray;
	m_pStaticTextExtractTo = new wxStaticText(this, IDC_STATIC_EXTRACT_EXTRACT_TO, wxT("E&xtract To:"));
	wxBoxSizer *pPathSizer = new wxBoxSizer(wxHORIZONTAL);
	m_pComboBoxExtractTo = new wxComboBox(this, IDC_EXTRACT_COMBO_PATH, wxEmptyString, wxDefaultPosition, wxDefaultSize, pathArray, wxCB_DROPDOWN|wxCB_SORT);
	m_pButtonBrowse = new wxButton(this, IDC_EXTRACT_BUTTON_SET_PATH, wxT("..."), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
	pPathSizer->Add(m_pComboBoxExtractTo, 1, wxLEFT|wxRIGHT|wxEXPAND, 5);
	pPathSizer->Add(m_pButtonBrowse, 0, wxLEFT|wxRIGHT|wxEXPAND, 5);

	wxBoxSizer *pControlSizer = new wxBoxSizer(wxHORIZONTAL);

	wxStaticBoxSizer * grpPathMode = new wxStaticBoxSizer(new wxStaticBox(this,IDC_EXTRACT_PATH_MODE,_T("Path mode")),wxVERTICAL);
	grpPathMode->Add(new wxRadioButton(this, IDC_EXTRACT_RADIO_FULL_PATHNAMES, wxT("Full pathnames"),wxDefaultPosition,  wxDefaultSize, wxRB_GROUP ) , 0 );
	grpPathMode->Add(new wxRadioButton(this, IDC_EXTRACT_RADIO_CURRENT_PATHNAMES, wxT("Current pathnames"),wxDefaultPosition,  wxDefaultSize) , 0 );
	grpPathMode->Add(new wxRadioButton(this, IDC_EXTRACT_RADIO_NO_PATHNAMES, wxT("no pathnames"),wxDefaultPosition,  wxDefaultSize) , 0 );

	wxBoxSizer *pRightSizer = new wxBoxSizer(wxVERTICAL);

	wxStaticBoxSizer * grpOverWriteMode = new wxStaticBoxSizer(new wxStaticBox(this,IDC_EXTRACT_OVERWRITE_MODE,wxT("Overwrite mode")),wxVERTICAL);
	grpOverWriteMode->Add(new wxRadioButton(this, IDC_EXTRACT_RADIO_ASK_BEFORE_OVERWRITE, wxT("Ask before overwrite"),wxDefaultPosition,  wxDefaultSize, wxRB_GROUP) , 1 );
	grpOverWriteMode->Add(new wxRadioButton(this, IDC_EXTRACT_RADIO_OVERWRITE_WITHOUT_PROMPT, wxT("Overwrite without prompt"),wxDefaultPosition,  wxDefaultSize) , 1 );
	grpOverWriteMode->Add(new wxRadioButton(this, IDC_EXTRACT_RADIO_SKIP_EXISTING_FILES, wxT("Skip existing files"),wxDefaultPosition,  wxDefaultSize) , 1 );
	grpOverWriteMode->Add(new wxRadioButton(this, IDC_EXTRACT_RADIO_AUTO_RENAME, wxT("Auto rename"),wxDefaultPosition,  wxDefaultSize) , 1 );
	grpOverWriteMode->Add(new wxRadioButton(this, IDC_EXTRACT_RADIO_AUTO_RENAME_EXISTING, wxT("Auto rename existing files"),wxDefaultPosition,  wxDefaultSize) , 1 );

	wxStaticBoxSizer *pPasswordSizer = new wxStaticBoxSizer(new wxStaticBox(this,IDC_EXTRACT_PASSWORD,wxT("Password mode")),wxVERTICAL);

	m_pTextCtrlPassword = new wxTextCtrl(this, IDC_EXTRACT_EDIT_PASSWORD, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD);
	m_pCheckBoxShowPassword = new wxCheckBox(this, IDC_EXTRACT_CHECK_SHOW_PASSWORD, wxT("Show Password"));
	pPasswordSizer->Add(m_pTextCtrlPassword, 0, wxALL|wxEXPAND, 5);
	pPasswordSizer->Add(m_pCheckBoxShowPassword, 0, wxALL|wxEXPAND, 5);

	pRightSizer->Add(grpOverWriteMode, 1, wxALL|wxEXPAND, 5);
	pRightSizer->Add(pPasswordSizer, 0, wxALL|wxEXPAND, 5);

	pControlSizer->Add(grpPathMode, 1, wxALL|wxEXPAND, 5);
	pControlSizer->Add(pRightSizer, 1, wxLEFT | wxRIGHT | wxEXPAND, 5);

	topsizer->Add(m_pStaticTextExtractTo, 0, wxALL | wxEXPAND , 10);
	topsizer->Add(pPathSizer, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND , 5);
	topsizer->Add(pControlSizer, 1, wxALL | wxEXPAND , 5);
	topsizer->Add(CreateButtonSizer(wxOK | wxCANCEL | wxHELP), 0, wxALL | wxEXPAND , 5);

	this->OnInit();

	SetSizer(topsizer); // use the sizer for layout
	topsizer->SetSizeHints(this); // set size hints to honour minimum size
  }
private:
	// Any class wishing to process wxWindows events must use this macro
	DECLARE_EVENT_TABLE()
};

REGISTER_DIALOG(IDD_DIALOG_EXTRACT,CExtractDialog,0)

BEGIN_EVENT_TABLE(CExtractDialogImpl, wxDialog)
	EVT_BUTTON(wxID_ANY, CModalDialogImpl::OnAnyButton)
	EVT_CHECKBOX(wxID_ANY, CModalDialogImpl::OnAnyButton)
END_EVENT_TABLE()

