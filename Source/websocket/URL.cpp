#include "URL.h"

// Copyright 2007, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Component ------------------------------------------------------------------

// Represents a substring for URL parsing.
namespace WPEFramework {

    ENUM_CONVERSION_BEGIN(URL::SchemeType)

        { URL::SCHEME_FILE, _TXT("file") },
        { URL::SCHEMA_MAIL, _TXT("mailto") },
        { URL::SCHEME_HTTP, _TXT("http") },
        { URL::SCHEME_HTTPS, _TXT("https") },
        { URL::SCHEME_FTP, _TXT("ftp") },
        { URL::SCHEME_TELNET, _TXT("telnet") },
        { URL::SCHEME_GOPHER, _TXT("gopher") },
        { URL::SCHEME_LDAP, _TXT("ldap") },
        { URL::SCHEME_RTSP, _TXT("rtsp") },
        { URL::SCHEME_RTP, _TXT("rtp") },
        { URL::SCHEME_RTCP, _TXT("rtcp") },
        { URL::SCHEME_RTP_UDP, _TXT("rtpudp") },
        { URL::SCHEME_RTP_TCP, _TXT("rtptcp") },
        { URL::SCHEME_WS, _TXT("ws") },
        { URL::SCHEME_WSS, _TXT("wss") },
        { URL::SCHEME_UNKNOWN, _TXT("????"), },

    ENUM_CONVERSION_END(URL::SchemeType)


namespace Core {
    struct Component {
        Component()
            : _begin(0)
            , _len(-1)
        {
        }

        // Normal constructor: takes an offset and a length.
        Component(int b, int l)
            : _begin(b)
            , _len(l)
        {
        }

        int length() const
        {
            return (_len);
        }

        int end() const
        {
            return _begin + _len;
        }

        int begin() const
        {
            return _begin;
        }
        // Helper that returns a component created with the given begin and ending
        // points. The ending point is non-inclusive.
        inline void SetRange(int start, int stop)
        {
            _begin = start;
            _len = stop - start;
        }

        // Returns true if this component is valid, meaning the length is given. Even
        // valid components may be empty to record the fact that they exist.
        bool is_valid() const
        {
            return (_len != -1);
        }

        // Returns true if the given component is specified on false, the component
        // is either empty or invalid.
        bool is_nonempty() const
        {
            return (_len > 0);
        }

        void reset()
        {
            _begin = 0;
            _len = -1;
        }

        bool operator==(const Component& other) const
        {
            return _begin == other._begin && _len == other._len;
        }

        int _begin; // Byte offset in the string of this component.
        int _len; // Will be -1 if the component is unspecified.
    };

    // Parsed ---------------------------------------------------------------------

    // A structure that holds the identified parts of an input URL. This structure
    // does NOT store the URL itself. The caller will have to store the URL text
    // and its corresponding Parsed structure separately.
    //
    // Typical usage would be:
    //
    //    url_parse::Parsed parsed;
    //    url_parse::Component scheme;
    //    if (!url_parse::ExtractScheme(url, url_len, &scheme))
    //      return I_CAN_NOT_FIND_THE_SCHEME_DUDE;
    //
    //    if (IsStandardScheme(url, scheme))  // Not provided by this component
    //      url_parseParseStandardURL(url, url_len, &parsed);
    //    else if (IsFileURL(url, scheme))    // Not provided by this component
    //      url_parse::ParseFileURL(url, url_len, &parsed);
    //    else
    //      url_parse::ParsePathURL(url, url_len, &parsed);
    //
    struct Parsed {
        // Identifies different components.
        enum ComponentType {
            SCHEME,
            USERNAME,
            PASSWORD,
            HOST,
            PORT,
            PATH,
            QUERY,
            REF,
        };

        // The default constructor is sufficient for the components.
        Parsed() {}

        // Returns the length of the URL (the end of the last component).
        //
        // Note that for some invalid, non-canonical URLs, this may not be the length
        // of the string. For example "http://": the parsed structure will only
        // contain an entry for the four-character scheme, and it doesn't know about
        // the "://". For all other last-components, it will return the real length.
        int Length() const;

        // Returns the number of characters before the given component if it exists,
        // or where the component would be if it did exist. This will return the
        // string length if the component would be appended to the end.
        //
        // Note that this can get a little funny for the port, query, and ref
        // components which have a delimiter that is not counted as part of the
        // component. The |include_delimiter| flag controls if you want this counted
        // as part of the component or not when the component exists.
        //
        // This example shows the difference between the two flags for two of these
        // delimited components that is present (the port and query) and one that
        // isn't (the reference). The components that this flag affects are marked
        // with a *.
        //                 0         1         2
        //                 012345678901234567890
        // Example input:  http://foo:80/?query
        //              include_delim=true,  ...=false  ("<-" indicates different)
        //      SCHEME: 0                    0
        //    USERNAME: 5                    5
        //    PASSWORD: 5                    5
        //        HOST: 7                    7
        //       *PORT: 10                   11 <-
        //        PATH: 13                   13
        //      *QUERY: 14                   15 <-
        //        *REF: 20                   20
        //
        int CountCharactersBefore(ComponentType type, bool include_delimiter) const;

        // Scheme without the colon: "http://foo"/ would have a scheme of "http".
        // The length will be -1 if no scheme is specified ("foo.com"), or 0 if there
        // is a colon but no scheme (":foo"). Note that the scheme is not guaranteed
        // to start at the beginning of the string if there are preceeding whitespace
        // or control characters.
        Component scheme;

        // Username. Specified in URLs with an @ sign before the host. See |password|
        Component username;

        // Password. The length will be -1 if unspecified, 0 if specified but empty.
        // Not all URLs with a username have a password, as in "http://me@host/".
        // The password is separated form the username with a colon, as in
        // "http://me:secret@host/"
        Component password;

        // Host name.
        Component host;

        // Port number.
        Component port;

        // Path, this is everything following the host name. Length will be -1 if
        // unspecified. This includes the preceeding slash, so the path on
        // http://www.google.com/asdf" is "/asdf". As a result, it is impossible to
        // have a 0 length path, it will be -1 in cases like "http://host?foo".
        // Note that we treat backslashes the same as slashes.
        Component path;

        // Stuff between the ? and the # after the path. This does not include the
        // preceeding ? character. Length will be -1 if unspecified, 0 if there is
        // a question mark but no query string.
        Component query;

        // Indicated by a #, this is everything following the hash sign (not
        // including it). If there are multiple hash signs, we'll use the last one.
        // Length will be -1 if there is no hash sign, or 0 if there is one but
        // nothing follows it.
        Component ref;
    };

    // Initialization functions ---------------------------------------------------
    //
    // These functions parse the given URL, filling in all of the structure's
    // components. These functions can not fail, they will always do their best
    // at interpreting the input given.
    //
    // The string length of the URL MUST be specified, we do not check for nullptrs
    // at any point in the process, and will actually handle embedded nullptrs.
    //
    // IMPORTANT: These functions do NOT hang on to the given pointer or copy it
    // in any way. See the comment above the struct.
    //
    // The 8-bit versions require UTF-8 encoding.

    // StandardURL is for when the scheme is known to be one that has an
    // authority (host) like "http". This function will not handle weird ones
    // like "about:" and "javascript:", or do the right thing for "file:" URLs.
    void ParseStandardURL(const TCHAR url[], int url_len, Parsed& parsed);

    // PathURL is for when the scheme is known not to have an authority (host)
    // section but that aren't file URLs either. The scheme is parsed, and
    // everything after the scheme is considered as the path. This is used for
    // things like "about:" and "javascript:"
    void ParsePathURL(const TCHAR url[], int url_len, Parsed& parsed);

    // FileURL is for file URLs. There are some special rules for interpreting
    // these.
    void ParseFileURL(const TCHAR url[], int url_len, Parsed& parsed);

    // MailtoURL is for mailto: urls. They are made up scheme,path,query
    void ParseMailtoURL(const TCHAR url[], int url_len, Parsed& parsed);

    // Helper functions -----------------------------------------------------------

    // Locates the scheme according to the URL  parser's rules. This function is
    // designed so the caller can find the scheme and call the correct Init*
    // function according to their known scheme types.
    //
    // It also does not perform any validation on the scheme.
    //
    // This function will return true if the scheme is found and will put the
    // scheme's range into *scheme. False means no scheme could be found. Note
    // that a URL beginning with a colon has a scheme, but it is empty, so this
    // function will return true but *scheme will = (0,0).
    //
    // The scheme is found by skipping spaces and control characters at the
    // beginning, and taking everything from there to the first colon to be the
    // scheme. The character at scheme.end() will be the colon (we may enhance
    // this to handle full width colons or something, so don't count on the
    // actual character value). The character at scheme.end()+1 will be the
    // beginning of the rest of the URL, be it the authority or the path (or the
    // end of the string).
    //
    // The 8-bit version requires UTF-8 encoding.
    bool ExtractScheme(const TCHAR url[], int url_len, Component& scheme);

    // Returns true if ch is a character that terminates the authority segment
    // of a URL.
    bool IsAuthorityTerminator(TCHAR ch);

    // Does a best effort parse of input |spec|, in range |auth|. If a particular
    // component is not found, it will be set to invalid.
    void ParseAuthority(const TCHAR spec[],
        const Component& auth,
        Component& username,
        Component& password,
        Component& hostname,
        Component& port_num);

    // Computes the integer port value from the given port component. The port
    // component should have been identified by one of the init functions on
    // |Parsed| for the given input url.
    //
    // The return value will be a positive integer between 0 and 64K, or one of
    // the two special values below.
    enum SpecialPort { PORT_UNSPECIFIED = -1,
        PORT_INVALID = -2 };
    int ParsePort(const TCHAR url[], const Component& port);

    // Extracts the range of the file name in the given url. The path must
    // already have been computed by the parse function, and the matching URL
    // and extracted path are provided to this function. The filename is
    // defined as being everything from the last slash/backslash of the path
    // to the end of the path.
    //
    // The file name will be empty if the path is empty or there is nothing
    // following the last slash.
    //
    // The 8-bit version requires UTF-8 encoding.
    void ExtractFileName(const TCHAR url[],
        const Component& path,
        Component& file_name);

    // Extract the first key/value from the range defined by |*query|. Updates
    // |*query| to start at the end of the extracted key/value pair. This is
    // designed for use in a loop: you can keep calling it with the same query
    // object and it will iterate over all items in the query.
    //
    // Some key/value pairs may have the key, the value, or both be empty (for
    // example, the query string "?&"). These will be returned. Note that an empty
    // last parameter "foo.com?" or foo.com?a&" will not be returned, this case
    // is the same as "done."
    //
    // The initial query component should not include the '?' (this is the default
    // for parsed URLs).
    //
    // If no key/value are found |*key| and |*value| will be unchanged and it will
    // return false.
    bool ExtractQueryKeyValue(const TCHAR url[],
        Component& query,
        Component& key,
        Component& value);

    // We treat slashes and backslashes the same for IE compatability.
    inline bool IsURLSlash(TCHAR ch)
    {
        return ch == '/' || ch == '\\';
    }

    // Returns true if we should trim this character from the URL because it is a
    // space or a control character.
    inline bool ShouldTrimFromURL(TCHAR ch)
    {
        return ch <= ' ';
    }

    // Given an already-initialized begin index and length, this shrinks the range
    // to eliminate "should-be-trimmed" characters. Note that the length does *not*
    // indicate the length of untrimmed data from |*begin|, but rather the position
    // in the input string (so the string starts at character |*begin| in the spec,
    // and goes until |*len|).
    template <typename CHAR>
    inline void TrimURL(const CHAR* spec, int* begin, int* len)
    {
        // Strip leading whitespace and control characters.
        while (*begin < *len && ShouldTrimFromURL(spec[*begin])) {
            (*begin)++;
        }

        // Strip trailing whitespace and control characters. We need the >i test for
        // when the input string is all blanks; we don't want to back past the input.
        while (*len > *begin && ShouldTrimFromURL(spec[*len - 1])) {
            (*len)--;
        }
    }

    // Counts the number of consecutive slashes starting at the given offset
    // in the given string of the given length.
    template <typename CHAR>
    inline int CountConsecutiveSlashes(const CHAR* str,
        int begin_offset, int str_len)
    {
        int count = 0;
        while (begin_offset + count < str_len && IsURLSlash(str[begin_offset + count])) {
            ++count;
        }
        return count;
    }

    // Internal functions in url_parse.cc that parse the path, that is, everything
    // following the authority section. The input is the range of everything
    // following the authority section, and the output is the identified ranges.
    //
    // This is designed for the file URL parser or other consumers who may do
    // special stuff at the beginning, but want regular path parsing, it just
    // maps to the internal parsing function for paths.
    void ParsePathInternal(const TCHAR spec[],
        const Component& path,
        Component& filepath,
        Component& query,
        Component& ref);

    // Given a spec and a pointer to the character after the colon following the
    // scheme, this parses it and fills in the structure, Every item in the parsed
    // structure is filled EXCEPT for the scheme, which is untouched.
    void ParseAfterScheme(const TCHAR spec[],
        int spec_len,
        int after_scheme,
        Parsed& parsed);

#ifdef __WIN32__

    // We allow both "c:" and "c|" as drive identifiers.
    inline bool IsWindowsDriveSeparator(TCHAR ch)
    {
        return ch == ':' || ch == '|';
    }
    inline bool IsWindowsDriveLetter(TCHAR ch)
    {
        return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z');
    }

#endif // __WIN32__

    // Returns the index of the next slash in the input after the given index, or
    // spec_len if the end of the input is reached.
    template <typename CHAR>
    inline int FindNextSlash(const CHAR* spec, int begin_index, int spec_len)
    {
        int idx = begin_index;
        while (idx < spec_len && !IsURLSlash(spec[idx])) {
            idx++;
        }
        return idx;
    }

#ifdef __WIN32__

    // Returns true if the start_offset in the given spec looks like it begins a
    // drive spec, for example "c:". This function explicitly handles start_offset
    // values that are equal to or larger than the spec_len to simplify callers.
    //
    // If this returns true, the spec is guaranteed to have a valid drive letter
    // plus a colon starting at |start_offset|.
    template <typename CHAR>
    inline bool DoesBeginWindowsDriveSpec(const CHAR* spec, int start_offset,
        int spec_len)
    {
        int remaining_len = spec_len - start_offset;
        if (remaining_len < 2) {
            return false; // Not enough room.
        }
        if (!IsWindowsDriveLetter(spec[start_offset])) {
            return false; // Doesn't start with a valid drive letter.
        }
        if (!IsWindowsDriveSeparator(spec[start_offset + 1])) {
            return false; // Isn't followed with a drive separator.
        }
        return true;
    }

    // Returns true if the start_offset in the given text looks like it begins a
    // UNC path, for example "\\". This function explicitly handles start_offset
    // values that are equal to or larger than the spec_len to simplify callers.
    //
    // When strict_slashes is set, this function will only accept backslashes as is
    // standard for Windows. Otherwise, it will accept forward slashes as well
    // which we use for a lot of URL handling.
    template <typename CHAR>
    inline bool DoesBeginUNCPath(const CHAR* text,
        int start_offset,
        int len,
        bool strict_slashes)
    {
        int remaining_len = len - start_offset;
        if (remaining_len < 2) {
            return false;
        }

        if (strict_slashes) {
            return text[start_offset] == '\\' && text[start_offset + 1] == '\\';
        }
        return IsURLSlash(text[start_offset]) && IsURLSlash(text[start_offset + 1]);
    }

#endif // __WIN32__

    /* Based on nsURLParsers.cc from Mozilla
 * -------------------------------------
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Darin Fisher (original author)
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

    // Returns true if the given character is a valid digit to use in a port.
    inline bool IsPortDigit(TCHAR ch)
    {
        return ch >= '0' && ch <= '9';
    }

    // Returns the offset of the next authority terminator in the input starting
    // from start_offset. If no terminator is found, the return value will be equal
    // to spec_len.
    template <typename CHAR>
    int FindNextAuthorityTerminator(const CHAR* spec,
        int start_offset,
        int spec_len)
    {
        for (int i = start_offset; i < spec_len; i++) {
            if (IsAuthorityTerminator(spec[i])) {
                return i;
            }
        }
        return spec_len; // Not found.
    }

    // Fills in all members of the Parsed structure except for the scheme.
    //
    // |spec| is the full spec being parsed, of length |spec_len|.
    // |after_scheme| is the character immediately following the scheme (after the
    //   colon) where we'll begin parsing.
    //
    // Compatability data points. I list "host", "path" extracted:
    // Input                IE6             Firefox                Us
    // -----                --------------  --------------         --------------
    // http://foo.com/      "foo.com", "/"  "foo.com", "/"         "foo.com", "/"
    // http:foo.com/        "foo.com", "/"  "foo.com", "/"         "foo.com", "/"
    // http:/foo.com/       fail(*)         "foo.com", "/"         "foo.com", "/"
    // http:\foo.com/       fail(*)         "\foo.com", "/"(fail)  "foo.com", "/"
    // http:////foo.com/    "foo.com", "/"  "foo.com", "/"         "foo.com", "/"
    //
    // (*) Interestingly, although IE fails to load these URLs, its history
    // canonicalizer handles them, meaning if you've been to the corresponding
    // "http://foo.com/" link, it will be colored.
    template <typename CHAR>
    void DoParseAfterScheme(const CHAR spec[],
        int spec_len,
        int after_scheme,
        Parsed& parsed)
    {
        int num_slashes = CountConsecutiveSlashes(spec, after_scheme, spec_len);
        int after_slashes = after_scheme + num_slashes;

        // First split into two main parts, the authority (username, password, host,
        // and port) and the full path (path, query, and reference).
        Component authority;
        Component full_path;

        // Found "//<some data>", looks like an authority section. Treat everything
        // from there to the next slash (or end of spec) to be the authority. Note
        // that we ignore the number of slashes and treat it as the authority.
        int end_auth = FindNextAuthorityTerminator(spec, after_slashes, spec_len);
        authority = Component(after_slashes, end_auth - after_slashes);

        if (end_auth == spec_len) { // No beginning of path found.
            full_path = Component();
        }
        else { // Everything starting from the slash to the end is the path.
            full_path = Component(end_auth, spec_len - end_auth);
        }

        // Now parse those two sub-parts.
        DoParseAuthority(spec, authority, parsed.username, parsed.password,
            parsed.host, parsed.port);
        ParsePath(spec, full_path, parsed.path, parsed.query, parsed.ref);
    }

    template <typename CHAR>
    void ParseUserInfo(const CHAR* spec,
        const Component& user,
        Component& username,
        Component& password)
    {
        // Find the first colon in the user section, which separates the username and
        // password.
        int colon_offset = 0;
        while (colon_offset < user._len && spec[user._begin + colon_offset] != ':') {
            colon_offset++;
        }

        if (colon_offset < user._len) {
            // Found separator: <username>:<password>
            username = Component(user._begin, colon_offset);
            password.SetRange(user._begin + colon_offset + 1,
                user._begin + user._len);
        }
        else {
            // No separator, treat everything as the username
            username = user;
            password = Component();
        }
    }

    template <typename CHAR>
    void ParseServerInfo(const CHAR* spec,
        const Component& serverinfo,
        Component& hostname,
        Component& port_num)
    {
        if (serverinfo._len == 0) {
            // No server info, host name is empty.
            hostname.reset();
            port_num.reset();
            return;
        }

        // If the host starts with a left-bracket, assume the entire host is an
        // IPv6 literal.  Otherwise, assume none of the host is an IPv6 literal.
        // This assumption will be overridden if we find a right-bracket.
        //
        // Our IPv6 address canonicalization code requires both brackets to exist,
        // but the ability to locate an incomplete address can still be useful.
        int ipv6_terminator = spec[serverinfo._begin] == '[' ? serverinfo.end() : -1;
        int colon = -1;

        // Find the last right-bracket, and the last colon.
        for (int i = serverinfo._begin; i < serverinfo.end(); i++) {
            switch (spec[i]) {
            case ']':
                ipv6_terminator = i;
                break;
            case ':':
                colon = i;
                break;
            }
        }

        if (colon > ipv6_terminator) {
            // Found a port number: <hostname>:<port>
            hostname.SetRange(serverinfo._begin, colon);
            if (hostname._len == 0) {
                hostname.reset();
            }
            port_num.SetRange(colon + 1, serverinfo.end());
        }
        else {
            // No port: <hostname>
            hostname = serverinfo;
            port_num.reset();
        }
    }

    // Given an already-identified auth section, breaks it into its consituent
    // parts. The port number will be parsed and the resulting integer will be
    // filled into the given *port variable, or -1 if there is no port number or it
    // is invalid.
    template <typename CHAR>
    void DoParseAuthority(const CHAR* spec,
        const Component& auth,
        Component& username,
        Component& password,
        Component& hostname,
        Component& port_num)
    {
        ASSERT(auth.is_valid());
        if (auth._len == 0) {
            username.reset();
            password.reset();
            hostname.reset();
            port_num.reset();
            return;
        }

        // Search backwards for @, which is the separator between the user info and
        // the server info.
        int i = auth._begin + auth._len - 1;
        while (i > auth._begin && spec[i] != '@') {
            i--;
        }

        if (spec[i] == '@') {
            // Found user info: <user-info>@<server-info>
            ParseUserInfo(spec, Component(auth._begin, i - auth._begin),
                username, password);
            Component tempItem;
            tempItem.SetRange(i + 1, auth._begin + auth._len);
            ParseServerInfo(spec, tempItem, hostname, port_num);
        }
        else {
            // No user info, everything is server info.
            username.reset();
            password.reset();
            ParseServerInfo(spec, auth, hostname, port_num);
        }
    }

    template <typename CHAR>
    void ParsePath(const CHAR* spec,
        const Component& path,
        Component& filepath,
        Component& query,
        Component& ref)
    {
        // path = [/]<segment1>/<segment2>/<...>/<segmentN>;<param>?<query>#<ref>

        // Special case when there is no path.
        if (path._len == -1) {
            filepath.reset();
            query.reset();
            ref.reset();
            return;
        }
        ASSERT(path._len > 0);

        // Search for first occurrence of either ? or #.
        int path_end = path._begin + path._len;

        int query_separator = -1; // Index of the '?'
        int ref_separator = -1; // Index of the '#'
        for (int i = path._begin; i < path_end; i++) {
            switch (spec[i]) {
            case '?':
                // Only match the query string if it precedes the reference fragment
                // and when we haven't found one already.
                if (ref_separator < 0 && query_separator < 0) {
                    query_separator = i;
                }
                break;
            case '#':
                // Record the first # sign only.
                if (ref_separator < 0) {
                    ref_separator = i;
                }
                break;
            }
        }

        // Markers pointing to the character after each of these corresponding
        // components. The code below words from the end back to the beginning,
        // and will update these indices as it finds components that exist.
        int file_end, query_end;

        // Ref fragment: from the # to the end of the path.
        if (ref_separator >= 0) {
            file_end = query_end = ref_separator;
            ref.SetRange(ref_separator + 1, path_end);
        }
        else {
            file_end = query_end = path_end;
            ref.reset();
        }

        // Query fragment: everything from the ? to the next boundary (either the end
        // of the path or the ref fragment).
        if (query_separator >= 0) {
            file_end = query_separator;
            query.SetRange(query_separator + 1, query_end);
        }
        else {
            query.reset();
        }

        // File path: treat an empty file path as no file path.
        if (file_end != path._begin) {
            filepath.SetRange(path._begin, file_end);
        }
        else {
            filepath.reset();
        }
    }

    template <typename CHAR>
    bool DoExtractScheme(const CHAR* url,
        int url_len,
        Component& scheme)
    {
        // Skip leading whitespace and control characters.
        int begin = 0;
        while (begin < url_len && ShouldTrimFromURL(url[begin])) {
            begin++;
        }
        if (begin == url_len) {
            return false; // Input is empty or all whitespace.
        }

        // Find the first colon character.
        for (int i = begin; i < url_len; i++) {
            if (url[i] == ':') {
                scheme.SetRange(begin, i);
                return true;
            }
        }
        return false; // No colon found: no scheme
    }

    // The main parsing function for standard URLs. Standard URLs have a scheme,
    // host, path, etc.
    template <typename CHAR>
    void DoParseStandardURL(const CHAR* spec, int spec_len, Parsed& parsed)
    {
        ASSERT(spec_len >= 0);

        // Strip leading & trailing spaces and control characters.
        int begin = 0;
        TrimURL(spec, &begin, &spec_len);

        int after_scheme;
        if (DoExtractScheme(spec, spec_len, parsed.scheme)) {
            after_scheme = parsed.scheme.end() + 1; // Skip past the colon.
        }
        else {
            // Say there's no scheme when there is a colon. We could also say that
            // everything is the scheme. Both would produce an invalid URL, but this way
            // seems less wrong in more cases.
            parsed.scheme.reset();
            after_scheme = begin;
        }
        DoParseAfterScheme(spec, spec_len, after_scheme, parsed);
    }

    // Initializes a path URL which is merely a scheme followed by a path. Examples
    // include "about:foo" and "javascript:alert('bar');"
    template <typename CHAR>
    void DoParsePathURL(const CHAR* spec, int spec_len, Parsed& parsed)
    {
        // Get the non-path and non-scheme parts of the URL out of the way, we never
        // use them.
        parsed.username.reset();
        parsed.password.reset();
        parsed.host.reset();
        parsed.port.reset();
        parsed.query.reset();
        parsed.ref.reset();

        // Strip leading & trailing spaces and control characters.
        int begin = 0;
        TrimURL(spec, &begin, &spec_len);

        // Handle empty specs or ones that contain only whitespace or control chars.
        if (begin == spec_len) {
            parsed.scheme.reset();
            parsed.path.reset();
            return;
        }

        // Extract the scheme, with the path being everything following. We also
        // handle the case where there is no scheme.
        if (ExtractScheme(&spec[begin], spec_len - begin, parsed.scheme)) {
            // Offset the results since we gave ExtractScheme a substring.
            parsed.scheme._begin += begin;

            // For compatability with the standard URL parser, we treat no path as
            // -1, rather than having a length of 0 (we normally wouldn't care so
            // much for these non-standard URLs).
            if (parsed.scheme.end() == spec_len - 1) {
                parsed.path.reset();
            }
            else {
                parsed.path.SetRange(parsed.scheme.end() + 1, spec_len);
            }
        }
        else {
            // No scheme found, just path.
            parsed.scheme.reset();
            parsed.path.SetRange(begin, spec_len);
        }
    }

    template <typename CHAR>
    void DoParseMailtoURL(const CHAR spec[], int spec_len, Parsed& parsed)
    {
        ASSERT(spec_len >= 0);

        // Get the non-path and non-scheme parts of the URL out of the way, we never
        // use them.
        parsed.username.reset();
        parsed.password.reset();
        parsed.host.reset();
        parsed.port.reset();
        parsed.ref.reset();
        parsed.query.reset(); // May use this; reset for convenience.

        // Strip leading & trailing spaces and control characters.
        int begin = 0;
        TrimURL(spec, &begin, &spec_len);

        // Handle empty specs or ones that contain only whitespace or control chars.
        if (begin == spec_len) {
            parsed.scheme.reset();
            parsed.path.reset();
            return;
        }

        int path_begin = -1;
        int path_end = -1;

        // Extract the scheme, with the path being everything following. We also
        // handle the case where there is no scheme.
        if (ExtractScheme(&spec[begin], spec_len - begin, parsed.scheme)) {
            // Offset the results since we gave ExtractScheme a substring.
            parsed.scheme._begin += begin;

            if (parsed.scheme.end() != spec_len - 1) {
                path_begin = parsed.scheme.end() + 1;
                path_end = spec_len;
            }
        }
        else {
            // No scheme found, just path.
            parsed.scheme.reset();
            path_begin = begin;
            path_end = spec_len;
        }

        // Split [path_begin, path_end) into a path + query.
        for (int i = path_begin; i < path_end; ++i) {
            if (spec[i] == '?') {
                parsed.query.SetRange(i + 1, path_end);
                path_end = i;
                break;
            }
        }

        // For compatability with the standard URL parser, treat no path as
        // -1, rather than having a length of 0
        if (path_begin == path_end) {
            parsed.path.reset();
        }
        else {
            parsed.path.SetRange(path_begin, path_end);
        }
    }

    // Converts a port number in a string to an integer. We'd like to just call
    // sscanf but our input is not nullptr-terminated, which sscanf requires. Instead,
    // we copy the digits to a small stack buffer (since we know the maximum number
    // of digits in a valid port number) that we can nullptr terminate.
    template <typename CHAR>
    int DoParsePort(const CHAR spec[], const Component& component)
    {
        // Easy success case when there is no port.
        const int kMaxDigits = 5;
        if (!component.is_nonempty()) {
            return PORT_UNSPECIFIED;
        }

        // Skip over any leading 0s.
        Component digits_comp(component.end(), 0);
        for (int i = 0; i < component._len; i++) {
            if (spec[component._begin + i] != '0') {
                digits_comp.SetRange(component._begin + i, component.end());
                break;
            }
        }
        if (digits_comp._len == 0) {
            return 0; // All digits were 0.
        }

        // Verify we don't have too many digits (we'll be copying to our buffer so
        // we need to double-check).
        if (digits_comp._len > kMaxDigits) {
            return PORT_INVALID;
        }

        // Copy valid digits to the buffer.
        char digits[kMaxDigits + 1]; // +1 for null terminator
        for (int i = 0; i < digits_comp._len; i++) {
            CHAR ch = spec[digits_comp._begin + i];
            if (!IsPortDigit(ch)) {
                // Invalid port digit, fail.
                return PORT_INVALID;
            }
            digits[i] = static_cast<char>(ch);
        }

        // Null-terminate the string and convert to integer. Since we guarantee
        // only digits, atoi's lack of error handling is OK.
        digits[digits_comp._len] = 0;
        int port = atoi(digits);
        if (port > 65535) {
            return PORT_INVALID; // Out of range.
        }
        return port;
    }

    template <typename CHAR>
    void DoExtractFileName(const CHAR spec[],
        const Component& path,
        Component& file_name)
    {
        // Handle empty paths: they have no file names.
        if (!path.is_nonempty()) {
            file_name.reset();
            return;
        }

        // Search backwards for a parameter, which is a normally unused field in a
        // URL delimited by a semicolon. We parse the parameter as part of the
        // path, but here, we don't want to count it. The last semicolon is the
        // parameter. The path should start with a slash, so we don't need to check
        // the first one.
        int file_end = path.end();
        for (int i = path.end() - 1; i > path.begin(); i--) {
            if (spec[i] == ';') {
                file_end = i;
                break;
            }
        }

        // Now search backwards from the filename end to the previous slash
        // to find the beginning of the filename.
        for (int i = file_end - 1; i >= path.begin(); i--) {
            if (IsURLSlash(spec[i])) {
                // File name is everything following this character to the end
                file_name.SetRange(i + 1, file_end);
                return;
            }
        }

        // No slash found, this means the input was degenerate (generally paths
        // will start with a slash). Let's call everything the file name.
        file_name.SetRange(path.begin(), file_end);
        return;
    }

    template <typename CHAR>
    bool DoExtractQueryKeyValue(const CHAR spec[],
        Component& query,
        Component& key,
        Component& value)
    {
        if (!query.is_nonempty()) {
            return false;
        }

        int start = query.begin();
        int cur = start;
        int end = query.end();

        // We assume the beginning of the input is the beginning of the "key" and we
        // skip to the end of it.
        key._begin = cur;
        while (cur < end && spec[cur] != '&' && spec[cur] != '=') {
            cur++;
        }
        key._len = cur - key.begin();

        // Skip the separator after the key (if any).
        if (cur < end && spec[cur] == '=') {
            cur++;
        }

        // Find the value part.
        value._begin = cur;
        while (cur < end && spec[cur] != '&') {
            cur++;
        }
        value._len = cur - value.begin();

        // Finally skip the next separator if any
        if (cur < end && spec[cur] == '&') {
            cur++;
        }

        // Save the new query
        query.SetRange(cur, end);
        return true;
    }

    int Parsed::Length() const
    {
        if (ref.is_valid()) {
            return ref.end();
        }
        return CountCharactersBefore(REF, false);
    }

    int Parsed::CountCharactersBefore(ComponentType type,
        bool include_delimiter) const
    {
        if (type == SCHEME) {
            return scheme.begin();
        }

        // There will be some characters after the scheme like "://" and we don't
        // know how many. Search forwards for the next thing until we find one.
        int cur = 0;
        if (scheme.is_valid()) {
            cur = scheme.end() + 1; // Advance over the ':' at the end of the scheme.
        }

        if (username.is_valid()) {
            if (type <= USERNAME) {
                return username.begin();
            }
            cur = username.end() + 1; // Advance over the '@' or ':' at the end.
        }

        if (password.is_valid()) {
            if (type <= PASSWORD) {
                return password.begin();
            }
            cur = password.end() + 1; // Advance over the '@' at the end.
        }

        if (host.is_valid()) {
            if (type <= HOST) {
                return host.begin();
            }
            cur = host.end();
        }

        if (port.is_valid()) {
            if (type < PORT || (type == PORT && include_delimiter)) {
                return port.begin() - 1; // Back over delimiter.
            }
            if (type == PORT) {
                return port.begin(); // Don't want delimiter counted.
            }
            cur = port.end();
        }

        if (path.is_valid()) {
            if (type <= PATH) {
                return path.begin();
            }
            cur = path.end();
        }

        if (query.is_valid()) {
            if (type < QUERY || (type == QUERY && include_delimiter)) {
                return query.begin() - 1; // Back over delimiter.
            }
            if (type == QUERY) {
                return query.begin(); // Don't want delimiter counted.
            }
            cur = query.end();
        }

        if (ref.is_valid()) {
            if (type == REF && !include_delimiter) {
                return ref.begin(); // Back over delimiter.
            }

            // When there is a ref and we get here, the component we wanted was before
            // this and not found, so we always know the beginning of the ref is right.
            return ref.begin() - 1; // Don't want delimiter counted.
        }

        return cur;
    }

    bool ExtractScheme(const TCHAR url[], int url_len, Component& scheme)
    {
        return DoExtractScheme(url, url_len, scheme);
    }

    // This handles everything that may be an authority terminator, including
    // backslash. For special backslash handling see DoParseAfterScheme.
    bool IsAuthorityTerminator(TCHAR ch)
    {
        return IsURLSlash(ch) || ch == '?' || ch == '#' || ch == ';';
    }

    void ExtractFileName(const TCHAR url[],
        const Component& path,
        Component& file_name)
    {
        DoExtractFileName(url, path, file_name);
    }

    bool ExtractQueryKeyValue(const TCHAR url[],
        Component& query,
        Component& key,
        Component& value)
    {
        return DoExtractQueryKeyValue(url, query, key, value);
    }

    void ParseAuthority(const TCHAR spec[],
        const Component& auth,
        Component& username,
        Component& password,
        Component& hostname,
        Component& port_num)
    {
        DoParseAuthority(spec, auth, username, password, hostname, port_num);
    }

    int ParsePort(const TCHAR url[], const Component& port)
    {
        return DoParsePort(url, port);
    }

    void ParsePathInternal(const TCHAR spec[],
        const Component& path,
        Component& filepath,
        Component& query,
        Component& ref)
    {
        ParsePath(spec, path, filepath, query, ref);
    }

    void ParseAfterScheme(const TCHAR spec[],
        int spec_len,
        int after_scheme,
        Parsed& parsed)
    {
        DoParseAfterScheme(spec, spec_len, after_scheme, parsed);
    }

    // Interesting IE file:isms...
    //
    //  INPUT                      OUTPUT
    //  =========================  ==============================
    //  file:/foo/bar              file:///foo/bar
    //      The result here seems totally invalid!?!? This isn't UNC.
    //
    //  file:/
    //  file:// or any other number of slashes
    //      IE6 doesn't do anything at all if you click on this link. No error:
    //      nothing. IE6's history system seems to always color this link, so I'm
    //      guessing that it maps internally to the empty URL.
    //
    //  C:\                        file:///C:/
    //      When on a file: URL source page, this link will work. When over HTTP,
    //      the file: URL will appear in the status bar but the link will not work
    //      (security restriction for all file URLs).
    //
    //  file:foo/                  file:foo/     (invalid?!?!?)
    //  file:/foo/                 file:///foo/  (invalid?!?!?)
    //  file://foo/                file://foo/   (UNC to server "foo")
    //  file:///foo/               file:///foo/  (invalid, seems to be a file)
    //  file:////foo/              file://foo/   (UNC to server "foo")
    //      Any more than four slashes is also treated as UNC.
    //
    //  file:C:/                   file://C:/
    //  file:/C:/                  file://C:/
    //      The number of slashes after "file:" don't matter if the thing following
    //      it looks like an absolute drive path. Also, slashes and backslashes are
    //      equally valid here.

    // A subcomponent of DoInitFileURL, the input of this function should be a UNC
    // path name, with the index of the first character after the slashes following
    // the scheme given in |after_slashes|. This will initialize the host, path,
    // query, and ref, and leave the other output components untouched
    // (DoInitFileURL handles these for us).
    template <typename CHAR>
    void DoParseUNC(const CHAR* spec,
        int after_slashes,
        int spec_len,
        Parsed& parsed)
    {
        int next_slash = FindNextSlash(spec, after_slashes, spec_len);
        if (next_slash == spec_len) {
            // No additional slash found, as in "file://foo", treat the text as the
            // host with no path (this will end up being UNC to server "foo").
            int host_len = spec_len - after_slashes;
            if (host_len) {
                parsed.host = Component(after_slashes, host_len);
            }
            else {
                parsed.host.reset();
            }
            parsed.path.reset();
            return;
        }

#ifdef WIN32
        // See if we have something that looks like a path following the first
        // component. As in "file://localhost/c:/", we get "c:/" out. We want to
        // treat this as a having no host but the path given. Works on Windows only.
        if (DoesBeginWindowsDriveSpec(spec, next_slash + 1, spec_len)) {
            parsed.host.reset();
            Component tempItem;
            tempItem.SetRange(next_slash, spec_len);
            ParsePathInternal(spec, tempItem, parsed.path, parsed.query, parsed.ref);
            return;
        }
#endif

        // Otherwise, everything up until that first slash we found is the host name,
        // which will end up being the UNC host. For example "file://foo/bar.txt"
        // will get a server name of "foo" and a path of "/bar". Later, on Windows,
        // this should be treated as the filename "\\foo\bar.txt" in proper UNC
        // notation.
        int host_len = next_slash - after_slashes;
        if (host_len) {
            parsed.host.SetRange(after_slashes, next_slash);
        }
        else {
            parsed.host.reset();
        }
        if (next_slash < spec_len) {
            Component tempItem;
            tempItem.SetRange(next_slash, spec_len);
            ParsePathInternal(spec, tempItem,
                parsed.path, parsed.query, parsed.ref);
        }
        else {
            parsed.path.reset();
        }
    }

    // A subcomponent of DoParseFileURL, the input should be a local file, with the
    // beginning of the path indicated by the index in |path_begin|. This will
    // initialize the host, path, query, and ref, and leave the other output
    // components untouched (DoInitFileURL handles these for us).
    template <typename CHAR>
    void DoParseLocalFile(const CHAR* spec,
        int path_begin,
        int spec_len,
        Parsed& parsed)
    {
        parsed.host.reset();
        Component tempItem;
        tempItem.SetRange(path_begin, spec_len);
        ParsePathInternal(spec, tempItem,
            parsed.path, parsed.query, parsed.ref);
    }

    // Backend for the external functions that operates on either char type.
    // We are handed the character after the "file:" at the beginning of the spec.
    // Usually this is a slash, but needn't be; we allow paths like "file:c:\foo".
    template <typename CHAR>
    void DoParseFileURL(const CHAR* spec, int spec_len, Parsed& parsed)
    {
        ASSERT(spec_len >= 0);

        // Get the parts we never use for file URLs out of the way.
        parsed.username.reset();
        parsed.password.reset();
        parsed.port.reset();

        // Many of the code paths don't set these, so it's convenient to just clear
        // them. We'll write them in those cases we need them.
        parsed.query.reset();
        parsed.ref.reset();

        // Strip leading & trailing spaces and control characters.
        int begin = 0;
        TrimURL(spec, &begin, &spec_len);

        // Find the scheme.
        int num_slashes;
        int after_scheme;
        int after_slashes;
#ifdef WIN32
        // See how many slashes there are. We want to handle cases like UNC but also
        // "/c:/foo". This is when there is no scheme, so we can allow pages to do
        // links like "c:/foo/bar" or "//foo/bar". This is also called by the
        // relative URL resolver when it determines there is an absolute URL, which
        // may give us input like "/c:/foo".
        num_slashes = CountConsecutiveSlashes(spec, begin, spec_len);
        after_slashes = begin + num_slashes;
        if (DoesBeginWindowsDriveSpec(spec, after_slashes, spec_len)) {
            // Windows path, don't try to extract the scheme (for example, "c:\foo").
            parsed.scheme.reset();
            after_scheme = after_slashes;
        }
        else if (DoesBeginUNCPath(spec, begin, spec_len, false)) {
            // Windows UNC path: don't try to extract the scheme, but keep the slashes.
            parsed.scheme.reset();
            after_scheme = begin;
        }
        else
#endif
        {
            if (ExtractScheme(&spec[begin], spec_len - begin, parsed.scheme)) {
                // Offset the results since we gave ExtractScheme a substring.
                parsed.scheme._begin += begin;
                after_scheme = parsed.scheme.end() + 1;
            }
            else {
                // No scheme found, remember that.
                parsed.scheme.reset();
                after_scheme = begin;
            }
        }

        // Handle empty specs ones that contain only whitespace or control chars,
        // or that are just the scheme (for example "file:").
        if (after_scheme == spec_len) {
            parsed.host.reset();
            parsed.path.reset();
            return;
        }

        num_slashes = CountConsecutiveSlashes(spec, after_scheme, spec_len);

        after_slashes = after_scheme + num_slashes;
#ifdef WIN32
        // Check whether the input is a drive again. We checked above for windows
        // drive specs, but that's only at the very beginning to see if we have a
        // scheme at all. This test will be duplicated in that case, but will
        // additionally handle all cases with a real scheme such as "file:///C:/".
        if (!DoesBeginWindowsDriveSpec(spec, after_slashes, spec_len) && num_slashes != 3) {
            // Anything not beginning with a drive spec ("c:\") on Windows is treated
            // as UNC, with the exception of three slashes which always means a file.
            // Even IE7 treats file:///foo/bar as "/foo/bar", which then fails.
            DoParseUNC(spec, after_slashes, spec_len, parsed);
            return;
        }
#else
        // file: URL with exactly 2 slashes is considered to have a host component.
        if (num_slashes == 2) {
            DoParseUNC(spec, after_slashes, spec_len, parsed);
            return;
        }
#endif // WIN32

        // Easy and common case, the full path immediately follows the scheme
        // (modulo slashes), as in "file://c:/foo". Just treat everything from
        // there to the end as the path. Empty hosts have 0 length instead of -1.
        // We include the last slash as part of the path if there is one.
        DoParseLocalFile(spec,
            num_slashes > 0 ? after_scheme + num_slashes - 1 : after_scheme,
            spec_len, parsed);
    }

    void ParseStandardURL(const TCHAR url[], int url_len, Parsed& parsed)
    {
        DoParseStandardURL(url, url_len, parsed);
    }

    void ParsePathURL(const TCHAR url[], int url_len, Parsed& parsed)
    {
        DoParsePathURL(url, url_len, parsed);
    }

    void ParseMailtoURL(const TCHAR url[], int url_len, Parsed& parsed)
    {
        DoParseMailtoURL(url, url_len, parsed);
    }

    void ParseFileURL(const TCHAR url[], int url_len, Parsed& parsed)
    {
        DoParseFileURL(url, url_len, parsed);
    }

    // StandardURL is for when the scheme is known to be one that has an
    // authority (host) like "http". This function will not handle weird ones
    // like "about:" and "javascript:", or do the right thing for "file:" URLs.
    // void ParseStandardURL(const TCHAR url[], int url_len, Parsed& parsed);

    // PathURL is for when the scheme is known not to have an authority (host)
    // section but that aren't file URLs either. The scheme is parsed, and
    // everything after the scheme is considered as the path. This is used for
    // things like "about:" and "javascript:"
    // void ParsePathURL(const TCHAR url[], int url_len, Parsed& parsed);

    // FileURL is for file URLs. There are some special rules for interpreting
    // these.
    // void ParseFileURL(const TCHAR url[], int url_len, Parsed& parsed);

    // MailtoURL is for mailto: urls. They are made up scheme,path,query
    // void ParseMailtoURL(const TCHAR url[], int url_len, Parsed& parsed);

    typedef void (*ParseFunction)(const TCHAR[], int, Parsed&);
    typedef void (*CreateFunction)(TCHAR[], int, const URL& info);

    typedef struct {
        URL::SchemeType m_Scheme;
        const TCHAR* m_Schemestring;
        unsigned short m_SchemeLength;
        unsigned short m_Port;
        ParseFunction m_ParseFunction;
        CreateFunction m_CreateFunction;

    } SchemeInfo;

    const SchemeInfo g_SchemeOverview[] = {
        { URL::SCHEME_FILE, _TXT("file"), 0, ParseFileURL, URL::CreateFileURL },
        { URL::SCHEMA_MAIL, _TXT("mailto"), 0, ParseMailtoURL, URL::CreateMailtoURL },
        { URL::SCHEME_HTTP, _TXT("http"), 80, ParseStandardURL, URL::CreateStandardURL },
        { URL::SCHEME_HTTPS, _TXT("https"), 443, ParseStandardURL, URL::CreateStandardURL },
        { URL::SCHEME_FTP, _TXT("ftp"), 21, ParseStandardURL, URL::CreateStandardURL },
        { URL::SCHEME_TELNET, _TXT("telnet"), 23, ParseStandardURL, URL::CreateStandardURL },
        { URL::SCHEME_GOPHER, _TXT("gopher"), 70, ParseStandardURL, URL::CreateStandardURL },
        { URL::SCHEME_LDAP, _TXT("ldap"), 389, ParseStandardURL, URL::CreateStandardURL },
        { URL::SCHEME_RTSP, _TXT("rtsp"), 554, ParseStandardURL, URL::CreateStandardURL },
        { URL::SCHEME_RTP, _TXT("rtp"), 554, ParseStandardURL, URL::CreateStandardURL },
        { URL::SCHEME_RTCP, _TXT("rtcp"), 554, ParseStandardURL, URL::CreateStandardURL },
        { URL::SCHEME_RTP_UDP, _TXT("rtpudp"), 554, ParseStandardURL, URL::CreateStandardURL },
        { URL::SCHEME_RTP_TCP, _TXT("rtptcp"), 554, ParseStandardURL, URL::CreateStandardURL },
		{ URL::SCHEME_NTP, _TXT("ntp"), 123, ParseStandardURL, URL::CreateStandardURL },
		{ URL::SCHEME_UNKNOWN, _TXT(""), 0, ParseStandardURL, URL::CreateStandardURL }
    };

            TextFragment URL::Text() const
    {
        TextFragment result;

        if (m_SchemeInfo != nullptr) {
            // Seems like we need to construct it...
            // Create somecharacter space...
            TCHAR* buffer = reinterpret_cast<TCHAR*>(alloca((MAX_URL_SIZE + 1) * sizeof(TCHAR)));

            reinterpret_cast<const SchemeInfo*>(m_SchemeInfo)->m_CreateFunction(buffer, MAX_URL_SIZE, *this);

            result = TextFragment(string(buffer));
        }

        return (result);
    }

    void URL::SetScheme(const URL::SchemeType type)
    {
        uint8_t index = 0;
        m_SchemeInfo = nullptr;

        while ((m_SchemeInfo == nullptr) && (index < (sizeof(g_SchemeOverview) / sizeof(SchemeInfo)))) {
            if (g_SchemeOverview[index].m_Scheme == type) {
                m_SchemeInfo = &(g_SchemeOverview[index]);
                m_Scheme = Core::TextFragment(g_SchemeOverview[index].m_Schemestring, g_SchemeOverview[index].m_SchemeLength);
            }
            else {
                ++index;
            }
        }
    }

    URL::SchemeType URL::Type() const
    {
        SchemeType result = URL::SCHEME_UNKNOWN;

        if (m_SchemeInfo != nullptr) {
            result = reinterpret_cast<const SchemeInfo*>(m_SchemeInfo)->m_Scheme;
        }

        return (result);
    }

    void URL::Parse(const string& urlStr)
    {
        bool caseSensitive = true;
        Component result;
        const TCHAR* byteArray(urlStr.c_str());
        int length = urlStr.length();

        if (ExtractScheme(byteArray, length, result) == true) {
            // SchemeInfo information found! Set it!
            m_Scheme = TextFragment(urlStr, result.begin(), result.length());

            unsigned int index = 0;

            // Find additional info:
            while ((index < (sizeof(g_SchemeOverview) / sizeof(SchemeInfo))) && ((g_SchemeOverview[index].m_SchemeLength != result.length()) || ((caseSensitive == true) && (_tcsncmp(g_SchemeOverview[index].m_Schemestring, m_Scheme.Value().Data(), g_SchemeOverview[index].m_SchemeLength) != 0)) || ((caseSensitive == false) && (_tcsnicmp(g_SchemeOverview[index].m_Schemestring, m_Scheme.Value().Data(), g_SchemeOverview[index].m_SchemeLength) != 0)))) {
                index++;
            }

            Parsed parseInfo;

            if (index == (sizeof(g_SchemeOverview) / sizeof(SchemeInfo))) {
                m_SchemeInfo = nullptr;
                ParseStandardURL(byteArray, length, parseInfo);
            }
            else {
                m_SchemeInfo = &(g_SchemeOverview[index]);
                g_SchemeOverview[index].m_ParseFunction(byteArray, length, parseInfo);
                m_Port = OptionalType<unsigned short>(g_SchemeOverview[index].m_Port);
            }

            // Copy all other components
            SetComponent(urlStr, parseInfo.username.begin(), parseInfo.username.length(), m_Username);
            SetComponent(urlStr, parseInfo.password.begin(), parseInfo.password.length(), m_Password);
            SetComponent(urlStr, parseInfo.host.begin(), parseInfo.host.length(), m_Host);
            if (parseInfo.path.length() > 0) {
                SetComponent(urlStr, parseInfo.path.begin() + 1, parseInfo.path.length() - 1, m_Path);
            }
            SetComponent(urlStr, parseInfo.query.begin(), parseInfo.query.length(), m_Query);
            SetComponent(urlStr, parseInfo.ref.begin(), parseInfo.ref.length(), m_Ref);

            if (parseInfo.port.is_valid()) {
                Core::Unsigned16 portNumber(&(urlStr[parseInfo.port.begin()]), parseInfo.port.end() - parseInfo.port.begin());
                m_Port = portNumber.Value();
            }
        }
    }

    void URL::SetComponent(const string& urlStr, const unsigned int begin, const unsigned int length, OptionalType<TextFragment>& setInfo)
    {
        if (length != static_cast<unsigned int>(~0)) {
            setInfo = TextFragment(urlStr, begin, length);
        }
    }

    static inline uint32_t CopyFragment(TCHAR* destination, const int maxLength, int index, const TextFragment& data)
    {
        const int count = (data.Length() > static_cast<unsigned int>(maxLength) ? maxLength : data.Length());

        ::memcpy(&(destination[index]), data.Data(), (count * sizeof(TCHAR)));

        return (index + count);
    }

    // StandardURL is for when the scheme is known to be one that has an
    // authority (host) like "http". This function will not handle weird ones
    // like "about:" and "javascript:", or do the right thing for "file:" URLs.
    void URL::CreateStandardURL(TCHAR url[], int url_len) const
    {
        if (IsValid()) {
            int index = CopyFragment(url, url_len, 0, TextFragment(Core::EnumerateType<URL::SchemeType>(Type()).Data()));

            index = CopyFragment(url, url_len, index, TextFragment(_T("://")));

            if (m_Username.IsSet()) {
                index = CopyFragment(url, url_len, index, m_Username.Value());

                if ((m_Password.IsSet()) && (index < url_len)) {
                    url[index] = ':';
                    index = CopyFragment(url, url_len, index + 1, m_Password.Value());
                }

                url[index] = '@';
                index += 1;
            }

            if (m_Host.IsSet()) {
                index = CopyFragment(url, url_len, index, m_Host.Value());

                if ((m_Port.IsSet()) && (index < url_len)) {
                    url[index] = ':';
                    index = CopyFragment(url, url_len, index + 1, TextFragment(Core::Unsigned16(m_Port.Value()).Text().c_str()));
                }
            }

            url[index] = '/';
            index += 1;

            if (m_Path.IsSet()) {
                index = CopyFragment(url, url_len, index, m_Path.Value());
            }

            if ((m_Query.IsSet()) && (index < url_len)) {
                url[index] = '?';
                index = CopyFragment(url, url_len, index + 1, m_Query.Value());
            }
            if ((m_Ref.IsSet()) && (index < url_len)) {
                url[index] = '#';
                index = CopyFragment(url, url_len, index + 1, m_Ref.Value());
            }

            if (index < url_len) {
                url[index] = '\0';
            }
        }
    }

    // PathURL is for when the scheme is known not to have an authority (host)
    // section but that aren't file URLs either. The scheme is parsed, and
    // everything after the scheme is considered as the path. This is used for
    // things like "about:" and "javascript:"
    void URL::CreatePathURL(TCHAR[] /* url */, int /* url_len*/) const
    {
        ASSERT(TRUE);
    }

    // FileURL is for file URLs. There are some special rules for interpreting
    // these.
    void URL::CreateFileURL(TCHAR url[], int url_len) const
    {
        if (IsValid()) {
            int index = CopyFragment(url, url_len, 0, TextFragment(Core::EnumerateType<URL::SchemeType>(Type()).Data()));

            index = CopyFragment(url, url_len, index, TextFragment(_T("://")));

            if (m_Host.IsSet()) {
                index = CopyFragment(url, url_len, index, m_Host.Value());
            }

            if (index < url_len) {
                url[index++] = '/';
            }

            if (m_Path.IsSet()) {
                index = CopyFragment(url, url_len, index, m_Path.Value());
            }

            if (index < url_len) {
                url[index] = '\0';
            }
        }
    }

    // MailtoURL is for mailto: urls. They are made up scheme,path,query
    void URL::CreateMailtoURL(TCHAR[] /* url */, int /* url_len*/) const
    {
        ASSERT(TRUE);
    }

    /* Returns a url-encoded version of source */
    /* static */ uint16_t URL::Encode(const TCHAR* source, const uint16_t sourceLength, TCHAR* destination, const uint16_t destinationLength)
    {
        static char hex[] = "0123456789abcdef";
        uint16_t srcLength = sourceLength;
        uint16_t dstLength = destinationLength;

        while ((*source != '\0') && (srcLength != 0) && (dstLength >= 3)) {
            TCHAR current = *source++;

            if ((isalnum(current) != 0) || (current == '-') || (current == '_') || (current == '.') || (current == '~')) {
                *destination++ = current;
                dstLength--;
            }
            else if (current == ' ') {
                *destination++ = '+';
                dstLength--;
            }
            else {
                *destination++ = '%';
                *destination++ = hex[(current >> 4) & 0x0F];
                *destination++ = hex[(current & 0x0F)];
                dstLength -= 3;
            }

            srcLength--;
        }

        if (dstLength != 0) {
            *destination = '\0';
        }

        return (destinationLength - dstLength);
    }

    /* Returns a url-decoded version of source */
    /* static */ uint16_t URL::Decode(const TCHAR* source, const uint16_t sourceLength, TCHAR* destination, const uint16_t destinationLength)
    {
        uint16_t srcLength = sourceLength;
        uint16_t dstLength = destinationLength;

        while ((*source != '\0') && (srcLength != 0) && (dstLength != 0)) {
            TCHAR current = *source++;

            if (current == '%') {
                if ((source[0] != '\0') && (source[1] != '\0')) {
                    *destination++ = (((isdigit(source[0]) ? (source[0] - '0') : (tolower(source[0]) - 'a' + 10)) & 0x0F) << 4) | ((isdigit(source[1]) ? (source[1] - '0') : (tolower(source[1]) - 'a' + 10)) & 0x0F);
                    source += 2;
                    srcLength -= 3;
                }
            }
            else if (current == '+') {
                *destination++ = ' ';
                srcLength--;
            }
            else {
                *destination++ = current;
                srcLength--;
            }

            dstLength--;
        }

        if (dstLength != 0) {
            *destination = '\0';
        }

        return (destinationLength - dstLength);
    }
}
} // namespace Core
