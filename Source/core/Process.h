 /*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 
#pragma once

/// https://msdn.microsoft.com/nl-nl/library/windows/desktop/ms682499(v=vs.85).aspx

#include "Module.h"
#include "Portability.h"

namespace Thunder {
namespace Core {

    class Process {
    public:
        class Options {
        private:
            Options() = delete;
            Options& operator=(const Options&) = delete;            
        public:
            typedef Core::IteratorType<const std::vector<string>, const string&, std::vector<string>::const_iterator> Iterator;

        public:
            Options(const string& command)
                : _command(command)
                , _options()
            {
                ASSERT(command.empty() == false);
            }
            Options(const string& command, const Iterator& options)
                : _command(command)
                , _options()
            {
                ASSERT(command.empty() == false);
                Add(options);
            }
            Options(const Options& copy)
                : _command(copy._command)
                , _options(copy._options)
            {
            }
            Options(Options&& move)
                : _command(std::move(move._command))
                , _options(std::move(move._options))
            {
            }
            ~Options()
            {
            }

        public:
            inline const string& Command() const
            {
                return (_command);
            }
            inline void Erase(const string& value)
            {
                std::vector<string>::iterator index(std::find(_options.begin(), _options.end(), value));
                if (index != _options.end()) {
                    _options.erase(index);
                }
            }
            inline void Clear()
            {
                _options.clear();
            }
            Options& Add(const string& parameter)
            {
                _options.push_back(parameter);

                return *this;
            }

            Options& Add(const Iterator& options)
            {
                Iterator index(options);

                _options.clear();

                while (index.Next() == true) {
                    _options.push_back(index.Current());
                }

                return *this;
            }
            Iterator Get() const
            {
                return (Iterator(_options));
            }
            uint16_t LineSize() const
            {
                uint16_t size = (static_cast<uint16_t>(_command.length()));
                Iterator index(_options);

                // First option needs to be the application start name
                while (index.Next() == true) {
                    // Add length of string plus 1 for '\0'
                    size += 1 + static_cast<uint16_t>(index.Current().length());
                }
                size++; // closing '\0'

                return (size);
            }
            uint16_t BlockSize() const
            {
                // Time to build the argument array !!!
                uint16_t argCount = 2; // at least a command and the closing nullptr;
                uint16_t size = static_cast<uint16_t>(_command.length()) + 1;
                Iterator index(_options);

                // First option needs to be the application start name
                while (index.Next() == true) {

                    argCount++;

                    // count for length of parameter + 1 for ending '\0'
                    size += static_cast<uint16_t>(index.Current().length()) + 1;
                }

                return ((sizeof(char*) * argCount) + (size * sizeof(char)));
            }
            uint16_t Line(void* result, const uint16_t length) const
            {
                uint16_t commandCount(1);
                uint16_t data(length);
                char* destination(reinterpret_cast<char*>(result));

                ASSERT(result != nullptr);

                if (data > _command.length()) {
                    ::strncpy(destination, _command.c_str(), _command.length());
                    data -= static_cast<uint16_t>(_command.length());
                    destination = &destination[_command.length()];
                }
                Iterator index(_options);

                // First option needs to be the application start name
                while ((data > 2) && (index.Next() == true)) {
                    commandCount++;
                    *destination++ = ' ';
                    data -= 1;

                    ::strncpy(destination, index.Current().c_str(), data);
                    destination = &destination[index.Current().length()];
                    data -= static_cast<uint16_t>(index.Current().length() > data ? data : index.Current().length());
                }

                if (data > 0) {
                    *destination = '\0';
                }

                return (commandCount);
            }
            uint16_t Block(void* result, const uint16_t length) const
            {
                // Time to build the argument array !!!
                uint16_t data(length);
                uint16_t commandCount = 1 + static_cast<uint16_t>(_options.size()) + 1;
                Iterator index(_options);

                ASSERT(result != nullptr);

                if ((result == nullptr) || (data <= (sizeof(char*) * commandCount))) {
                    commandCount = 0;
                } else {
                    // Alright, create and fill the arrays.
                    char** indexer = reinterpret_cast<char**>(result);
                    char* destination = reinterpret_cast<char*>(&(static_cast<uint8_t*>(result)[(sizeof(char*) * commandCount)]));

                    data -= (sizeof(char*) * commandCount);
                    commandCount = 1;

                    // Set the first parameter, the command.
                    *indexer++ = destination;

                    // Copy the commad line in
                    ::strncpy(destination, _command.c_str(), data);
                    destination = &destination[_command.length() + 1];
                    data -= static_cast<uint16_t>((_command.length() + 1) > data ? data : (_command.length() + 1));
                    index.Reset(0);

                    // First option needs to be the application start name
                    while ((data >= 2) && (index.Next() == true)) {
                        commandCount++;
                        *indexer++ = destination;

                        ::strncpy(destination, index.Current().c_str(), data);
                        destination = &destination[index.Current().length() + 1];
                        data -= static_cast<uint16_t>((index.Current().length() + 1) > data ? data : (index.Current().length() + 1));
                    }

                    *indexer = nullptr;
                }

                return (commandCount);
            }

        private:
            const string _command;
            std::vector<string> _options;
        };

    public:
        Process(const Process&) = delete;
        Process& operator=(const Process&) = delete;

        explicit Process(const bool capture, const process_t pid = 0)
            : _argc(0)
            , _parameters(nullptr)
            , _exitCode(static_cast<uint32_t>(~0))
#ifndef __WINDOWS__
            , _stdin(capture ? -1 : 0)
            , _stdout(capture ? -1 : 0)
            , _stderr(capture ? -1 : 0)
            , _PID(pid)
#else
            , _stdin(capture ? reinterpret_cast<HANDLE>(~0) : nullptr)
            , _stdout(capture ? reinterpret_cast<HANDLE>(~0) : nullptr)
            , _stderr(capture ? reinterpret_cast<HANDLE>(~0) : nullptr)
#endif
        {
#ifdef __WINDOWS__
            ::memset(&_info, 0, sizeof(_info));
#endif
        }
        ~Process()
        {
#ifdef __WINDOWS__
            if (_info.hProcess != 0) {
                //// Close process and thread handles.
                CloseHandle(_info.hProcess);
                CloseHandle(_info.hThread);
            }
#endif

            if (_parameters != nullptr) {
                ::free(_parameters);
            }
        }

    public:
        inline process_t Id() const
        {
#ifdef __WINDOWS__
            return (_info.dwProcessId);
#else
            return (_PID);
#endif
        }

        inline bool IsActive() const
        {
#ifdef __WINDOWS__
            DWORD exitCode = _exitCode;
            if ((_info.hProcess != 0) && (_exitCode == static_cast<uint32_t>(~0)) && (GetExitCodeProcess(_info.hProcess, &exitCode) != 0) && (exitCode == STILL_ACTIVE)) {
                return (true);
            } else {
                _exitCode = static_cast<uint32_t>(exitCode);
            }
#else
            if ((_PID == 0) || (_exitCode != static_cast<uint32_t>(~0))) {
                return (false);
            } else if (::kill(_PID, 0) == 0) {
                int status;
                int result = waitpid(_PID, &status, WNOHANG);

                if (result == static_cast<int>(_PID)) {
                    if (WIFEXITED(status) == true) {
                        _exitCode = (WEXITSTATUS(status) & 0xFF);
                    } else {
                        // Turn on highest bit to signal a SIGKILL was received.
                        _exitCode = 0x80000000;
                    }
                }

                return (_exitCode == static_cast<uint32_t>(~0));
            }
#endif

            return (false);
        }
        inline bool HasConnector() const
        {
#ifdef __WINDOWS__
            return ((_stdin != reinterpret_cast<HANDLE>(~0)) && (_stdin != nullptr));
#else
            return ((_stdin != -1) && (_stdin != 0));
#endif
        }
        inline HANDLE Input() const
        {
            return (_stdin);
        }
        inline HANDLE Output() const
        {
            return (_stdout);
        }
        inline HANDLE Error() const
        {
            return (_stderr);
        }
        inline bool IsKilled() const
        {
            return ((_exitCode != static_cast<uint32_t>(~0)) && ((_exitCode & 0x80000000) != 0));
        }
        inline bool IsTerminated() const
        {
            return ((_exitCode != static_cast<uint32_t>(~0)) && ((_exitCode & 0x80000000) == 0));
        }
#ifdef __WINDOWS__
        inline uint16_t Input(const uint8_t data[], const uint16_t length)
        {

            DWORD bytesWritten;

            BOOL success = WriteFile(_stdin, data, length, &bytesWritten, nullptr);

            return (success ? static_cast<uint16_t>(bytesWritten) : 0);
        }
        inline uint16_t Output(uint8_t data[], const uint16_t length) const
        {

            DWORD bytesRead;

            BOOL success = ReadFile(_stdout, data, length, &bytesRead, nullptr);

            return (success ? static_cast<uint16_t>(bytesRead) : 0);
        }
        inline uint16_t Error(uint8_t data[], const uint16_t length) const
        {

            DWORD bytesRead;

            BOOL success = ReadFile(_stdout, data, length, &bytesRead, nullptr);

            return (success ? static_cast<uint16_t>(bytesRead) : 0);
        }
#else
        inline uint16_t Input(const uint8_t data[], const uint16_t length)
        {
            return (static_cast<uint16_t>(write(_stdin, data, length)));
        }
        inline uint16_t Output(uint8_t data[], const uint16_t length) const
        {
            return (static_cast<uint16_t>(read(_stdout, data, length)));
        }
        inline uint16_t Error(uint8_t data[], const uint16_t length) const
        {
            return (static_cast<uint16_t>(read(_stderr, data, length)));
        }
#endif
        uint32_t Launch(const Process::Options& parameters, uint32_t* pid)
        {
            uint32_t error = Core::ERROR_NONE;

            if (IsActive() == false) {
                if (_parameters != nullptr) {
                    // Destruct what we got, we sare starting again!!!
                    ::free(_parameters);
                }

                // If we are "relaunched" make sure we reset the _exitCode.
                _exitCode = static_cast<uint32_t>(~0);

#ifdef __WINDOWS__
                STARTUPINFO si;
                ZeroMemory(&si, sizeof(si));
                si.cb = sizeof(si);
                ZeroMemory(&_info, sizeof(_info));
                BOOL inheritance = FALSE;

                HANDLE stdinfd[2], stdoutfd[2], stderrfd[2];

                stdinfd[0] = nullptr;
                stdinfd[1] = nullptr;
                stdoutfd[0] = nullptr;
                stdoutfd[1] = nullptr;
                stderrfd[0] = nullptr;
                stderrfd[1] = nullptr;

                /* Create the pipe and set non-blocking on the readable end. */
                if (_stdin == reinterpret_cast<HANDLE>(~0)) {
                    SECURITY_ATTRIBUTES saAttr;

                    // Set the bInheritHandle flag so pipe handles are inherited.
                    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
                    saAttr.bInheritHandle = TRUE;
                    saAttr.lpSecurityDescriptor = nullptr;

                    // Create a pipes for the child process.
                    if ((CreatePipe(&stdoutfd[0], &stdoutfd[1], &saAttr, 0) == 0) && (CreatePipe(&stderrfd[0], &stderrfd[1], &saAttr, 0) == 0) && (CreatePipe(&stdinfd[0], &stdinfd[1], &saAttr, 0) == 0) &&
                        // Ensure the write/read handle to the pipe for STDIN/STDOUT,STDERR are not inherited.
                        (SetHandleInformation(stdoutfd[0], HANDLE_FLAG_INHERIT, 0) == 0) && (SetHandleInformation(stderrfd[0], HANDLE_FLAG_INHERIT, 0) == 0) && (SetHandleInformation(stdinfd[1], HANDLE_FLAG_INHERIT, 0) == 0)) {

                        si.hStdError = stderrfd[1];
                        si.hStdOutput = stdoutfd[1];
                        si.hStdInput = stdinfd[0];
                        _stdout = stdoutfd[0];
                        _stderr = stderrfd[0];
                        _stdin = stdinfd[1];
                        si.dwFlags |= STARTF_USESTDHANDLES;
                        inheritance = TRUE;
                    } else {
                        _stdin = 0;
                    }
                }

                uint16_t size = parameters.LineSize();
                _parameters = ::malloc(size);
                _argc = parameters.Line(_parameters, size);

                if (CreateProcess(
                        parameters.Command().c_str(),
                        reinterpret_cast<char*>(_parameters),
                        nullptr,
                        nullptr,
                        inheritance,
                        0, // CREATE_NEW_CONSOLE,
                        nullptr,
                        nullptr,
                        &si,
                        &_info)
                    == 0) {
                    int status = GetLastError();
                    TRACE_L1("Failed to start a process, Error <%d>.", status);
                    error = (status == 2 ? Core::ERROR_UNAVAILABLE : Core::ERROR_GENERAL);
                } else {
                    *pid = _info.dwProcessId;
                }

                if (_stdin == 0) {
                    //* check descriptors if they need to be closed
                    if (stdinfd[0] != nullptr) {
                        CloseHandle(stdinfd[0]);
                        CloseHandle(stdinfd[1]);
                    }
                    if (stdoutfd[0] != nullptr) {
                        CloseHandle(stdoutfd[0]);
                        CloseHandle(stdoutfd[1]);
                    }
                    if (stderrfd[0] != nullptr) {
                        CloseHandle(stderrfd[0]);
                        CloseHandle(stderrfd[1]);
                    }
                }
#endif

#ifdef __LINUX__
                uint16_t size = parameters.BlockSize();
                _parameters = ::malloc(size);
                _argc = parameters.Block(_parameters, size);
                int stdinfd[2], stdoutfd[2], stderrfd[2];

                stdinfd[0] = -1;
                stdinfd[1] = -1;
                stdoutfd[0] = -1;
                stdoutfd[1] = -1;
                stderrfd[0] = -1;
                stderrfd[1] = -1;

                /* Create the pipe and set non-blocking on the readable end. */
                if ((_stdin == -1) && (pipe2(stdinfd, O_CLOEXEC) == 0) && (pipe2(stdoutfd, O_CLOEXEC) == 0) && (pipe2(stderrfd, O_CLOEXEC) == 0)) {
                    // int flags = ( fcntl(p[0], F_GETFL, 0) & (~O_NONBLOCK) );
                    int input = (fcntl(stdinfd[1], F_GETFL, 0) | O_NONBLOCK);
                    int output = (fcntl(stdoutfd[0], F_GETFL, 0) | O_NONBLOCK);

                    if ((fcntl(stdinfd[1], F_SETFL, input) != 0) || (fcntl(stdoutfd[0], F_SETFL, output) != 0) || (fcntl(stderrfd[0], F_SETFL, output) != 0)) {
                        _stdin = 0;
                        _stdout = 0;
                        _stderr = 0;
                    }
                }

                if ((*pid = fork()) == 0) {
                    char** actualParameters = reinterpret_cast<char**>(_parameters);
                    if (_stdin == -1) {
                        /* Close STDIN and STDOUT as we will redirect them. */
                        close(0);
                        close(1);
                        close(2);

                        /* Close master end of pipe */
                        close(stdinfd[1]);
                        close(stdoutfd[0]);
                        close(stderrfd[0]);

                        /* Make stdin into a readable end */
                        dup2(stdinfd[0], 0);

                        /* Make stdout into writable end */
                        dup2(stdoutfd[1], 1);

                        /* Make stdout into writable end */
                        dup2(stderrfd[1], 2);
                    }

                    /* fork a child process           */
                    if (execvp(*actualParameters, actualParameters) < 0) {
                        // TRACE_L1("Failed to start process: %s.", explain_execvp(*actualParameters, actualParameters));
                        int result = errno;
                        TRACE_L1("Failed to start process: %d - %s.", getpid(), strerror(result));
                        // No glory, so lets quit our selves, avoid the _atexit handlers they should not be there yet...
                        _exit(result);
                    }
                } else if (static_cast<int>(_PID) != -1) {
                    /* Parent process... */
                    if (_stdin == -1) {
                        close(stdinfd[0]);
                        _stdin = stdinfd[1];
                    }
                    if (_stdout == -1) {
                        close(stdoutfd[1]);
                        _stdout = stdoutfd[0];
                    }
                    if (_stderr == -1) {
                        close(stderrfd[1]);
                        _stderr = stderrfd[0];
                    }
                }
                _PID = *pid;
                if (_stdin == 0) {
                    //* check descriptors if they need to be closed
                    if (stdinfd[0] != -1) {
                        close(stdinfd[0]);
                        close(stdinfd[1]);
                    }
                    if (stdoutfd[0] != -1) {
                        close(stdoutfd[0]);
                        close(stdoutfd[1]);
                    }
                    if (stderrfd[0] != -1) {
                        close(stderrfd[0]);
                        close(stderrfd[1]);
                    }
                }
#endif
            }

            return (error);
        }

        void Kill(const bool hardKill)
        {
#ifdef __WINDOWS__
            if (hardKill == true) {
                TerminateProcess(_info.hProcess, 1234);
            }
#else
            ::kill(_PID, (hardKill ? SIGKILL : SIGTERM));
#endif
        }

        uint32_t WaitProcessCompleted(const uint32_t waitTime)
        {
#ifdef __WINDOWS__
            if (WaitForSingleObject(_info.hProcess, waitTime) == 0) {
                return (Core::ERROR_NONE);
            }
            return (Core::ERROR_TIMEDOUT);
#else
            uint32_t timeLeft(waitTime);
            while ((IsActive() == true) && (timeLeft > 0)) {
                if (timeLeft == Core::infinite) {
                    SleepMs(500);
                } else {
                    uint32_t sleepTime = (timeLeft > 500 ? 500 : timeLeft);

                    SleepMs(sleepTime);

                    timeLeft -= sleepTime;
                }
            }

            return ((IsActive() == false) ? Core::ERROR_NONE : Core::ERROR_TIMEDOUT);
#endif
        }

        inline uint32_t ExitCode() const
        {
            return (_exitCode);
        }

    private:
        uint16_t _argc;
        void* _parameters;
        mutable uint32_t _exitCode;
#ifdef __WINDOWS__
        HANDLE _stdin;
        HANDLE _stdout;
        HANDLE _stderr;
        PROCESS_INFORMATION _info;
#else
        int _stdin;
        int _stdout;
        int _stderr;
        process_t _PID;
#endif
    };
}
}
