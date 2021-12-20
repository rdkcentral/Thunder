#include<iostream>
#define MODULE_NAME ProxyPoolStressTest
#include <core/core.h>
#include <vector>
#include <TypeTraits.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory>
#include <algorithm>

typedef struct TestStruct {
  TestStruct():a(0){}
  TestStruct(int i):a(i){}
  ~TestStruct(){}
  const int getA() const{return a;}
  int a;
}TestStruct_t;
namespace WPEFramework {

class TrafficGeneratorInterface
{
  public:
    virtual uint32_t getNextTimerValue() = 0;
    virtual uint32_t getObjectCount() = 0;
    virtual const string getName() const = 0;
    virtual ~TrafficGeneratorInterface() = default;
};

//Start of Traffic Generators Implementation
class TriangleTrafficGenerator: public TrafficGeneratorInterface {
  public:
    TriangleTrafficGenerator() = delete;
    TriangleTrafficGenerator(const TriangleTrafficGenerator&) = default;
    TriangleTrafficGenerator& operator=(const TriangleTrafficGenerator&) = delete;
    TriangleTrafficGenerator(uint32_t duration, uint32_t freq, uint32_t maxObjectCount): duration(duration)
                                                                     , freq(freq)
								     , maxObjectCount(maxObjectCount)
								     , elapsedDuration(0)
								     , stepValue(1)
								     , waveName("TriangularWave") {
    }
    TriangleTrafficGenerator(uint32_t duration, uint32_t freq, uint32_t maxObjectCount, uint32_t stepValue): duration(duration)
                                                                     , freq(freq)
								     , maxObjectCount(maxObjectCount)
								     , elapsedDuration(0)
								     , stepValue(stepValue)
								     , waveName("StepWave") {
    }


    uint32_t getNextTimerValue() override {
      elapsedDuration += stepValue;
      if (elapsedDuration > duration)
        return 0;
      else
        return stepValue;
    }
    uint32_t getObjectCount() override {
      uint32_t period = duration / (freq * 2);
      uint32_t objectCount = (maxObjectCount/period) * (period - abs( (int32_t)((elapsedDuration % (2 * period)) - period)));
      return objectCount;
    }
    const string getName() const override {
     return waveName;
    }
  private:
    uint32_t duration;
    uint32_t freq;
    uint32_t maxObjectCount;
    uint32_t elapsedDuration;
    uint32_t stepValue;
    string waveName;
    std::map<uint64_t, int32_t> report;
};

class SineTrafficGenerator : public TrafficGeneratorInterface {
  public:
    SineTrafficGenerator() = delete;
    SineTrafficGenerator(const SineTrafficGenerator&) = default;
    SineTrafficGenerator& operator=(const SineTrafficGenerator&) = delete;
    SineTrafficGenerator(uint32_t duration, uint32_t freq, uint32_t maxObjectCount): duration(duration)
                                                                     , freq(freq)
								     , maxObjectCount(maxObjectCount)
								     , elapsedTime(0)
								     , stepValue(1)
								     , waveName("SineWave") {
    }
    
    uint32_t getNextTimerValue() override {
      elapsedTime += stepValue;
      if(elapsedTime > duration)
        return 0;
      else
        return stepValue;
    }
    uint32_t getObjectCount() override {
      uint32_t objectCount  = ((maxObjectCount/2) * sin(elapsedTime * freq)) + (maxObjectCount/2);
      return objectCount;
    }
    const string getName() const override {
      return waveName;
    }
  private:
    uint32_t duration;
    uint32_t freq;
    uint32_t maxObjectCount;
    uint32_t elapsedTime;
    uint32_t stepValue;
    string waveName;
};
//End of Traffic Generators Implementation
template<typename TESTTYPE>
class TestManager;
template<typename TESTTYPE>
class TimeHandler {
  public:
    TimeHandler() = delete;
  
    TimeHandler(std::shared_ptr<TrafficGeneratorInterface> waveGenerator, TestManager<TESTTYPE>* parent, TESTTYPE& pool, string timerName): waveGenerator(waveGenerator)
                                                                                                                             , _parent(parent)
                                                                                                                             , proxy(pool)
															     , lastObjectReqCount(0)
															     , timerName(timerName)
															     , localList(){
    
    }

    ~TimeHandler() {
  }

  public:

    #if 0
    void addElement();
    void addInstance();
    #else
    IS_MEMBER_AVAILABLE(Element, hasElement);
    template<typename T = TESTTYPE>
    void addElement(typename Core::TypeTraits::enable_if<hasElement<T, WPEFramework::Core::ProxyType<TestStruct_t> >::value>::type* = nullptr ) {
      std::cout<<"Has Element function\n";
      localList.push_back(proxy.Element());
      return;
    }

    template<typename T = TESTTYPE>
    void addElement(typename Core::TypeTraits::enable_if<!hasElement<T, WPEFramework::Core::ProxyType<TestStruct_t> >::value>::type* = nullptr ) {
      addInstance();
      return;
    }

    IS_MEMBER_AVAILABLE(Vist, hasVisit);
    template<typename T = TESTTYPE>
    void addInstance(typename Core::TypeTraits::enable_if<hasVisit<T, WPEFramework::Core::ProxyType<TestStruct_t> >::value>::type* = nullptr ) {
      std::cout<<"Has Visit function\n";
      WPEFramework::Core::Time now = WPEFramework::Core::Time::Now();
      localList.push_back(proxy.template Instance<TestStruct_t>(now.Ticks(), now.Ticks() % 57 ));
      return;
    }
    template<typename T = TESTTYPE>
    void addInstance(typename Core::TypeTraits::enable_if<!hasVisit<T, WPEFramework::Core::ProxyType<TestStruct_t> >::value>::type* = nullptr ) {
      WPEFramework::Core::Time now = WPEFramework::Core::Time::Now();
      localList.push_back(proxy.template Instance<TestStruct_t>(now.Ticks() % 73));
      return;
    }
    #endif
    
    uint64_t Timed(const uint64_t scheduledTime);
    std::map<uint64_t, int32_t>& getReport();
  private:
    std::shared_ptr<TrafficGeneratorInterface> waveGenerator;
    TestManager<TESTTYPE>* _parent;
    TESTTYPE &proxy;
    uint32_t lastObjectReqCount;
    string timerName;
    std::vector<Core::ProxyType<TestStruct_t>> localList;
    std::map<uint64_t, int32_t> report;

};
#if 0
template<>
void TimeHandler<Core::ProxyPoolType<TestStruct_t>>::addElement() {
  WPEFramework::Core::Time now = WPEFramework::Core::Time::Now();
  localList.push_back(proxy.Element());
  return;
}

template<>
void TimeHandler<Core::ProxyListType<TestStruct_t>>::addElement() {
  WPEFramework::Core::Time now = WPEFramework::Core::Time::Now();
  localList.push_back(proxy.template Instance<TestStruct_t>(now.Ticks() % 73));
  return;
}

template<>
void TimeHandler<Core::ProxyMapType<uint64_t, TestStruct_t>>::addElement() {
  WPEFramework::Core::Time now = WPEFramework::Core::Time::Now();
  localList.push_back(proxy.template Instance<TestStruct_t>(now.Ticks(), now.Ticks() % 57 ));
  return;
}
#endif

template<typename TESTTYPE>
class TestManager {
  public:
    TestManager() = delete;
    TestManager(TESTTYPE& testpool, uint32_t duration, uint32_t freq, uint32_t maxObjectCount):  _duration(duration), _freq(freq), _maxObjectCount(maxObjectCount), _timerCount(0), cs(0,3),_pool(testpool){
      Initialize();
    }
    void Initialize() {
      CreateTrafficGeneratorList();
      CreateTimersList();
    }

    void StartTimers() {
      Core::Time nextTick = Core::Time::Now();
      for (unsigned int index = 0; index < _timersList.size(); index++ ) {
        std::cout<<"Starting timer: "<<index<<'\n';
        _timersList[index]->Schedule(nextTick.Ticks(), TimeHandler<TESTTYPE>(_waveGenerators[index], this, _pool, _waveGenerators[index]->getName()));
	_timerCount++;
        
      }
    }

    void NotifyTimerComplete(std::map<uint64_t, int32_t>& report) {
      for(const auto & elem : report) {
        masterReport.insert(std::pair<uint64_t, int32_t>(elem.first, elem.second));
      }
      std::cout<<"Timer Completed\n";
      cs.Unlock();
    }

    void WaitForTimersToComplete() {
      while(_timerCount > 0) {
	std::cout<<"TimerCount: "<<_timerCount<<"\n";
        cs.Lock();
	_timerCount--;
      }
      generateReport();
    }
    void generateReport() {
      int32_t peak{1024};
      int32_t total{0};
      uint32_t noOfRequest = std::count_if(masterReport.cbegin(), masterReport.cend(), [](std::pair<uint64_t, int32_t> element){ return element.second > 0? true:false;});
      uint32_t noOfRelease = std::count_if(masterReport.cbegin(), masterReport.cend(), [](std::pair<uint64_t, int32_t> element){ return element.second < 0? true:false;});
      auto lambda =  [&peak, &total](std::pair<uint64_t, int32_t> element)mutable{
                        total += element.second;
			if (total > peak)
			{
			  peak = total;
			}};
      std::for_each(masterReport.begin(), masterReport.end(), lambda);
      std::cout<<"No. of Request: "<< noOfRequest<<"\n";
      std::cout<<"No. of Release: "<< noOfRelease<<"\n";
      std::cout<<"Peak request: "<< peak<<"\n";


      return;
    }

    void setDuration(uint32_t duration) {
      _duration = duration;
    }


    void setFreq(uint32_t freq) {
      _freq = freq;
    }

    void setMaxObjectCount(uint32_t maxObjectCount) {
      _maxObjectCount = maxObjectCount;
    }

  private:
    void CreateTrafficGeneratorList(){
      _waveGenerators.emplace_back(new TriangleTrafficGenerator(_duration, _freq, _maxObjectCount));
      _waveGenerators.emplace_back(new TriangleTrafficGenerator(_duration, _freq, _maxObjectCount, 10));
      _waveGenerators.emplace_back(new SineTrafficGenerator(_duration, _freq, _maxObjectCount));
    }

    void CreateTimersList() {
      for (unsigned int index =0; index < _waveGenerators.size(); index++){
        _timersList.emplace_back(new Core::TimerType<TimeHandler<TESTTYPE>>(Core::Thread::DefaultStackSize(), _T(_waveGenerators[index]->getName().c_str())));
      }
    }
  private:
    uint32_t _duration;
    uint32_t _freq;
    uint32_t _maxObjectCount;
    uint32_t _timerCount;
    Core::CountingSemaphore cs;
    TESTTYPE& _pool;
    std::vector<std::shared_ptr<TrafficGeneratorInterface>> _waveGenerators;
    std::vector<std::unique_ptr<Core::TimerType<TimeHandler<TESTTYPE> > > > _timersList;
    std::multimap<uint64_t, uint32_t> masterReport;
};

template<typename TESTTYPE>
uint64_t TimeHandler<TESTTYPE>::Timed(const uint64_t scheduledTime) {
  Core::Time nextTick = Core::Time::Now();
  uint32_t time = waveGenerator->getNextTimerValue() * 1000;
  uint32_t objCount = waveGenerator->getObjectCount();
  int32_t delta = lastObjectReqCount - objCount;
  report.insert(std::pair<uint64_t, int32_t>(nextTick.Ticks(), delta * -1));
  if (lastObjectReqCount > objCount) {
    //Remove the delta from the list
    for(int index = 0; index <= abs(delta) && localList.size() > 0;index++ ) {
      localList.erase(localList.begin());
    }
  }
  else {
    //Request the delta from the Pool and add it to the pool
    for(int index =0; index<=abs(delta);index++) {
      addElement();
    }
  }
  lastObjectReqCount = objCount;
  if(time !=0){
    nextTick.Add(time);
    return nextTick.Ticks();
  }
  else {
    if(_parent != nullptr)
      _parent->NotifyTimerComplete(report);
    }
    return 0;
  }

  void performTest(uint32_t testObj, uint32_t duration, uint32_t freq, uint32_t maxMem)
  {
    WPEFramework::Core::ProxyPoolType<TestStruct_t> proxyPool(1024);
    WPEFramework::Core::ProxyListType<TestStruct_t> proxyList;
    WPEFramework::Core::ProxyMapType<uint64_t, TestStruct_t> proxyMap;
    if(testObj == 1) {
      std::cout<<"Running Stress test on ProxyMapType\n";
      TestManager <WPEFramework::Core::ProxyMapType<uint64_t, TestStruct_t>>tm(proxyMap, duration, freq, maxMem);
      tm.StartTimers();
      tm.WaitForTimersToComplete();
    } else if(testObj == 2) {
      std::cout<<"Running Stress test on ProxyListType\n";
      TestManager <WPEFramework::Core::ProxyListType<TestStruct_t> > tm(proxyList, duration, freq, maxMem);
      tm.StartTimers();
      tm.WaitForTimersToComplete();
    }
    else {
      std::cout<<"Running Stress test on ProxyPoolType\n";
      TestManager <WPEFramework::Core::ProxyPoolType<TestStruct_t>>  tm(proxyPool, duration, freq, maxMem);
      tm.StartTimers();
      tm.WaitForTimersToComplete();
    } 
    return;
  }

}

int main(int argc, char* argv[])
{
  int opt = 0;
  uint32_t duration = 120;
  uint32_t maxMem = 1024;
  uint32_t freq = 1;
  uint32_t objType = 0;
  while ((opt = getopt(argc, argv, "hd:m:f:t:")) !=-1) {
    switch(opt) {
     case 'd':
	std::cout<<"Duration provided:"<< atoi(optarg)<<" mins\n";
        duration = strtoul(optarg, NULL, 10) * 60;
        break;
     case 'm':
	std::cout<<"MaxMem provided:"<< atoi(optarg)<<"\n";
        maxMem = strtoul(optarg, NULL, 10);
        break;
     case 'f':
	std::cout<<"Freq provided:"<< atoi(optarg)<<"\n";
        freq = strtoul(optarg, NULL, 10);
        break;
     case 't':
         std::cout<<"Selecting Object Type:";
         objType = strtoul(optarg, NULL, 10);
       break;
     case 'h':
     default:
	std::cout<<"Usage: "<<argv[0]<<"-[h] [-d <Duration in mins>] [-m <max mem>] [-f <frequency>] [-t <0-ProxyPool|1-ProxyMap|2-ProxyList>]\n";
        exit(-1);
    }
  }
  WPEFramework::performTest(objType,duration, freq, maxMem);
  return 0;
}
