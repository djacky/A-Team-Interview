
const { decrypt, getPriceInUSD, getSecret } = require('../utils/helperFunctions');
var jwt = require('jsonwebtoken');
const { CLIENT_GAME_TOKEN_PASS, ADMIN_WALLET, AWS_SECRET, ADMIN_NFT_FEE_USD } = require('../config');
const { responseSuccess, responseServerSideError } = require('../utils/responseTypes')
const { sendSimpleTransaction } = require('../services/blockchain.service');   
const { checkWeaponName } = require('../helpers/nft.helper');  
const { User } = require('../models')
const CONSTANTS = require('../constants')

const sendTokens = async (req, res) => {
    try 
    {
        const user = await User.findOne({accountId: req.tokenData.accountId})
        //if (!user || (user && !user.accessInfo.passedKyc))
        if (!user)
        {
            return responseServerSideError(res, {MESSAGE: "Invalid user, or KYC not passed"});
        }

        // Decrypt the private key
        const { PRIVATE_PEM } = await getSecret(AWS_SECRET);
        const privateKey = await decrypt(req.body.Key, req.body.IV, req.body.Cipher, PRIVATE_PEM);

        // Send the transaction
        let response = await sendSimpleTransaction(
            req.body.network, 
            privateKey, 
            req.body.toAddress, 
            req.body.amount);
        const txHash = response.txHash;
        return response.success ? responseSuccess(res, {txHash}) : responseServerSideError(res, {MESSAGE: response.error});
    } 
    catch (error) 
    {
        return responseServerSideError(res, {MESSAGE: error.message});
    }
}

const sendNFTAdminFee = async (req, res) => {
    try 
    {
        // First check for duplicate of weapon name
        const duplicate = await checkWeaponName(req);
        if (duplicate)
        {
            return responseServerSideError(res, { duplicate });
        }
        // Decrypt the private key
        const { PRIVATE_PEM } = await getSecret(AWS_SECRET);
        const privateKey = await decrypt(req.body.key, req.body.iv, req.body.cipher, PRIVATE_PEM);

        const usdPrice = await getPriceInUSD(req.body.network);
        const fee = parseFloat(ADMIN_NFT_FEE_USD) / usdPrice;
        //const toAddress = "0x8aBde6c55ca701CcEEd8FA47896c66919C59372C";
        // Send the transaction
        const transactionResponse = await sendSimpleTransaction(
            req.body.network,
            privateKey, 
            ADMIN_WALLET, 
            fee);
        if (transactionResponse.success)
        {
            const txHash = transactionResponse.txHash;
            const timeNow = Date.now().toString();
            await User.updateOne(
            { accountId: req.tokenData.account_id },
            { 
                $set: { 
                'nftFeeId': txHash + '-' + timeNow,
                'nftUploadStatus': "pending"
                }
            }
            );
            return responseSuccess(res, {txHash, timeNow});
        }
        else
        {
            return responseServerSideError(res, {MESSAGE: transactionResponse.logs[0]});
        }
    } 
    catch (error) 
    {
        return responseServerSideError(res, {MESSAGE: error.message});
    }
}

const NFTFeeNotify = async (req, res) => {
    try 
    {
        const usdPrice = await getPriceInUSD(req.body.network);
        const fee = parseFloat(ADMIN_NFT_FEE_USD) / usdPrice;
        const feeInfo = {"nativeFee": fee.toFixed(7), "usdFee": ADMIN_NFT_FEE_USD};
        responseSuccess(res, {feeInfo})
    } 
    catch (error) 
    {
        return responseServerSideError(res, {MESSAGE: error.message});
    }
}

const updateTransactionId = async (req, res) => {
    try 
    {
        const response = await fetch(CONSTANTS.AUTH_INFO_URL, {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: `token=${req.headers.token}`});
        const data = await response.json();
        if(!("active" in data) || data.active != true){
            return responseServerSideError(res, {MESSAGE: "UNAUTHORIZED_CLIENT"});
        }
        await User.updateOne(
            { accountId: data.account_id },
            { 
                $set: { 
                'nftFeeId': '',
                'nftUploadStatus': "completed"
                }
            }
          );
        return responseSuccess(res, {});
    } 
    catch (error) 
    {
        return responseServerSideError(res, {MESSAGE: error.message});
    }
}


module.exports = {
    sendTokens,
    sendNFTAdminFee,
    NFTFeeNotify,
    updateTransactionId
}