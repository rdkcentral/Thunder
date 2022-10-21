/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Metrological
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
#define MODULE_NAME FDPassing

#include <core/core.h>

#include "PriviligedRequest.h"

using namespace WPEFramework;

bool ParseOptions(int argc, char** argv, string& identifier, bool& server)
{
    int index = 1;
    bool showHelp = false;

    while ((index < argc) && (showHelp == false)) {

        if (strncmp(argv[index], "-server\0", 8) == 0) {
            server = true;
        } else if ((argv[index][0] == '-') || (identifier.empty() == false)) {
            showHelp = true;
        } else {
            identifier = argv[index];
        }
        index++;
    }

    return (showHelp);
}

namespace {
class Server : public Core::PriviligedRequest {
public:
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    Server()
        : _sharedFile(fileName)
    {
        _sharedFile.Destroy();

        _sharedFile.Create();
        _sharedFile.Open(false);

        printf("Opened shared file %d\n", int(_sharedFile));

        std::stringstream line;

        line << "(" << Core::Time::Now().Ticks() << ") opened from PID=" << getpid() << "!" << std::endl;

        _sharedFile.Write(reinterpret_cast<const uint8_t*>(line.str().c_str()), line.str().size());
    }

    ~Server()
    {
        std::stringstream line;

        line << "(" << Core::Time::Now().Ticks() << ") closed from PID=" << getpid() << "!" << std::endl;

        _sharedFile.Write(reinterpret_cast<const uint8_t*>(line.str().c_str()), line.str().size());

        _sharedFile.Close();

        printf("Closed shared file.\n");
    }

    int Service(const uint32_t id) override
    {
        TRACE_L1("FDPassing %s, %d identifier=%d", __FUNCTION__, int(_sharedFile), id);
        return int(_sharedFile);
    }

private:
    static constexpr char const* fileName = "/tmp/shared_file";

    Core::File _sharedFile;
};
}

int main(int argc, char** argv)
{
    bool server = false;
    string identifier;

    if ((ParseOptions(argc, argv, identifier, server) == true) || (identifier.empty() == true)) {
        printf("\nFDPassing [-server] <domain path name>\n");
    } else {
        int keyPress;

        Core::PriviligedRequest& channel = (server == true) ? *new Server() : *new Core::PriviligedRequest();

        if ((server == true) && (channel.Open(identifier) != Core::ERROR_NONE)) {
            printf("Something wrong with the identifier path to the server: %s\n", identifier.c_str());
        } else {
            do {
                if (server == true) {
                    printf("\nserver [c,q]>");
                } else {
                    printf("\nclient [r,q]>");
                }
                keyPress = toupper(getchar());

                switch (keyPress) {
                case 'R': {
                    uint32_t id = 1;
                    int fd = channel.Request(1000, identifier, id);

                    if (fd > 0) {
                        printf("descriptor: %d\n", fd);

                        FILE* file = fdopen(fd, "w");

                        if (file) {
                            fprintf(file, "(%" PRIu64 ") write from PID=%d!\n", Core::Time::Now().Ticks(), getpid());
                            fclose(file);
                        } else {
                            printf("fdopen failed\n");
                        }

                        close(fd);
                    }
                    break;
                }
                case 'C': {

                    break;
                }
                }

            } while (keyPress != 'Q');

            channel.Close();
            delete &channel;
        }
    }

    Core::Singleton::Dispose();
    return (0);
}