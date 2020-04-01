#pragma once

#include "SyncManager.h"
#include "ScheduleOptions.h"
#include <log4cpp/Category.hh>

class ListenerManager;

class SyncSMPTE : public SyncBase
{
    int _frameRate = 3;

    public:

        SyncSMPTE(SYNCMODE sm, REMOTEMODE rm, const ScheduleOptions& options, ListenerManager* listenerManager);
        SyncSMPTE(SyncSMPTE&& from);
        virtual ~SyncSMPTE();
        virtual void SendSync(uint32_t frameMS, uint32_t stepLengthMS, uint32_t stepMS, uint32_t playlistMS, const std::string& fseq, const std::string& media, const std::string& step, const std::string& timeItem) const override;
        virtual std::string GetType() const override { return "SMPTE"; }
        virtual void SendStop() const override;
};
