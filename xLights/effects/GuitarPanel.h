#pragma once

/***************************************************************
 * This source files comes from the xLights project
 * https://www.xlights.org
 * https://github.com/smeighan/xLights
 * See the github commit history for a record of contributing
 * developers.
 * Copyright claimed based on commit dates recorded in Github
 * License: https://github.com/smeighan/xLights/blob/master/License.txt
 **************************************************************/

//(*Headers(GuitarPanel)
#include <wx/panel.h>
class wxCheckBox;
class wxChoice;
class wxFlexGridSizer;
class wxSlider;
class wxStaticText;
class wxTextCtrl;
//*)

#include <wx/progdlg.h>
#include "../BulkEditControls.h"
#include "EffectPanelUtils.h"

class MidiFile;

class GuitarPanel: public xlEffectPanel
{
	public:

		GuitarPanel(wxWindow* parent);
		virtual ~GuitarPanel();
		virtual void ValidateWindow() override;
		void SetTimingTracks(wxCommandEvent& event);

		//(*Declarations(GuitarPanel)
		BulkEditCheckBox* CheckBox_Collapse;
		BulkEditCheckBox* CheckBox_Fade;
		BulkEditCheckBox* CheckBox_ShowStrings;
		BulkEditChoice* Choice_Guitar_MIDITrack_APPLYLAST;
		BulkEditChoice* Choice_Guitar_Type;
		BulkEditSlider* Slider_MaxFrets;
		BulkEditTextCtrl* TextCtrl_MaxFrets;
		wxChoice* Choice_StringAppearance;
		wxStaticText* StaticText1;
		wxStaticText* StaticText2;
		wxStaticText* StaticText7;
		wxStaticText* StaticText8;
		//*)

	protected:

		//(*Identifiers(GuitarPanel)
		static const long ID_STATICTEXT_Guitar_Type;
		static const long ID_CHOICE_Guitar_Type;
		static const long ID_STATICTEXT_Guitar_MIDITrack_APPLYLAST;
		static const long ID_CHOICE_Guitar_MIDITrack_APPLYLAST;
		static const long ID_STATICTEXT1;
		static const long ID_CHOICE_StringAppearance;
		static const long ID_STATICTEXT_Piano_Scale;
		static const long ID_SLIDER_MaxFrets;
		static const long IDD_TEXTCTRL_MaxFrets;
		static const long ID_CHECKBOX_Fade;
		static const long ID_CHECKBOX_Collapse;
		static const long ID_CHECKBOX_ShowStrings;
		//*)

	public:

		//(*Handlers(GuitarPanel)
		void OnChoice_StringAppearanceSelect(wxCommandEvent& event);
		//*)

		DECLARE_EVENT_TABLE()
};
