
#include "IBluetooth.h"
#include "IBrowser.h"
#include "IComposition.h"
#include "IDictionary.h"
#include "INetflix.h"
#include "IContentDecryption.h"
#include "IProvisioning.h"
#include "IRPCLink.h"
#include "IResourceCenter.h"
#include "IStreaming.h"
#include "IGuide.h"
#include "IWebDriver.h"
#include "IWebServer.h"
#include "IPower.h"

MODULE_NAME_DECLARATION(BUILDREF_WEBBRIDGE)

namespace WPEFramework {
namespace ProxyStubs {

    using namespace Exchange;

    // -------------------------------------------------------------------------------------------
    // STUB
    // -------------------------------------------------------------------------------------------

    //
    // IBrowser interface stub definitions (interface/IBrowser.h)
    //
    ProxyStub::MethodHandler BrowserStubMethods[] = {
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual void SetURL(const string& URL) = 0;
            //
            RPC::Data::Frame::Reader parameters(message->Parameters().Reader());

            message->Parameters().Implementation<IBrowser>()->SetURL(parameters.Text());
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual string GetURL() const = 0;
            //
            RPC::Data::Frame::Writer response(message->Response().Writer());
            response.Text(message->Parameters().Implementation<IBrowser>()->GetURL());
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual uint32_t GetFPS() const = 0;
            //
            RPC::Data::Frame::Writer response(message->Response().Writer());
            response.Number(message->Parameters().Implementation<IBrowser>()->GetFPS());
        },
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual void Register(IBrowser::INotification* sink) = 0;
            //
            RPC::Data::Input& parameters(message->Parameters());
            RPC::Data::Frame::Reader reader(parameters.Reader());

            IBrowser::INotification* implementation = reader.Number<IBrowser::INotification*>();
            IBrowser::INotification* proxy = RPC::Administrator::Instance().CreateProxy<IBrowser::INotification>(channel,
                implementation,
                true, false);

            ASSERT((proxy != nullptr) && "Failed to create proxy");

            if (proxy == nullptr) {
                TRACE_L1(_T("Could not create a stub for IBrowserNotification: %p"), implementation);
            } else {
                parameters.Implementation<IBrowser>()->Register(proxy);
                if (proxy->Release() != Core::ERROR_NONE) {
                    TRACE_L1("Oops seems like we did not maintain a reference to this sink. %d", __LINE__);
                }
            }
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual void Unregister(IBrowser::INotification* sink) = 0;
            //
            RPC::Data::Input& parameters(message->Parameters());
            RPC::Data::Frame::Reader reader(parameters.Reader());

            // Need to find the proxy that goes with the given implementation..
            IBrowser::INotification* stub = reader.Number<IBrowser::INotification*>();

            // NOTE: FindProxy does *NOT* AddRef the result. Do not release what is obtained via FindProxy..
            IBrowser::INotification* proxy = RPC::Administrator::Instance().FindProxy<IBrowser::INotification>(stub);


            if (proxy == nullptr) {
                TRACE_L1(_T("Could not find stub for IBrowserNotification: %p"), stub);
            } else {
                parameters.Implementation<IBrowser>()->Unregister(proxy);
            }
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // Careful, this method is out of interface order
            //
            // virtual void Hide(const bool hidden) = 0
            //
            RPC::Data::Frame::Reader parameters(message->Parameters().Reader());

            message->Parameters().Implementation<IBrowser>()->Hide(parameters.Boolean());
        },
        nullptr
    };
    // IBrowser interface stub definitions

    //
    // IBrowser::INotification interface stub definitions (interface/IBrowser.h)
    //
    ProxyStub::MethodHandler BrowserNotificationStubMethods[] = {
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual void LoadFinished(const string& URL) = 0;
            //
            RPC::Data::Frame::Reader parameters(message->Parameters().Reader());
            string URL(parameters.Text());

            message->Parameters().Implementation<IBrowser::INotification>()->LoadFinished(URL);
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual void URLChanged(const string& URL) = 0;
            //
            RPC::Data::Frame::Reader parameters(message->Parameters().Reader());
            string URL(parameters.Text());

            ASSERT(message.IsValid() == true);
            ASSERT(message->Parameters().Implementation<IBrowser::INotification>() != nullptr);

            message->Parameters().Implementation<IBrowser::INotification>()->URLChanged(URL);
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual void Hidden(const bool hidden) = 0;
            //
            RPC::Data::Frame::Reader parameters(message->Parameters().Reader());
            bool hidden(parameters.Boolean());

            ASSERT(message.IsValid() == true);
            ASSERT(message->Parameters().Implementation<IBrowser::INotification>() != nullptr);

            message->Parameters().Implementation<IBrowser::INotification>()->Hidden(hidden);
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual void Closure() = 0;
            //
            ASSERT(message.IsValid() == true);
            ASSERT(message->Parameters().Implementation<IBrowser::INotification>() != nullptr);

            message->Parameters().Implementation<IBrowser::INotification>()->Closure();
        },
        nullptr
    };
    // IBrowser::INotification interface stub definitions

    ProxyStub::MethodHandler GuideStubMethods[] = {
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {

            RPC::Data::Input& parameters(message->Parameters());
            RPC::Data::Frame::Reader reader(parameters.Reader());
            RPC::Data::Frame::Writer writer(message->Response().Writer());

            PluginHost::IShell* implementation = reader.Number<PluginHost::IShell*>();
            PluginHost::IShell* proxy = RPC::Administrator::Instance().CreateProxy<PluginHost::IShell>(channel, implementation, true, false);

            ASSERT((proxy != nullptr) && "Failed to create proxy");

            if (proxy == nullptr) {
                TRACE_L1(_T("Could not create a stub for IGuide: %p"), implementation);
                writer.Number<uint32_t>(Core::ERROR_RPC_CALL_FAILED);
            }
            else {
                writer.Number(parameters.Implementation<IGuide>()->StartParser(proxy));
                if (proxy->Release() != Core::ERROR_NONE) {
                    TRACE_L1("Oops seems like we did not maintain a reference to this sink. %d", __LINE__);
                }
            }
        },
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& parameters(message->Parameters());
            RPC::Data::Frame::Reader reader(parameters.Reader());

            IGuide::INotification* implementation = reader.Number<IGuide::INotification*>();
            IGuide::INotification* proxy = RPC::Administrator::Instance().CreateProxy<IGuide::INotification>(channel, implementation, true, false);

            ASSERT((proxy != nullptr) && "Failed to create proxy");

            if (proxy == nullptr) {
                TRACE_L1(_T("Could not create a stub for IGuide::INotification: %p"), implementation);
            }
            else {
                parameters.Implementation<IGuide>()->Register(proxy);
                if (proxy->Release() != Core::ERROR_NONE) {
                    TRACE_L1("Oops seems like we did not maintain a reference to this sink. %d", __LINE__);
                }
            }
        },
        [](Core::ProxyType<Core::IPCChannel>&, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& parameters(message->Parameters());
            RPC::Data::Frame::Reader reader(parameters.Reader());

            IGuide::INotification* stub = reader.Number<IGuide::INotification*>();
            IGuide::INotification* proxy = RPC::Administrator::Instance().FindProxy<IGuide::INotification>(stub);

            if (proxy == nullptr) {
                TRACE_L1(_T("Could not find stub for IGuide::Notification: %p"), stub);
            }
            else {
                parameters.Implementation<IGuide>()->Unregister(proxy);
            }
        },
        [](Core::ProxyType<Core::IPCChannel>&, Core::ProxyType<RPC::InvokeMessage>& message) {
            // const string GetChannels()
            RPC::Data::Frame::Writer response(message->Response().Writer());
            response.Text(message->Parameters().Implementation<IGuide>()->GetChannels());
        },
        [](Core::ProxyType<Core::IPCChannel>&, Core::ProxyType<RPC::InvokeMessage>& message) {
            // const string GetPrograms(const uint32_t channelNum)
            RPC::Data::Frame::Writer response(message->Response().Writer());
            response.Text(message->Parameters().Implementation<IGuide>()->GetPrograms());
        },
        [](Core::ProxyType<Core::IPCChannel>&, Core::ProxyType<RPC::InvokeMessage>& message) {
            // const string GetCurrentProgram()
            RPC::Data::Frame::Writer response(message->Response().Writer());
            RPC::Data::Frame::Reader parameters(message->Parameters().Reader());
            response.Text(message->Parameters().Implementation<IGuide>()->GetCurrentProgram(parameters.Text()));
        },
        [](Core::ProxyType<Core::IPCChannel>&, Core::ProxyType<RPC::InvokeMessage>& message) {
            // const string GetAudioLanguages()
            RPC::Data::Frame::Writer response(message->Response().Writer());
            RPC::Data::Frame::Reader parameters(message->Parameters().Reader());
            response.Text(message->Parameters().Implementation<IGuide>()->GetAudioLanguages(parameters.Number<uint32_t>()));
        },
        [](Core::ProxyType<Core::IPCChannel>&, Core::ProxyType<RPC::InvokeMessage>& message) {
            // const string GetSubtitleLanguages()
            RPC::Data::Frame::Writer response(message->Response().Writer());
            RPC::Data::Frame::Reader parameters(message->Parameters().Reader());
            response.Text(message->Parameters().Implementation<IGuide>()->GetSubtitleLanguages(parameters.Number<uint32_t>()));
        },
        [](Core::ProxyType<Core::IPCChannel>&, Core::ProxyType<RPC::InvokeMessage>& message) {
            // bool SetParentalControlPin(const string&, const string&)
            RPC::Data::Frame::Writer response(message->Response().Writer());
            RPC::Data::Frame::Reader parameters(message->Parameters().Reader());
            string oldPin(parameters.Text());
            string newPin(parameters.Text());
            response.Boolean(message->Parameters().Implementation<IGuide>()->SetParentalControlPin(oldPin, newPin));
        },
        [](Core::ProxyType<Core::IPCChannel>&, Core::ProxyType<RPC::InvokeMessage>& message) {
            // bool SetParentalControl(const string&, const bool)
            RPC::Data::Frame::Writer response(message->Response().Writer());
            RPC::Data::Frame::Reader parameters(message->Parameters().Reader());
            string pin(parameters.Text());
            bool isLocked(parameters.Boolean());
            response.Boolean(message->Parameters().Implementation<IGuide>()->SetParentalControl(pin, isLocked));
        },
        [](Core::ProxyType<Core::IPCChannel>&, Core::ProxyType<RPC::InvokeMessage>& message) {
            // bool IsParentalControlled()
            RPC::Data::Frame::Writer response(message->Response().Writer());
            response.Boolean(message->Parameters().Implementation<IGuide>()->IsParentalControlled());

        },
        [](Core::ProxyType<Core::IPCChannel>&, Core::ProxyType<RPC::InvokeMessage>& message) {
            // bool SetParentalLock(const string&, const bool, const string&)
            RPC::Data::Frame::Writer response(message->Response().Writer());
            RPC::Data::Frame::Reader parameters(message->Parameters().Reader());
            string pin(parameters.Text());
            bool isLocked(parameters.Boolean());
            string channelNum(parameters.Text());
            response.Boolean(message->Parameters().Implementation<IGuide>()->SetParentalLock(pin, isLocked, channelNum));
        },
        [](Core::ProxyType<Core::IPCChannel>&, Core::ProxyType<RPC::InvokeMessage>& message) {
            // bool IsParentalLocked(const uint32_t);
            RPC::Data::Frame::Writer response(message->Response().Writer());
            RPC::Data::Frame::Reader parameters(message->Parameters().Reader());
            string channelNum(parameters.Text());
            response.Boolean(message->Parameters().Implementation<IGuide>()->IsParentalLocked(channelNum));
        },
        nullptr
    };

    //
    // IWebDriver interface stub definitions (interface/IWebDriver.h)
    //
    ProxyStub::MethodHandler WebDriverStubMethods[] = {
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual uint32_t Configure(PluginHost::IShell* framework) = 0;
            //
            RPC::Data::Input& parameters(message->Parameters());
            RPC::Data::Frame::Reader reader(parameters.Reader());
            RPC::Data::Frame::Writer writer(message->Response().Writer());

            PluginHost::IShell* implementation = reader.Number<PluginHost::IShell*>();
            PluginHost::IShell* proxy = RPC::Administrator::Instance().CreateProxy<PluginHost::IShell>(channel, implementation,
                true, false);

            ASSERT((proxy != nullptr) && "Failed to create proxy");

            if (proxy == nullptr) {
                TRACE_L1(_T("Could not create a stub for WebDriver: %p"), implementation);
                writer.Number<uint32_t>(Core::ERROR_RPC_CALL_FAILED);
            } else {
                writer.Number(parameters.Implementation<IWebDriver>()->Configure(proxy));
                if (proxy->Release() != Core::ERROR_NONE) {
                    TRACE_L1("Oops seems like we did not maintain a reference to this sink. %d", __LINE__);
                }
            }
        },
        nullptr
    };
    // IWebDriver interface stub definitions

    //
    // IBluetooth interface stub definitions (interface/IBluetooth.h)
    //
     ProxyStub::MethodHandler BluetoothStubMethods[] = {
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            // virtual uint32_t Configure(PluginHost::IShell* service) = 0;
            RPC::Data::Input& parameters(message->Parameters());
            RPC::Data::Frame::Reader reader(parameters.Reader());
            RPC::Data::Frame::Writer writer(message->Response().Writer());

            PluginHost::IShell* implementation = reader.Number<PluginHost::IShell*>();
            PluginHost::IShell* proxy = RPC::Administrator::Instance().CreateProxy<PluginHost::IShell>(channel, implementation, true, false);

            ASSERT((proxy != nullptr) && "Failed to create proxy");

            if (proxy == nullptr) {
                TRACE_L1(_T("Could not create a stub for Bluetooth: %p"), implementation);
                writer.Number<uint32_t>(Core::ERROR_RPC_CALL_FAILED);
            } else {
                writer.Number(parameters.Implementation<IBluetooth>()->Configure(proxy));
                if (proxy->Release() != Core::ERROR_NONE) {
                    TRACE_L1("Oops seems like we did not maintain a reference to this sink. %d", __LINE__);
                }
            }
        },
        [](Core::ProxyType<Core::IPCChannel>&, Core::ProxyType<RPC::InvokeMessage>& message) {
            // Scan()
            RPC::Data::Frame::Writer response(message->Response().Writer());
            response.Boolean(message->Parameters().Implementation<IBluetooth>()->Scan());
        },
        [](Core::ProxyType<Core::IPCChannel>&, Core::ProxyType<RPC::InvokeMessage>& message) {
            // StopScan()
            RPC::Data::Frame::Writer response(message->Response().Writer());
            response.Boolean(message->Parameters().Implementation<IBluetooth>()->StopScan());
        },
        [](Core::ProxyType<Core::IPCChannel>&, Core::ProxyType<RPC::InvokeMessage>& message) {
            // DiscoveredDevices()
            RPC::Data::Frame::Writer response(message->Response().Writer());
            response.Text(message->Parameters().Implementation<IBluetooth>()->DiscoveredDevices());
        },
        [](Core::ProxyType<Core::IPCChannel>&, Core::ProxyType<RPC::InvokeMessage>& message) {
            // PairedDevices()
            RPC::Data::Frame::Writer response(message->Response().Writer());
            response.Text(message->Parameters().Implementation<IBluetooth>()->PairedDevices());
        },
        [](Core::ProxyType<Core::IPCChannel>&, Core::ProxyType<RPC::InvokeMessage>& message) {
            // Pair()
            RPC::Data::Frame::Writer response(message->Response().Writer());
            RPC::Data::Frame::Reader parameters(message->Parameters().Reader());
            response.Boolean(message->Parameters().Implementation<IBluetooth>()->Pair(parameters.Text()));
        },
        [](Core::ProxyType<Core::IPCChannel>&, Core::ProxyType<RPC::InvokeMessage>& message) {
            // Connect()
            RPC::Data::Frame::Writer response(message->Response().Writer());
            RPC::Data::Frame::Reader parameters(message->Parameters().Reader());
            response.Boolean(message->Parameters().Implementation<IBluetooth>()->Connect(parameters.Text()));
        },
        [](Core::ProxyType<Core::IPCChannel>&, Core::ProxyType<RPC::InvokeMessage>& message) {
            // Disconnect()
            RPC::Data::Frame::Writer response(message->Response().Writer());
            response.Boolean(message->Parameters().Implementation<IBluetooth>()->Disconnect());
        },
        [](Core::ProxyType<Core::IPCChannel>&, Core::ProxyType<RPC::InvokeMessage>& message) {
            // IsScanning()
            RPC::Data::Frame::Writer response(message->Response().Writer());
            response.Boolean(message->Parameters().Implementation<IBluetooth>()->IsScanning());
        },
        [](Core::ProxyType<Core::IPCChannel>&, Core::ProxyType<RPC::InvokeMessage>& message) {
            // Connected()
            RPC::Data::Frame::Writer response(message->Response().Writer());
            response.Text(message->Parameters().Implementation<IBluetooth>()->Connected());
        },
        nullptr
    };
    // IBluetooth interface stub definitions

    //
    // IOCDM interface stub definitions (interface/IOCDM.h)
    //
    ProxyStub::MethodHandler OpenCDMiStubMethods[] = {
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual uint32_t Configure(PluginHost::IShell* service) = 0;
            //
            RPC::Data::Input& parameters(message->Parameters());
            RPC::Data::Frame::Reader reader(parameters.Reader());
            RPC::Data::Frame::Writer writer(message->Response().Writer());

            PluginHost::IShell* implementation = reader.Number<PluginHost::IShell*>();
            PluginHost::IShell* proxy = RPC::Administrator::Instance().CreateProxy<PluginHost::IShell>(channel, implementation,
                true, false);

            ASSERT((proxy != nullptr) && "Failed to create proxy");

            if (proxy == nullptr) {
                TRACE_L1(_T("Could not create a stub for WebDriver: %p"), implementation);
                writer.Number<uint32_t>(Core::ERROR_RPC_CALL_FAILED);
            } else {
                writer.Number(parameters.Implementation<IContentDecryption>()->Configure(proxy));
                if (proxy->Release() != Core::ERROR_NONE) {
                    TRACE_L1("Oops seems like we did not maintain a reference to this sink. %d", __LINE__);
                }
            }
        },
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual void Reset() = 0;
            //
            RPC::Data::Frame::Writer response(message->Response().Writer());
            response.Number(message->Parameters().Implementation<IContentDecryption>()->Reset());
        },
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual RPC::IStringIterator* Systems() const = 0;
            //
            RPC::Data::Frame::Writer response(message->Response().Writer());
            response.Number(message->Parameters().Implementation<IContentDecryption>()->Systems());
        },
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual RPC::IStringIterator* Designators(const string& keySystem) const = 0;
            //
            RPC::Data::Frame::Reader reader(message->Parameters().Reader());
            RPC::Data::Frame::Writer response(message->Response().Writer());
            string keySystem (reader.Text());
            response.Number(message->Parameters().Implementation<IContentDecryption>()->Designators(keySystem));
        },
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual RPC::IStringIterator* Sessions(const string& keySystem) const = 0;
            //
            RPC::Data::Frame::Reader reader(message->Parameters().Reader());
            RPC::Data::Frame::Writer response(message->Response().Writer());
            string keySystem (reader.Text());
            response.Number(message->Parameters().Implementation<IContentDecryption>()->Sessions(keySystem));
        },
        nullptr
    };
    // IContentDecryption interface stub definitions

    //
    // INetflix interface stub definitions (interface/INetflix.h)
    //
    ProxyStub::MethodHandler NetflixStubMethods[] = {
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual void Register(INetflix::INotification* netflix) = 0;
            //
            RPC::Data::Input& parameters(message->Parameters());
            RPC::Data::Frame::Reader reader(parameters.Reader());

            INetflix::INotification* implementation = reader.Number<INetflix::INotification*>();
            INetflix::INotification* proxy = RPC::Administrator::Instance().CreateProxy<INetflix::INotification>(channel,
                implementation,
                true, false);

            ASSERT((proxy != nullptr) && "Failed to create proxy");

            if (proxy == nullptr) {
                TRACE_L1(_T("Could not create a stub for INetflixNotification: %p"), implementation);
            } else {
                parameters.Implementation<INetflix>()->Register(proxy);
                if (proxy->Release() != Core::ERROR_NONE) {
                    TRACE_L1("Oops seems like we did not maintain a reference to this sink. %d", __LINE__);
                }
            }
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual void Unregister(INetflix::INotification* netflix) = 0;
            //
            RPC::Data::Input& parameters(message->Parameters());
            RPC::Data::Frame::Reader reader(parameters.Reader());

            // Need to find the proxy that goes with the given implementation..
            INetflix::INotification* stub = reader.Number<INetflix::INotification*>();

            // NOTE: FindProxy does *NOT* AddRef the result. Do not release what is obtained via FindProxy..
            INetflix::INotification* proxy = RPC::Administrator::Instance().FindProxy<INetflix::INotification>(stub);

            if (proxy == nullptr) {
                TRACE_L1(_T("Could not find stub for IBrowserNotification: %p"), stub);
            } else {
                parameters.Implementation<INetflix>()->Unregister(proxy);
            }
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual string GetESN() const = 0;
            //
            RPC::Data::Frame::Writer response(message->Response().Writer());
            response.Text(message->Parameters().Implementation<INetflix>()->GetESN());
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual void SystemCommand(const string& command) = 0;
            //
            RPC::Data::Frame::Reader parameters(message->Parameters().Reader());
            string element(parameters.Text());
            message->Parameters().Implementation<INetflix>()->SystemCommand(element);
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual void Language(const string& language) = 0;
            //
            RPC::Data::Frame::Reader parameters(message->Parameters().Reader());
            string element(parameters.Text());
            message->Parameters().Implementation<INetflix>()->Language(element);
        },
        nullptr
    };
    // INetflix interface stub definitions

    //
    // INetflix::INotification interface stub definitions (interface/INetflix.h)
    //
    ProxyStub::MethodHandler NetflixNotificationStubMethods[] = {
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual void StateChange(const INetflix::state state) = 0;
            //
            RPC::Data::Frame::Reader parameters(message->Parameters().Reader());
            INetflix::state newState(parameters.Number<INetflix::state>());

            message->Parameters().Implementation<INetflix::INotification>()->StateChange(newState);
        },
        nullptr
    };
    // INetflix::INotification interface stub definitions

    //
    // IResourceCenter interface stub definitions (interface/IResourceCenter.h)
    //
    ProxyStub::MethodHandler ResourceCenterStubMethods[] = {
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual uint32_t Configure(PluginHost::IShell* service) = 0;
            //
            RPC::Data::Input& parameters(message->Parameters());
            RPC::Data::Frame::Reader reader(parameters.Reader());
            RPC::Data::Frame::Writer writer(message->Response().Writer());

            PluginHost::IShell* implementation = reader.Number<PluginHost::IShell*>();
            PluginHost::IShell* proxy = RPC::Administrator::Instance().CreateProxy<PluginHost::IShell>(
                channel, implementation, true, false);

            ASSERT((proxy != nullptr) && "Failed to create proxy");

            if (proxy == nullptr) {
                TRACE_L1(_T("Could not create a stub for IResourceCenter::INotification: %p"), implementation);
            } else {
                writer.Number(parameters.Implementation<IResourceCenter>()->Configure(proxy));
                if (proxy->Release() != Core::ERROR_NONE) {
                    TRACE_L1("Oops seems like we did not maintain a reference to this sink. %d", __LINE__);
                }
            }
        },
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual void Register(IResourceCenter::INotification* provisioning) = 0;
            //
            RPC::Data::Input& parameters(message->Parameters());
            RPC::Data::Frame::Reader reader(parameters.Reader());

            IResourceCenter::INotification* implementation = reader.Number<IResourceCenter::INotification*>();
            IResourceCenter::INotification* proxy = RPC::Administrator::Instance().CreateProxy<IResourceCenter::INotification>(
                channel, implementation, true, false);

            ASSERT((proxy != nullptr) && "Failed to create proxy");

            if (proxy == nullptr) {
                TRACE_L1(_T("Could not create a stub for IResourceCenter::INotification: %p"), implementation);
            } else {
                parameters.Implementation<IResourceCenter>()->Register(proxy);
                if (proxy->Release() != Core::ERROR_NONE) {
                    TRACE_L1("Oops seems like we did not maintain a reference to this sink. %d", __LINE__);
                }
            }
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual void Unregister(IResourceCenter::INotification* provisioning) = 0;
            //
            RPC::Data::Input& parameters(message->Parameters());
            RPC::Data::Frame::Reader reader(parameters.Reader());

            // Need to find the proxy that goes with the given implementation..
            IResourceCenter::INotification* stub = reader.Number<IResourceCenter::INotification*>();

            // NOTE: FindProxy does *NOT* AddRef the result. Do not release what is obtained via FindProxy..
            IResourceCenter::INotification* proxy = RPC::Administrator::Instance().FindProxy<IResourceCenter::INotification>(
                stub);

            if (proxy == nullptr) {
                TRACE_L1(_T("Could not find stub for IResourceCenter::INotification: %p"), stub);
            } else {
                parameters.Implementation<IResourceCenter>()->Unregister(proxy);
            }
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual uint8_t Identifier(const uint8_t maxLength, uint8_t buffer[]) const = 0;
            //
            RPC::Data::Frame::Writer response(message->Response().Writer());
            response.Number<Exchange::IResourceCenter::hardware_state>(
                message->Parameters().Implementation<IResourceCenter>()->HardwareState());
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual hardware_state HardwareState() const = 0;
            //
            RPC::Data::Frame::Writer response(message->Response().Writer());
            RPC::Data::Frame::Reader parameters(message->Parameters().Reader());
            uint8_t length = parameters.Number<uint8_t>();
            uint8_t buffer[256];

            length = message->Parameters().Implementation<IResourceCenter>()->Identifier(length, buffer);
            response.Buffer(length, buffer);
        },
        nullptr
    };
    // IResourceCenter interface stub definitions

    //
    // IResourceCenter::INotification interface stub definitions (interface/IResourceCenter.h)
    //
    ProxyStub::MethodHandler ResourceCenterNotificationStubMethods[] = {
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual void StateChange(Exchange::IResourceCenter::hardware_state state) = 0;
            //
            RPC::Data::Input& parameters(message->Parameters());
            RPC::Data::Frame::Reader reader(parameters.Reader());

            // virtual void StateChange (Exchange::IResourceCenter::hardware_state state) = 0;
            message->Parameters().Implementation<IResourceCenter::INotification>()->StateChange(
                reader.Number<Exchange::IResourceCenter::hardware_state>());
        },
        nullptr
    };
    // IResourceCenter::INotification interface stub definitions

    //
    // IProvisioning interface stub definitions (interface/IProvisioning.h)
    //
    ProxyStub::MethodHandler ProvisioningStubMethods[] = {
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual void Register(IProvisioning::INotification* provisioning) = 0;
            //
            RPC::Data::Input& parameters(message->Parameters());
            RPC::Data::Frame::Reader reader(parameters.Reader());

            IProvisioning::INotification* implementation = reader.Number<IProvisioning::INotification*>();
            IProvisioning::INotification* proxy = RPC::Administrator::Instance().CreateProxy<IProvisioning::INotification>(
                channel, implementation, true, false);

            ASSERT((proxy != nullptr) && "Failed to create proxy");

            if (proxy == nullptr) {
                TRACE_L1(_T("Could not create a stub for IProvisioningNotification: %p"), implementation);
            } else {
                parameters.Implementation<IProvisioning>()->Register(proxy);
                if (proxy->Release() != Core::ERROR_NONE) {
                    TRACE_L1("Oops seems like we did not maintain a reference to this sink. %d", __LINE__);
                }
            }
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual void Unregister(IProvisioning::INotification* provisioning) = 0;
            //
            RPC::Data::Input& parameters(message->Parameters());
            RPC::Data::Frame::Reader reader(parameters.Reader());

            // Need to find the proxy that goes with the given implementation..
            IProvisioning::INotification* stub = reader.Number<IProvisioning::INotification*>();

            // NOTE: FindProxy does *NOT* AddRef the result. Do not release what is obtained via FindProxy..
            IProvisioning::INotification* proxy = RPC::Administrator::Instance().FindProxy<IProvisioning::INotification>(stub);

            if (proxy == nullptr) {
                TRACE_L1(_T("Could not find a stub for IProvisioningNotification: %p"), stub);
            } else {
                parameters.Implementation<IProvisioning>()->Unregister(proxy);
            }
        },
        nullptr
    };
    // IProvisioning interface stub definitions

    //
    // IProvisioning::INotification interface stub definitions (interface/IProvisioning.h)
    //
    ProxyStub::MethodHandler ProvisioningNotificationStubMethods[] = {
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual void Provisioned(const string& component) = 0;
            //
            RPC::Data::Frame::Reader parameters(message->Parameters().Reader());
            string element(parameters.Text());

            message->Parameters().Implementation<IProvisioning::INotification>()->Provisioned(element);
        },
        nullptr
    };
    // IProvisioning::INotification interface stub definitions

    //
    // IComposition interface stub definitions (interface/IComposition.h)
    //
    ProxyStub::MethodHandler CompositionStubMethods[] = {
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual void Register(IComposition::INotification* notification) = 0;
            //
            RPC::Data::Input& parameters(message->Parameters());
            RPC::Data::Frame::Reader reader(parameters.Reader());

            IComposition::INotification* implementation = reader.Number<IComposition::INotification*>();
            IComposition::INotification* proxy = RPC::Administrator::Instance().CreateProxy<IComposition::INotification>(
                channel, implementation, true, false);

            ASSERT((proxy != nullptr) && "Failed to create proxy");

            if (proxy == nullptr) {
                TRACE_L1(_T("Could not create a stub for ICompositionNotification: %p"), implementation);
            } else {
                parameters.Implementation<IComposition>()->Register(proxy);
                if (proxy->Release() != Core::ERROR_NONE) {
                    TRACE_L1("Oops seems like we did not maintain a reference to this sink. %d", __LINE__);
                }
            }
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual void Unregister(IComposition::INotification* notification) = 0;
            //
            RPC::Data::Input& parameters(message->Parameters());
            RPC::Data::Frame::Reader reader(parameters.Reader());

            // Need to find the proxy that goes with the given implementation..
            IComposition::INotification* stub = reader.Number<IComposition::INotification*>();

            // NOTE: FindProxy does *NOT* AddRef the result. Do not release what is obtained via FindProxy..
            IComposition::INotification* proxy = RPC::Administrator::Instance().FindProxy<IComposition::INotification>(stub);

            if (proxy == nullptr) {
                TRACE_L1(_T("Could not find a stub for ICompositionNotification: %p"), stub);
            } else {
                parameters.Implementation<IComposition>()->Unregister(proxy);
            }
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual IClient* Client(const uint8_t index) = 0;
            //
            RPC::Data::Input& parameters(message->Parameters());
            RPC::Data::Frame::Reader reader(parameters.Reader());
            RPC::Data::Frame::Writer output(message->Response().Writer());

            output.Number<IComposition::IClient*>(parameters.Implementation<IComposition>()->Client(reader.Number<uint8_t>()));
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual IClient* Client(const string& name) = 0;
            //
            RPC::Data::Input& parameters(message->Parameters());
            RPC::Data::Frame::Reader reader(parameters.Reader());
            RPC::Data::Frame::Writer output(message->Response().Writer());

            output.Number<IComposition::IClient*>(parameters.Implementation<IComposition>()->Client(reader.Text()));
        },
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual uint32_t Configure(PluginHost::IShell* service) = 0;
            //
            RPC::Data::Input& parameters(message->Parameters());
            RPC::Data::Frame::Reader reader(parameters.Reader());
            RPC::Data::Frame::Writer writer(message->Response().Writer());

            PluginHost::IShell* implementation = reader.Number<PluginHost::IShell*>();
            PluginHost::IShell* proxy = RPC::Administrator::Instance().CreateProxy<PluginHost::IShell>(
                    channel, implementation, true, false);

            ASSERT((proxy != nullptr) && "Failed to create proxy");

            if (proxy == nullptr) {
                TRACE_L1(_T("Could not create a proxy for PluginHost::IShell: %p"), implementation);
            } else {
                writer.Number(parameters.Implementation<IComposition>()->Configure(proxy));
                if (proxy->Release() != Core::ERROR_NONE) {
                    TRACE_L1("Oops seems like we did not maintain a reference to this sink. %d", __LINE__);
                }
            }
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual void SetResolution(const ScreenResolution) = 0;
            //
            RPC::Data::Input& parameters(message->Parameters());
            RPC::Data::Frame::Reader reader(parameters.Reader());

            parameters.Implementation<IComposition>()->SetResolution(reader.Number<IComposition::ScreenResolution>());
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual const ScreenResolution GetResolution() = 0;
            //
            RPC::Data::Input& parameters(message->Parameters());
            RPC::Data::Frame::Writer writer(message->Response().Writer());

            writer.Number<IComposition::ScreenResolution>(parameters.Implementation<IComposition>()->GetResolution());
        },
        nullptr
    };
    // IComposition interface stub definitions

    //
    // IComposition::IClient interface stub definitions (interface/IComposition.h)
    //
    ProxyStub::MethodHandler CompositionClientStubMethods[] = {
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual string Name() const = 0;
            //
            RPC::Data::Input& parameters(message->Parameters());
            RPC::Data::Frame::Writer writer(message->Response().Writer());

            writer.Text(parameters.Implementation<IComposition::IClient>()->Name());
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual void Kill() = 0;
            //
            RPC::Data::Input& parameters(message->Parameters());

            parameters.Implementation<IComposition::IClient>()->Kill();
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual void Opacity(const uint32_t value) = 0;
            //
            RPC::Data::Input& parameters(message->Parameters());
            RPC::Data::Frame::Reader reader(parameters.Reader());

            parameters.Implementation<IComposition::IClient>()->Opacity(reader.Number<uint32_t>());
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual void Geometry(const uint32_t X, const uint32_t Y, const uint32_t width, const uint32_t height) = 0;
            //
            RPC::Data::Input& parameters(message->Parameters());
            RPC::Data::Frame::Reader reader(parameters.Reader());

            uint32_t X(reader.Number<uint32_t>());
            uint32_t Y(reader.Number<uint32_t>());
            uint32_t width(reader.Number<uint32_t>());
            uint32_t height(reader.Number<uint32_t>());

            parameters.Implementation<IComposition::IClient>()->Geometry(X, Y, width, height);
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual void Visible(const bool visible) = 0;
            //
            RPC::Data::Input& parameters(message->Parameters());
            RPC::Data::Frame::Reader reader(parameters.Reader());

            parameters.Implementation<IComposition::IClient>()->Visible(reader.Boolean());
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual void SetTop() = 0;
            //
            RPC::Data::Input& parameters(message->Parameters());

            parameters.Implementation<IComposition::IClient>()->SetTop();
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual void SetInput() = 0;
            //
            RPC::Data::Input& parameters(message->Parameters());

            parameters.Implementation<IComposition::IClient>()->SetInput();
        },
        nullptr
    };
    // IComposition::IClient interface stub definitions

    //
    // IComposition::INotification interface stub definitions (interface/IComposition.h)
    //
    ProxyStub::MethodHandler CompositionNotificationStubMethods[] = {
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual void Attached(IClient* client) = 0;
            //
            RPC::Data::Input& parameters(message->Parameters());
            RPC::Data::Frame::Reader reader(parameters.Reader());
            RPC::Data::Frame::Writer writer(message->Response().Writer());

            IComposition::IClient* implementation = reader.Number<IComposition::IClient*>();
            IComposition::IClient* proxy = RPC::Administrator::Instance().CreateProxy<IComposition::IClient>(channel,
                implementation,
                true, false);

            ASSERT((proxy != nullptr) && "Failed to create proxy");

            if (proxy == nullptr) {
                TRACE_L1(_T("Could not create a stub for IComposition::IClient %p"), implementation);
            } else {
                parameters.Implementation<IComposition::INotification>()->Attached(proxy);
                if (proxy->Release() != Core::ERROR_NONE) {
                    TRACE_L1("Oops seems like we did not maintain a reference to this sink. %d", __LINE__);
                }
            }
        },
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual void Detached(IClient* client) = 0;
            //
            RPC::Data::Input& parameters(message->Parameters());
            RPC::Data::Frame::Reader reader(parameters.Reader());
            RPC::Data::Frame::Writer writer(message->Response().Writer());

            IComposition::IClient* implementation = reader.Number<IComposition::IClient*>();
            IComposition::IClient* proxy = RPC::Administrator::Instance().CreateProxy<IComposition::IClient>(channel,
                implementation,
                true, false);

            ASSERT((proxy != nullptr) && "Failed to create proxy");

            if (proxy == nullptr) {
                TRACE_L1(_T("Could not create a stub for IComposition::IClient %p"), implementation);
            } else {
                parameters.Implementation<IComposition::INotification>()->Detached(proxy);
                if (proxy->Release() != Core::ERROR_NONE) {
                    TRACE_L1("Oops seems like we did not maintain a reference to this sink. %d", __LINE__);
                }
            }
        },
        nullptr
    };
    // IComposition::INotification interface stub definitions

    //
    // IWebServer interface stub definitions (interface/IWebServer.h)
    //
    ProxyStub::MethodHandler WebServerStubMethods[] = {
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual void AddProxy(const string& path, const string& subst, const string& address) = 0;
            //
            RPC::Data::Frame::Reader parameters(message->Parameters().Reader());
            string path(parameters.Text());
            string subst(parameters.Text());
            string address(parameters.Text());

            message->Parameters().Implementation<IWebServer>()->AddProxy(path, subst, address);
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual void RemoveProxy(const string& path) = 0;
            //
            RPC::Data::Frame::Reader parameters(message->Parameters().Reader());
            string path(parameters.Text());

            message->Parameters().Implementation<IWebServer>()->RemoveProxy(path);
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual string Accessor() const = 0;
            //
            string accessorUrl = message->Parameters().Implementation<IWebServer>()->Accessor();
            RPC::Data::Frame::Writer output(message->Response().Writer());
            output.Text(accessorUrl);
        },
        nullptr
    };
    // IWebServer interface stub definitions

    ProxyStub::MethodHandler TunerStubMethods[] = {
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            // virtual uint32_t Configure(PluginHost::IShell* framework) = 0;
            RPC::Data::Input& parameters(message->Parameters());
            RPC::Data::Frame::Reader reader(parameters.Reader());
            RPC::Data::Frame::Writer writer(message->Response().Writer());

            PluginHost::IShell* implementation = reader.Number<PluginHost::IShell*>();
            PluginHost::IShell* proxy = RPC::Administrator::Instance().CreateProxy<PluginHost::IShell>(channel, implementation, true, false);

            ASSERT((proxy != nullptr) && "Failed to create proxy");

            if (proxy == nullptr) {
                TRACE_L1(_T("Could not create a stub for IGuide: %p"), implementation);
                writer.Number<uint32_t>(Core::ERROR_RPC_CALL_FAILED);
            }
            else {
                writer.Number(parameters.Implementation<IStreaming>()->Configure(proxy));
                if (proxy->Release() != Core::ERROR_NONE) {
                    TRACE_L1("Oops seems like we did not maintain a reference to this sink. %d", __LINE__);
                }
            }
        },
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& parameters(message->Parameters());
            RPC::Data::Frame::Reader reader(parameters.Reader());

            IStreaming::INotification* implementation = reader.Number<IStreaming::INotification*>();
            IStreaming::INotification* proxy = RPC::Administrator::Instance().CreateProxy<IStreaming::INotification>(channel, implementation, true, false);

            ASSERT((proxy != nullptr) && "Failed to create proxy");

            if (proxy == nullptr) {
                TRACE_L1(_T("Could not create a stub for IStreaming::INotification: %p"), implementation);
            }
            else {
                parameters.Implementation<IStreaming>()->Register(proxy);
                if (proxy->Release() != Core::ERROR_NONE) {
                    TRACE_L1("Oops seems like we did not maintain a reference to this sink. %d", __LINE__);
                }
            }
        },
        [](Core::ProxyType<Core::IPCChannel>&, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& parameters(message->Parameters());
            RPC::Data::Frame::Reader reader(parameters.Reader());

            IStreaming::INotification* stub = reader.Number<IStreaming::INotification*>();
            IStreaming::INotification* proxy = RPC::Administrator::Instance().FindProxy<IStreaming::INotification>(stub);

            if (proxy == nullptr) {
                TRACE_L1(_T("Could not find stub for IStreaming::INotification: %p"), stub);
            }
            else {
                parameters.Implementation<IStreaming>()->Unregister(proxy);
            }
        },
       [](Core::ProxyType<Core::IPCChannel>&, Core::ProxyType<RPC::InvokeMessage>& message) {
            // void StartScan()
            RPC::Data::Frame::Reader parameters(message->Parameters().Reader());
            message->Parameters().Implementation<IStreaming>()->StartScan();
        },
        [](Core::ProxyType<Core::IPCChannel>&, Core::ProxyType<RPC::InvokeMessage>& message) {
            // void StoptScan()
            RPC::Data::Frame::Reader parameters(message->Parameters().Reader());
            message->Parameters().Implementation<IStreaming>()->StopScan();
        },
        [](Core::ProxyType<Core::IPCChannel>&, Core::ProxyType<RPC::InvokeMessage>& message) {
            // void SetCurrentChannel(const uint32_t channelId)
            RPC::Data::Frame::Reader parameters(message->Parameters().Reader());
            message->Parameters().Implementation<IStreaming>()->SetCurrentChannel(parameters.Text());
        },
        [](Core::ProxyType<Core::IPCChannel>&, Core::ProxyType<RPC::InvokeMessage>& message) {
            // const string GetCurrentChannel()
            RPC::Data::Frame::Writer response(message->Response().Writer());
            response.Text(message->Parameters().Implementation<IStreaming>()->GetCurrentChannel());
        },
        [](Core::ProxyType<Core::IPCChannel>&, Core::ProxyType<RPC::InvokeMessage>& message) {
            // bool IsScanning()
            RPC::Data::Frame::Writer response(message->Response().Writer());
            response.Boolean(message->Parameters().Implementation<IStreaming>()->IsScanning());
        },
        [](Core::ProxyType<Core::IPCChannel>&, Core::ProxyType<RPC::InvokeMessage>& message) {
            // void Test(const string& str) = 0;
            RPC::Data::Frame::Reader parameters(message->Parameters().Reader());
            message->Parameters().Implementation<IStreaming>()->Test(parameters.Text());
        },
        nullptr
    };

    ProxyStub::MethodHandler GuideNotificationStubMethods[] = {
        [](Core::ProxyType<Core::IPCChannel>&, Core::ProxyType<RPC::InvokeMessage>& message) {
            // void EITBroadcast()
            RPC::Data::Frame::Reader parameters(message->Parameters().Reader());
            message->Parameters().Implementation<IGuide::INotification>()->EITBroadcast();
        },
        [](Core::ProxyType<Core::IPCChannel>&, Core::ProxyType<RPC::InvokeMessage>& message) {
            // void EmergencyAlert()
            RPC::Data::Frame::Reader parameters(message->Parameters().Reader());
            message->Parameters().Implementation<IGuide::INotification>()->EmergencyAlert();
        },
        [](Core::ProxyType<Core::IPCChannel>&, Core::ProxyType<RPC::InvokeMessage>& message) {
            // void ParentalControlChanged()
            RPC::Data::Frame::Reader parameters(message->Parameters().Reader());
            message->Parameters().Implementation<IGuide::INotification>()->ParentalControlChanged();
        },
        [](Core::ProxyType<Core::IPCChannel>&, Core::ProxyType<RPC::InvokeMessage>& message) {
            // void ParentalLockChanged()
            RPC::Data::Frame::Reader parameters(message->Parameters().Reader());
            string lcn(parameters.Text());
            message->Parameters().Implementation<IGuide::INotification>()->ParentalLockChanged(lcn);
        },
        [](Core::ProxyType<Core::IPCChannel>&, Core::ProxyType<RPC::InvokeMessage>& message) {
            // void TestNotification(const string& msg)
            RPC::Data::Frame::Reader parameters(message->Parameters().Reader());
            string data(parameters.Text());
            message->Parameters().Implementation<IGuide::INotification>()->TestNotification(data);
        },
        nullptr
    };
    ProxyStub::MethodHandler TunerNotificationStubMethods[] = {
        [](Core::ProxyType<Core::IPCChannel>&, Core::ProxyType<RPC::InvokeMessage>& message) {
            // void ScanningStateChanged()
            RPC::Data::Frame::Reader parameters(message->Parameters().Reader());
            const uint32_t state(parameters.Number<uint32_t>());
            message->Parameters().Implementation<IStreaming::INotification>()->ScanningStateChanged(state);
        },
        [](Core::ProxyType<Core::IPCChannel>&, Core::ProxyType<RPC::InvokeMessage>& message) {
            // void CurrentChannelChanged()
            RPC::Data::Frame::Reader parameters(message->Parameters().Reader());
            string lcn(parameters.Text());
            message->Parameters().Implementation<IStreaming::INotification>()->CurrentChannelChanged(lcn);
        },
        [](Core::ProxyType<Core::IPCChannel>&, Core::ProxyType<RPC::InvokeMessage>& message) {
            // void TestNotification(const std::string &msg)
            RPC::Data::Frame::Reader parameters(message->Parameters().Reader());
            string data(parameters.Text());
            message->Parameters().Implementation<IStreaming::INotification>()->TestNotification(data);
        },
        nullptr
    };

    //
    // IRPCLink interface stub definitions (interface/IRPCLink.h)
    //
    ProxyStub::MethodHandler RPCLinkStubMethods[] = {
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual void Register(INotification* notification) = 0;
            //
            RPC::Data::Input& parameters(message->Parameters());
            RPC::Data::Frame::Reader reader(parameters.Reader());

            IRPCLink::INotification* implementation = reader.Number<IRPCLink::INotification*>();
            IRPCLink::INotification* proxy = RPC::Administrator::Instance().CreateProxy<IRPCLink::INotification>(channel,
                implementation,
                true, false);

            ASSERT((proxy != nullptr) && "Failed to create proxy");

            if (proxy == nullptr) {
                TRACE_L1(_T("Could not create a stub for IRPCLink::INotification: %p"), implementation);
            } else {
                parameters.Implementation<IRPCLink>()->Register(proxy);
                if (proxy->Release() != Core::ERROR_NONE) {
                    TRACE_L1("Oops seems like we did not maintain a reference to this sink. %d", __LINE__);
                }
            }
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual void Unregister(INotification* notification) = 0;
            //
            RPC::Data::Input& parameters(message->Parameters());
            RPC::Data::Frame::Reader reader(parameters.Reader());

            // Need to find the proxy that goes with the given implementation..
            IRPCLink::INotification* stub = reader.Number<IRPCLink::INotification*>();

            // NOTE: FindProxy does *NOT* AddRef the result. Do not release what is obtained via FindProxy..
            IRPCLink::INotification* proxy = RPC::Administrator::Instance().FindProxy<IRPCLink::INotification>(stub);

            if (proxy == nullptr) {
                TRACE_L1(_T("Could not find a stub for IRPCLink::INotification: %p"), stub);
            } else {
                parameters.Implementation<IRPCLink>()->Unregister(proxy);
            }
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual uint32_t Start(const uint32_t id, const string& name) = 0;
            //
            RPC::Data::Input& parameters(message->Parameters());
            RPC::Data::Frame::Reader reader(parameters.Reader());
            RPC::Data::Frame::Writer output(message->Response().Writer());
            const uint32_t id(reader.Number<uint32_t>());
            const string name(reader.Text());

            output.Number<uint32_t>(parameters.Implementation<IRPCLink>()->Start(id, name));
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual uint32_t Stop() = 0;
            //
            RPC::Data::Input& parameters(message->Parameters());
            RPC::Data::Frame::Reader reader(parameters.Reader());
            RPC::Data::Frame::Writer output(message->Response().Writer());

            output.Number<uint32_t>(parameters.Implementation<IRPCLink>()->Stop());
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual uint32_t ForceCallback() = 0;
            //
            RPC::Data::Input& parameters(message->Parameters());
            RPC::Data::Frame::Reader reader(parameters.Reader());
            RPC::Data::Frame::Writer output(message->Response().Writer());

            output.Number<uint32_t>(parameters.Implementation<IRPCLink>()->ForceCallback());
        },

        nullptr
    };
    // IRPCLink interface stub definitions

    //
    // IRPCLink::INotification interface stub definitions (interface/IRPCLink.h)
    //
    ProxyStub::MethodHandler RPCLinkNotificationStubMethods[] = {
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual void Completed(const uint32_t id, const string& name) = 0;
            //
            RPC::Data::Frame::Reader parameters(message->Parameters().Reader());
            const uint32_t id(parameters.Number<uint32_t>());
            const string name(parameters.Text());

            message->Parameters().Implementation<IRPCLink::INotification>()->Completed(id, name);
        },

        nullptr
    };
    ProxyStub::MethodHandler PowerStubMethods[] = {
       [](Core::ProxyType<Core::IPCChannel>&, Core::ProxyType<RPC::InvokeMessage>& message) {
            //virtual PCStatus SetState(const State state, uint32_t timeout);
            RPC::Data::Frame::Writer response(message->Response().Writer());
            RPC::Data::Frame::Reader parameters(message->Parameters().Reader());
            response.Number(message->Parameters().Implementation<IPower>()->SetState(parameters.Number<IPower::PCState>(), parameters.Number<uint32_t>()));
        },
        [](Core::ProxyType<Core::IPCChannel>&, Core::ProxyType<RPC::InvokeMessage>& message) {
            //virtual State GetState();
            RPC::Data::Frame::Writer response(message->Response().Writer());
            response.Number(message->Parameters().Implementation<IPower>()->GetState());
        },
        [](Core::ProxyType<Core::IPCChannel>&, Core::ProxyType<RPC::InvokeMessage>& message) {
            //virtual void PowerKey();
            message->Parameters().Implementation<IPower>()->PowerKey();
        },

        nullptr
    };

    // IRPCLink::INotification interface stub definitions

    typedef ProxyStub::StubType<IBrowser, BrowserStubMethods, ProxyStub::UnknownStub> BrowserStub;
    typedef ProxyStub::StubType<IBrowser::INotification, BrowserNotificationStubMethods, ProxyStub::UnknownStub> BrowserNotificationStub;
    typedef ProxyStub::StubType<IGuide, GuideStubMethods, ProxyStub::UnknownStub> IGuideStub;
    typedef ProxyStub::StubType<IGuide::INotification, GuideNotificationStubMethods, ProxyStub::UnknownStub> GuideNotificationStub;
    typedef ProxyStub::StubType<IWebDriver, WebDriverStubMethods, ProxyStub::UnknownStub> WebDriverStub;
    typedef ProxyStub::StubType<IContentDecryption, OpenCDMiStubMethods, ProxyStub::UnknownStub> OpenCDMiStub;
    typedef ProxyStub::StubType<IBluetooth, BluetoothStubMethods, ProxyStub::UnknownStub> BluetoothStub;
    typedef ProxyStub::StubType<INetflix, NetflixStubMethods, ProxyStub::UnknownStub> NetflixStub;
    typedef ProxyStub::StubType<INetflix::INotification, NetflixNotificationStubMethods, ProxyStub::UnknownStub> NetflixNotificationStub;
    typedef ProxyStub::StubType<IResourceCenter, ResourceCenterStubMethods, ProxyStub::UnknownStub> ResourceCenterStub;
    typedef ProxyStub::StubType<IResourceCenter::INotification, ResourceCenterNotificationStubMethods, ProxyStub::UnknownStub> ResourceCenterNotificationStub;
    typedef ProxyStub::StubType<IProvisioning, ProvisioningStubMethods, ProxyStub::UnknownStub> ProvisioningStub;
    typedef ProxyStub::StubType<IProvisioning::INotification, ProvisioningNotificationStubMethods, ProxyStub::UnknownStub> ProvisioningNotificationStub;
    typedef ProxyStub::StubType<IWebServer, WebServerStubMethods, ProxyStub::UnknownStub> WebServerStub;
    typedef ProxyStub::StubType<IComposition, CompositionStubMethods, ProxyStub::UnknownStub> CompositionStub;
    typedef ProxyStub::StubType<IComposition::IClient, CompositionClientStubMethods, ProxyStub::UnknownStub> CompositionClientStub;
    typedef ProxyStub::StubType<IComposition::INotification, CompositionNotificationStubMethods, ProxyStub::UnknownStub> CompositionNotificationStub;
    typedef ProxyStub::StubType<IStreaming, TunerStubMethods, ProxyStub::UnknownStub> TunerStub;
    typedef ProxyStub::StubType<IStreaming::INotification, TunerNotificationStubMethods, ProxyStub::UnknownStub> TunerNotificationStub;
    typedef ProxyStub::StubType<IRPCLink, RPCLinkStubMethods, ProxyStub::UnknownStub> RPCLinkStub;
    typedef ProxyStub::StubType<IRPCLink::INotification, RPCLinkNotificationStubMethods, ProxyStub::UnknownStub> RPCLinkNotificationStub;
    typedef ProxyStub::StubType<IPower, PowerStubMethods, ProxyStub::UnknownStub> PowerStub;

    // -------------------------------------------------------------------------------------------
    // PROXY
    // -------------------------------------------------------------------------------------------
    class BrowserProxy : public ProxyStub::UnknownProxyType<IBrowser> {
    public:
        BrowserProxy(Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        virtual ~BrowserProxy()
        {
        }

    public:
        // Stub order:
        // virtual void SetURL(const string& URL) = 0;
        // virtual string GetURL() const = 0;
        // virtual uint32_t GetFPS() const = 0;
        // virtual void Register(IBrowser::INotification* sink) = 0;
        // virtual void Unregister(IBrowser::INotification* sink) = 0;
        // virtual void Hide(const bool hidden) = 0
        virtual void SetURL(const string& URL)
        {
            IPCMessage newMessage(BaseClass::Message(0));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(URL);
            Invoke(newMessage);
        }

        virtual string GetURL() const
        {
            IPCMessage newMessage(BaseClass::Message(1));
            Invoke(newMessage);
            RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
            return reader.Text();
        }

        virtual uint32_t GetFPS() const
        {
            IPCMessage newMessage(BaseClass::Message(2));
            Invoke(newMessage);
            RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
            return reader.Number<uint32_t>();
        }

        virtual void Register(IBrowser::INotification* notification)
        {
            IPCMessage newMessage(BaseClass::Message(3));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<IBrowser::INotification*>(notification);
            Invoke(newMessage);
        }

        virtual void Unregister(IBrowser::INotification* notification)
        {
            IPCMessage newMessage(BaseClass::Message(4));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<IBrowser::INotification*>(notification);
            Invoke(newMessage);
        }

        virtual void Hide(const bool hide)
        {
            IPCMessage newMessage(BaseClass::Message(5));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Boolean(hide);
            Invoke(newMessage);
        }
    };

    class BrowserNotificationProxy : public ProxyStub::UnknownProxyType<IBrowser::INotification> {
    public:
        BrowserNotificationProxy(Core::ProxyType<Core::IPCChannel>& channel, void* implementation,
            const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        virtual ~BrowserNotificationProxy()
        {
        }

    public:
        // Stub order:
        // virtual void LoadFinished(const string& URL) = 0;
        // virtual void URLChanged(const string& URL) = 0;
        // virtual void Hidden(const bool hidden) = 0;
        // virtual void Closure() = 0;
        virtual void LoadFinished(const string& URL)
        {
            IPCMessage newMessage(BaseClass::Message(0));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(URL);
            Invoke(newMessage);
        }

        virtual void URLChanged(const string& URL)
        {
            IPCMessage newMessage(BaseClass::Message(1));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(URL);
            Invoke(newMessage);
        }

        virtual void Hidden(const bool hidden)
        {
            IPCMessage newMessage(BaseClass::Message(2));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Boolean(hidden);
            Invoke(newMessage);
        }

        virtual void Closure()
        {
            IPCMessage newMessage(BaseClass::Message(3));
            Invoke(newMessage);
        }
    };

    class IGuideProxy : public ProxyStub::UnknownProxyType<IGuide> {
    public:
        IGuideProxy(Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }
        virtual ~IGuideProxy()
        {
        }

    public:
        virtual uint32_t StartParser(PluginHost::IShell* webbridge)
        {
            IPCMessage newMessage(BaseClass::Message(0));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<PluginHost::IShell*>(webbridge);
            Invoke(newMessage);
            return (newMessage->Response().Reader().Number<uint32_t>());
        }
        virtual void Register(IGuide::INotification* notification)
        {
            IPCMessage newMessage(BaseClass::Message(1));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<IGuide::INotification*>(notification);
            Invoke(newMessage);

        }
        virtual void Unregister(IGuide::INotification* notification)
        {
            IPCMessage newMessage(BaseClass::Message(2));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<IGuide::INotification*>(notification);
            Invoke(newMessage);
        }
        virtual const string GetChannels()
        {
            IPCMessage newMessage(BaseClass::Message(3));
            Invoke(newMessage);
            RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
            return reader.Text();
        }
        virtual const string GetPrograms()
        {
            IPCMessage newMessage(BaseClass::Message(4));
            Invoke(newMessage);
            RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
            return reader.Text();
        }
        virtual const string GetCurrentProgram(const string& channelNum)
        {
            IPCMessage newMessage(BaseClass::Message(5));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(channelNum);
            Invoke(newMessage);
            RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
            return reader.Text();
        }
        virtual const string GetAudioLanguages(const uint32_t eventId)
        {
            IPCMessage newMessage(BaseClass::Message(6));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<uint32_t>(eventId);
            Invoke(newMessage);
            RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
            return reader.Text();
        }
        virtual const string GetSubtitleLanguages(const uint32_t eventId)
        {
            IPCMessage newMessage(BaseClass::Message(7));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<uint32_t>(eventId);
            Invoke(newMessage);
            RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
            return reader.Text();
        }
        virtual bool SetParentalControlPin(const string& oldPin, const string& newPin)
        {
            IPCMessage newMessage(BaseClass::Message(8));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(oldPin);
            writer.Text(newPin);
            Invoke(newMessage);
            RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
            return reader.Boolean();
        }
        virtual bool SetParentalControl(const string& pin, const bool isLocked)
        {
            IPCMessage newMessage(BaseClass::Message(9));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(pin);
            writer.Boolean(isLocked);
            Invoke(newMessage);
            RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
            return reader.Boolean();
        }
        virtual bool IsParentalControlled()
        {
            IPCMessage newMessage(BaseClass::Message(10));
            Invoke(newMessage);
            RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
            return reader.Boolean();
        }
        virtual bool SetParentalLock(const string& pin, const bool isLocked, const string& channelNum)
        {
            IPCMessage newMessage(BaseClass::Message(11));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(pin);
            writer.Boolean(isLocked);
            writer.Text(channelNum);
            Invoke(newMessage);
            RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
            return reader.Boolean();
        }
        virtual bool IsParentalLocked(const string& channelNum)
        {
            IPCMessage newMessage(BaseClass::Message(12));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(channelNum);
            Invoke(newMessage);
            RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
            return reader.Boolean();
        }

    };

    class WebDriverProxy : public ProxyStub::UnknownProxyType<IWebDriver> {
    public:
        WebDriverProxy(Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        virtual ~WebDriverProxy()
        {
        }

    public:
        // Stub order:
        // virtual uint32_t Configure(PluginHost::IShell* framework) = 0;
        virtual uint32_t Configure(PluginHost::IShell* webbridge)
        {
            IPCMessage newMessage(BaseClass::Message(0));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<PluginHost::IShell*>(webbridge);
            Invoke(newMessage);
            return (newMessage->Response().Reader().Number<uint32_t>());
        }
    };

    class OpenCDMiProxy : public ProxyStub::UnknownProxyType<IContentDecryption> {
    public:
        OpenCDMiProxy(Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        virtual ~OpenCDMiProxy()
        {
        }

    public:
        // Stub order:
        // virtual uint32_t Configure(PluginHost::IShell* service) = 0;
        virtual uint32_t Configure(PluginHost::IShell* service)
        {
            IPCMessage newMessage(BaseClass::Message(0));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<PluginHost::IShell*>(service);
            Invoke(newMessage);
            return (newMessage->Response().Reader().Number<uint32_t>());
        }
        virtual uint32_t Reset() {
            IPCMessage newMessage(BaseClass::Message(1));
            Invoke(newMessage);
            return (newMessage->Response().Reader().Number<uint32_t>());
        }
        virtual RPC::IStringIterator* Systems() const {
            IPCMessage newMessage(BaseClass::Message(2));
            Invoke(newMessage);
            RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
            return (const_cast<OpenCDMiProxy&>(*this).CreateProxy<RPC::IStringIterator>(reader.Number<RPC::IStringIterator*>()));
        }
        virtual RPC::IStringIterator* Designators(const string& keySystem) const {
            IPCMessage newMessage(BaseClass::Message(3));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(keySystem);
            Invoke(newMessage);
            RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
            return (const_cast<OpenCDMiProxy&>(*this).CreateProxy<RPC::IStringIterator>(reader.Number<RPC::IStringIterator*>()));
        }
        virtual RPC::IStringIterator* Sessions(const string& keySystem) const {
            IPCMessage newMessage(BaseClass::Message(4));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(keySystem);
            Invoke(newMessage);
            RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
            return (const_cast<OpenCDMiProxy&>(*this).CreateProxy<RPC::IStringIterator>(reader.Number<RPC::IStringIterator*>()));
        }
    };

    class BluetoothProxy : public ProxyStub::UnknownProxyType<IBluetooth> {
    public:
        BluetoothProxy(Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        virtual ~BluetoothProxy()
        {
        }
    public:
        virtual uint32_t Configure(PluginHost::IShell* service)
        {
            IPCMessage newMessage(BaseClass::Message(0));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<PluginHost::IShell*>(service);
            Invoke(newMessage);
            return (newMessage->Response().Reader().Number<uint32_t>());
        }

        virtual bool Scan()
        {
            IPCMessage newMessage(BaseClass::Message(1));
            Invoke(newMessage);

            RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
            return reader.Boolean();
        }

        virtual bool StopScan()
        {
            IPCMessage newMessage(BaseClass::Message(2));
            Invoke(newMessage);

            RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
            return reader.Boolean();
        }

        virtual string DiscoveredDevices()
        {
            IPCMessage newMessage(BaseClass::Message(3));
            Invoke(newMessage);

            RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
            return reader.Text();
        }

        virtual string PairedDevices()
        {
            IPCMessage newMessage(BaseClass::Message(4));
            Invoke(newMessage);

            RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
            return reader.Text();
        }

        virtual bool Pair(string deviceId)
        {
            IPCMessage newMessage(BaseClass::Message(5));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(deviceId);
            Invoke(newMessage);

            RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
            return reader.Boolean();
        }

        virtual bool Connect(string deviceId)
        {
            IPCMessage newMessage(BaseClass::Message(6));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(deviceId);
            Invoke(newMessage);

            RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
            return reader.Boolean();
        }

        virtual bool Disconnect()
        {
            IPCMessage newMessage(BaseClass::Message(7));
            Invoke(newMessage);

            RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
            return reader.Boolean();
        }

        virtual bool IsScanning()
        {
            IPCMessage newMessage(BaseClass::Message(8));
            Invoke(newMessage);

            RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
            return reader.Boolean();
        }

        virtual string Connected()
        {
            IPCMessage newMessage(BaseClass::Message(9));
            Invoke(newMessage);

            RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
            return reader.Text();
        }
    };


    class NetflixProxy : public ProxyStub::UnknownProxyType<INetflix> {
    public:
        NetflixProxy(Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        virtual ~NetflixProxy()
        {
        }

    public:
        // Stub order:
        // virtual void Register(INetflix::INotification* netflix) = 0;
        // virtual void Unregister(INetflix::INotification* netflix) = 0;
        // virtual string GetESN() const = 0;
        // virtual void SystemCommand(const string& command) = 0;
        // virtual void Language(const string& language) = 0;
        virtual void Register(INetflix::INotification* notification)
        {
            IPCMessage newMessage(BaseClass::Message(0));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<INetflix::INotification*>(notification);
            Invoke(newMessage);
        }

        virtual void Unregister(INetflix::INotification* notification)
        {
            IPCMessage newMessage(BaseClass::Message(1));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<INetflix::INotification*>(notification);
            Invoke(newMessage);
        }

        virtual string GetESN() const
        {
            IPCMessage newMessage(BaseClass::Message(2));
            Invoke(newMessage);
            RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
            return reader.Text();
        }

        virtual void SystemCommand(const string& command)
        {
            IPCMessage newMessage(BaseClass::Message(3));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(command);
            Invoke(newMessage);
        }

        virtual void Language(const string& language)
        {
            IPCMessage newMessage(BaseClass::Message(4));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(language);
            Invoke(newMessage);
        }
    };

    class NetflixNotificationProxy : public ProxyStub::UnknownProxyType<INetflix::INotification> {
    public:
        NetflixNotificationProxy(Core::ProxyType<Core::IPCChannel>& channel, void* implementation,
            const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        virtual ~NetflixNotificationProxy()
        {
        }

    public:
        // Stub order:
        // virtual void StateChange(const INetflix::state state) = 0;
        virtual void StateChange(const Exchange::INetflix::state newState)
        {
            IPCMessage newMessage(BaseClass::Message(0));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<Exchange::INetflix::state>(newState);

            Invoke(newMessage);
        }
    };

    class ResourceCenterProxy : public ProxyStub::UnknownProxyType<IResourceCenter> {
    public:
        ResourceCenterProxy(Core::ProxyType<Core::IPCChannel>& channel, void* implementation,
            const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        virtual ~ResourceCenterProxy()
        {
        }

    public:
        // Stub order:
        // virtual uint32_t Configure(PluginHost::IShell* service) = 0;
        // virtual void Register(IResourceCenter::INotification* provisioning) = 0;
        // virtual void Unregister(IResourceCenter::INotification* provisioning) = 0;
        // virtual uint8_t Identifier(const uint8_t maxLength, uint8_t buffer[]) const = 0;
        // virtual hardware_state HardwareState() const = 0;
        virtual uint32_t Configure(PluginHost::IShell* service)
        {
            IPCMessage newMessage(BaseClass::Message(0));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<PluginHost::IShell*>(service);
            Invoke(newMessage);
            return (newMessage->Response().Reader().Number<uint32_t>());
        }

        virtual void Register(IResourceCenter::INotification* notification)
        {
            IPCMessage newMessage(BaseClass::Message(1));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<IResourceCenter::INotification*>(notification);
            Invoke(newMessage);
        }

        virtual void Unregister(IResourceCenter::INotification* notification)
        {
            IPCMessage newMessage(BaseClass::Message(2));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<IResourceCenter::INotification*>(notification);
            Invoke(newMessage);
        }

        virtual Exchange::IResourceCenter::hardware_state HardwareState() const
        {
            IPCMessage newMessage(BaseClass::Message(3));
            Invoke(newMessage);
            RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
            return reader.Number<Exchange::IResourceCenter::hardware_state>();
        }

        virtual uint8_t Identifier(const uint8_t length, uint8_t buffer[]) const
        {
            IPCMessage newMessage(BaseClass::Message(4));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<uint8_t>(length);
            Invoke(newMessage);
            RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
            uint8_t result(reader.Buffer<uint8_t>(length, buffer));

            return (result > length ? length : result);
        }
    };

    class ResourceCenterNotificationProxy : public ProxyStub::UnknownProxyType<IResourceCenter::INotification> {
    public:
        ResourceCenterNotificationProxy(Core::ProxyType<Core::IPCChannel>& channel, void* implementation,
            const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        virtual ~ResourceCenterNotificationProxy()
        {
        }

    public:
        // Stub order:
        // virtual void StateChange(Exchange::IResourceCenter::hardware_state state) = 0;
        virtual void StateChange(Exchange::IResourceCenter::hardware_state state)
        {
            IPCMessage newMessage(BaseClass::Message(0));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<Exchange::IResourceCenter::hardware_state>(state);
            Invoke(newMessage);
        }
    };

    class ProvisioningProxy : public ProxyStub::UnknownProxyType<IProvisioning> {
    public:
        ProvisioningProxy(Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        virtual ~ProvisioningProxy()
        {
        }

    public:
        // Stub order:
        // virtual void Register(IProvisioning::INotification* provisioning) = 0;
        // virtual void Unregister(IProvisioning::INotification* provisioning) = 0;
        virtual void Register(IProvisioning::INotification* notification)
        {
            IPCMessage newMessage(BaseClass::Message(0));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<IProvisioning::INotification*>(notification);
            Invoke(newMessage);
        }

        virtual void Unregister(IProvisioning::INotification* notification)
        {
            IPCMessage newMessage(BaseClass::Message(1));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<IProvisioning::INotification*>(notification);
            Invoke(newMessage);
        }
    };

    class ProvisioningNotificationProxy : public ProxyStub::UnknownProxyType<IProvisioning::INotification> {
    public:
        ProvisioningNotificationProxy(Core::ProxyType<Core::IPCChannel>& channel, void* implementation,
            const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        virtual ~ProvisioningNotificationProxy()
        {
        }

    public:
        // Stub order:
        // virtual void Provisioned(const string& component) = 0;
        virtual void Provisioned(const string& element)
        {
            IPCMessage newMessage(BaseClass::Message(0));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(element);
            Invoke(newMessage);
        }
    };

    class CompositionProxy : public ProxyStub::UnknownProxyType<IComposition> {
    public:
        CompositionProxy(Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        virtual ~CompositionProxy()
        {
        }

    public:
        // Stub order:
        // virtual void Register(IComposition::INotification* notification) = 0;
        // virtual void Unregister(IComposition::INotification* notification) = 0;
        // virtual IClient* Client(const uint8_t index) = 0;
        // virtual IClient* Client(const string& name) = 0;
        // virtual uint32_t Configure(PluginHost::IShell* service) = 0;
        // virtual void SetResolution(const ScreenResolution) = 0;
        // virtual const ScreenResolution GetResolution() = 0;
        virtual void Register(IComposition::INotification* notification)
        {
            IPCMessage newMessage(BaseClass::Message(0));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<IComposition::INotification*>(notification);
            Invoke(newMessage);
        }

        virtual void Unregister(IComposition::INotification* notification)
        {
            IPCMessage newMessage(BaseClass::Message(1));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<IComposition::INotification*>(notification);
            Invoke(newMessage);
        }

        virtual IClient* Client(const uint8_t index)
        {
            IPCMessage newMessage(BaseClass::Message(2));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<uint32_t>(index);
            Invoke(newMessage);
            RPC::Data::Frame::Reader reader(newMessage->Response().Reader());

            return (CreateProxy<IClient>(reader.Number<IClient*>()));
        }

        virtual IClient* Client(const string& name)
        {
            IPCMessage newMessage(BaseClass::Message(3));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(name);
            Invoke(newMessage);
            RPC::Data::Frame::Reader reader(newMessage->Response().Reader());

            return (CreateProxy<IClient>(reader.Number<IClient*>()));
        }

        virtual uint32_t Configure(PluginHost::IShell* service)
        {
            IPCMessage newMessage(BaseClass::Message(4));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<PluginHost::IShell*>(service);
            Invoke(newMessage);
            return (newMessage->Response().Reader().Number<uint32_t>());
        }

        virtual void SetResolution(const ScreenResolution format)
        {
            IPCMessage newMessage(BaseClass::Message(5));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<IComposition::ScreenResolution>(format);
            Invoke(newMessage);
        }

        virtual const ScreenResolution GetResolution()
        {
            IPCMessage newMessage(BaseClass::Message(6));
            Invoke(newMessage);
            RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
            return (reader.Number<IComposition::ScreenResolution>());
        }
    };

    class CompositionClientProxy : public ProxyStub::UnknownProxyType<IComposition::IClient> {
    public:
        CompositionClientProxy(Core::ProxyType<Core::IPCChannel>& channel, void* implementation,
            const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        virtual ~CompositionClientProxy()
        {
        }

    public:
        // Stub order:
        // virtual string Name() const = 0;
        // virtual void Kill() = 0;
        // virtual void Opacity(const uint32_t value) = 0;
        // virtual void Geometry(const uint32_t X, const uint32_t Y, const uint32_t width, const uint32_t height) = 0;
        // virtual void Visible(const bool visible) = 0;
        // virtual void SetTop() = 0;
        // virtual void SetInput() = 0;
        virtual string Name() const
        {
            IPCMessage newMessage(BaseClass::Message(0));
            Invoke(newMessage);
            RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
            return (reader.Text());
        }

        virtual void Kill()
        {
            IPCMessage newMessage(BaseClass::Message(1));
            Invoke(newMessage);
        }

        virtual void Opacity(const uint32_t value)
        {
            IPCMessage newMessage(BaseClass::Message(2));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<uint32_t>(value);
            Invoke(newMessage);
        }

        virtual void Geometry(const uint32_t X, const uint32_t Y, const uint32_t width, const uint32_t height)
        {
            IPCMessage newMessage(BaseClass::Message(3));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<uint32_t>(X);
            writer.Number<uint32_t>(Y);
            writer.Number<uint32_t>(width);
            writer.Number<uint32_t>(height);
            Invoke(newMessage);
        }

        virtual void Visible(const bool value)
        {
            IPCMessage newMessage(BaseClass::Message(4));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Boolean(value);
            Invoke(newMessage);
        }

        virtual void SetTop()
        {
            IPCMessage newMessage(BaseClass::Message(5));
            Invoke(newMessage);
        }

        virtual void SetInput()
        {
            IPCMessage newMessage(BaseClass::Message(6));
            Invoke(newMessage);
        }
    };

    class CompositionNotificationProxy : public ProxyStub::UnknownProxyType<IComposition::INotification> {
    public:
        // Stub order:
        CompositionNotificationProxy(Core::ProxyType<Core::IPCChannel>& channel, void* implementation,
            const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        virtual ~CompositionNotificationProxy()
        {
        }

    public:
        // Stub order:
        // virtual void Attached(IClient* client) = 0;
        // virtual void Detached(IClient* client) = 0;
        virtual void Attached(IComposition::IClient* element)
        {
            IPCMessage newMessage(BaseClass::Message(0));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<IComposition::IClient*>(element);
            Invoke(newMessage);
        }

        virtual void Detached(IComposition::IClient* element)
        {
            IPCMessage newMessage(BaseClass::Message(1));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<IComposition::IClient*>(element);
            Invoke(newMessage);
        }
    };

    class WebServerProxy : public ProxyStub::UnknownProxyType<IWebServer> {
    public:
        WebServerProxy(Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        virtual ~WebServerProxy()
        {
        }

    public:
        // Stub order:
        // virtual void AddProxy(const string& path, const string& subst, const string& address) = 0;
        // virtual void RemoveProxy(const string& path) = 0;
        // virtual string Accessor() const = 0;
        virtual void AddProxy(const string& path, const string& subst, const string& address)
        {
            IPCMessage newMessage(BaseClass::Message(0));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(path);
            writer.Text(subst);
            writer.Text(address);
            Invoke(newMessage);
        }

        virtual void RemoveProxy(const string& path)
        {
            IPCMessage newMessage(BaseClass::Message(1));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(path);
            Invoke(newMessage);
        }

        virtual string Accessor() const
        {
            IPCMessage newMessage(BaseClass::Message(2));
            Invoke(newMessage);

            RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
            string accessorURL = reader.Text();
            return accessorURL;
        }
    };

    class TunerProxy : public ProxyStub::UnknownProxyType<IStreaming> {
    public:
        TunerProxy(Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }
        virtual ~TunerProxy()
        {
        }

    public:
        virtual uint32_t Configure(PluginHost::IShell* service)
        {
            IPCMessage newMessage(BaseClass::Message(0));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<PluginHost::IShell*>(service);
            Invoke(newMessage);
            return (newMessage->Response().Reader().Number<uint32_t>());
        }
        virtual void Register(IStreaming::INotification* notification)
        {
            IPCMessage newMessage(BaseClass::Message(1));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<IStreaming::INotification*>(notification);
            Invoke(newMessage);

        }
        virtual void Unregister(IStreaming::INotification* notification)
        {
            IPCMessage newMessage(BaseClass::Message(2));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<IStreaming::INotification*>(notification);
            Invoke(newMessage);
        }

        virtual void StartScan()
        {
            IPCMessage newMessage(BaseClass::Message(3));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            Invoke(newMessage);
        }
        virtual void StopScan()
        {
            IPCMessage newMessage(BaseClass::Message(4));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            Invoke(newMessage);
        }
        virtual void SetCurrentChannel(const string& channelId)
        {
            IPCMessage newMessage(BaseClass::Message(5));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(channelId);
            Invoke(newMessage);
        }
        virtual const string GetCurrentChannel()
        {
            IPCMessage newMessage(BaseClass::Message(6));
            Invoke(newMessage);
            RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
            return reader.Text();
        }
        virtual bool IsScanning()
        {
            IPCMessage newMessage(BaseClass::Message(7));
            Invoke(newMessage);
            RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
            return reader.Boolean();
        }
        // Add methods above this line - Dont forget to update the id for the test
        virtual void Test(const string& str)
        {
            IPCMessage newMessage(BaseClass::Message(8));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(str);
            Invoke(newMessage);
        }
    };

    class TunerNotificationProxy : public ProxyStub::UnknownProxyType<IStreaming::INotification> {
    public:
        TunerNotificationProxy(Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }
        virtual ~TunerNotificationProxy()
        {
        }

    public:
        virtual void ScanningStateChanged(const uint32_t state)
        {
            IPCMessage newMessage(BaseClass::Message(0));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<uint32_t>(state);
            Invoke(newMessage);
        }
        virtual void CurrentChannelChanged(const string& lcn)
        {
            IPCMessage newMessage(BaseClass::Message(1));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(lcn);
            Invoke(newMessage);
        }
        // Add methods above this line- Dont forget to update the id for the TestNotification
        virtual void TestNotification(const string& str)
        {
            IPCMessage newMessage(BaseClass::Message(0));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(str);
            Invoke(newMessage);
        }
    };

    class GuideNotificationProxy : public ProxyStub::UnknownProxyType<IGuide::INotification> {
    public:
        GuideNotificationProxy(Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }
        virtual ~GuideNotificationProxy()
        {
        }
    public:
        virtual void EITBroadcast()
        {
            IPCMessage newMessage(BaseClass::Message(0));
            Invoke(newMessage);
        }
        virtual void EmergencyAlert()
        {
            IPCMessage newMessage(BaseClass::Message(1));
            Invoke(newMessage);
        }
        virtual void ParentalControlChanged()
        {
            IPCMessage newMessage(BaseClass::Message(2));
            Invoke(newMessage);
        }
        virtual void ParentalLockChanged(const string& lcn)
        {
            IPCMessage newMessage(BaseClass::Message(3));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(lcn);
            Invoke(newMessage);
        }
       // Add methods above this line- Dont forget to update the id for the TestNotification
        virtual void TestNotification(const string& str)
        {
            IPCMessage newMessage(BaseClass::Message(4));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(str);
            Invoke(newMessage);
        }

    };

   class RPCLinkProxy : public ProxyStub::UnknownProxyType<IRPCLink> {
   public:
        RPCLinkProxy(Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        virtual ~RPCLinkProxy()
        {
        }

    public:
        // Stub order:
        // virtual void Register(INotification* notification) = 0;
        // virtual void Unregister(INotification* notification) = 0;
        // virtual uint32_t Start(const uint32_t id, const string& name) = 0;
        // virtual uint32_t Stop() = 0;
        // virtual uint32_t ForceCallback() = 0;
        virtual void Register(IRPCLink::INotification* notification)
        {
            IPCMessage newMessage(BaseClass::Message(0));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<IRPCLink::INotification*>(notification);
            Invoke(newMessage);
         }

        virtual void Unregister(IRPCLink::INotification* notification)
        {
            IPCMessage newMessage(BaseClass::Message(1));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<IRPCLink::INotification*>(notification);
            Invoke(newMessage);
        }

        virtual uint32_t Start(const uint32_t id, const string& name)
        {
            IPCMessage newMessage(BaseClass::Message(2));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<uint32_t>(id);
            writer.Text(name);
            Invoke(newMessage);

            return (newMessage->Response().Reader().Number<uint32_t>());
        }

        virtual uint32_t Stop()
        {
            IPCMessage newMessage(BaseClass::Message(3));
            Invoke(newMessage);

            return (newMessage->Response().Reader().Number<uint32_t>());
        }

        virtual uint32_t ForceCallback()
        {
            IPCMessage newMessage(BaseClass::Message(4));
            Invoke(newMessage);

            return (newMessage->Response().Reader().Number<uint32_t>());
        }
    };

    class RPCLinkNotificationProxy : public ProxyStub::UnknownProxyType<IRPCLink::INotification> {
    public:
        RPCLinkNotificationProxy(Core::ProxyType<Core::IPCChannel>& channel, void* implementation,
            const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        virtual ~RPCLinkNotificationProxy()
        {
        }

    public:
        // Stub order:
        // virtual void Completed(const uint32_t id, const string& name) = 0;
        virtual void Completed(const uint32_t id, const string& name)
        {
            IPCMessage newMessage(BaseClass::Message(0));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<uint32_t>(id);
            writer.Text(name);
            Invoke(newMessage);
        }
    };

    class PowerProxy : public ProxyStub::UnknownProxyType<IPower> {
    public:
        PowerProxy(Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }
 
        virtual ~PowerProxy()
        {
        }

    public:
        virtual PCStatus SetState(const PCState state, const uint32_t timeout)
        {
            IPCMessage newMessage(BaseClass::Message(0));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<IPower::PCState>(state);
            writer.Number(timeout);
            Invoke(newMessage);

            RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
            return reader.Number<IPower::PCStatus>();
        }

        virtual PCState GetState() const
        {
            IPCMessage newMessage(BaseClass::Message(1));
            Invoke(newMessage);

            RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
            return reader.Number<IPower::PCState>();
        }

        virtual void PowerKey()
        {
            IPCMessage newMessage(BaseClass::Message(2));
            Invoke(newMessage);
        }
 
    };

    // -------------------------------------------------------------------------------------------
    // Registration
    // -------------------------------------------------------------------------------------------
    static class Instantiation {
    public:
        Instantiation()
        {
            RPC::Administrator::Instance().Announce<IBrowser, BrowserProxy, BrowserStub>();
            RPC::Administrator::Instance().Announce<IBrowser::INotification, BrowserNotificationProxy, BrowserNotificationStub>();
            RPC::Administrator::Instance().Announce<IGuide, IGuideProxy, IGuideStub>();
            RPC::Administrator::Instance().Announce<IGuide::INotification, GuideNotificationProxy, GuideNotificationStub>();
            RPC::Administrator::Instance().Announce<IWebDriver, WebDriverProxy, WebDriverStub>();
            RPC::Administrator::Instance().Announce<IContentDecryption, OpenCDMiProxy, OpenCDMiStub>();
            RPC::Administrator::Instance().Announce<IBluetooth, BluetoothProxy, BluetoothStub>();
            RPC::Administrator::Instance().Announce<INetflix, NetflixProxy, NetflixStub>();
            RPC::Administrator::Instance().Announce<INetflix::INotification, NetflixNotificationProxy, NetflixNotificationStub>();
            RPC::Administrator::Instance().Announce<IResourceCenter, ResourceCenterProxy, ResourceCenterStub>();
            RPC::Administrator::Instance().Announce<IResourceCenter::INotification, ResourceCenterNotificationProxy, ResourceCenterNotificationStub>();
            RPC::Administrator::Instance().Announce<IProvisioning, ProvisioningProxy, ProvisioningStub>();
            RPC::Administrator::Instance().Announce<IProvisioning::INotification, ProvisioningNotificationProxy, ProvisioningNotificationStub>();
            RPC::Administrator::Instance().Announce<IWebServer, WebServerProxy, WebServerStub>();
            RPC::Administrator::Instance().Announce<IComposition, CompositionProxy, CompositionStub>();
            RPC::Administrator::Instance().Announce<IComposition::IClient, CompositionClientProxy, CompositionClientStub>();
            RPC::Administrator::Instance().Announce<IComposition::INotification, CompositionNotificationProxy, CompositionNotificationStub>();
            RPC::Administrator::Instance().Announce<IStreaming, TunerProxy, TunerStub>();
            RPC::Administrator::Instance().Announce<IStreaming::INotification, TunerNotificationProxy, TunerNotificationStub>();
            RPC::Administrator::Instance().Announce<IRPCLink, RPCLinkProxy, RPCLinkStub>();
            RPC::Administrator::Instance().Announce<IRPCLink::INotification, RPCLinkNotificationProxy, RPCLinkNotificationStub>();
            RPC::Administrator::Instance().Announce<IPower, PowerProxy, PowerStub>();
        }

        ~Instantiation()
        {
        }

    } ProxyStubRegistration;
} // namespace ProxyStubs
} // namespace WPEFramework
