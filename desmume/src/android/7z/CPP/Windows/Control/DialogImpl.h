// Windows/Control/DialogImpl.h

#ifndef __WINDOWS_CONTROL_DIALOGIMPL_H
#define __WINDOWS_CONTROL_DIALOGIMPL_H

#include "Windows/Window.h"
#include "Windows/Control/Dialog.h"

namespace NWindows {
	namespace NControl {

#define TIMER_ID_IMPL (1234)

		class CModalDialogImpl : public wxDialog
		{
			wxTimer _timer;

			CDialog *_dialog;
		public:
			CModalDialogImpl(CDialog *dialog, wxWindow* parent, wxWindowID id, const wxString& title,
				       	const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
				       	long style = wxDEFAULT_DIALOG_STYLE );

			CDialog * Detach()
			{
				CDialog * oldDialog = _dialog;
				_dialog = NULL;
				return oldDialog;
			}

			void OnInit()
			{
				if (_dialog) _dialog->OnInit(this);
			}

			void OnAnyButton(wxCommandEvent& event);
			void OnAnyChoice(wxCommandEvent &event);
			void OnAnyTimer(wxTimerEvent &event);

			virtual void SetLabel(const wxString &title)
			{
				// Why we must do this "alias" ?
				this->SetTitle(title);
			}

			//////////////////
			UINT_PTR SetTimer(UINT_PTR /* FIXME idEvent */, unsigned milliseconds)
			{
				_timer.Start(milliseconds);
				return TIMER_ID_IMPL;
			}
			void KillTimer(UINT_PTR idEvent)
			{
				if (idEvent == TIMER_ID_IMPL) _timer.Stop();
			}
		};
}
}

#endif

