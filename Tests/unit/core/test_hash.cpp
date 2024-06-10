/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2021 Metrological
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

#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>
#include <cryptalgo/cryptalgo.h>
#include <thread>

namespace Thunder {
namespace Tests {

    std::string GetSystemHash(std::string result)
    {
        std::stringstream iss(result);

        iss >> result;
        return result;
    }

    std::string ExecuteCmd(const string cmd) {
        char buffer[256];
        std::string result = "";
        FILE* pipe = popen(cmd.c_str(), "r");

#ifdef __CORE_EXCEPTION_CATCHING__
        try {
#endif

            EXPECT_TRUE(pipe != nullptr);
            while (fgets(buffer, sizeof buffer, pipe) != NULL) {
                result = GetSystemHash(buffer);
            }
#ifdef __CORE_EXCEPTION_CATCHING__
        } catch (...) {
            pclose(pipe);
            throw;
        }
#endif
        pclose(pipe);

        return result;
    }

    inline bool HashStringToBytes(const string& hash, uint8_t hashHex[], uint16_t length)
    {
        bool status = true;

        for (uint8_t i = 0; i < length; i++) {
            char highNibble = hash.c_str()[i * 2];
            char lowNibble = hash.c_str()[(i * 2) + 1];
            if (isxdigit(highNibble) && isxdigit(lowNibble)) {
                std::string byteStr = hash.substr(i * 2, 2);
                hashHex[i] = static_cast<uint8_t>(strtol(byteStr.c_str(), nullptr, 16));
            }
            else {
                status = false;
                break;
            }
        }
        return status;
    }

    std::string HashBytesToString(const uint8_t buffer[], size_t length) {
        std::string hash;
        hash.reserve(length * 2);

        for (const unsigned char *ptr = buffer; ptr < buffer + length; ++ptr) {
            char buf[3];
            sprintf(buf, "%02x", (*ptr)&0xff);
            hash += buf;
        }
        return hash;
    }
    template <typename HASHALGORITHM>
    class SignedFileType : public Core::File {
    private:
        static int16_t constexpr BlockSize = 1024;
    public:
        using HashType = HASHALGORITHM;

        SignedFileType(const SignedFileType<HASHALGORITHM>&) = delete;
        SignedFileType<HASHALGORITHM>& operator=(const SignedFileType<HASHALGORITHM>&) = delete;

        SignedFileType()
            : Core::File()
            , _hash()
            , _startPosition(0)
        {
            _hash.Reset();
        }
        SignedFileType(const string& path)
            : Core::File(path)
            , _hash()
            , _startPosition(0)
        {
            _hash.Reset();
        }
        ~SignedFileType()
        {
            //Destroy();
        }

    public:
        void Open()
        {
            // Are we opening the file ?
            if (Core::File::IsOpen() == false) {
                if (Core::File::Open() == true) {
                    _startPosition = 0;
                    Core::File::LoadFileInfo();
                    Core::File::Position(false, _startPosition);
                }
            }
        }
        void Close()
        {
            if (Core::File::IsOpen() == true) {
                Core::File::Close();
            }
        }
        inline HashType& Hash()
        {
            return (_hash);
        }
        inline const HashType& Hash() const
        {
            return (_hash);
        }
        void CreateFileData(const uint8_t data, const uint32_t length)
        {
            if (Core::File::Create() == true) {
                Write(data, length);
            }
        }
        void AppendFileData(const uint8_t data, const uint32_t length)
        {
            Core::File::Append();
            Write(data, length);
        }
        void AppendFileData(const uint8_t data[], const uint32_t length)
        {
            Core::File::Append();
            Write(data, length);
        }
        void ReadFileData(const int64_t position, uint8_t data[], uint32_t& length)
        {
            if (Core::File::IsOpen() != true) {
                Open();
            }
            if (Core::File::IsOpen() == true) {
                int64_t currentPosition = Core::File::Position();
                Core::File::Position(false, position);
                length = Core::File::Read(data, length);
                Core::File::Position(false, currentPosition);
            }
        }
        void DestroyFile()
        {
            if (Core::File::IsOpen() == true) {
                Close();
            }
            Core::File::Destroy();
        }
        int64_t FileSize()
        {
            Core::File::LoadFileInfo();
            return Core::File::Size();
        }
        uint32_t CalculateHash(const uint32_t start, const uint32_t end)
        {
            uint32_t status = Core::ERROR_NONE;
            HashType& hash = _hash;

            uint8_t   buffer[64];
            if (Core::File::IsOpen() != true) {
                Open();
            }
            if (Core::File::IsOpen() == true) {
                if ((start < Core::File::Size()) && (end <= Core::File::Size())) {
                    uint32_t pos = Core::File::Position();
                    uint32_t size = end;

                    Core::File::Position(false, start);
                    // Read all Data to calculate the HASH/HMAC
                    while (size > 0) {
                        uint16_t chunk = std::min(static_cast<uint16_t>(sizeof(buffer)), static_cast<uint16_t>(size));
                        Core::File::Read(buffer, chunk);
                        hash.Input(buffer, chunk);
                        size -= chunk;
                    }

                    Core::File::Position(false, pos);

                } else {
                    status = Core::ERROR_GENERAL;
                    printf("Wrong file position given start = %u end = %u, cross check\n", start, end);
                }
            }
            return status;
        }
        void CreateHashFile(const string& hashFile, const uint8_t data[], const uint32_t length)
        {
            _hashFileStorage = hashFile;
            if (_hashFileStorage.Create() == true) {
                _hashFileStorage.Write(data, length);
            }
        }
        void ReadHashFile(uint8_t data[], uint32_t& length)
        {
            if (_hashFileStorage.IsOpen() != true) {
                _hashFileStorage.Open();
            }
            if (_hashFileStorage.IsOpen() == true) {
                _hashFileStorage.Position(false, 0);
                length = _hashFileStorage.Read(data, length);
            }
        }
        void CloseHashFile()
        {
            _hashFileStorage.Close();
        }
        void DestroyHashFile()
        {
            _hashFileStorage.Destroy();
        }
    private:
        inline void Write(const uint8_t data, const uint32_t length)
        {
            uint8_t buffer[64];
            memset(buffer, data, sizeof(buffer));
            Write(buffer, length);
        }
        inline void Write(const uint8_t buffer[], const uint32_t length)
        {
            uint32_t size = length;
            uint32_t position = 0;
            while (size > 0) {
                uint16_t chunk = std::min(static_cast<uint16_t>(64), static_cast<uint16_t>(size));
                Core::File::Write(buffer + position, chunk);
                size -= chunk;
                position += chunk;
            }
        }

    private:
        HashType _hash;
        int64_t _startPosition;
        Core::File _hashFileStorage;
    };

    // MD5 Tests
    TEST(HashMd5, TestHash_File)
    {
        SignedFileType<Crypto::MD5> signedFile("test.txt");
        signedFile.CreateFileData('B', 25);
        EXPECT_EQ(signedFile.CalculateHash(0, signedFile.FileSize()), Core::ERROR_NONE);
        Crypto::MD5& md5 = signedFile.Hash();

        uint8_t thunderHMAC[Crypto::HASH_MD5];
        memcpy(thunderHMAC, md5.Result(), Crypto::HASH_MD5);

        EXPECT_STREQ(HashBytesToString(thunderHMAC, Crypto::HASH_MD5).c_str(),
                     ExecuteCmd("md5sum test.txt").c_str());
        signedFile.DestroyFile();
    }
    TEST(HashMd5, TestHash_AppendFile)
    {
        SignedFileType<Crypto::MD5> signedFile("test.txt");
        Crypto::MD5& md5 = signedFile.Hash();

        signedFile.CreateFileData('B', 25);
        signedFile.AppendFileData('C', 30);
        EXPECT_EQ(signedFile.CalculateHash(0, signedFile.FileSize()), Core::ERROR_NONE);

        uint8_t thunderHMAC[Crypto::HASH_MD5];
        memcpy(thunderHMAC, md5.Result(), Crypto::HASH_MD5);

        EXPECT_STREQ(HashBytesToString(thunderHMAC, Crypto::HASH_MD5).c_str(),
                     ExecuteCmd("md5sum test.txt").c_str());
        signedFile.DestroyFile();
    }
    TEST(HashMd5, TestHash_SaveAndLoadHash)
    {
        SignedFileType<Crypto::MD5> signedFile("test.txt");
        Crypto::MD5& md5 = signedFile.Hash();
        md5.Reset();
        signedFile.CreateFileData('B', 25);
        EXPECT_EQ(signedFile.CalculateHash(0, signedFile.FileSize()), Core::ERROR_NONE);

        // Read current context from MD5 handle
        Crypto::MD5::Context context = md5.CurrentContext();

        md5.Reset();
        md5.Load(context);

        uint8_t thunderHMAC[Crypto::HASH_MD5];
        memcpy(thunderHMAC, md5.Result(), Crypto::HASH_MD5);

        EXPECT_STREQ(HashBytesToString(thunderHMAC, Crypto::HASH_MD5).c_str(),
                     ExecuteCmd("md5sum test.txt").c_str());
        signedFile.DestroyFile();
    }
    TEST(HashMd5, TestHash_SaveAndLoadHash_AppendFile)
    {
        SignedFileType<Crypto::MD5> signedFile("test.txt");
        Crypto::MD5& md5 = signedFile.Hash();
        md5.Reset();
        signedFile.CreateFileData('B', 25);
        EXPECT_EQ(signedFile.CalculateHash(0, signedFile.FileSize()), Core::ERROR_NONE);

        // Read current context from MD5 handle
        Crypto::MD5::Context context = md5.CurrentContext();

        // Load context back to MD5 handle
        md5.Reset();
        md5.Load(context);

        // Append some more data to file
        int64_t start = signedFile.FileSize();
        signedFile.AppendFileData('C', 30);
        EXPECT_EQ(signedFile.CalculateHash(start, signedFile.FileSize() - start), Core::ERROR_NONE);

        uint8_t thunderHMAC[Crypto::HASH_MD5];
        memcpy(thunderHMAC, md5.Result(), Crypto::HASH_MD5);

        EXPECT_STREQ(HashBytesToString(thunderHMAC, Crypto::HASH_MD5).c_str(),
                     ExecuteCmd("md5sum test.txt").c_str());
        signedFile.DestroyFile();
    }
    TEST(HashMd5, TestHash_SaveHashToFileAndLoadHashFromFile_AppendFile)
    {
        SignedFileType<Crypto::MD5> signedFile("test.txt");
        Crypto::MD5& md5 = signedFile.Hash();
        md5.Reset();
        signedFile.CreateFileData('B', 25);
        EXPECT_EQ(signedFile.CalculateHash(0, signedFile.FileSize()), Core::ERROR_NONE);

        // Read current context from MD5 handle
        Crypto::MD5::Context context = md5.CurrentContext();

        // Save current context to file
        uint32_t contextLength = sizeof(Crypto::MD5::Context);
        signedFile.CreateHashFile("hashFile", reinterpret_cast<uint8_t*>(&context), contextLength);
        signedFile.CloseHashFile();

        // Read back current context to memory
        signedFile.ReadHashFile(reinterpret_cast<uint8_t*>(&context), contextLength);
        signedFile.DestroyHashFile();

        // Load context back to MD5 handle
        md5.Reset();
        md5.Load(context);

        // Append some more data to file
        uint8_t data[100];
        memset(data, 'C', sizeof(data));
        int64_t start = signedFile.FileSize();
        signedFile.AppendFileData(data, sizeof(data));

        // Calculate final hash
        EXPECT_EQ(signedFile.CalculateHash(start, signedFile.FileSize() - start), Core::ERROR_NONE);

        uint8_t thunderHMAC[Crypto::HASH_MD5];
        memcpy(thunderHMAC, md5.Result(), Crypto::HASH_MD5);

        EXPECT_STREQ(HashBytesToString(thunderHMAC, Crypto::HASH_MD5).c_str(),
                     ExecuteCmd("md5sum test.txt").c_str());
        signedFile.DestroyFile();
    }

    // SHA1 Tests
    TEST(HashSha1, TestHash_File)
    {
        SignedFileType<Crypto::SHA1> signedFile("test.txt");
        signedFile.CreateFileData('B', 25);
        EXPECT_EQ(signedFile.CalculateHash(0, signedFile.FileSize()), Core::ERROR_NONE);
        Crypto::SHA1& sha1 = signedFile.Hash();

        uint8_t thunderHMAC[Crypto::HASH_SHA1];
        memcpy(thunderHMAC, sha1.Result(), Crypto::HASH_SHA1);

        EXPECT_STREQ(HashBytesToString(thunderHMAC, Crypto::HASH_SHA1).c_str(),
                     ExecuteCmd("sha1sum test.txt").c_str());
        signedFile.DestroyFile();
    }
    TEST(HashSha1, TestHash_AppendFile)
    {
        SignedFileType<Crypto::SHA1> signedFile("test.txt");
        Crypto::SHA1& sha1 = signedFile.Hash();

        signedFile.CreateFileData('B', 25);
        signedFile.AppendFileData('C', 30);
        EXPECT_EQ(signedFile.CalculateHash(0, signedFile.FileSize()), Core::ERROR_NONE);

        uint8_t thunderHMAC[Crypto::HASH_SHA1];
        memcpy(thunderHMAC, sha1.Result(), Crypto::HASH_SHA1);

        EXPECT_STREQ(HashBytesToString(thunderHMAC, Crypto::HASH_SHA1).c_str(),
                     ExecuteCmd("sha1sum test.txt").c_str());
        signedFile.DestroyFile();
    }
    TEST(HashSha1, TestHash_SaveAndLoadHash)
    {
        SignedFileType<Crypto::SHA1> signedFile("test.txt");
        Crypto::SHA1& sha1 = signedFile.Hash();
        sha1.Reset();
        signedFile.CreateFileData('B', 25);
        EXPECT_EQ(signedFile.CalculateHash(0, signedFile.FileSize()), Core::ERROR_NONE);

        // Read current context from SHA1 handle
        Crypto::SHA1::Context context = sha1.CurrentContext();

        sha1.Reset();
        sha1.Load(context);

        uint8_t thunderHMAC[Crypto::HASH_SHA1];
        memcpy(thunderHMAC, sha1.Result(), Crypto::HASH_SHA1);

        EXPECT_STREQ(HashBytesToString(thunderHMAC, Crypto::HASH_SHA1).c_str(),
                     ExecuteCmd("sha1sum test.txt").c_str());
        signedFile.DestroyFile();
    }
    TEST(HashSha1, TestHash_SaveAndLoadHash_AppendFile)
    {
        SignedFileType<Crypto::SHA1> signedFile("test.txt");
        Crypto::SHA1& sha1 = signedFile.Hash();
        sha1.Reset();
        signedFile.CreateFileData('B', 25);
        EXPECT_EQ(signedFile.CalculateHash(0, signedFile.FileSize()), Core::ERROR_NONE);

        // Read current context from SHA1 handle
        Crypto::SHA1::Context context = sha1.CurrentContext();

        // Load context back to SHA1 handle
        sha1.Reset();
        sha1.Load(context);

        // Append some more data to file
        int64_t start = signedFile.FileSize();
        signedFile.AppendFileData('C', 30);
        EXPECT_EQ(signedFile.CalculateHash(start, signedFile.FileSize() - start), Core::ERROR_NONE);

        uint8_t thunderHMAC[Crypto::HASH_SHA1];
        memcpy(thunderHMAC, sha1.Result(), Crypto::HASH_SHA1);

        EXPECT_STREQ(HashBytesToString(thunderHMAC, Crypto::HASH_SHA1).c_str(),
                     ExecuteCmd("sha1sum test.txt").c_str());
        signedFile.DestroyFile();
    }
    TEST(HashSha1, TestHash_SaveHashToFileAndLoadHashFromFile_AppendFile)
    {
        SignedFileType<Crypto::SHA1> signedFile("test.txt");
        Crypto::SHA1& sha1 = signedFile.Hash();
        sha1.Reset();
        signedFile.CreateFileData('B', 25);
        EXPECT_EQ(signedFile.CalculateHash(0, signedFile.FileSize()), Core::ERROR_NONE);

        // Read current context from SHA1 handle
        Crypto::SHA1::Context context = sha1.CurrentContext();

        // Save current context to file
        uint32_t contextLength = sizeof(Crypto::SHA1::Context);
        signedFile.CreateHashFile("hashFile", reinterpret_cast<uint8_t*>(&context), contextLength);
        signedFile.CloseHashFile();

        // Read back current context to memory
        signedFile.ReadHashFile(reinterpret_cast<uint8_t*>(&context), contextLength);
        signedFile.DestroyHashFile();

        // Load context back to SHA1 handle
        sha1.Reset();
        sha1.Load(context);

        // Append some more data to file
        uint8_t data[100];
        memset(data, 'C', sizeof(data));
        int64_t start = signedFile.FileSize();
        signedFile.AppendFileData(data, sizeof(data));

        // Calculate final hash
        EXPECT_EQ(signedFile.CalculateHash(start, signedFile.FileSize() - start), Core::ERROR_NONE);

        uint8_t thunderHMAC[Crypto::HASH_SHA1];
        memcpy(thunderHMAC, sha1.Result(), Crypto::HASH_SHA1);

        EXPECT_STREQ(HashBytesToString(thunderHMAC, Crypto::HASH_SHA1).c_str(),
                     ExecuteCmd("sha1sum test.txt").c_str());
        signedFile.DestroyFile();
    }

    // SHA224 Tests
    TEST(HashSha224, TestHash_File)
    {
        SignedFileType<Crypto::SHA224> signedFile("test.txt");
        signedFile.CreateFileData('B', 25);
        EXPECT_EQ(signedFile.CalculateHash(0, signedFile.FileSize()), Core::ERROR_NONE);
        Crypto::SHA224& sha224 = signedFile.Hash();

        uint8_t thunderHMAC[Crypto::HASH_SHA224];
        memcpy(thunderHMAC, sha224.Result(), Crypto::HASH_SHA224);

        EXPECT_STREQ(HashBytesToString(thunderHMAC, Crypto::HASH_SHA224).c_str(),
                     ExecuteCmd("sha224sum test.txt").c_str());
        signedFile.DestroyFile();
    }
    TEST(HashSha224, TestHash_AppendFile)
    {
        SignedFileType<Crypto::SHA224> signedFile("test.txt");
        Crypto::SHA224& sha224 = signedFile.Hash();

        signedFile.CreateFileData('B', 25);
        signedFile.AppendFileData('C', 30);
        EXPECT_EQ(signedFile.CalculateHash(0, signedFile.FileSize()), Core::ERROR_NONE);

        uint8_t thunderHMAC[Crypto::HASH_SHA224];
        memcpy(thunderHMAC, sha224.Result(), Crypto::HASH_SHA224);

        EXPECT_STREQ(HashBytesToString(thunderHMAC, Crypto::HASH_SHA224).c_str(),
                     ExecuteCmd("sha224sum test.txt").c_str());
        signedFile.DestroyFile();
    }
    TEST(HashSha224, TestHash_SaveAndLoadHash)
    {
        SignedFileType<Crypto::SHA224> signedFile("test.txt");
        Crypto::SHA224& sha224 = signedFile.Hash();
        sha224.Reset();
        signedFile.CreateFileData('B', 25);
        EXPECT_EQ(signedFile.CalculateHash(0, signedFile.FileSize()), Core::ERROR_NONE);

        // Read current context from SHA224 handle
        Crypto::SHA1::Context context = sha224.CurrentContext();

        sha224.Reset();
        sha224.Load(context);

        uint8_t thunderHMAC[Crypto::HASH_SHA224];
        memcpy(thunderHMAC, sha224.Result(), Crypto::HASH_SHA224);

        EXPECT_STREQ(HashBytesToString(thunderHMAC, Crypto::HASH_SHA224).c_str(),
                     ExecuteCmd("sha224sum test.txt").c_str());
        signedFile.DestroyFile();
    }
    TEST(HashSha224, TestHash_SaveAndLoadHash_AppendFile)
    {
        SignedFileType<Crypto::SHA224> signedFile("test.txt");
        Crypto::SHA224& sha224 = signedFile.Hash();
        sha224.Reset();
        signedFile.CreateFileData('B', 25);
        EXPECT_EQ(signedFile.CalculateHash(0, signedFile.FileSize()), Core::ERROR_NONE);

        // Read current context from SHA224 handle
        Crypto::SHA224::Context context = sha224.CurrentContext();

        // Load context back to SHA224 handle
        sha224.Reset();
        sha224.Load(context);

        // Append some more data to file
        int64_t start = signedFile.FileSize();
        signedFile.AppendFileData('C', 30);
        EXPECT_EQ(signedFile.CalculateHash(start, signedFile.FileSize() - start), Core::ERROR_NONE);

        uint8_t thunderHMAC[Crypto::HASH_SHA224];
        memcpy(thunderHMAC, sha224.Result(), Crypto::HASH_SHA224);

        EXPECT_STREQ(HashBytesToString(thunderHMAC, Crypto::HASH_SHA224).c_str(),
                     ExecuteCmd("sha224sum test.txt").c_str());
        signedFile.DestroyFile();
    }
    TEST(HashSha224, TestHash_SaveHashToFileAndLoadHashFromFile_AppendFile)
    {
        SignedFileType<Crypto::SHA224> signedFile("test.txt");
        Crypto::SHA224& sha224 = signedFile.Hash();
        sha224.Reset();
        signedFile.CreateFileData('B', 25);
        EXPECT_EQ(signedFile.CalculateHash(0, signedFile.FileSize()), Core::ERROR_NONE);

        // Read current context from SHA224 handle
        Crypto::SHA224::Context context = sha224.CurrentContext();

        // Save current context to file
        uint32_t contextLength = sizeof(Crypto::SHA224::Context);
        signedFile.CreateHashFile("hashFile", reinterpret_cast<uint8_t*>(&context), contextLength);
        signedFile.CloseHashFile();

        // Read back current context to memory
        signedFile.ReadHashFile(reinterpret_cast<uint8_t*>(&context), contextLength);
        signedFile.DestroyHashFile();

        // Load context back to SHA224 handle
        sha224.Reset();
        sha224.Load(context);

        // Append some more data to file
        uint8_t data[100];
        memset(data, 'C', sizeof(data));
        int64_t start = signedFile.FileSize();
        signedFile.AppendFileData(data, sizeof(data));

        // Calculate final hash
        EXPECT_EQ(signedFile.CalculateHash(start, signedFile.FileSize() - start), Core::ERROR_NONE);

        uint8_t thunderHMAC[Crypto::HASH_SHA224];
        memcpy(thunderHMAC, sha224.Result(), Crypto::HASH_SHA224);

        EXPECT_STREQ(HashBytesToString(thunderHMAC, Crypto::HASH_SHA224).c_str(),
                     ExecuteCmd("sha224sum test.txt").c_str());
        signedFile.DestroyFile();
    }

    // SHA256 Tests
    TEST(HashSha256, TestHash_File)
    {
        SignedFileType<Crypto::SHA256> signedFile("test.txt");
        signedFile.CreateFileData('B', 25);
        EXPECT_EQ(signedFile.CalculateHash(0, signedFile.FileSize()), Core::ERROR_NONE);
        Crypto::SHA256& sha256 = signedFile.Hash();

        uint8_t thunderHMAC[Crypto::HASH_SHA256];
        memcpy(thunderHMAC, sha256.Result(), Crypto::HASH_SHA256);

        EXPECT_STREQ(HashBytesToString(thunderHMAC, Crypto::HASH_SHA256).c_str(), ExecuteCmd("sha256sum test.txt").c_str());
        signedFile.DestroyFile();
    }
    TEST(HashSha256, TestHash_AppendFile)
    {
        SignedFileType<Crypto::SHA256> signedFile("test.txt");
        Crypto::SHA256& sha256 = signedFile.Hash();

        signedFile.CreateFileData('B', 25);
        signedFile.AppendFileData('C', 30);
        EXPECT_EQ(signedFile.CalculateHash(0, signedFile.FileSize()), Core::ERROR_NONE);

        uint8_t thunderHMAC[Crypto::HASH_SHA256];
        memcpy(thunderHMAC, sha256.Result(), Crypto::HASH_SHA256);

        EXPECT_STREQ(HashBytesToString(thunderHMAC, Crypto::HASH_SHA256).c_str(),
                     ExecuteCmd("sha256sum test.txt").c_str());
        signedFile.DestroyFile();
    }
    TEST(HashSha256, TestHash_SaveAndLoadHash)
    {
        SignedFileType<Crypto::SHA256> signedFile("test.txt");
        Crypto::SHA256& sha256 = signedFile.Hash();
        sha256.Reset();
        signedFile.CreateFileData('B', 25);
        EXPECT_EQ(signedFile.CalculateHash(0, signedFile.FileSize()), Core::ERROR_NONE);

        // Read current context from SHA256 handle
        Crypto::SHA256::Context context = sha256.CurrentContext();

        sha256.Reset();
        sha256.Load(context);

        uint8_t thunderHMAC[Crypto::HASH_SHA256];
        memcpy(thunderHMAC, sha256.Result(), Crypto::HASH_SHA256);

        EXPECT_STREQ(HashBytesToString(thunderHMAC, Crypto::HASH_SHA256).c_str(),
                     ExecuteCmd("sha256sum test.txt").c_str());
        signedFile.DestroyFile();
    }
    TEST(HashSha256, TestHash_SaveAndLoadHash_AppendFile)
    {
        SignedFileType<Crypto::SHA256> signedFile("test.txt");
        Crypto::SHA256& sha256 = signedFile.Hash();
        sha256.Reset();
        signedFile.CreateFileData('B', 25);
        EXPECT_EQ(signedFile.CalculateHash(0, signedFile.FileSize()), Core::ERROR_NONE);

        // Read current context from SHA256 handle
        Crypto::SHA256::Context context = sha256.CurrentContext();

        // Load context back to SHA256 handle
        sha256.Reset();
        sha256.Load(context);

        // Append some more data to file
        int64_t start = signedFile.FileSize();
        signedFile.AppendFileData('C', 30);
        EXPECT_EQ(signedFile.CalculateHash(start, signedFile.FileSize() - start), Core::ERROR_NONE);

        uint8_t thunderHMAC[Crypto::HASH_SHA256];
        memcpy(thunderHMAC, sha256.Result(), Crypto::HASH_SHA256);

        EXPECT_STREQ(HashBytesToString(thunderHMAC, Crypto::HASH_SHA256).c_str(),
                     ExecuteCmd("sha256sum test.txt").c_str());
        signedFile.DestroyFile();
    }

    TEST(HashSha256, TestHash_SaveHashToFileAndLoadHashFromFile_AppendFile)
    {
        SignedFileType<Crypto::SHA256> signedFile("test.txt");
        Crypto::SHA256& sha256 = signedFile.Hash();
        sha256.Reset();
        signedFile.CreateFileData('B', 25);
        EXPECT_EQ(signedFile.CalculateHash(0, signedFile.FileSize()), Core::ERROR_NONE);

        // Read current context from SHA256 handle
        Crypto::SHA256::Context context = sha256.CurrentContext();

        // Save current context to file
        uint32_t contextLength = sizeof(Crypto::SHA256::Context);
        signedFile.CreateHashFile("hashFile", reinterpret_cast<uint8_t*>(&context), contextLength);
        signedFile.CloseHashFile();

        // Read back current context to memory
        signedFile.ReadHashFile(reinterpret_cast<uint8_t*>(&context), contextLength);
        signedFile.DestroyHashFile();

        // Load context back to SHA256 handle
        sha256.Reset();
        sha256.Load(context);

        // Append some more data to file
        uint8_t data[100];
        memset(data, 'C', sizeof(data));
        int64_t start = signedFile.FileSize();
        signedFile.AppendFileData(data, sizeof(data));

        // Calculate final hash
        EXPECT_EQ(signedFile.CalculateHash(start, signedFile.FileSize() - start), Core::ERROR_NONE);

        uint8_t thunderHMAC[Crypto::HASH_SHA256];
        memcpy(thunderHMAC, sha256.Result(), Crypto::HASH_SHA256);

        EXPECT_STREQ(HashBytesToString(thunderHMAC, Crypto::HASH_SHA256).c_str(),
                     ExecuteCmd("sha256sum test.txt").c_str());
        //signedFile.DestroyFile();
    }

    // SHA384 Tests
    TEST(HashSha384, TestHash_File)
    {
        SignedFileType<Crypto::SHA384> signedFile("test.txt");
        signedFile.CreateFileData('B', 25);
        EXPECT_EQ(signedFile.CalculateHash(0, signedFile.FileSize()), Core::ERROR_NONE);
        Crypto::SHA384& sha384 = signedFile.Hash();

        uint8_t thunderHMAC[Crypto::HASH_SHA384];
        memcpy(thunderHMAC, sha384.Result(), Crypto::HASH_SHA384);

        EXPECT_STREQ(HashBytesToString(thunderHMAC, Crypto::HASH_SHA384).c_str(),
                     ExecuteCmd("sha384sum test.txt").c_str());
        signedFile.DestroyFile();
    }
    TEST(HashSha384, TestHash_AppendFile)
    {
        SignedFileType<Crypto::SHA384> signedFile("test.txt");
        Crypto::SHA384& sha384 = signedFile.Hash();

        signedFile.CreateFileData('B', 25);
        signedFile.AppendFileData('C', 30);
        EXPECT_EQ(signedFile.CalculateHash(0, signedFile.FileSize()), Core::ERROR_NONE);

        uint8_t thunderHMAC[Crypto::HASH_SHA384];
        memcpy(thunderHMAC, sha384.Result(), Crypto::HASH_SHA384);

        EXPECT_STREQ(HashBytesToString(thunderHMAC, Crypto::HASH_SHA384).c_str(),
                     ExecuteCmd("sha384sum test.txt").c_str());
        signedFile.DestroyFile();
    }
    TEST(HashSha384, TestHash_SaveAndLoadHash)
    {
        SignedFileType<Crypto::SHA384> signedFile("test.txt");
        Crypto::SHA384& sha384 = signedFile.Hash();
        sha384.Reset();
        signedFile.CreateFileData('B', 25);
        EXPECT_EQ(signedFile.CalculateHash(0, signedFile.FileSize()), Core::ERROR_NONE);

        // Read current context from SHA384 handle
        Crypto::SHA384::Context context = sha384.CurrentContext();

        sha384.Reset();
        sha384.Load(context);

        uint8_t thunderHMAC[Crypto::HASH_SHA384];
        memcpy(thunderHMAC, sha384.Result(), Crypto::HASH_SHA384);

        EXPECT_STREQ(HashBytesToString(thunderHMAC, Crypto::HASH_SHA384).c_str(),
                     ExecuteCmd("sha384sum test.txt").c_str());
        signedFile.DestroyFile();
    }
    TEST(HashSha384, TestHash_SaveAndLoadHash_AppendFile)
    {
        SignedFileType<Crypto::SHA384> signedFile("test.txt");
        Crypto::SHA384& sha384 = signedFile.Hash();
        sha384.Reset();
        signedFile.CreateFileData('B', 25);
        EXPECT_EQ(signedFile.CalculateHash(0, signedFile.FileSize()), Core::ERROR_NONE);

        // Read current context from SHA384 handle
        Crypto::SHA384::Context context = sha384.CurrentContext();

        // Load context back to SHA384 handle
        sha384.Reset();
        sha384.Load(context);

        // Append some more data to file
        int64_t start = signedFile.FileSize();
        signedFile.AppendFileData('C', 30);
        EXPECT_EQ(signedFile.CalculateHash(start, signedFile.FileSize() - start), Core::ERROR_NONE);

        uint8_t thunderHMAC[Crypto::HASH_SHA384];
        memcpy(thunderHMAC, sha384.Result(), Crypto::HASH_SHA384);

        EXPECT_STREQ(HashBytesToString(thunderHMAC, Crypto::HASH_SHA384).c_str(),
                     ExecuteCmd("sha384sum test.txt").c_str());
        signedFile.DestroyFile();
    }
    TEST(HashSha384, TestHash_SaveHashToFileAndLoadHashFromFile_AppendFile)
    {
        SignedFileType<Crypto::SHA384> signedFile("test.txt");
        Crypto::SHA384& sha384 = signedFile.Hash();
        sha384.Reset();
        signedFile.CreateFileData('B', 25);
        EXPECT_EQ(signedFile.CalculateHash(0, signedFile.FileSize()), Core::ERROR_NONE);

        // Read current context from SHA384 handle
        Crypto::SHA384::Context context = sha384.CurrentContext();

        // Save current context to file
        uint32_t contextLength = sizeof(Crypto::SHA384::Context);
        signedFile.CreateHashFile("hashFile", reinterpret_cast<uint8_t*>(&context), contextLength);
        signedFile.CloseHashFile();

        // Read back current context to memory
        signedFile.ReadHashFile(reinterpret_cast<uint8_t*>(&context), contextLength);
        signedFile.DestroyHashFile();

        // Load context back to SHA384 handle
        sha384.Reset();
        sha384.Load(context);

        // Append some more data to file
        uint8_t data[100];
        memset(data, 'C', sizeof(data));
        int64_t start = signedFile.FileSize();
        signedFile.AppendFileData(data, sizeof(data));

        // Calculate final hash
        EXPECT_EQ(signedFile.CalculateHash(start, signedFile.FileSize() - start), Core::ERROR_NONE);

        uint8_t thunderHMAC[Crypto::HASH_SHA384];
        memcpy(thunderHMAC, sha384.Result(), Crypto::HASH_SHA384);

        EXPECT_STREQ(HashBytesToString(thunderHMAC, Crypto::HASH_SHA384).c_str(),
                     ExecuteCmd("sha384sum test.txt").c_str());
        signedFile.DestroyFile();
    }

    // SHA512 Tests
    TEST(HashSha512, TestHash_File)
    {
        SignedFileType<Crypto::SHA512> signedFile("test.txt");
        signedFile.CreateFileData('B', 25);
        EXPECT_EQ(signedFile.CalculateHash(0, signedFile.FileSize()), Core::ERROR_NONE);
        Crypto::SHA512& sha512 = signedFile.Hash();

        uint8_t thunderHMAC[Crypto::HASH_SHA512];
        memcpy(thunderHMAC, sha512.Result(), Crypto::HASH_SHA512);

        EXPECT_STREQ(HashBytesToString(thunderHMAC, Crypto::HASH_SHA512).c_str(),
                     ExecuteCmd("sha512sum test.txt").c_str());
        signedFile.DestroyFile();
    }
    TEST(HashSha512, TestHash_AppendFile)
    {
        SignedFileType<Crypto::SHA512> signedFile("test.txt");
        Crypto::SHA512& sha512 = signedFile.Hash();

        signedFile.CreateFileData('B', 25);
        signedFile.AppendFileData('C', 30);
        EXPECT_EQ(signedFile.CalculateHash(0, signedFile.FileSize()), Core::ERROR_NONE);

        uint8_t thunderHMAC[Crypto::HASH_SHA512];
        memcpy(thunderHMAC, sha512.Result(), Crypto::HASH_SHA512);

        EXPECT_STREQ(HashBytesToString(thunderHMAC, Crypto::HASH_SHA512).c_str(),
                     ExecuteCmd("sha512sum test.txt").c_str());
        signedFile.DestroyFile();
    }
    TEST(HashSha512, TestHash_SaveAndLoadHash)
    {
        SignedFileType<Crypto::SHA512> signedFile("test.txt");
        Crypto::SHA512& sha512 = signedFile.Hash();
        sha512.Reset();
        signedFile.CreateFileData('B', 25);
        EXPECT_EQ(signedFile.CalculateHash(0, signedFile.FileSize()), Core::ERROR_NONE);

        Crypto::SHA512::Context context = sha512.CurrentContext();

        sha512.Reset();
        sha512.Load(context);

        uint8_t thunderHMAC[Crypto::HASH_SHA512];
        memcpy(thunderHMAC, sha512.Result(), Crypto::HASH_SHA512);

        EXPECT_STREQ(HashBytesToString(thunderHMAC, Crypto::HASH_SHA512).c_str(),
                     ExecuteCmd("sha512sum test.txt").c_str());
        signedFile.DestroyFile();
    }
    TEST(HashSha512, TestHash_SaveAndLoadHash_AppendFile)
    {
        SignedFileType<Crypto::SHA512> signedFile("test.txt");
        Crypto::SHA512& sha512 = signedFile.Hash();
        sha512.Reset();
        signedFile.CreateFileData('B', 25);
        EXPECT_EQ(signedFile.CalculateHash(0, signedFile.FileSize()), Core::ERROR_NONE);

        // Read current context from SHA512 handle
        Crypto::SHA512::Context context = sha512.CurrentContext();

        // Load context back to SHA512 handle
        sha512.Reset();
        sha512.Load(context);

        // Append some more data to file
        int64_t start = signedFile.FileSize();
        signedFile.AppendFileData('C', 30);
        EXPECT_EQ(signedFile.CalculateHash(start, signedFile.FileSize() - start), Core::ERROR_NONE);

        uint8_t thunderHMAC[Crypto::HASH_SHA512];
        memcpy(thunderHMAC, sha512.Result(), Crypto::HASH_SHA512);

        EXPECT_STREQ(HashBytesToString(thunderHMAC, Crypto::HASH_SHA512).c_str(),
                     ExecuteCmd("sha512sum test.txt").c_str());
        signedFile.DestroyFile();
    }
    TEST(HashSha512, TestHash_SaveHashToFileAndLoadHashFromFile_AppendFile)
    {
        SignedFileType<Crypto::SHA512> signedFile("test.txt");
        Crypto::SHA512& sha512 = signedFile.Hash();
        sha512.Reset();
        signedFile.CreateFileData('B', 25);
        EXPECT_EQ(signedFile.CalculateHash(0, signedFile.FileSize()), Core::ERROR_NONE);

        // Read current context from SHA512 handle
        Crypto::SHA512::Context context = sha512.CurrentContext();

        // Save current context to file
        uint32_t contextLength = sizeof(Crypto::SHA512::Context);
        signedFile.CreateHashFile("hashFile", reinterpret_cast<uint8_t*>(&context), contextLength);
        signedFile.CloseHashFile();

        // Read back current context to memory
        signedFile.ReadHashFile(reinterpret_cast<uint8_t*>(&context), contextLength);
        signedFile.DestroyHashFile();

        // Load context back to SHA512 handle
        sha512.Reset();
        sha512.Load(context);

        // Append some more data to file
        uint8_t data[100];
        memset(data, 'C', sizeof(data));
        int64_t start = signedFile.FileSize();
        signedFile.AppendFileData(data, sizeof(data));

        // Calculate final hash
        EXPECT_EQ(signedFile.CalculateHash(start, signedFile.FileSize() - start), Core::ERROR_NONE);

        uint8_t thunderHMAC[Crypto::HASH_SHA512];
        memcpy(thunderHMAC, sha512.Result(), Crypto::HASH_SHA512);

        EXPECT_STREQ(HashBytesToString(thunderHMAC, Crypto::HASH_SHA512).c_str(),
                     ExecuteCmd("sha512sum test.txt").c_str());
        signedFile.DestroyFile();
    }
}
}
