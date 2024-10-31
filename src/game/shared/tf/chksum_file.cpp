#include "cbase.h"
#include "chksum_file.h"
#include "utlbuffer.h"
#include <checksum_sha1.h>
#include <checksum_md5.h>
#include <xxhash/xxhash.h>

bool CChecksum::MD5__File(const char* file, char* outstring, const char* searchpath /* = "GAME" */)
{
    // boiler
    CUtlBuffer filebuf;
    if (!filesystem->ReadFile(file, searchpath, filebuf))
    {
        Warning("Couldn't read file!\n");
        return false;
    }
    FileHandle_t fh = filesystem->Open(file, "rb", searchpath);

    // modified from CContentControlDialog::HashPassword
    // init our md5ctx
    MD5Context_t ctx;
    MD5Init(&ctx);

    // loop thru file in 8192 byte chunks
    unsigned char buf[8192] = {};
    size_t len = 0;

    while ((len = filesystem->Read((void*)buf, sizeof(buf), fh)) != 0)
    {
        // update hash each chunk
        MD5Update(&ctx, (unsigned char const*)buf, len);
    }

    // close our file
    filesystem->Close(fh);

    // finalize hash and stick raw bytes of our hash this char
    unsigned char md5hash[MD5_DIGEST_LENGTH] = {};
    MD5Final(md5hash, &ctx);

    // convert it to a hex string
    char md5_hexstring[MD5_charsize] = {};
    V_binarytohex(md5hash, sizeof(md5hash), md5_hexstring, sizeof(md5_hexstring));

    // copy it to our outstring
    V_strncpy(outstring, md5_hexstring, sizeof(md5_hexstring));

    return true;
}

bool CChecksum::SHA1__File(const char* file, char* outstring, const char* searchpath /* = "GAME" */)
{
    // boiler
    CUtlBuffer filebuf;
    if (!filesystem->ReadFile(file, searchpath, filebuf))
    {
        Warning("Couldn't read buffer!\n");
        return false;
    }
    FileHandle_t fh = filesystem->Open(file, "rb", searchpath);

    // init our sha1 ctx
    CSHA1 sha1;
    sha1.Reset();

    // loop thru file in 8192 byte chunks
    unsigned char buf[8192];
    size_t len = 0;

    while ((len = filesystem->Read((void*)buf, sizeof(buf), fh)) != 0)
    {
        // update hash each chunk
        sha1.Update(buf, len);
    }

    // close our file
    filesystem->Close(fh);

    // finalize hash
    sha1.Final();

    // stick raw bytes of our hash in here
    unsigned char sha1_outhash[SHA1_charsize] = {};
    sha1.GetHash(sha1_outhash);

    // convert our hash to a hex string
    char sha1_hexstring[SHA1_charsize];
    V_binarytohex(sha1_outhash, sizeof(sha1_outhash), sha1_hexstring, sizeof(sha1_hexstring));

    // copy it to our outstring
    V_strncpy(outstring, sha1_hexstring, sizeof(sha1_hexstring));

    return true;
}


bool CChecksum::XXH3_64__File(const char* file, char* outstring, const char* searchpath /* = "GAME" */)
{
    // boiler
    CUtlBuffer filebuf;
    if (!filesystem->ReadFile(file, searchpath, filebuf))
    {
        Warning("Couldn't read buffer!\n");
        return false;
    }
    FileHandle_t fh = filesystem->Open(file, "rb", searchpath);

    // Allocate a state struct. Do not just use malloc() or new.
    XXH3_state_t* state = XXH3_createState();
    if (state == NULL)
    {
        Error("Out of memory!");
    }

    // Reset the state to start a new hashing session.
    XXH3_64bits_reset(state);

    // loop thru file in 4096 byte chunks
    char buf[4096];
    size_t len = 0;

    while ((len = filesystem->Read((void*)buf, sizeof(buf), fh)) != 0)
    {
        // update hash each chunk
        XXH3_64bits_update(state, buf, len);
    }

    // Retrieve the finalized hash. This will not change the state.
    XXH64_hash_t result = XXH3_64bits_digest(state);
    // Free the state. Do not use free().
    XXH3_freeState(state);
    
    V_snprintf(outstring, XXH3_64_charsize, "%llx", result);

    return true;
}


bool CChecksum::XXH3_128__File(const char* file, char* outstring, const char* searchpath /* = "GAME" */)
{
    // boiler
    CUtlBuffer filebuf;
    if (!filesystem->ReadFile(file, searchpath, filebuf))
    {
        Warning("Couldn't read buffer!\n");
        return false;
    }
    FileHandle_t fh = filesystem->Open(file, "rb", searchpath);

    // Allocate a state struct. Do not just use malloc() or new.
    XXH3_state_t* state = XXH3_createState();
    if (state == NULL)
    {
        Error("Out of memory!");
        return false;
    }

    // Reset the state to start a new hashing session.
    XXH3_64bits_reset(state);

    // loop thru file in 4096 byte chunks
    char buf[4096];
    size_t len = 0;

    while ((len = filesystem->Read((void*)buf, sizeof(buf), fh)) != 0)
    {
        // update hash each chunk
        XXH3_128bits_update(state, buf, len);
    }

    // Retrieve the finalized hash. This will not change the state.
    XXH128_hash_t result = XXH3_128bits_digest(state);
    // Free the state. Do not use free().
    XXH3_freeState(state);

    V_snprintf(outstring, XXH3_128_charsize, "%.16llx%.16llx", result.high64, result.low64);

    return true;
}