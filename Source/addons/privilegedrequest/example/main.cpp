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

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

using namespace Thunder;

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

using Files = std::vector<Core::File>;

void LoadFiles(const uint8_t nFiles, const string& who, Files& files) {
    files.clear();

    for (uint8_t i = 0; i < nFiles; i++) {
        char tmpfile[sizeof(fileNameTemplate) + 1];
        strcpy(tmpfile, fileNameTemplate);

        int fd = mkstemp(tmpfile); // creates a temp file and the generated filename is written in tmpfile

        files.emplace_back(tmpfile);

        if (fd > 0) {
            close(fd);

            Core::File& newFile = files.back();

            newFile.Open(false);

            printf("%s opened shared file [%d] %s\n", who.c_str(), newFile.operator Handle(), newFile.FileName().c_str());

            std::stringstream line;

            line << "[" << who << "] (" << Core::Time::Now().Ticks() << ") opened from PID=" << getpid() << "! Sequence= " << static_cast<unsigned>(i) << std::endl;

            newFile.Write(reinterpret_cast<const uint8_t*>(line.str().c_str()), line.str().size());
        } else {
            printf("Failed to create shared file: %s\n", strerror(errno));
        }
    }
}
 
class Server : public Core::PrivilegedRequest {
private:
    class Callback : public Core::PrivilegedRequest::ICallback {
    public:
        Callback() = delete;
        Callback(Callback&&) = delete;
        Callback(const Callback&&) = delete;
        Callback& operator= (Callback&&) = delete;
        Callback& operator= (const Callback&&) = delete;

        Callback(Server& parent) : _parent(parent) {
        }
        ~Callback() override = default;

    public:
        void Request(const uint32_t id, Container& descriptors) override {
            _parent.Request(id, descriptors);
            printf("Request descriptors for [%d]: Amount: [%d]\n", id, static_cast<uint32_t>(descriptors.size()));
        }
        void Offer(const uint32_t id, Container&& descriptors) override {
            printf("Offered descriptors for [%d]: Amount: [%d]\n", id, static_cast<uint32_t>(descriptors.size()));
            _parent.Offer(id, std::move(descriptors));
        }

    private:
        Server& _parent;
    };

public:
    Server() = delete;
    Server(Server&&) = delete;
    Server(const Server&) = delete;
    Server& operator=(Server&&) = delete;
    Server& operator=(const Server&) = delete;

    Server(const uint8_t nFiles)
        : Core::PrivilegedRequest(&_callback)
        , _sharedFiles()
        , _callback(*this)
    {
        LoadFiles(nFiles, "Server", _sharedFiles);
    }

    ~Server()
    {
        for (auto& file : _sharedFiles) {
            std::stringstream line;

            line << "[Server] (" << Core::Time::Now().Ticks() << ") closed from PID=" << getpid() << "!" << std::endl;

            file.Write(reinterpret_cast<const uint8_t*>(line.str().c_str()), line.str().size());

            file.Close();

            printf("Closed shared file.\n");
        }
    }
    void Request(const uint32_t id VARIABLE_IS_NOT_USED, Container& descriptors) {
        descriptors.clear();
        uint8_t i(0);

        for (auto& file : _sharedFiles) {
            if (i < Core::PrivilegedRequest::MaxDescriptorsPerRequest) {
                descriptors.emplace_back(file);
                i++;
            }
        }

        printf("Service passed nFd=%d\n", i);
    }
    void Offer(const uint32_t id VARIABLE_IS_NOT_USED, Container&& descriptors) {
        for (const auto& fd : descriptors) {
            if (fd > 0) {
                int current = static_cast<int>(fd);
                printf("descriptor: %d\n", current);

                FILE* file = fdopen(current, "w");

                if (file) {
                    fprintf(file, "[Server] (%" PRIu64 ") write from PID=%d!\n", Core::Time::Now().Ticks(), getpid());
                    fclose(file);
                } else {
                    printf("fdopen failed\n");
                }

                close(fd);
            }
        }
        descriptors.clear();
    }

private:
    Files _sharedFiles;
    Callback _callback;
};
}

int main(int argc, char** argv)
{
    bool server(false);
    uint8_t nFiles(12);
    string identifier;

    if ((ParseOptions(argc, argv, identifier, server) == true) || (identifier.empty() == true)) {
        printf("\nFDPassing [-server] <domain path name>\n");
    } else {
        int keyPress;

        Core::PrivilegedRequest& channel = (server == true) ? *new Server(nFiles) : *new Core::PrivilegedRequest();

        if ((server == true) && (channel.Open(identifier) != Core::ERROR_NONE)) {
            printf("Something wrong with the identifier path to the server: %s\n", identifier.c_str());
        } else {
            Files clientFiles;
            if (server == false) {
                LoadFiles(nFiles, "Client", clientFiles);
            }
            do {
                if (server == true) {
                    printf("\nserver [c,q]>");
                } else {
                    printf("\nclient [r,o,q]>");
                }
                keyPress = toupper(getchar());

                switch (keyPress) {
                case 'R': {
                    uint32_t id = 1;
                    Core::PrivilegedRequest::Container fds;

                    channel.Request(1000, identifier, id, fds);

                    printf ("Received: %d descriptors.\n", static_cast<uint32_t>(fds.size()));

                    for (const auto& fd : fds) {
                        if (fd > 0) {
                            int current = static_cast<int>(fd);
                            printf("descriptor: %d\n", current);

                            FILE* file = fdopen(current, "w");

                            if (file) {
                                fprintf(file, "[Client] (%" PRIu64 ") write from PID=%d!\n", Core::Time::Now().Ticks(), getpid());
                                fclose(file);
                            } else {
                                printf("fdopen failed\n");
                            }

                            close(fd);
                        }
                    }
                    break;
                }
                case 'O': {
                    uint32_t id = 1;
                    Core::PrivilegedRequest::Container fds;

                    for (auto& file : clientFiles) {
                        fds.emplace_back(static_cast<Core::File::Handle>(file));
                    }

                    uint32_t result = channel.Offer(1000, identifier, id, fds);

                    printf ("Offer: %d descriptors, resulted in %d.\n", static_cast<uint32_t>(fds.size()), result);

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
