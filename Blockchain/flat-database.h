// Copyright (c) 2014-2017 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef FLAT_DATABASE_H
#define FLAT_DATABASE_H

#include "chainparams.h"
#include "clientversion.h"
#include "hash.h"
#include "streams.h"
#include "util.h"
#include "Log.h"

#include <boost/filesystem.hpp>

/** 
*   Generic Dumping and Loading
*   ---------------------------
*/

template<typename T>
class CFlatDB
{
private:

    enum ReadResult {
        Ok,
        FileError,
        HashReadError,
        IncorrectHash,
        IncorrectMagicMessage,
        IncorrectMagicNumber,
        IncorrectFormat
    };

    boost::filesystem::path pathDB;
    std::string strFilename;
    std::string strMagicMessage;

    bool Write(const T& objToSave)
    {
        // LOCK(objToSave.cs);

        int64_t nStart = GetTimeMillis();

        // serialize, checksum data up to that point, then append checksum
        CDataStream ssObj(SER_DISK, CLIENT_VERSION);
        ssObj << strMagicMessage; // specific magic message for this type of object
        ssObj << FLATDATA(Params().MessageStart()); // network specific magic number
        ssObj << objToSave;
        uint256 hash = Hash(ssObj.begin(), ssObj.end());
        ssObj << hash;

        // open output file, and associate with CAutoFile
        FILE *file = fopen(pathDB.string().c_str(), "wb");
        CAutoFile fileout(file, SER_DISK, CLIENT_VERSION);
		if (fileout.IsNull()) {
			LOG_ERROR("%s: Failed to open file %s", __func__, pathDB.string());
			return false;
		}

        // Write and commit header, data
        try {
            fileout << ssObj;
        }
        catch (std::exception &e) {
			LOG_ERROR("%s: Serialize or I/O error - %s", __func__, e.what());
			return false;
        }
        fileout.fclose();

        LOG_INFO("Written info to %s  %dms\n", strFilename, GetTimeMillis() - nStart);
        LOG_INFO("     %s\n", objToSave.ToString());

        return true;
    }

    ReadResult Read(T& objToLoad, bool fDryRun = false)
    {
        //LOCK(objToLoad.cs);

        int64_t nStart = GetTimeMillis();
        // open input file, and associate with CAutoFile
        FILE *file = fopen(pathDB.string().c_str(), "rb");
        CAutoFile filein(file, SER_DISK, CLIENT_VERSION);
        if (filein.IsNull())
        {
			LOG_ERROR("%s: Failed to open file %s", __func__, pathDB.string());
            return FileError;
        }

        // use file size to size memory buffer
        int fileSize = boost::filesystem::file_size(pathDB);
        int dataSize = fileSize - sizeof(uint256);
        // Don't try to resize to a negative number if file is small
        if (dataSize < 0)
            dataSize = 0;
        std::vector<unsigned char> vchData;
        vchData.resize(dataSize);
        uint256 hashIn;

        // read data and checksum from file
        try {
            filein.read((char *)&vchData[0], dataSize);
            filein >> hashIn;
        }
        catch (std::exception &e) {
			LOG_ERROR("%s: Deserialize or I/O error - %s", __func__, e.what());
            return HashReadError;
        }
        filein.fclose();

        CDataStream ssObj(vchData, SER_DISK, CLIENT_VERSION);

        // verify stored checksum matches input data
        uint256 hashTmp = Hash(ssObj.begin(), ssObj.end());
        if (hashIn != hashTmp)
        {
			LOG_ERROR("%s: Checksum mismatch, data corrupted", __func__);
            return IncorrectHash;
        }


        CMessageHeader::MessageStartChars pchMsgTmp;
        std::string strMagicMessageTmp;
        try {
            // de-serialize file header (file specific magic message) and ..
            ssObj >> strMagicMessageTmp;

            // ... verify the message matches predefined one
            if (strMagicMessage != strMagicMessageTmp)
            {
				LOG_ERROR("%s: Invalid magic message", __func__);
                return IncorrectMagicMessage;
            }


            // de-serialize file header (network specific magic number) and ..
            ssObj >> FLATDATA(pchMsgTmp);

            // ... verify the network matches ours
            if (pchMsgTmp != Params().MessageStart())
            {
				LOG_ERROR("%s: Invalid network magic number", __func__);
                return IncorrectMagicNumber;
            }

            // de-serialize data into T object
            ssObj >> objToLoad;
        }
        catch (std::exception &e) {
            objToLoad.Clear();
			LOG_ERROR("%s: Deserialize or I/O error - %s", __func__, e.what());
            return IncorrectFormat;
        }

        LOG_INFO("Loaded info from %s  %dms\n", strFilename, GetTimeMillis() - nStart);
        LOG_INFO("     %s\n", objToLoad.ToString());
        if(!fDryRun) {
            LOG_INFO("%s: Cleaning....\n", __func__);
            objToLoad.CheckAndRemove();
            LOG_INFO("     %s\n", objToLoad.ToString());
        }

        return Ok;
    }


public:
    CFlatDB(std::string strFilenameIn, std::string strMagicMessageIn)
    {
        pathDB = GetDataDir() / strFilenameIn;
        strFilename = strFilenameIn;
        strMagicMessage = strMagicMessageIn;
    }

    bool Load(T& objToLoad)
    {
        LOG_INFO("Reading info from %s...\n", strFilename);
        ReadResult readResult = Read(objToLoad);
        if (readResult == FileError)
            LOG_INFO("Missing file %s, will try to recreate\n", strFilename);
        else if (readResult != Ok)
        {
            LOG_INFO("Error reading %s: ", strFilename);
            if(readResult == IncorrectFormat)
            {
                LOG_INFO("%s: Magic is ok but data has invalid format, will try to recreate\n", __func__);
            }
            else {
                LOG_INFO("%s: File format is unknown or invalid, please fix it manually\n", __func__);
                // program should exit with an error
                return false;
            }
        }
        return true;
    }

    bool Dump(T& objToSave)
    {
        int64_t nStart = GetTimeMillis();

        LOG_INFO("Verifying %s format...\n", strFilename);
        T tmpObjToLoad;
        ReadResult readResult = Read(tmpObjToLoad, true);

        // there was an error and it was not an error on file opening => do not proceed
        if (readResult == FileError)
            LOG_INFO("Missing file %s, will try to recreate\n", strFilename);
        else if (readResult != Ok)
        {
            LOG_INFO("Error reading %s: ", strFilename);
            if(readResult == IncorrectFormat)
                LOG_INFO("%s: Magic is ok but data has invalid format, will try to recreate\n", __func__);
            else
            {
                LOG_INFO("%s: File format is unknown or invalid, please fix it manually\n", __func__);
                return false;
            }
        }

        LOG_INFO("Writting info to %s...\n", strFilename);
        Write(objToSave);
        LOG_INFO("%s dump finished  %dms\n", strFilename, GetTimeMillis() - nStart);

        return true;
    }

};


#endif
