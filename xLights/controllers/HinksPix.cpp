/***************************************************************
 * This source files comes from the xLights project
 * https://www.xlights.org
 * https://github.com/smeighan/xLights
 * See the github commit history for a record of contributing
 * developers.
 * Copyright claimed based on commit dates recorded in Github
 * License: https://github.com/smeighan/xLights/blob/master/License.txt
 **************************************************************/

#include "HinksPix.h"
#include "../models/Model.h"
#include "../outputs/OutputManager.h"
#include "../outputs/Output.h"
#include "../outputs/ControllerEthernet.h"
#include "../models/ModelManager.h"
#include "ControllerCaps.h"
#include "../UtilFunctions.h"

#include "../xSchedule/wxJSON/jsonreader.h"
#include "../xSchedule/wxJSON/jsonwriter.h"

#include <curl/curl.h>

#include <wx/msgdlg.h>
#include <wx/sstream.h>
#include <wx/regex.h>
#include <wx/progdlg.h>

#include <log4cpp/Category.hh>

#pragma region HinksPixOutput
void HinksPixOutput::Dump() const
{
    static log4cpp::Category &logger_base = log4cpp::Category::getInstance(std::string("log_base"));
    logger_base.debug("    Output %d Uni %d StartChan %d Pixels %d Dir %d Proto %d Nulls %d ColorOrder %d Brightness %d Gamma %d ControlerStartChannel %d ControlerEndChannel %d",
        output,
        universe,
        startChannel,
        pixels,
        direction,
        protocol,
        nullPixel,
        colorOrder,
        brightness,
        gamma,
        controlerStartChannel,
        controlerEndChannel
        );
}

void HinksPixOutput::SetConfig(wxString const& data)
{
    const wxArrayString config = Split(data, { ',' });
    if (config.size() != 10) {
        static log4cpp::Category& logger_base = log4cpp::Category::getInstance(std::string("log_base"));
        logger_base.error("Invalid config data '%s'", (const char*)data.c_str());
        return;
    }

    if (wxAtoi(config[0]) != output) {
        static log4cpp::Category& logger_base = log4cpp::Category::getInstance(std::string("log_base"));
        logger_base.error("Mismatched output ports data port:'%d' data:'%s'", output, (const char*)data.c_str());
        return;
    }
    if (config[1] != "undefined") {
        protocol = wxAtoi(config[1]);
    } else { 
        protocol = 0; 
    }
    controlerStartChannel = wxAtoi(config[2]);
    pixels = wxAtoi(config[3]);
    controlerEndChannel = wxAtoi(config[4]);
    direction = wxAtoi(config[5]);
    colorOrder = wxAtoi(config[6]);
    nullPixel = wxAtoi(config[7]);
    brightness = wxAtoi(config[8]);
    gamma = wxAtoi(config[9]);
}

wxString HinksPixOutput::BuildCommand() const
{
    return wxString::Format("{\"V\":\"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\"}",
        output, protocol, controlerStartChannel, pixels, controlerEndChannel,
        direction, colorOrder, nullPixel, brightness, gamma);
}

void HinksPixOutput::setControllerChannels(const int startChan)
{
    controlerStartChannel = startChan;
    int chanPerPix = 3;//RGB nodes
    if (colorOrder == 6 || colorOrder == 7) {//RGBW nodes
        chanPerPix = 4;
    }
    controlerEndChannel = controlerStartChannel + (pixels * chanPerPix) - 1;
}
#pragma endregion

#pragma region HinksPixSerial
void HinksPixSerial::Dump() const
{
    static log4cpp::Category& logger_base = log4cpp::Category::getInstance(std::string("log_base"));
    logger_base.debug("   E131 Uni %d E131 StartChan %d E131 NumOfChan %d E131 Enabled %s Mode %d DDP StartChan %d DDP NumOfChan %d DPP Enabled %s Upload %s",
        e131Universe,
        e131StartChannel,
        e131NumOfChan,
        toStr(e131Enabled),
        mode,
        ddpStartChannel,
        ddpNumOfChan,
        toStr(ddpEnabled),
        toStr(upload)
    );
}

void HinksPixSerial::SetConfig(std::map<wxString, wxString> const& data)
{
    if (data.size() != 8) {
        static log4cpp::Category& logger_base = log4cpp::Category::getInstance(std::string("log_base"));
        logger_base.error("Invalid serial config map data");
        return;
    }

    mode = wxAtoi(data.at("A"));

    e131Enabled = wxAtoi(data.at("B"));
    e131Universe = wxAtoi(data.at("D"));
    e131StartChannel = wxAtoi(data.at("E"));
    e131NumOfChan = wxAtoi(data.at("F"));

    ddpEnabled = wxAtoi(data.at("C"));
    ddpStartChannel = wxAtoi(data.at("G"));
    ddpNumOfChan = wxAtoi(data.at("H"));
}

wxString HinksPixSerial::BuildCommand() const
{
    return wxString::Format("A,%d,B,%d,C,%d,D,%d,E,%d,F,%d,G,%d,H,%d",
        mode, e131Enabled, ddpEnabled, e131Universe, e131StartChannel,
        e131NumOfChan, ddpStartChannel, ddpNumOfChan);
}
#pragma endregion

#pragma region HinksPixSmart
void HinksSmartOutput::Dump() const
{
    static log4cpp::Category& logger_base = log4cpp::Category::getInstance(std::string("log_base"));
    logger_base.debug("  ID %d Type %d Port 1 Start Pixel %d Port 2 Start Pixel %d Port 3 Start Pixel %d Port 4 Start Pixel %d",
        id,
        type,
        portStartPixel[0],
        portStartPixel[1],
        portStartPixel[2],
        portStartPixel[3]
    );
}

void HinksSmartOutput::SetConfig(wxString const& data)
{
    const wxArrayString config = Split(data, { ',' });
    if (config.size() != 6) {
        static log4cpp::Category& logger_base = log4cpp::Category::getInstance(std::string("log_base"));
        logger_base.error("Invalid config data '%s'", (const char*)data.c_str());
        return;
    }

    id = wxAtoi(config[0]);
    type = wxAtoi(config[1]);
    portStartPixel[0] = wxAtoi(config[2]);
    portStartPixel[1] = wxAtoi(config[3]);
    portStartPixel[2] = wxAtoi(config[4]);
    portStartPixel[3] = wxAtoi(config[5]);
}

wxString HinksSmartOutput::BuildCommand() const
{
    //{"V":"1,0,51,51,51,51"}
    return wxString::Format("{\"V\":\"%d,%d,%d,%d,%d,%d,\"}",
        id, type, portStartPixel[0], portStartPixel[1],
        portStartPixel[2], portStartPixel[3]);
}
#pragma endregion

#pragma region HinksPixInputUniverse
void HinksPixInputUniverse::Dump() const
{
    static log4cpp::Category& logger_base = log4cpp::Category::getInstance(std::string("log_base"));
    logger_base.debug("Index %d Uni %d UniSize %d HinksStart %d",
        index,
        universe,
        numOfChan,
        hinksPixStartChannel
    );
}

wxString HinksPixInputUniverse::BuildCommand() const
{
    return wxString::Format("{\"V\":\"%d,%d,%d,1,%d,%d\"}", index,
        universe, numOfChan, hinksPixStartChannel,
        hinksPixStartChannel + numOfChan - 1);
    
}
#pragma endregion


#pragma region Private Functions
bool HinksPix::InitControllerOutputData()
{
    _pixelOutputs.clear();

    for (int i = 0; i < 48; i++) {
        _pixelOutputs.push_back(HinksPixOutput (i+1));
    }

    if (_EXP_Outputs[0] == EXPType::Local_SPI || _EXP_Outputs[0] == EXPType::Long_Range){
        InitExpansionBoardData(1, 1, 16);
    }

    if(_EXP_Outputs[1] == EXPType::Local_SPI || _EXP_Outputs[1] == EXPType::Long_Range) {
        InitExpansionBoardData(2, 17, 16);
    }

    if(_EXP_Outputs[2] == EXPType::Local_SPI || _EXP_Outputs[2] == EXPType::Long_Range) {
        InitExpansionBoardData(3, 33, 16);
    }

    _serialOutput = InitSerialData();
    return true;
}

void HinksPix::InitExpansionBoardData(int expansion, int startport, int length)
{
    static log4cpp::Category& logger_base = log4cpp::Category::getInstance(std::string("log_base"));
    wxJSONValue data;

    bool worked = GetControllerDataJSON(GetJSONPortURL(), data, wxString::Format("BLK: %d",expansion - 1));

    if (!worked || !data.HasMember("LIST")) {
        logger_base.error("Invalid Data from controller");
        return;
    }
    auto ports = data.ItemAt("LIST").AsArray();

    if (ports->Count() != length) {
        logger_base.error("Data from controller size and Expansion Size don't match");
        logger_base.error(data.ItemAt("LIST").AsString());
        return;
    }

    for (int i = 0; i < ports->Count(); i++) {
        auto stringValue = ports->Item(i)["V"].AsString();
        _pixelOutputs[(startport - 1) + i].SetConfig(stringValue);
    }
}

std::unique_ptr < HinksPixSerial> HinksPix::InitSerialData()
{
    std::unique_ptr < HinksPixSerial> serial = std::make_unique<HinksPixSerial>();
    auto const data = GetControllerData(908);//get serial config command

    if (!data.empty()) {
        auto const serialConfig = StringToMap(data);
        serial->SetConfig(serialConfig);
    }
    else {
        static log4cpp::Category& logger_base = log4cpp::Category::getInstance(std::string("log_base"));
        logger_base.error("Invalid Return data %s", (const char*)data.c_str());
    }

    return serial;
}

bool HinksPix::UploadInputUniverses(ControllerEthernet* controller, std::vector<HinksPixInputUniverse> const& inputUniverses) const
{
    static log4cpp::Category& logger_base = log4cpp::Category::getInstance(std::string("log_base"));
    logger_base.debug("HinksPix Inputs Upload: Uploading to %s", (const char*)_ip.c_str());

    wxString request;

    // Get universes based on IP
    std::list<Output*> outputs = controller->GetOutputs();

    if (controller->GetOutputCount() > _numberOfUniverses) {
        DisplayError( wxString::Format(
                "Attempt to upload %d universes to HinksPix controller but only %d are supported.",
                controller->GetOutputCount(), _numberOfUniverses).ToStdString());
        return false;
    }

    auto out = outputs.front();
    int type = 0;//e131=0, ddp=1, artnet=2
    int multi = 0;
    int DDPStart = 0;
    if (out->GetType() == OUTPUT_E131) {
        type = 0;
    }
    else if (out->GetType() == OUTPUT_ARTNET) {
        type = 2;
    }
    else if (out->GetType() == OUTPUT_DDP) {
        type = 1;
        DDPStart = out->GetStartChannel();
    }

    if (out->GetIP() == "MULTICAST") {
        multi = 1;
    }

    auto const cmd = wxString::Format("A,%d,B,%d,C,%d,D,%d,E,%d",
        multi, type, _numberOfUniverses, controller->GetOutputCount(), DDPStart);

    //Set Controller Input mode
    auto const setRet = GetControllerData(4902, cmd);
    if (setRet != "done")
    {
        logger_base.error("4902 Return %s", (const char*)setRet.c_str());
        DisplayError("Changed HinksPix Input Mode FAILED.");
        return false;
    }

    //if DDP, skip the next part
    if (type == 1) {
        return true;
    }

    //Joes code allows 6 universe settings uploaded at a time
    //loop though and submit every 6

    int numberOfCalls;
    if (_numberOfUniverses <= 65) {
        numberOfCalls = 11;
    }
    else {
        numberOfCalls = 25;
    }

    bool worked = true;
    int index = 0;

    for (int j = 0; j < numberOfCalls; j++) {
        wxString requestString = wxString::Format("DATA: {\"CMD\":\"E131\",\"BLK\":\"%d\",\"LIST\":[", j);
        for (int i = 0; i < 6; i++) {
            auto inpUn = std::find_if(inputUniverses.begin(), inputUniverses.end(), [index](auto const& inp) {return inp.index == index; });
            if (inpUn != inputUniverses.end()) {
                if (i != 0) {
                    requestString += ",";
                }
                requestString += inpUn->BuildCommand();
                index++;
            }
            else if (index <= _numberOfUniverses) {
                if (i != 0) {
                    requestString += ",";
                }
                requestString += wxString::Format("{\"V\":\"%d,0,0,0,0,0\"}", index);
                index++;
            }
            else {
                requestString += "{\"V\":\"0,0,0,0,0,0\"}";
            }
        }
        requestString += "]}";
        //post data
        auto const ret = GetJSONControllerData(GetJSONPostURL(), requestString);
        if (ret.find("\"OK\"") == std::string::npos) {
            logger_base.error("Failed Return %s", (const char*)ret.c_str());
            worked = false;
        }
    }

    //reboot
    //auto const resetres = GetControllerData(1111);
    //if (resetres != "done") {
    //    worked = false;
    //}

    return worked;
}

void HinksPix::UploadPixelOutputs(bool& worked) const
{
    if (_EXP_Outputs[0] == EXPType::Local_SPI || _EXP_Outputs[0] == EXPType::Long_Range) {
        UploadExpansionBoardData(1, 1, 16, worked);
    }

    if (_EXP_Outputs[1] == EXPType::Local_SPI || _EXP_Outputs[1] == EXPType::Long_Range) {
        UploadExpansionBoardData(2, 17, 16, worked);
    }

    if (_EXP_Outputs[2] == EXPType::Local_SPI || _EXP_Outputs[2] == EXPType::Long_Range) {
        UploadExpansionBoardData(3, 33, 16, worked);
    }
}

void HinksPix::UploadExpansionBoardData(int expansion, int startport, int length, bool& worked) const
{
    static log4cpp::Category& logger_base = log4cpp::Category::getInstance(std::string("log_base"));

    logger_base.debug("Building pixel upload Expansion %d:", expansion);
    //{"CMD":"PCONFIG","BOARD":"0","LIST":[
    wxString requestString = wxString::Format("DATA: {\"CMD\":\"PCONFIG\",\"BOARD\":\"%d\",\"LIST\":[", expansion - 1);

    for (int i = 0; i < length; i++) {
        _pixelOutputs[(startport - 1) + i].Dump();
        requestString += _pixelOutputs[(startport - 1) + i].BuildCommand();
        requestString += ",";
    }

    requestString.RemoveLast();//remove last ","
    requestString += "]}";

    auto const ret = GetJSONControllerData(GetJSONPostURL(), requestString);
    if (ret.find("\"OK\"") == std::string::npos) {
        logger_base.error("Failed Return %s", (const char*)ret.c_str());
        worked = false;
    }
}

void HinksPix::UpdatePortData(HinksPixOutput &pd, UDControllerPort* stringData, int32_t hinkstartChan) const
{
    int const protocol = EncodeStringPortProtocol(stringData->GetFirstModel()->GetProtocol());
    if (protocol != -1) {
        pd.protocol = protocol;
    }

    int const nullPix = stringData->GetFirstModel()->GetStartNullPixels(-1);
    if (nullPix != -1) {
        pd.nullPixel = nullPix;
    }

    int const brightness = stringData->GetFirstModel()->GetBrightness(-1);
    if (brightness != -1) {
        pd.brightness = EncodeBrightness(brightness);
    }

    std::string const direction = stringData->GetFirstModel()->GetDirection("unknown");
    if (direction != "unknown" ) {
        pd.direction = EncodeDirection(direction);
    }

    int const gamma = stringData->GetFirstModel()->GetGamma(-1);
    if (gamma != -1) {
        pd.gamma = gamma;
    }

    std::string const color = stringData->GetFirstModel()->GetColourOrder("");
    if (!color.empty()) {
        pd.colorOrder = EncodeColorOrder(color);
    }
    pd.universe = stringData->GetUniverse();
    pd.startChannel = stringData->GetUniverseStartChannel();
    pd.pixels = stringData->Pixels();

    pd.setControllerChannels(hinkstartChan);
}

void HinksPix::UpdateUniverseControlerChannels(UDControllerPort* stringData, std::vector<HinksPixInputUniverse>& inputUniverses, int32_t& hinkstartChan, int& index, bool individualUniverse)
{
    if (individualUniverse) {
        for (auto const& m : stringData->GetModels()) {
            auto uni = m->GetUniverse();
            auto sizeofchan = m->Channels();

            while (sizeofchan > 0) {
                auto inpUn = std::find_if(inputUniverses.begin(), inputUniverses.end(), [uni](auto const& inp) {return inp.universe == uni; });
                if (inpUn != inputUniverses.end()) {
                    (*inpUn).index = index;
                    (*inpUn).hinksPixStartChannel = hinkstartChan;
                    sizeofchan -= (*inpUn).numOfChan;
                    uni++;
                    index++;
                    hinkstartChan += (*inpUn).numOfChan;
                }
            }
        }
    } else {
        for (auto const& m : stringData->GetModels()) {
            hinkstartChan += m->Channels();
        }
    }
}

void HinksPix::UpdateSerialData( HinksPixSerial & pd, UDControllerPort* serialData, int mode) const
{
    if (pd.mode != mode) {
        pd.mode = mode;
        pd.upload = true;
    }

    int dmxOffset = 1;
    UDControllerPortModel* m = serialData->GetFirstModel();
    if (m != nullptr) {
        dmxOffset = m->GetDMXChannelOffset();
        if (dmxOffset < 1 || dmxOffset > 512) dmxOffset = 1; // a value less than 1 makes no sense
    }

    const int sc = serialData->GetStartChannel();
    const int usc = serialData->GetUniverseStartChannel() - dmxOffset + 1; // it would be good if these functions behaved more like GetStartChannel does on serial ports 
    int maxChan = serialData->GetEndChannel() - sc + 1;

    if (maxChan < 16) maxChan = 16;

    if (pd.mode != 1) {//0 and 2 are E131 and Artnet
        if (pd.e131Universe != (serialData->GetUniverse())) {
            pd.e131Universe = serialData->GetUniverse();
            pd.upload = true;
        }

        if (pd.e131StartChannel != usc) {
            pd.e131StartChannel = usc;
            pd.upload = true;
        }

        if (pd.e131NumOfChan != maxChan) {
            pd.e131NumOfChan = maxChan;
            pd.upload = true;
        }

        if (!pd.e131Enabled) {
            pd.e131Enabled = true;
            pd.upload = true;
        }
    } else {//1 is DDP
        if (pd.ddpStartChannel != sc) {
            pd.ddpStartChannel = sc;
            pd.upload = true;
        }
        if (pd.ddpNumOfChan != maxChan) {
            pd.ddpNumOfChan = maxChan;
            pd.upload = true;
        }

        if (!pd.ddpEnabled) {
            pd.ddpEnabled = true;
            pd.upload = true;
        }
    }
}

void HinksPix::UploadSmartRecievers(bool& worked) const
{
    for (int exp = 0; exp < std::size(_smartOutputs); ++exp) {
        if (_EXP_Outputs[exp] != EXPType::Long_Range) {
            continue;
        }
        for (int bnk = 0; bnk < std::size(_smartOutputs[exp]); ++bnk) {
            UploadSmartRecieverData(exp, bnk, _smartOutputs[exp][bnk], worked);
        }
    }
}

void HinksPix::UploadSmartRecieverData(int expan, int bank, std::vector<HinksSmartOutput> const& receivers, bool& worked) const
{
    static log4cpp::Category& logger_base = log4cpp::Category::getInstance(std::string("log_base"));

    logger_base.debug("Building SmartReciever upload Expansion %d Bank %d:", expan, bank);
    if (receivers.size() == 0) {
        logger_base.info("No SmartReciever found");
        return;
    }
    //{"CMD":"SCONFIG","BOARD":"0","Port4":"0","LIST":[{"V":"0,1,1,1,1,1"},{"V":"1,0,51,51,51,51"},{"V":"2,0,101,101,101,101"},{"V":"3,0,151,151,151,151"},{"V":"6,2,0,1,0,0"},{"V":"8,0,201,1,1,1"}]}
    wxString requestString = wxString::Format("DATA: {\"CMD\":\"SCONFIG\",\"BOARD\":\"%d\",\"Port4\":\"%d\",\"LIST\":[",
        expan, bank);
    
    for (int i = 0; i < receivers.size(); i++) {
        receivers[i].Dump();
        requestString += receivers[i].BuildCommand();
        requestString += ",";
    }
    requestString.RemoveLast();//remove last ","
    requestString += "]}";

    auto const ret = GetJSONControllerData(GetJSONPostURL(), requestString);
    if (ret.find("\"OK\"") == std::string::npos) {
        logger_base.error("Failed Return %s", (const char*)ret.c_str());
        worked = false;
    }
}

void HinksPix::CalculateSmartRecievers(UDControllerPort* stringData)
{
    if (!stringData->AtLeastOneModelIsUsingSmartRemote()) {
        return;
    }
    int port = stringData->GetPort() - 1;
    int expansionBoard = port / 16;
    int expansionPort = port % 16;
    int bank = expansionPort / 4;
    int subPort = (expansionPort % 4);

    int prevID = -1;
    int start_pixels = 1;
    for (const auto& it : stringData->GetModels()) {
        if (it->GetSmartRemote() > 0) {
            int id = it->GetSmartRemote() - 1;
            if (prevID != id) {//set data when going from A->B
                auto smartOut = std::find_if(_smartOutputs[expansionBoard][bank].begin(), _smartOutputs[expansionBoard][bank].end(), [id](auto const& so) {
                    return so.id == id;
                    });
                if (smartOut != _smartOutputs[expansionBoard][bank].end()) {
                    smartOut->portStartPixel[subPort] = start_pixels;
                }
                else {
                    auto& smartPort = _smartOutputs[expansionBoard][bank].emplace_back(id);
                    //if (it->GetSmartRemoteType().find("16") != std::string::npos && ((id % 4) == 0 )){
                    //    smartPort.type = 1;
                    //}
                    smartPort.portStartPixel[subPort] = start_pixels;
                }
            }

            int32_t chans = it->GetEndChannel() - it->GetStartChannel() + 1;
            int pixs = chans/it->GetChannelsPerPixel();
            start_pixels += pixs;
            prevID = id;
        }
    }
}

//Most data uses a GET .cgi endpoint
wxString HinksPix::GetControllerData(int rowIndex, std::string const& data) const
{
    return GetControllerRowData(rowIndex, GetInfoURL(), data);
}

//E131 data uses a different .cgi endpoint
wxString HinksPix::GetControllerE131Data(int rowIndex) const
{
    return GetControllerRowData(rowIndex, GetE131URL(), std::string());
}

//all of the Controller data is retrieved/set by "GET"ing different ROW values
wxString HinksPix::GetControllerRowData(int rowIndex, std::string const& url, std::string const& data) const
{
    static log4cpp::Category& logger_base = log4cpp::Category::getInstance(std::string("log_base"));
    std::string res;
    std::string const baseIP = _fppProxy.empty() ? _ip : _fppProxy;

    logger_base.debug("Making request to HinksPix '%s'.", (const char*)url.c_str());

    CURL* curl = curl_easy_init();
    struct curl_slist* list = NULL;

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, std::string("http://" + baseIP + _baseUrl + url).c_str());
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20);
       
        list = curl_slist_append(list, "Content-type: text/plain");
        logger_base.debug("Row='%d'.", rowIndex);
        list = curl_slist_append(list, std::string("ROW: " + std::to_string(rowIndex)).c_str());
        
        if (!data.empty()) {
            list = curl_slist_append(list, std::string("DATA: " + data).c_str());
            logger_base.debug("DATA='%s'.", (const char*)data.c_str());
        }

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);

        std::string response_string;

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFunction);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);

        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 1L);

        /* Perform the request, res will get the return code */
        CURLcode r = curl_easy_perform(curl);

        if (r != CURLE_OK) {
            logger_base.error("Failure to access %s: %s.", (const char*)url.c_str(), curl_easy_strerror(r));
        }
        else {
            res = response_string;
        }

        /* always cleanup */
        curl_easy_cleanup(curl);
    }
    return res;
}

//all of the Controller data is retrieved/set by "GET"ing different values
std::string HinksPix::GetJSONControllerData(std::string const& url, std::string const& data) const
{
    static log4cpp::Category& logger_base = log4cpp::Category::getInstance(std::string("log_base"));
    std::string res;
    std::string const baseIP = _fppProxy.empty() ? _ip : _fppProxy;

    logger_base.debug("Making request to HinksPix '%s'.", (const char*)url.c_str());

    CURL* curl = curl_easy_init();
    struct curl_slist* list = NULL;

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, std::string("http://" + baseIP + _baseUrl + url).c_str());
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20);

        list = curl_slist_append(list, "Content-type: text/plain");

        if (!data.empty()) {
            list = curl_slist_append(list, std::string(data).c_str());
            logger_base.debug("'%s'.", (const char*)data.c_str());
        }

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);

        std::string response_string;

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFunction);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);

        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 1L);

        /* Perform the request, res will get the return code */
        CURLcode r = curl_easy_perform(curl);

        if (r != CURLE_OK) {
            logger_base.error("Failure to access %s: %s.", (const char*)url.c_str(), curl_easy_strerror(r));
        }
        else {
            res = response_string;
        }

        /* always cleanup */
        curl_easy_cleanup(curl);
    }
    return res;
}

bool HinksPix::GetControllerDataJSON(const std::string& url, wxJSONValue& val, std::string const& data) const{
    std::string const sval = GetJSONControllerData(url, data);
    if (!sval.empty()) {
        wxJSONReader reader;
        reader.Parse(sval, &val);
        return true;
    }
    return false;
}

std::map<wxString, wxString> HinksPix::StringToMap(wxString const& text) const
{
    std::map<wxString, wxString> map;
    const wxArrayString items = Split(text, {','});
    if(items.size() % 2 == 0) {
        for (int i =0; i < items.size() - 1; i += 2) {
            map[items[i]] = items[i + 1];
        }
    }
    return map;
}
#pragma endregion

#pragma region Encode and Decode
int HinksPix::EncodeStringPortProtocol(const std::string& protocol) const
{
    wxString p(protocol);
    p = p.Lower();

    if (p == "ws2811") return 1;
    if (p == "ws2812") return 2;//unused
    if (p == "ws2812b") return 3;//unused
    if (p == "ws2813") return 4;//unused
    if (p == "ws2801") return 5;
    if (p == "tls3001") return 6;
    if (p == "apa102") return 7;
    wxASSERT(false);
    return -1;
}

int HinksPix::EncodeColorOrder(const std::string& colorOrder) const
{
    wxString c(colorOrder);
    c = c.Lower();

    if (c == "rgb") return 0;
    if (c == "rbg") return 1;
    if (c == "grb") return 2;
    if (c == "gbr") return 3;
    if (c == "brg") return 4;
    if (c == "bgr") return 5;
    if (c == "rgbw") return 6;
    if (c == "wrgb") return 7;
    wxASSERT(false);
    return -1;
}

int HinksPix::EncodeDirection(const std::string& direction) const
{
    return direction == "Reverse";
}

int HinksPix::EncodeBrightness(int brightness) const
{
    //HinkPix Allows brightness values of 100,90,80,70,60,50,40,30,20,15
    const int newVal = (brightness / 10) * 10;//round to the 10's
    if (newVal < 20) {
        return 15;
    }
    return newVal;
}

int HinksPix::EncodeGamma(int gamma) const
{
    //HinkPix Allows gamma value of 1-4 as integer values
    if (gamma > 4) {
        return 4;
    }
    return gamma;
}

HinksPix::EXPType HinksPix::DecodeExpansionType(const std::string& type) const
{
    if (type == "S") {
        return EXPType::Local_SPI;
    }
    if (type == "L") {
        return EXPType::Long_Range;
    }
    if (type == "A") {
        return EXPType::Local_AC;
    }
    return EXPType::Not_Present;
}

#pragma endregion

#pragma region Constructors and Destructors
HinksPix::HinksPix(const std::string& ip, const std::string& proxy) : 
    BaseController(ip, proxy), 
    _EXP_Outputs{ EXPType::Not_Present, EXPType::Not_Present, EXPType::Not_Present },
    _numberOfOutputs(48)
{
    static log4cpp::Category& logger_base = log4cpp::Category::getInstance(std::string("log_base"));

    //Get Controller Info
    wxJSONValue data;

    for (int x = 0; x < 3; x++) {
        if (GetControllerDataJSON(GetJSONInfoURL(), data)) {
            break;
        }
    }

    if (data.Size() > 0) {
        //get output type options
        _EXP_Outputs[0] = DecodeExpansionType(data.Get("BD1","N").AsString());
        _EXP_Outputs[1] = DecodeExpansionType(data.Get("BD2", "N").AsString());
        _EXP_Outputs[2] = DecodeExpansionType(data.Get("BD3", "N").AsString());
        _connected = true;

        auto const pix_type = data.Get("Type", "").AsString();//"P" for Pixel, "A" for AC
        _controllerType = data.Get("Controller", "").AsString();//"H" for Pro, "E" for EasyLights

        if (_controllerType == "E") {
            _numberOfOutputs = 16;
        }

        _numberOfUniverses = wxAtoi(data.Get("MaxU", "0").AsString());

        _version = wxString::Format("MAIN:%s,POWER:%s,WIFI:%s,WEB:%s",
            data.Get("MCPU", "0").AsString().Mid(3),
            data.Get("PCPU", "0").AsString().Mid(3),
            data.Get("ECPU", "0").AsString().Mid(3),
            data.Get("WEB", "0").AsString().Mid(3));

        if (_controllerType == "E") {
            _model = "EasyLights Pix16";
        } else if (_controllerType == "H") {
            _model = "HinksPix PRO";
        } else {
            _model = "Unknown";
        }

        logger_base.debug("Connected to HinksPix controller model %s.", (const char*)GetFullName().c_str());
    } else {
        _connected = false;
        logger_base.error("Error connecting to HinksPix controller on %s.", (const char*)_ip.c_str());
        DisplayError(wxString::Format("Error connecting to HinksPix controller on %s.", _ip).ToStdString());
    }
}

HinksPix::~HinksPix()
{
    _pixelOutputs.clear();
}
#pragma endregion

#pragma region Getters and Setters
bool HinksPix::SetOutputs(ModelManager* allmodels, OutputManager* outputManager, ControllerEthernet* controller, wxWindow* parent)
{
    wxProgressDialog progress("Uploading ...", "", 100, parent, wxPD_APP_MODAL | wxPD_AUTO_HIDE);
    progress.Show();

    static log4cpp::Category& logger_base = log4cpp::Category::getInstance(std::string("log_base"));
    logger_base.debug("HinksPix Outputs Upload: Uploading to %s", (const char*)_ip.c_str());

    // Get universes based on IP
    std::list<Output*> outputs = controller->GetOutputs();

    auto o = outputs.front();

    int mode = 0;

    if (o->GetType() == OUTPUT_E131) {
        mode = 0;
    }
    else if (o->GetType() == OUTPUT_ARTNET) {
        mode = 2;
    }
    else if (o->GetType() == OUTPUT_DDP) {
        mode = 1;

        if (controller->IsUniversePerString()) {
            DisplayError("HinksPix Upload Error:\nUniverse Per String not allowws with DDP Ouptup", parent);
            progress.Update(100, "Aborting.");
            return false;
        }
    }

    progress.Update(0, "Scanning models");
    logger_base.info("Scanning models.");

    std::string check;
    UDController cud(controller, outputManager, allmodels, check, false);

    //first check rules
    auto rules = ControllerCaps::GetControllerConfig(controller);
    const bool success = cud.Check(rules, check);

    logger_base.debug(check);

    cud.Dump();
    if (!success) {
        DisplayError("HinksPix Upload Error:\n" + check, parent);
        progress.Update(100, "Aborting.");
        return false;
    }
    bool worked = true;

    logger_base.info("Initializing Pixel Output Information.");
    progress.Update(10, "Initializing Pixel Output Information.");

    InitControllerOutputData();

    logger_base.info("Calculating Universe Start Channel Mappings.");
    progress.Update(20, "Calculating Universe Start Channel Mappings.");
    std::vector<HinksPixInputUniverse> inputUniverses;

    for (auto const& it : outputs) {
        inputUniverses.emplace_back(it->GetUniverse(), it->GetChannels());
    }

    if (!controller->IsUniversePerString() && o->GetType() != OUTPUT_DDP) {
        //old way
        auto it = outputs.begin();
        int index = 0;
        int32_t startChan = 1;
        for (auto const& it : outputs) {
            auto uni = it->GetUniverse();
            auto inpUn = std::find_if(inputUniverses.begin(), inputUniverses.end(), [uni](auto const& inp) {return inp.universe == uni; });
            if (inpUn != inputUniverses.end()) {
                inpUn->index = index;
                inpUn->hinksPixStartChannel = startChan;
                index++;
                startChan += it->GetChannels();
            }
            inputUniverses.emplace_back(it->GetUniverse(), it->GetChannels());
        }
    }

    logger_base.info("Figuring Out Pixel Output Information.");
    progress.Update(30, "Figuring Out Pixel Output Information.");
    int32_t hinkstartChan = 1;
    int univIdx = 0;
    //loop to setup string outputs
    for (int port = 1; port <= GetNumberOfOutputs(); port++) {
        if (cud.HasPixelPort(port)) {
            UDControllerPort* portData = cud.GetControllerPixelPort(port);
            auto pixOut = std::find_if(_pixelOutputs.begin(), _pixelOutputs.end(), [port](auto const& po) {return po.output == port; });
            if (pixOut != _pixelOutputs.end()) {
                UpdatePortData(*pixOut, portData, hinkstartChan);
                UpdateUniverseControlerChannels(portData, inputUniverses, hinkstartChan, univIdx, controller->IsUniversePerString());
            }

            CalculateSmartRecievers(portData);
        }
    }

    logger_base.info("Uploading Input Universes Information.");
    progress.Update(40, "Uploading Input Universes Information.");
    UploadInputUniverses(controller, inputUniverses);

    logger_base.info("Uploading SmartRecievers Information.");
    progress.Update(50, "Uploading SmartRecievers Information.");
    UploadSmartRecievers(worked);

    logger_base.info("Uploading String Output Information.");
    progress.Update(60, "Uploading String Output Information.");
    UploadPixelOutputs(worked);

    logger_base.info("Figuring Out DMX Output Information.");
    progress.Update(70, "Figuring Out DMX Output Information.");

    if (cud.HasSerialPort(1)) {
        UDControllerPort* portData = cud.GetControllerSerialPort(1);
        UpdateSerialData(*_serialOutput, portData, mode);
    }

    logger_base.info("Uploading DMX Output Information.");
    progress.Update(80, "Uploading DMX Output Information.");

    _serialOutput->Dump();
    if (_serialOutput->upload) {
        const std::string serialRequest = _serialOutput->BuildCommand();
        auto const serRet = GetControllerData(4908, serialRequest);
        if (serRet != "done") {
            logger_base.error("4908 Return %s", (const char*)serRet.c_str());
            worked = false;
        }
    }

    //reboot
    logger_base.info("Rebooting Controller.");
    progress.Update(90, "Rebooting Controller.");
    auto const resetres = GetControllerData(1111);
    if (resetres != "done") {
        worked = false;
    }

    progress.Update(100, "Done.");
    return worked;
}
#pragma endregion 

