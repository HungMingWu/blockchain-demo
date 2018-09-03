
#include "netbase.h"
#include "masternodeconfig.h"
#include "util.h"
#include "chainparams.h"

#include "coins.h"
#include "main.h"
#include "Log.h"

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

CMasternodeConfig masternodeConfig;

void CMasternodeConfig::add(std::string alias, std::string ip, std::string privKey, std::string txHash, std::string outputIndex) {
    CMasternodeEntry cme(alias, ip, privKey, txHash, outputIndex);
    entries.push_back(cme);
}

bool CMasternodeConfig::read(std::string& strErr) {


    bool masternodeflag = GetBoolArg("-masternode", false);
    if(masternodeflag)
    {
        std::string alias, ip, privKey, txHash, outputIndex;
        alias = GetArg("-alias", "");
        if(alias.empty())
        {
            strErr = "please add your masternode name into ulord.conf; for example: alias=mynode\n";
            return false;
        }
        ip = GetArg("-externalip", "");
        if(ip.empty())
        {
            strErr = "Invalid masternode ip, please add your ip into ulord.conf; for example: externalip=0.0.0.0\n";
            return false;
        }
        ip = ip + ":" + std::to_string(Params().GetDefaultPort());
        
        privKey = GetArg("-masternodeprivkey", "");
        if(privKey.empty())
        {
            strErr = "Invalid masternode privKey, please add your privKey into ulord.conf; for example: masternodeprivkey=***\n";
            return false;
        }
        txHash = GetArg("-collateraloutputtxid", "");
        if(txHash.empty())
        {
            strErr = "Invalid masternode collateral txid, please add your collateral txid into ulord.conf; for example: collateraloutputtxid=***\n";
            return false;
        }

        outputIndex = GetArg("-collateraloutputindex", "");
        if(outputIndex.empty())
        {
            strErr = "Invalid masternode collateral Index, please add your collateral Index into ulord.conf; for example: collateraloutputindex=0\n";
            return false;
        }
        
        int port = 0;
        std::string hostname = "";
        SplitHostPort(ip, port, hostname);
        if(port == 0 || hostname == "") {
            strErr = "Failed to parse host:port string\n";
            return false;
        }
        int mainnetDefaultPort = Params(CBaseChainParams::MAIN).GetDefaultPort();
        if(Params().NetworkIDString() == CBaseChainParams::MAIN) {
            if(port != mainnetDefaultPort) {
                strErr = "Invalid port detected in ulord.conf\n" +
                        fmt::format("Port: %d", port) + "\n" +
                        fmt::format("(must be %d for mainnet)", mainnetDefaultPort);
                return false;
            }
        } else if(port == mainnetDefaultPort) {
            strErr = "Invalid port detected in ulord.conf\n" +
                    fmt::format("(%d could be used only on mainnet)", mainnetDefaultPort);
            return false;
        }
            
        add(alias, ip, privKey, txHash, outputIndex);
    }
    return true;
}

CMasternodeConfig::CMasternodeEntry CMasternodeConfig::GetLocalEntry()
{
	if(fMasterNode)
	{
		for(auto & mn : entries)
		{
			if(mn.getPrivKey() == GetArg("-masternodeprivkey", ""))
				return mn;
		}
	}
	return CMasternodeEntry();
}



bool CMasternodeConfig::AvailableCoins(uint256 txHash, unsigned int index)
{
    boost::optional<CTransaction> tx;
    boost::optional<uint256> hashBlock;
    std::tie(tx, hashBlock) = GetTransaction(txHash, Params().GetConsensus(), true);
    if (!tx)
    {
        LOG_INFO("CMasternodeConfig::AvailableCoins -- masternode collateraloutputtxid or collateraloutputindex is error,please check it\n");
        return false;
    }
    if (!CheckFinalTx(*tx) || tx->IsCoinBase()) {
        return false;
    }

    Opt<CCoins> coins = pcoinsTip->GetCoins(txHash);
    if (!coins || index >=coins->vout.size() || coins->vout[index].IsNull())
    {
        LOG_INFO("CMasternodeConfig::AvailableCoins -- masternode collateraloutputtxid or collateraloutputindex is error,please check it\n");
        return false;
    }

    const int64_t ct = Params().GetConsensus().colleteral;     // colleteral amount
    if (coins->vout[index].nValue != ct)
    {
        LOG_INFO("CMasternodeConfig::AvailableCoins -- colleteral amount must be:%d, but now is:%d\n", ct, coins->vout[index].nValue);
        return false;
    }

    if(chainActive.Height() - coins->nHeight + 1 < Params().GetConsensus().nMasternodeMinimumConfirmations) 
    {
        LOG_INFO("CMasternodeConfig::AvailableCoins -- Masternode UTXO must have at least %d confirmations\n",Params().GetConsensus().nMasternodeMinimumConfirmations);
        return false;
    }

    return true;
}

bool CMasternodeConfig::GetMasternodeVin(CTxIn& txinRet,  std::string strTxHash, std::string strOutputIndex)
{
    // wait for reindex and/or import to finish
    if (fImporting || fReindex) return false;


    if(strTxHash.empty()) // No output specified, select the one specified by masternodeConfig
    {
        CMasternodeConfig::CMasternodeEntry mne = masternodeConfig.GetLocalEntry();
        unsigned int index = atoi(mne.getOutputIndex().c_str());
        uint256 txHash = uint256S(mne.getTxHash());
        txinRet = CTxIn(txHash, index);
        
        int nInputAge = GetInputAge(txinRet);
        if(nInputAge <= 0)
        {
            LOG_INFO("CMasternodeConfig::GetMasternodeVin -- collateraloutputtxid or collateraloutputindex is not exist,please check it\n");
            return false;
        }

        if(!masternodeConfig.AvailableCoins(txHash, index))
        {
            LOG_INFO("CMasternodeConfig::GetMasternodeVin -- collateraloutputtxid or collateraloutputindex is AvailableCoins,please check it\n");
            return false;
        }
        
        return true;
    }

    // Find specific vin
    uint256 txHash = uint256S(strTxHash);
    int nOutputIndex = atoi(strOutputIndex.c_str());

    txinRet = CTxIn(txHash,nOutputIndex);
    int nInputAge = GetInputAge(txinRet);
    if(nInputAge <= 0)
    {
    	LOG_INFO("CMasternodeConfig::GetMasternodeVin -- collateraloutputtxid or collateraloutputindex is not exist,please check it\n");
        return false;
    }

    if(!masternodeConfig.AvailableCoins(txHash, nOutputIndex))
    {
        LOG_INFO("CMasternodeConfig::GetMasternodeVin -- collateraloutputtxid or collateraloutputindex is AvailableCoins,please check it\n");
        return false;
    }
        
    return true;
}
