#ifndef XGETOPT_H
#define XGETOPT_H

#include "Portability.h"
#include "Module.h"

namespace WPEFramework {
namespace Core {
    class EXTERNAL Options {
    private:
        Options();
        Options(const Options&);
        Options& operator=(const Options&);

    public:
        Options(int argumentCount, TCHAR* arguments[], const TCHAR options[])
            : _argumentCount(argumentCount)
            , _arguments(arguments)
            , _options(options)
            , _command(nullptr)
            , _valid(false)
            , _requestUsage(false)
        {
        }
        virtual ~Options()
        {
        }

    public:
        inline bool HasErrors() const
        {
            return (!_valid);
        }
        inline const TCHAR* Command() const
        {
            return (_command);
        }
        inline bool RequestUsage() const
        {
            return (_valid || _requestUsage);
        }

        virtual void Option(const TCHAR option, const TCHAR* argument) = 0;
        void Parse();

    protected:
        inline void RequestUsage(const bool value)
        {
            _requestUsage = value;
        }

    private:
        int _argumentCount;
        TCHAR** _arguments;
        const TCHAR* _options;
        TCHAR* _command;
        bool _valid;
        bool _requestUsage;
    };
}
} // namespace Core

#endif //XGETOPT_H
