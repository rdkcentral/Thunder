#ifndef __IDICTIONARY_H
#define __IDICTIONARY_H

#include "Module.h"

namespace WPEFramework {
namespace Exchange {

    // This interface gives direct access to a Browser to change
    // Browser specific properties like displayed URL.
    struct IDictionary : virtual public Core::IUnknown {
        enum { ID = 0x00000049 };

        struct INotification : public Core::IUnknown {

            enum { ID = 0x0000004A };

            virtual ~INotification() {}

            // Signal changes on the subscribed namespace..
            virtual void Modified(const string& nameSpace, const string& key, const string& value) = 0;
        };

		struct IIterator : public Core::IUnknown {

			enum { ID = 0x0000004B };

			virtual ~IIterator() {}

			virtual void Reset() = 0;
			virtual bool IsValid() const = 0;
			virtual bool Next() = 0;

			// Signal changes on the subscribed namespace..
			virtual const string Key() const = 0;
			virtual const string Value() const = 0;
		};

		virtual ~IDictionary() {}

        // Allow to observe values in the dictionary. If they are changed, the sink gets notified.
        virtual void Register(const string& nameSpace, struct IDictionary::INotification* sink) = 0;
        virtual void Unregister(const string& nameSpace, struct IDictionary::INotification* sink) = 0;

        // Getters and Setters for the dictionary.
        virtual bool Get(const string& nameSpace, const string& key, string& value) const = 0;
        virtual bool Set(const string& nameSpace, const string& key, const string& value) = 0;
		virtual IIterator* Get(const string& nameSpace) const = 0;

    };
}
}

#endif // __IDICTIONARY_H
