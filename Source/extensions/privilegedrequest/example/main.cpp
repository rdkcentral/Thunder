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

#include "privilegedrequest/PrivilegedRequest.h"

using namespace WPEFramework;

constexpr char fileNameTemplate[] = "/tmp/shared_file_test_XXXXXX";

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
class Server : public Core::PrivilegedRequest {
public:
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    Server(const uint8_t nFiles)
        : _sharedFiles()
    {
        _sharedFiles.clear();

        for (uint8_t i = 0; i < nFiles; i++) {
            char tmpfile[sizeof(fileNameTemplate) + 1];
            strcpy(tmpfile, fileNameTemplate);

            _sharedFiles.emplace_back(mktemp(tmpfile));

            Core::File& newFile = _sharedFiles.back();

            newFile.Create();
            newFile.Open(false);

            printf("Server opened shared file [%d] %s\n", int(newFile), newFile.FileName().c_str());

            std::stringstream line;

            line << "(" << Core::Time::Now().Ticks() << ") opened from PID=" << getpid() << "!" << std::endl;

            newFile.Write(reinterpret_cast<const uint8_t*>(line.str().c_str()), line.str().size());
        }

        printf("Server for %d file%s\n", nFiles, (nFiles == 1) ? "" : "s");
    }

    ~Server()
    {
        for (auto& file : _sharedFiles) {
            std::stringstream line;

            line << "(" << Core::Time::Now().Ticks() << ") closed from PID=" << getpid() << "!" << std::endl;

            file.Write(reinterpret_cast<const uint8_t*>(line.str().c_str()), line.str().size());

            file.Close();

            printf("Closed shared file.\n");
        }

        _sharedFiles.clear();
    }

    uint8_t Service(const uint32_t id, const uint8_t maxSize, int container[]) override
    {
        // container.clear();
        memset(container, -1, maxSize);

        uint8_t i(0);

        for (auto& file : _sharedFiles) {
            if (i < maxSize) {
                printf("identifier=%d -> %s fd=%d nFd=%d", id, file.FileName().c_str(), int(file), i);
                container[i++] = int(file);
            }
        }

        printf("Service passed nFd=%d", i);

        return i;
    }

private:
    std::vector<Core::File> _sharedFiles;
};
}

int main(int argc, char** argv)
{
    bool server(false);
    uint8_t nFiles(20);
    string identifier;

    if ((ParseOptions(argc, argv, identifier, server) == true) || (identifier.empty() == true)) {
        printf("\nFDPassing [-server] <domain path name>\n");
    } else {
        int keyPress;

        Core::PrivilegedRequest& channel = (server == true) ? *new Server(nFiles) : *new Core::PrivilegedRequest();

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
                    Core::PrivilegedRequest::Container fds;

                    channel.Request(1000, identifier, id, fds);

                    for (auto& fd : fds) {
                        if (fd > 0) {
                            printf("descriptor: %d\n", int(fd));

                            FILE* file = fdopen(fd, "w");

                            if (file) {
                                fprintf(file, "(%" PRIu64 ") write from PID=%d!\n", Core::Time::Now().Ticks(), getpid());
                                fclose(file);
                            } else {
                                printf("fdopen failed\n");
                            }

                            close(fd);
                        }
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