#ifndef __URL_H
#define __URL_H

#include "Module.h"

//
//  foo://username:password@example.com:8042/over/there/index.dtb;type=animal?name=ferret#nose
//  \ /   \________________/\_________/ \__/            \___/ \_/ \_________/ \_________/ \__/
//   |           |               |       |                |    |       |           |       |
//   |       userinfo         hostname  port              |    |       parameter query  fragment
//   |    \_______________________________/ \_____________|____|____________/
//scheme                  |                               | |  |
//   |                authority                           |path|
//   |                                                    |    |
//   |            path                       interpretable as filename
//   |   ___________|____________                              |
//  / \ /                        \                             |
//  urn:example:animal:ferret:nose               interpretable as extension
//
#define MAX_URL_SIZE 1024

namespace WPEFramework {

namespace Core {
    class EXTERNAL URL {
    public:
        typedef enum {
            SCHEME_FILE = 0,
            SCHEMA_MAIL,
            SCHEME_HTTP,
            SCHEME_HTTPS,
            SCHEME_FTP,
            SCHEME_TELNET,
            SCHEME_GOPHER,
            SCHEME_LDAP,
            SCHEME_RTSP,
            SCHEME_RTP,
            SCHEME_RTP_UDP,
            SCHEME_RTP_TCP,
            SCHEME_RTCP,
            SCHEME_WS,
            SCHEME_WSS,
			SCHEME_NTP,
            SCHEME_UNKNOWN

        } SchemeType;

		class KeyValue
		{
		public:
			KeyValue(const string& data) : _data(data) {}
			KeyValue(const Core::TextFragment& data) : _data(data) {}
			~KeyValue() {}

			inline bool Exists(const TCHAR key[], const bool caseSensitive = true) const {
				Core::TextFragment result;
				return (LoadKey(key, result, caseSensitive));
			}
			// By definition is this method case sensitive..
			const Core::TextFragment operator[] (const TCHAR key[]) const
			{
				Core::TextFragment result;
				LoadKey(key, result, true);
				return (result);
			}
			const Core::TextFragment Value (const TCHAR key[], const bool caseSensitive = true) const
			{
				Core::TextFragment result;
				LoadKey(key, result, caseSensitive);
				return (result);
			}
			template<typename NUMBERTYPE>
			NUMBERTYPE Number(const TCHAR key[], NUMBERTYPE defaultValue = Core::NumberType<NUMBERTYPE>::Max(), const bool caseSensitive = true) const
			{
				Core::TextFragment result;
				if (LoadKey(key, result, caseSensitive) == true)
				{
					return (Core::NumberType<NUMBERTYPE>(result).Value());
				}
				return (defaultValue);
			}
			bool HasKey(const TCHAR key[], const bool caseSensitive = true) const
			{
				Core::TextFragment result;
				return (LoadKey(key, result, caseSensitive));
			}
			bool Boolean(const TCHAR key[], const bool defaultValue, const bool caseSensitive = true) const
			{
				Core::TextFragment result;
				if (LoadKey(key, result, caseSensitive) == true)
				{
					if (result.Length() == 1) {
						TCHAR value = ::toupper(*(result.Data()));
						if ( (value == '0') || (value == 'F') ) {
							return (false);
						}
						else if ((value == '1') || (value == 'T')) {
							return (true);
						}
					}
					else if (result.Length() == 4) {
						if (result.EqualText(_T("TRUE"), 0, 4, false) == true) {
							return (true);
						}
					}
					else if (result.Length() == 5) {
						if (result.EqualText(_T("FALSE"), 0, 5, false) == true) {
							return (false);
						}
					}
				}
				return (defaultValue);
			}

		private:
			bool LoadKey(const TCHAR key[], Core::TextFragment& data, const bool caseSensitive) const
			{
				bool found = false;
				const Core::TextFragment comparator(key, strlen(key));
				Core::TextSegmentIterator options(_data, false, '&');

				while ((options.Next() == true) && (found == false))
				{
					Core::TextSegmentIterator keyvalue(options.Current(), false, '=');

					if (keyvalue.Next() == true) {
						if (keyvalue.Current().EqualText(comparator, caseSensitive) == true) {
							found = true;
							if (keyvalue.Next() == true)
							{
								data = keyvalue.Current();
							}
							else
							{
								data.Clear();
							}
						}
					}
				}
				return (found);
			}

		private:
			Core::TextFragment _data;
		};

    public:
        /* constructors */
        /* The constructors throw InternalException if there is an error
         * in the format of the URL, such as invalid protocol or port number.
         * The exception will contain an message for the error.
         */
        URL()
            : m_SchemeInfo(nullptr)
            , m_Scheme()
            , m_Username()
            , m_Password()
            , m_Host()
            , m_Port()
            , m_Path()
            , m_Query()
            , m_Ref()
            , m_Maintainer()
        {
        }
        URL(const SchemeType type)
            : m_SchemeInfo(nullptr)
            , m_Scheme()
            , m_Username()
            , m_Password()
            , m_Host()
            , m_Port()
            , m_Path()
            , m_Query()
            , m_Ref()
            , m_Maintainer()
        {
            SetScheme(type);
        }
        URL(const TCHAR urlStr[])
            : m_SchemeInfo(nullptr)
            , m_Scheme()
            , m_Username()
            , m_Password()
            , m_Host()
            , m_Port()
            , m_Path()
            , m_Query()
            , m_Ref()
            , m_Maintainer(urlStr)
        {
            Parse(m_Maintainer);
        }
        URL(const string& urlStr)
            : m_SchemeInfo(nullptr)
            , m_Scheme()
            , m_Username()
            , m_Password()
            , m_Host()
            , m_Port()
            , m_Path()
            , m_Query()
            , m_Ref()
            , m_Maintainer(urlStr)
        {
            Parse(m_Maintainer);
        }
        URL(const TextFragment& text)
            : m_SchemeInfo(nullptr)
            , m_Scheme()
            , m_Username()
            , m_Password()
            , m_Host()
            , m_Port()
            , m_Path()
            , m_Query()
            , m_Ref()
            , m_Maintainer(text.Text())
        {
            Parse(m_Maintainer);
        }
        URL(const URL& copy)
            : m_SchemeInfo(copy.m_SchemeInfo)
            , m_Scheme(copy.m_Scheme)
            , m_Username(copy.m_Username)
            , m_Password(copy.m_Password)
            , m_Host(copy.m_Host)
            , m_Port(copy.m_Port)
            , m_Path(copy.m_Path)
            , m_Query(copy.m_Query)
            , m_Ref(copy.m_Ref)
            , m_Maintainer(copy.m_Maintainer)
        {
        }
        ~URL()
        {
        }

    public:
        inline URL& operator=(const URL& copy)
        {
            m_SchemeInfo = copy.m_SchemeInfo;
            m_Scheme = copy.m_Scheme;
            m_Username = copy.m_Username;
            m_Password = copy.m_Password;
            m_Host = copy.m_Host;
            m_Port = copy.m_Port;
            m_Path = copy.m_Path;
            m_Query = copy.m_Query;
            m_Ref = copy.m_Ref;
            m_Maintainer = copy.m_Maintainer;

            return (*this);
        }

        SchemeType Type() const;

        inline bool IsValid() const
        {
            return (m_SchemeInfo != nullptr);
        }

        inline const OptionalType<TextFragment>& Scheme() const
        {
            return (m_Scheme);
        }

        inline const OptionalType<TextFragment>& UserName() const
        {
            return (m_Username);
        }

        inline const OptionalType<TextFragment>& Password() const
        {
            return (m_Password);
        }

        inline const OptionalType<TextFragment>& Host() const
        {
            return (m_Host);
        }

        inline const OptionalType<unsigned short>& Port() const
        {
            return (m_Port);
        }

        inline const OptionalType<TextFragment>& Path() const
        {
            return (m_Path);
        }

        inline const OptionalType<TextFragment>& Query() const
        {
            return (m_Query);
        }

        inline const OptionalType<TextFragment>& Ref() const
        {
            return (m_Ref);
        }

        inline void UserName(const OptionalType<TextFragment>& value)
        {
            m_Username = value;
        }

        inline void Password(const OptionalType<TextFragment>& value)
        {
            m_Password = value;
        }

        inline void Host(const OptionalType<TextFragment>& value)
        {
            m_Host = value;
        }

        inline void Port(const OptionalType<unsigned short> port)
        {
            m_Port = port;
        }

        inline void Path(const OptionalType<TextFragment>& value)
        {
            m_Path = value;
        }

        inline void Query(const OptionalType<TextFragment>& value)
        {
            m_Query = value;
        }

        inline void Ref(const OptionalType<TextFragment>& value)
        {
            m_Ref = value;
        }

        TextFragment Text() const;

        static void CreateStandardURL(TCHAR url[], int url_len, const URL& info)
        {
            info.CreateStandardURL(url, url_len);
        }
        static void CreatePathURL(TCHAR url[], int url_len, const URL& info)
        {
            info.CreatePathURL(url, url_len);
        }
        static void CreateFileURL(TCHAR url[], int url_len, const URL& info)
        {
            info.CreateFileURL(url, url_len);
        }
        static void CreateMailtoURL(TCHAR url[], int url_len, const URL& info)
        {
            info.CreateMailtoURL(url, url_len);
        }
        bool IsDomain (const TCHAR domain[], const unsigned int length) const
        {
            bool result = false;

            if ((m_Host.IsSet() == true) && (m_Host.Value().Length() >= length)) {
		        uint32_t offset = m_Host.Value().Length() - length;
 
				if ( (offset == 0) || (m_Host.Value()[(offset -1)] == '.') )
				{
					uint32_t index = 0;
					while ((index < length) && (tolower(domain[index]) == m_Host.Value()[index + offset])) {
						index++;
					}
					result = (index == length);
				}
            }
            return (result);
        }
        static uint16_t Encode(const TCHAR* source, const uint16_t sourceLength, TCHAR* destination, const uint16_t destinationLength);
        static uint16_t Decode(const TCHAR* source, const uint16_t sourceLength, TCHAR* destination, const uint16_t destinationLength);

    private:
        void Parse(const string& urlStr);
        void SetComponent(const string& urlStr, const unsigned int begin, const unsigned int end, OptionalType<TextFragment>& SetInfo);
        void SetScheme(const SchemeType type);

        // StandardURL is for when the scheme is known to be one that has an
        // authority (host) like "http". This function will not handle weird ones
        // like "about:" and "javascript:", or do the right thing for "file:" URLs.
        void CreateStandardURL(TCHAR url[], int url_len) const;

        // PathURL is for when the scheme is known not to have an authority (host)
        // section but that aren't file URLs either. The scheme is parsed, and
        // everything after the scheme is considered as the path. This is used for
        // things like "about:" and "javascript:"
        void CreatePathURL(TCHAR url[], int url_len) const;

        // FileURL is for file URLs. There are some special rules for interpreting
        // these.
        void CreateFileURL(TCHAR url[], int url_len) const;

        // MailtoURL is for mailto: urls. They are made up scheme,path,query
        void CreateMailtoURL(TCHAR url[], int url_len) const;

    private:
        const void* m_SchemeInfo;
        OptionalType<TextFragment> m_Scheme;
        OptionalType<TextFragment> m_Username;
        OptionalType<TextFragment> m_Password;
        OptionalType<TextFragment> m_Host;
        OptionalType<unsigned short> m_Port;
        OptionalType<TextFragment> m_Path;
        OptionalType<TextFragment> m_Query;
        OptionalType<TextFragment> m_Ref;

        // This is a placeholder to reference the string in case a string is ingested..
        string m_Maintainer;
    };
}
} // namespace Core

#endif // _URL
