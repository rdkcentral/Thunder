#ifndef __IELEMENT_OBJECT_H
#define __IELEMENT_OBJECT_H

// ---- Include system wide include files ----
#include "Module.h"

// ---- Include local include files ----

// ---- Helper types and constants ----

// ---- Helper functions ----

// ---- Referenced classes and types ----

// ---- Class Definition ----

namespace WPEFramework {
	namespace Exchange {
		struct IElement : virtual public Core::IUnknown {
			virtual ~IElement() {}

			enum { ID = 0x0000004C };


			struct INotification : virtual public Core::IUnknown {

				virtual ~INotification() {}

				enum { ID = 0x0000004D };

				// Push changes. If the Current value changes, the next method is called.
				virtual void Update() = 0;
			};

			struct IFactory : virtual public Core::IUnknown {
				virtual ~IFactory() {}

				enum { ID = 0x0000004E };

				struct INotification : virtual public Core::IUnknown {

					virtual ~INotification() {}

					enum { ID = 0x0000004F };

					virtual void Activated(IElement* source) = 0;
					virtual void Deactivated(IElement* source) = 0;
				};

				// Pushing notifications to interested sinks
				virtual void Register(INotification* sink) = 0;
				virtual void Unregister(INotification* sink) = 0;
			};

			enum identification {
				ZWAVE = 0x10000000,
				GPIO  = 0x20000000,
				I2C   = 0x30000000
			};

			//  Basic/specific and dimension together define the Type.
			// 32     13    | 3 |  4  |     12     |
			//  +---------------+------------------+
			//  | dimension |FLT|basic|  specific  |
			//  +---------------+------------------+
			//  FLT = Floating point. The number of decimals thats 
			//        should be considerd to be the remainder.
			//        3 bits (0..7)
			//
			enum basic {          /* 4 bits (16)*/
				regulator       = 0x0,
				measurement     = 0x1
			};

			enum specific {       /* 12 bits (4096) */
				general         = 0x000,
				electricity     = 0x001,
				water           = 0x002,
				gas             = 0x003,
				air             = 0x004,
				smoke           = 0x005,
				carbonMonoxide  = 0x006,
				carbonDioxide   = 0x007,
				temperature     = 0x008,
				accessControl   = 0x009,
				burglar         = 0x00A,
				powerManagement = 0x00B,
				system          = 0x00C,
				emergency       = 0x00D,
				clock           = 0x00E
			};

			enum dimension {      /* 13 bits (8192) */
				logic           = 0x0000,        /* values 0 or 1  */
				percentage      = 0x0001,        /* values 0 - 100 */
				kwh             = 0x0002,        /* kilo Watt hours  */
				kvah            = 0x0003,        /* kilo Volt Ampere hours */
				pulses          = 0x0004,        /* counter */
				degrees		= 0x0005,	 /* temperture in degrees celsius */
				units		= 0x0006,	 /* unqualified value, just units */
			};

			enum condition {
				constructing    = 0x0000,
				activated       = 0x0001,
				deactivated     = 0x0002
			};

			// Pushing notifications to interested sinks
			virtual void Register(INotification* sink) = 0;
			virtual void Unregister(INotification* sink) = 0;

			// Element require communication, so might fail, report our condition
			virtual condition Condition() const = 0;

			// Identification of this element.
			virtual uint32_t Identification() const = 0;

			// Characteristics of this element
			virtual uint32_t Type() const = 0;

			// Value determination of this element
			virtual int32_t Minimum() const = 0;
			virtual int32_t Maximum() const = 0;

			virtual uint32_t Get(int32_t& value) const = 0;
			virtual uint32_t Set(const int32_t value) = 0;

			// Periodically we might like to be triggered, call this method at a set time.
			virtual void Trigger() = 0;
		};

	template<enum IElement::identification IDENTIFIER>
	class ElementBase : public IElement {
	private:
		ElementBase(const ElementBase<IDENTIFIER>&) = delete;
		ElementBase<IDENTIFIER>& operator= (const ElementBase<IDENTIFIER>&) = delete;

		class Job : public Core::IDispatch {
		private:
			Job() = delete;
			Job(const Job&) = delete;
			Job& operator=(const Job&) = delete;

		public:
#ifdef __WIN32__ 
#pragma warning( disable : 4355 )
#endif
			Job(ElementBase* parent)
				: _parent(*parent)
				, _submitted(false)
				, _job(*this)
			{
				ASSERT(parent != nullptr);
			}
#ifdef __WIN32__ 
#pragma warning( default : 4355 )
#endif
			virtual ~Job()
			{
			}

		public:
			void Submit() {

				_parent.Lock();
				if (_submitted == false) {
					_submitted = true;
					_parent.Schedule(Core::Time(), _job);
				}
				_parent.Unlock();
			}
			void Dispose() {
				_parent.Lock();
				if (_submitted == true) {
					_parent.Revoke(_job);
					_submitted = false;
				}
				_job.Release();
				_parent.Unlock();
			}
			virtual void Dispatch()
			{
				_parent.Lock();
				std::list<IElement::INotification*>::iterator index(_parent._clients.begin());
				_submitted = false;
				_parent.RecursiveCall(index);
			}

		private:
			ElementBase& _parent;
			bool _submitted;
			Core::ProxyType< Core::IDispatch > _job;
		};
		class Timed : public Core::IDispatch {
		private:
			Timed() = delete;
			Timed(const Timed&) = delete;
			Timed& operator=(const Timed&) = delete;

		public:
#ifdef __WIN32__ 
#pragma warning( disable : 4355 )
#endif
			Timed(ElementBase* parent)
				: _parent(*parent)
				, _nextTime(0)
				, _periodicity(0)
				, _job(*this) {
				ASSERT(parent != nullptr);
			}
#ifdef __WIN32__ 
#pragma warning( default : 4355 )
#endif
			virtual ~Timed()
			{
			}

		public:
			inline uint16_t Period() const {
				return (_periodicity / (1000 * Core::Time::TicksPerMillisecond));
			}
			void Period(const uint16_t periodicity) {

				_parent.Lock();
				if (_job.IsValid()) {

					_periodicity = 0;

					_parent.Revoke(_job);
				}
				else if (periodicity != 0) {
					_job = Core::ProxyType<Timed>(*this);
				}

				_parent.Unlock();

				if (periodicity != 0) {

					ASSERT(_job.IsValid() == true);
					_periodicity = periodicity * 1000 * Core::Time::TicksPerMillisecond;
					_nextTime = Core::Time::Now().Ticks();

					_parent.Schedule(Core::Time(), _job);
				}
				else {
					_job.Release();
				}
			}
			virtual void Dispatch()
			{
				_parent.Trigger();

				_parent.Lock();
				if (_periodicity != 0) {

					Core::ProxyType< Core::IDispatchType<void> > job(*this);

					_nextTime += _periodicity;
					_parent.Schedule(Core::Time(_nextTime), job);
				}
				_parent.Unlock();
			}

		private:
			ElementBase& _parent;
			uint64_t _nextTime;
			uint32_t _periodicity;
			Core::ProxyType<Timed> _job;
		};

	public:
#ifdef __WIN32__ 
#pragma warning( disable : 4355 )
#endif
		inline ElementBase(const uint32_t id, const basic base, const specific spec, const dimension dim, const uint8_t decimals) 
			: _adminLock()
			, _id(ID | (id & 0x0FFFFFFF))
			, _type((dim << 19) | ((decimals & 0x07) << 16) | (base << 12) | spec)
			, _condition(IElement::constructing)
			, _clients()
			, _job(this)
			, _timed(this) {
			_job.AddRef();
			_timed.AddRef();
		}
		inline ElementBase(const uint32_t id, const uint32_t type)
			: _adminLock()
			, _id(ID | (id & 0x0FFFFFFF))
			, _type(type)
			, _condition(IElement::constructing)
			, _clients()
			, _job(this)
			, _timed(this) {
			_job.AddRef();
			_timed.AddRef();
		}
#ifdef __WIN32__ 
#pragma warning( default : 4355 )
#endif

		virtual ~ElementBase() {
			_job.Dispose();
			_timed.Period(0);
			_job.CompositRelease();
			_timed.CompositRelease();
		}


	public:
		// ------------------------------------------------------------------------
		// Convenience methods to extract interesting information from the type
		// ------------------------------------------------------------------------
		inline basic Basic() const {
			return (static_cast<basic>((_type >> 12) & 0xF));
		}
		inline dimension Dimension() const {
			return (static_cast<dimension>((_type >> 19) & 0x1FFF));
		}
		inline specific Specific() const {
			return (static_cast<specific>(_type & 0xFFF));
		}
		inline uint8_t Decimals() const {
			return ((_type >> 16) & 0x07);
		}
	
		// ------------------------------------------------------------------------
		// Polling interface methods
		// ------------------------------------------------------------------------
		// Define the polling time in Seconds. This value has a maximum of a 24 hour.
		inline uint16_t Period() const {
			return (_timed.Period());
		}
		inline void Period(const uint16_t value) {
			_timed.Period(value);
		}

		// ------------------------------------------------------------------------
		// IElement default interface implementation
		// ------------------------------------------------------------------------
		// Pushing notifications to interested sinks
		virtual void Register(IElement::INotification* sink) override {

			Lock();

			std::list<IElement::INotification*>::iterator index = std::find(_clients.begin(), _clients.end(), sink);

			if (index == _clients.end()) {
				sink->AddRef();
				_clients.push_back(sink);
				sink->Update();
			}

			Unlock();
		}

		virtual void Unregister(IElement::INotification* sink) override {

			Lock();

			std::list<IElement::INotification*>::iterator index = std::find(_clients.begin(), _clients.end(), sink);

			if (index != _clients.end()) {
				sink->Release();
				_clients.erase(index);
			}

			Unlock();
		}

		virtual condition Condition() const override {
			return (_condition.load());
		}

		// Identification of this element.
		virtual uint32_t Identification() const override {
			return(_id);
		}

		// Characteristics of this element
		virtual uint32_t Type() const override {
			return (_type);
		}

		virtual int32_t Minimum() const override {
			int32_t result = 0;

			switch (Dimension()) {
			case logic:
			case percentage:
			case kwh:
			case kvah:
			case pulses:
			{
				break;
			}
			case degrees:
			{
				result = -127;
				break;
			}
			case units:
			{	
				result = Core::NumberType<int32_t>::Min();
				break;
			}
			}
			return(result);
		}

		virtual int32_t Maximum() const override {
			int32_t result = 1;
			switch (Dimension()) {
			case logic:
			{
				break;
			}
			case percentage:
			{
				result = 100;
				break;
			}
			case kwh:
			case kvah:
			case pulses:
			case units:
			{
				result = Core::NumberType<int32_t>::Max();
				break;
			}
			case degrees:
			{
				result = +512;
				break;
			}
			}
			return(result);
		}
		virtual void Activate() {
			IElement::condition expected = IElement::constructing;

			if (_condition.compare_exchange_strong(expected, IElement::activated) == true) {
				_job.Submit();
			}
		}
		virtual void Deactivate() {
			IElement::condition expected = IElement::activated;

			if (_condition.compare_exchange_strong(expected, IElement::deactivated) == true) {
				_job.Submit();
				_timed.Period(0);
			}
			else {
				expected = IElement::constructing;
				_condition.compare_exchange_strong(expected, IElement::deactivated);
			}
		}

		virtual void Trigger() = 0;
		virtual uint32_t Get(int32_t& value) const = 0;
		virtual uint32_t Set(const int32_t value) = 0;
		virtual void Schedule(const Core::Time& time, const Core::ProxyType<Core::IDispatch>& job) = 0;
		virtual void Revoke(const Core::ProxyType<Core::IDispatch>& job) = 0;

		BEGIN_INTERFACE_MAP(ElementBase)
			INTERFACE_ENTRY(Exchange::IElement)
		END_INTERFACE_MAP

	protected:
		inline void Updated() {
			Lock();
			_job.Submit();
			Unlock();
		}
		inline void Lock() const {
			_adminLock.Lock();
		}
		inline void Unlock() const {
			_adminLock.Unlock();
		}
		inline void ChangeTypeId(const uint32_t id, const uint32_t type) {
			_id = id;
			_type = type;
		}

	private:
		void RecursiveCall(std::list<IElement::INotification*>::iterator& position) {
			if (position == _clients.end()) {
				Unlock();
			}
			else {
				IElement::INotification* client(*position);
				client->AddRef();
				position++;
				RecursiveCall(position);
				client->Update();
				client->Release();
			}
		}

	private:
		mutable Core::CriticalSection _adminLock;
		uint32_t _id;
		uint32_t _type;
		std::atomic<condition> _condition;
		std::list<IElement::INotification*> _clients;
		Core::ProxyObject<Job> _job;
		Core::ProxyObject<Timed> _timed;
	};
	
	}

} // Namespace Exchange

#endif // __IELEMENT_OBJECT_H

