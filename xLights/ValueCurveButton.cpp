#include "ValueCurveButton.h"
#include "../include/valuecurveselected.xpm"


#include <wx/dcmemory.h>

wxDEFINE_EVENT(EVT_VC_CHANGED, wxCommandEvent);

ValueCurveButton::ValueCurveButton(wxWindow *parent,
    wxWindowID id,
    const wxBitmap& bitmap,
    const wxPoint& pos,
    const wxSize& size,
    long style,
    const wxValidator& validator,
    const wxString& name) : wxBitmapButton(parent, id, bitmap, pos, size, style, validator, name)
{
    _vc = new ValueCurve(name.ToStdString());
}


ValueCurveButton::~ValueCurveButton()
{
    if (_vc != NULL)
    {
        delete _vc;
    }
}

void ValueCurveButton::SetActive(bool active)
{
    _vc->SetActive(active);
    UpdateState();
}

void ValueCurveButton::ToggleActive()
{
    _vc->ToggleActive();
    UpdateState();
}

wxBitmap ValueCurveButton::disabledBitmap;

void ValueCurveButton::UpdateBitmap() {
    if (GetValue()->IsActive())
    {
        RenderNewBitmap();
    }
    else
    {
        if (!disabledBitmap.IsOk()) {
            wxBitmap bmp(valuecurvenotselected_24);
            disabledBitmap = bmp;
        }
        SetBitmap(disabledBitmap);
    }
}

void ValueCurveButton::UpdateState()
{
    UpdateBitmap();
    NotifyChange();
}

void ValueCurveButton::RenderNewBitmap() {
    int sz = 24;
    if (GetContentScaleFactor() > 1.9) {
        sz *= GetContentScaleFactor();
    }
    wxBitmap bmp = _vc->GetImage(sz, sz);
    if (GetContentScaleFactor() > 1.9) {
        SetBitmap(wxBitmap(bmp.ConvertToImage(), -1, GetContentScaleFactor()));
    } else {
        SetBitmap(bmp);
    }
}

void ValueCurveButton::SetValue(const wxString& value)
{
    _vc->Deserialise(value.ToStdString());
    UpdateState();
}

void ValueCurveButton::NotifyChange()
{
    wxCommandEvent eventVCChange(EVT_VC_CHANGED);
    eventVCChange.SetEventObject(this);
    wxPostEvent(GetParent(), eventVCChange);
}

void ValueCurveButton::SetLimits(float min, float max)
{
    _vc->SetLimits(min, max);
    UpdateState();
}

ValueCurve* ValueCurveButton::GetValue()
{
    return _vc;
}
