/***************************************************************
 * This source files comes from the xLights project
 * https://www.xlights.org
 * https://github.com/smeighan/xLights
 * See the github commit history for a record of contributing
 * developers.
 * Copyright claimed based on commit dates recorded in Github
 * License: https://github.com/smeighan/xLights/blob/master/License.txt
 **************************************************************/

#include <wx/propgrid/propgrid.h>
#include <wx/propgrid/advprops.h>
//#include <wx/xml/xml.h>
//#include <wx/msgdlg.h>
//#include <wx/log.h>
//#include <wx/filedlg.h>

//#include <glm/glm.hpp>
//#include <glm/gtx/matrix_transform_2d.hpp>
//#include <glm/gtx/rotate_vector.hpp>
//#include <glm/mat3x3.hpp>

#include "MultiPointModel.h"
//#include "../support/VectorMath.h"
//#include "../xLightsMain.h"
//#include "../xLightsVersion.h"
//#include "UtilFunctions.h"
//#include "../ModelPreview.h"

//#include <log4cpp/Category.hh>

MultiPointModel::MultiPointModel(const ModelManager &manager) : ModelWithScreenLocation(manager) {
    parm1 = parm2 = parm3 = 0;
}

MultiPointModel::MultiPointModel(wxXmlNode *node, const ModelManager &manager, bool zeroBased) : ModelWithScreenLocation(manager)
{
    MultiPointModel::SetFromXml(node, zeroBased);
}

MultiPointModel::~MultiPointModel()
{
    //dtor
}

bool MultiPointModel::IsNodeFirst(int n) const
{
    return (GetIsLtoR() && n == 0) || (!GetIsLtoR() && n == Nodes.size() - 1);
}

int MultiPointModel::MapToNodeIndex(int strand, int node) const {
    return strand * parm2 + node;
}

void MultiPointModel::InitModel()
{
    _strings = wxAtoi(ModelXml->GetAttribute("MultiStrings", "1"));

    int num_points = wxAtoi(ModelXml->GetAttribute("NumPoints", "2"));

    if (num_points < 2) {
        // This is not good ... so add in a second point
        num_points = 2;
    }

    parm1 = 1;
    parm2 = num_points;
    
    InitLine();

    // read in the point data from xml
    std::vector<xlMultiPoint> pPos(num_points);
    wxString point_data = ModelXml->GetAttribute("PointData", "0.0, 0.0, 0.0, 0.0, 0.0, 0.0");
    wxArrayString point_array = wxSplit(point_data, ',');
    while (point_array.size() < num_points * 3) point_array.push_back("0.0");
    for (int i = 0; i < num_points; ++i) {
        pPos[i].x = wxAtof(point_array[i * 3]);
        pPos[i].y = wxAtof(point_array[i * 3 + 1]);
        pPos[i].z = wxAtof(point_array[i * 3 + 2]);
    }

    // calculate min/max for the model
    float minX = 100000.0f;
    float minY = 100000.0f;
    float minZ = 100000.0f;
    float maxX = 0.0f;
    float maxY = 0.0f;
    float maxZ = 0.0f;

    for (int i = 0; i < num_points; ++i) {
        if (pPos[i].x < minX) minX = pPos[i].x;
        if (pPos[i].y < minY) minY = pPos[i].y;
        if (pPos[i].z < minZ) minZ = pPos[i].z;
        if (pPos[i].x > maxX) maxX = pPos[i].x;
        if (pPos[i].y > maxY) maxY = pPos[i].y;
        if (pPos[i].z > maxZ) maxZ = pPos[i].z;
    }
    float deltax = maxX - minX;
    float deltay = maxY - minY;
    float deltaz = maxZ - minZ;

    // normalize all the point data
    for (int i = 0; i < num_points; ++i) {
        if (deltax == 0.0f) {
            pPos[i].x = 0.0f;
        } else {
            pPos[i].x = (pPos[i].x - minX) / deltax;
        }
        if (deltay == 0.0f) {
            pPos[i].y = 0.0f;
        } else {
            pPos[i].y = (pPos[i].y - minY) / deltay;
        }
        if (deltaz == 0.0f) {
            pPos[i].z = 0.0f;
        } else {
            pPos[i].z = (pPos[i].z - minZ) / deltaz;
        }
    }

    screenLocation.SetRenderSize(1.0, 1.0);

    height = wxAtof(GetModelXml()->GetAttribute("ModelHeight", "1.0"));
    double model_height = deltay;
    if (model_height < GetModelScreenLocation().GetRenderHt()) {
        model_height = GetModelScreenLocation().GetRenderHt();
    }

    // place the nodes/coords along each line segment
    int i = 0;
    if ( Nodes.size() > 0 && (BufferWi > 1 || Nodes.front()->Coords.size() > 1))
    {
        for (auto& n : Nodes)
        {
            for (auto& c : n->Coords)
            {
                c.screenX = pPos[i].x;
                c.screenY = pPos[i].y;
                c.screenZ = pPos[i].z;
            }
            i++;
        }
    }
    else if (Nodes.size() > 0)
    {
        // 1 node 1 light
        Nodes.front()->Coords.front().screenY = 0.0;
        Nodes.front()->Coords.front().screenX = 0.5;
    }
}

// initialize buffer coordinates
// parm1=Number of Strings/Arches/Canes
// parm2=Pixels Per String/Arch/Cane
void MultiPointModel::InitLine() {
    int numLights = parm1 * parm2;
    SetNodeCount(parm1,parm2,rgbOrder);
    SetBufferSize(1,SingleNode?parm1:numLights);
    int LastStringNum=-1;
    int chan = 0;
    int ChanIncr = GetNodeChannelCount(StringType);
    size_t NodeCount=GetNodeCount();
    if (!IsLtoR) {
        ChanIncr = -ChanIncr;
    }

    int idx = 0;
    for(size_t n=0; n<NodeCount; n++) {
        if (Nodes[n]->StringNum != LastStringNum) {
            LastStringNum=Nodes[n]->StringNum;
            chan=stringStartChan[LastStringNum];
            if (!IsLtoR) {
                chan += NodesPerString(LastStringNum) * GetNodeChannelCount(StringType);
                chan += ChanIncr;
            }
        }
        Nodes[n]->ActChan=chan;
        chan+=ChanIncr;
        Nodes[n]->Coords.resize(SingleNode?parm2:parm3);
        size_t CoordCount=GetCoordCount(n);
        for(size_t c=0; c < CoordCount; c++) {
            Nodes[n]->Coords[c].bufX=idx;
            Nodes[n]->Coords[c].bufY=0;
        }
        idx++;
    }
}
void MultiPointModel::AddTypeProperties(wxPropertyGridInterface* grid)
{
    wxPGProperty* p;
    if (SingleNode) {
        p = grid->Append(new wxUIntProperty("# Lights", "MultiPointNodes", parm2));
        p->SetAttribute("Min", 1);
        p->SetAttribute("Max", 10000);
        p->SetEditor("SpinCtrl");
        p->Enable(false); // number of nodes is determined by number of points
    }
    else {
        p = grid->Append(new wxUIntProperty("# Nodes", "MultiPointNodes", parm2));
        p->SetAttribute("Min", 1);
        p->SetAttribute("Max", 10000);
        p->SetEditor("SpinCtrl");
        p->Enable(false); // number of nodes is determined by number of points
    }

    p = grid->Append(new wxUIntProperty("Strings", "MultiPointStrings", _strings));
    p->SetAttribute("Min", 1);
    p->SetAttribute("Max", 48);
    p->SetEditor("SpinCtrl");
    p->SetHelpString("This is typically the number of connections from the prop to your controller.");

    if (_strings == 1) {
        // cant set start node
    } else {
        wxString nm = StartNodeAttrName(0);
        bool hasIndivNodes = ModelXml->HasAttribute(nm);

        p = grid->Append(new wxBoolProperty("Indiv Start Nodes", "ModelIndividualStartNodes", hasIndivNodes));
        p->SetAttribute("UseCheckbox", true);

        wxPGProperty* psn = grid->AppendIn(p, new wxUIntProperty(nm, nm, wxAtoi(ModelXml->GetAttribute(nm, "1"))));
        psn->SetAttribute("Min", 1);
        psn->SetAttribute("Max", (int)GetNodeCount());
        psn->SetEditor("SpinCtrl");

        if (hasIndivNodes) {
            int c = _strings;
            for (int x = 0; x < c; x++) {
                nm = StartNodeAttrName(x);
                std::string val = ModelXml->GetAttribute(nm, "").ToStdString();
                if (val == "") {
                    val = ComputeStringStartNode(x);
                    ModelXml->DeleteAttribute(nm);
                    ModelXml->AddAttribute(nm, val);
                }
                int v = wxAtoi(val);
                if (v < 1)
                    v = 1;
                if (v > NodesPerString())
                    v = NodesPerString();
                if (x == 0) {
                    psn->SetValue(v);
                } else {
                    grid->AppendIn(p, new wxUIntProperty(nm, nm, v));
                }
            }
        } else {
            psn->Enable(false);
        }
    }

    p = grid->Append(new wxFloatProperty("Height", "ModelHeight", height));
    p->SetAttribute("Precision", 2);
    p->SetAttribute("Step", 0.1);
    p->SetEditor("SpinCtrl");
}

int MultiPointModel::OnPropertyGridChange(wxPropertyGridInterface* grid, wxPropertyGridEvent& event)
{
    if ("MultiPointNodes" == event.GetPropertyName()) {
        ModelXml->DeleteAttribute("parm2");
        ModelXml->AddAttribute("parm2", wxString::Format("%d", (int)event.GetPropertyValue().GetLong()));
        wxPGProperty* sp = grid->GetPropertyByLabel("# Nodes");
        if (sp == nullptr) {
            sp = grid->GetPropertyByLabel("# Lights");
        }
        sp->SetValueFromInt((int)event.GetPropertyValue().GetLong());
        AddASAPWork(OutputModelManager::WORK_RGBEFFECTS_CHANGE, "MultiPointModel::OnPropertyGridChange::MultiPointNodes");
        AddASAPWork(OutputModelManager::WORK_MODELS_CHANGE_REQUIRING_RERENDER, "MultiPointModel::OnPropertyGridChange::MultiPointNodes");
        AddASAPWork(OutputModelManager::WORK_RELOAD_MODEL_FROM_XML, "MultiPointModel::OnPropertyGridChange::MultiPointNodes");
        AddASAPWork(OutputModelManager::WORK_REDRAW_LAYOUTPREVIEW, "MultiPointModel::OnPropertyGridChange::MultiPointNodes");
        AddASAPWork(OutputModelManager::WORK_CALCULATE_START_CHANNELS, "MultiPointModel::OnPropertyGridChange::MultiPointNodes");
        AddASAPWork(OutputModelManager::WORK_MODELS_REWORK_STARTCHANNELS, "MultiPointModel::OnPropertyGridChange::MultiPointNodes");
        AddASAPWork(OutputModelManager::WORK_RELOAD_PROPERTYGRID, "MultiPointModel::OnPropertyGridChange::MultiPointNodes");
        return 0;
    }
    else if ("MultiPointStrings" == event.GetPropertyName()) {
        int old_string_count = _strings;
        int new_string_count = event.GetValue().GetInteger();
        _strings = new_string_count;
        if (old_string_count != new_string_count) {
            wxString nm = StartNodeAttrName(0);
            bool hasIndivNodes = ModelXml->HasAttribute(nm);
            if (hasIndivNodes) {
                for (int x = 0; x < old_string_count; x++) {
                    wxString nm = StartNodeAttrName(x);
                    ModelXml->DeleteAttribute(nm);
                }
                for (int x = 0; x < new_string_count; x++) {
                    wxString nm = StartNodeAttrName(x);
                    ModelXml->AddAttribute(nm, ComputeStringStartNode(x));
                }
            }
        }
        ModelXml->DeleteAttribute("MultiStrings");
        ModelXml->AddAttribute("MultiStrings", wxString::Format("%d", _strings));
        AddASAPWork(OutputModelManager::WORK_RGBEFFECTS_CHANGE, "MultiPointModel::OnPropertyGridChange::MultiPointStrings");
        AddASAPWork(OutputModelManager::WORK_MODELS_CHANGE_REQUIRING_RERENDER, "MultiPointModel::OnPropertyGridChange::MultiPointStrings");
        AddASAPWork(OutputModelManager::WORK_RELOAD_MODEL_FROM_XML, "MultiPointModel::OnPropertyGridChange::MultiPointStrings");
        AddASAPWork(OutputModelManager::WORK_RELOAD_MODELLIST, "MultiPointModel::OnPropertyGridChange::MultiPointStrings");
        AddASAPWork(OutputModelManager::WORK_RELOAD_PROPERTYGRID, "MultiPointModel::OnPropertyGridChange::MultiPointStrings");
        return 0;
    }
    else if (!GetModelScreenLocation().IsLocked() && "ModelHeight" == event.GetPropertyName()) {
        height = event.GetValue().GetDouble();
        if (std::abs(height) < 0.01f) {
            if (height < 0.0f) {
                height = -0.01f;
            }
            else {
                height = 0.01f;
            }
        }
        ModelXml->DeleteAttribute("ModelHeight");
        ModelXml->AddAttribute("ModelHeight", event.GetPropertyValue().GetString());
        AddASAPWork(OutputModelManager::WORK_RGBEFFECTS_CHANGE, "MultiPointModel::OnPropertyGridChange::ModelHeight");
        AddASAPWork(OutputModelManager::WORK_MODELS_CHANGE_REQUIRING_RERENDER, "MultiPointModel::OnPropertyGridChange::ModelHeight");
        AddASAPWork(OutputModelManager::WORK_RELOAD_MODEL_FROM_XML, "MultiPointModel::OnPropertyGridChange::ModelHeight");
        AddASAPWork(OutputModelManager::WORK_REDRAW_LAYOUTPREVIEW, "MultiPointModel::OnPropertyGridChange::ModelHeight");
        return 0;
    }
    else if (GetModelScreenLocation().IsLocked() && "ModelHeight" == event.GetPropertyName()) {
        event.Veto();
        return 0;
    }
    return Model::OnPropertyGridChange(grid, event);
}

std::string MultiPointModel::ComputeStringStartNode(int x) const
{
    if (x == 0)
        return "1";

    int strings = GetNumPhysicalStrings();
    int nodes = GetNodeCount();
    float nodesPerString = (float)nodes / (float)strings;

    return wxString::Format("%d", (int)(x * nodesPerString + 1)).ToStdString();
}

int MultiPointModel::NodesPerString() const
{
    return Model::NodesPerString();
}

 int MultiPointModel::NodesPerString(int string) const
{
     int num_nodes = 0;
     if (_strings == 1) {
        return NodesPerString();
     } else {
        if (SingleNode) {
            return 1;
        } else {
            wxString nm = StartNodeAttrName(0);
            bool hasIndivNodes = ModelXml->HasAttribute(nm);
            int v1 = 0;
            int v2 = 0;
            if (hasIndivNodes) {
                nm = StartNodeAttrName(string);
                std::string val = ModelXml->GetAttribute(nm, "").ToStdString();
                v1 = wxAtoi(val);
                if (string < _strings - 1) { // not last string
                    nm = StartNodeAttrName(string + 1);
                    val = ModelXml->GetAttribute(nm, "").ToStdString();
                    v2 = wxAtoi(val);
                }
            } else {
                v1 = wxAtoi(ComputeStringStartNode(string));
                if (string < _strings - 1) { // not last string
                    v2 = wxAtoi(ComputeStringStartNode(string + 1));
                }
            }
            if (string < _strings - 1) { // not last string
                num_nodes = v2 - v1;
            } else {
                num_nodes = GetNodeCount() - v1 + 1;
            }
        }
        int ts = GetSmartTs();
        if (ts <= 1) {
            return num_nodes;
        } else {
            return num_nodes * ts;
        }
     }
}

int MultiPointModel::OnPropertyGridSelection(wxPropertyGridInterface* grid, wxPropertyGridEvent& event)
{
    if (event.GetPropertyName().StartsWith("ModelIndividualSegments.")) {
        wxString str = event.GetPropertyName();
        str = str.SubString(str.Find(".") + 1, str.length());
        str = str.SubString(3, str.length());
        int segment = wxAtoi(str) - 1;
        return segment;
    }
    return -1;
}

// This is required because users dont need to have their start nodes for each string in ascending
// order ... this helps us name the strings correctly
int MultiPointModel::MapPhysicalStringToLogicalString(int string) const
{
    if (_strings == 1)
        return string;

    // FIXME
    // This is not very efficient ... n^2 algorithm ... but given most people will have a small
    // number of strings and it is super simple and only used on controller upload i am hoping
    // to get away with it

    std::vector<int> stringOrder;
    for (int curr = 0; curr < _strings; curr++) {
        int count = 0;
        for (int s = 0; s < _strings; s++) {
            if (stringStartChan[s] < stringStartChan[curr] && s != curr) {
                count++;
            }
        }
        stringOrder.push_back(count);
    }
    return stringOrder[string];
}

int MultiPointModel::GetNumPhysicalStrings() const
{
    int ts = GetSmartTs();
    if (ts <= 1) {
        return _strings;
    } else {
        int strings = _strings / ts;
        if (strings == 0)
            strings = 1;
        return strings;
    }
}
