#ifndef ITVPLATFORM_H
#define ITVPLATFORM_H

#include <inttypes.h>
#include <string>
#include <vector>

enum TvmRc {
    TvmSuccess = 0,
    TvmError = -1
};

class ChannelDetails {
public:
    ChannelDetails()
        : type(Normal)
    {
    }
    enum Type {
        Normal,
        Radio,
        Data
    };

    const char* typeStr[3] = { "Normal", "Radio", "Data" };
    uint32_t frequency;
    uint16_t programNumber;
    Type type;
    // Audio Languages.
    // Subtitles.
};

class TSInfo {
public:
    TSInfo() = default;
    uint32_t frequency;
    uint16_t programNumber;
    uint16_t videoPid;
    uint16_t audioPid; // FIXME Multiple audio pids.
    uint16_t videoPcrPid;
    uint16_t audioPcrPid;
    uint16_t pmtPid;
    uint8_t videoCodec;
    uint8_t audioCodec;
};

typedef std::vector<TSInfo> TSInfoList;
typedef std::vector<ChannelDetails> ChannelMap;

enum ScanningState {
    Cleared,
    Scanned,
    Completed,
    Stopped
};

namespace TVPlatform {

class ITVPlatform {
public:
    class ISectionHandler {
    public:
        virtual void SectionDataCB(const std::string& msg) = 0;
    };

    class ITunerHandler {
    public:
        virtual void ScanningStateChanged(const ScanningState) = 0;
        virtual void CurrentChannelChanged(const std::string&) = 0;
        virtual void StreamingFrequencyChanged(uint32_t) = 0;
    };

    virtual TvmRc Init() = 0;
    virtual TvmRc Deinit() = 0;
    virtual TvmRc Tune(uint32_t) = 0;
    virtual TvmRc Scan(std::vector<uint32_t>, ITunerHandler&) = 0;
    virtual TvmRc Tune(TSInfo&, ITunerHandler&) = 0;
    virtual TvmRc StopScanning() = 0;
    virtual TvmRc GetChannelMap(ChannelMap&) = 0;
    virtual TvmRc GetTSInfo(TSInfoList&) = 0;
    virtual TvmRc Disconnect() = 0;
    virtual std::vector<uint32_t>& GetFrequencyList() = 0;

    virtual TvmRc SetHomeTS(uint32_t, uint32_t) = 0;
    virtual TvmRc StartFilter(uint16_t, uint8_t, ISectionHandler*) = 0;
    virtual TvmRc StopFilter(uint16_t, uint8_t) = 0;
    virtual TvmRc StopFilters() = 0;
    virtual void SetDbs(bool) = 0;
    virtual void SetTuneParameters(const std::string&) = 0;
    virtual bool IsScanning() = 0;
};

struct ISystemTVPlatform {
    virtual ITVPlatform* GetInstance() = 0;
};

template <typename IMPLEMENTATION>
class SystemTVPlatformType : public ISystemTVPlatform {
private:
    SystemTVPlatformType(const SystemTVPlatformType<IMPLEMENTATION>&) = delete;
    SystemTVPlatformType<IMPLEMENTATION>& operator=(const SystemTVPlatformType<IMPLEMENTATION>&) = delete;

public:
    SystemTVPlatformType () {
    }
    virtual ~SystemTVPlatformType () {
    }

public:
    virtual ITVPlatform* GetInstance() {
        return (&_instance);
    }

private:
    IMPLEMENTATION _instance;
};

} //namespace TVPlatform

#ifdef __cplusplus
extern "C" {
#endif

TVPlatform::ISystemTVPlatform*  GetSystemTVPlatform();

#ifdef __cplusplus
}
#endif

#endif // ITVPLATFORM_H
