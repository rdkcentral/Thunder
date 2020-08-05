#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <openssl/aes.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/modes.h>
#include <openssl/rand.h>

using namespace std;

const uint8_t g_key[16] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 
                            0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x11 };

struct NetflixData
{
    uint8_t m_salt[16];
    uint8_t m_kpe[16];
    uint8_t m_kph[32];
    uint8_t m_esn[];
} __attribute__((packed));

void DecodeBase64(const char input[], uint8_t output[], uint32_t outputSize)
{
    BIO *bio, *b64;

    int inputLength = strlen(input);

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new_mem_buf(input, inputLength);
    bio = BIO_push(b64, bio);
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    BIO_read(bio, output, inputLength);

    BIO_free_all(bio);
}

void EncodeAes(const uint8_t input[], uint8_t output[], uint32_t bufferSize, const uint8_t iv[16])
{
    unsigned char localIv[AES_BLOCK_SIZE];
    memcpy(localIv, iv, 16);

    unsigned char ecount[AES_BLOCK_SIZE];
    memset(ecount, 0, 16); 

    unsigned int num = 0;

    AES_KEY aesKey;
    AES_set_encrypt_key(g_key, 128, &aesKey);
    CRYPTO_ctr128_encrypt(input, output, bufferSize, &aesKey, localIv, ecount, &num, reinterpret_cast<block128_f>(AES_encrypt));
}

int main(int argc, const char * argv[])
{
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " [input file] [output file]" << endl;
        cerr << "Example:" << endl;
        cerr << "   " << argv[0] << " idfile netflix-vault.bin" << endl;
        return EINVAL;
    }

    // Initialize OpenSSL's RNG.
    RAND_poll();

    ifstream inFile(argv[1]);
    if (!inFile) {
        cerr << "Failed to open " << argv[1] << " for reading" << endl;
        return ENOENT;
    }

    string line;

    // Read ESN.
    std::getline(inFile, line);
    uint32_t esnSize = line.length();

    // Allocate buffer.
    uint32_t totalBufferSize = sizeof(NetflixData) + esnSize;
    NetflixData * netflixData = static_cast<NetflixData *>(malloc(totalBufferSize));
    memset(netflixData, 0, totalBufferSize);

    // Copy ESN.
    strncpy(reinterpret_cast<char *>(netflixData->m_esn), line.c_str(), esnSize);

    // Create random IV vector and salt.
    uint8_t iv[16];

    RAND_bytes(iv, sizeof(iv));
    RAND_bytes(netflixData->m_salt, sizeof(netflixData->m_salt));

    // Read KPE (base 64 encoded) and store in NetflixData struct.
    const uint32_t expectedKpeB64Length = 24;
    std::getline(inFile, line);
    if (line.length() != expectedKpeB64Length) {
        cerr << "Expected KPE to be " << expectedKpeB64Length << " chars long, got " << line.length() << " instead." << endl;
        return EINVAL;
    }

    DecodeBase64(line.c_str(), netflixData->m_kpe, sizeof(netflixData->m_kpe));

    // Read KPH (base 64 encoded) and store in NetflixData struct.
    const uint32_t expectedKphB64Length = 44;
    std::getline(inFile, line);
    if (line.length() != expectedKphB64Length) {
        cerr << "Expected KPH to be " << expectedKphB64Length << " chars long, got " << line.length() << " instead." << endl;
        return EINVAL;
    }

    DecodeBase64(line.c_str(), netflixData->m_kph, sizeof(netflixData->m_kph));

    // Encrypt NetflixData buffer.
    uint8_t * encryptBuffer = static_cast<uint8_t *>(malloc(totalBufferSize));
    memset(encryptBuffer, 0, totalBufferSize);
    EncodeAes(reinterpret_cast<uint8_t *>(netflixData), encryptBuffer, totalBufferSize, iv);

    ofstream outFile(argv[2], ofstream::binary);
    if (!outFile) {
        cerr << "Failed to open " << argv[2] << " for writing" << endl;
        return ENOENT;
    }

    // Write IV followed by encrypted NetflixData.
    outFile.write(reinterpret_cast<const char *>(iv), sizeof(iv));
    outFile.write(reinterpret_cast<const char *>(encryptBuffer), totalBufferSize);

    outFile.close();
    inFile.close();

    cout << "SUCCESS: Written netflix vault to " << argv[2] << endl;

    free(encryptBuffer);
    free(netflixData);

    return 0;
}
