const { Web3 }  = require('web3')
const ethers = require('ethers')

const ERRORS = require('../utils/errorTypes')
const CONSTANTS = require('../constants')
const { AWS_SECRET, RPC_ETH, RPC_POLYGON, RPC_POLYGON_TEST } = require('../config')
const { getSecret } = require('../utils/helperFunctions')


const getSignature = async (contractAddress, rpc, walletAddress) => {
    try {
        //old public key: 0x2219c22fAE04B198D4A0B61feAa763c33FB2cfD4
        // 0x477030e2709c58e6f379f5c4ad796cca47ab3bb6
        let { VERIFIER_PRIVATE_KEY } = await getSecret(AWS_SECRET);
        const web3 = new Web3(rpc)
        const nonce = await web3.eth.getTransactionCount(walletAddress) //Need to increment this for each new transfer
        const message = web3.utils.soliditySha3({ t: 'address', v: contractAddress }, { t: 'address', v: walletAddress }, { t: 'uint256', v: nonce }).toString('hex')
        const { signature } = web3.eth.accounts.sign(message, "0x" + VERIFIER_PRIVATE_KEY)
        return { signature, nonce }
    } catch (error) {
        console.log(error);
        throw new Error(ERRORS.SIGNATURE_FAILED.CODE);
    }
}

const getBalance = async (walletAddress, rpc) => {
    try {
        const providerNetwork = new ethers.providers.JsonRpcProvider(rpc);
        const balance = await providerNetwork.getBalance(walletAddress);
        return Number(balance);
    } catch (error) {
        throw new Error(ERRORS.SIGNATURE_FAILED.CODE)
    }
}

const signTransaction = async (contractAddress, abi, rpc, privateKey, network, gasBuffer = 1) => {
    try {
        let maxFeePerGas, maxPriorityFeePerGas
        const providerNetwork = new ethers.providers.JsonRpcProvider(rpc)
        const walletPrivateKey = new ethers.Wallet(privateKey)
        const wallet = walletPrivateKey.connect(providerNetwork)
        if(network == CONSTANTS.POLYGON_NETWORK){
            const response = await fetch(CONSTANTS.POLYGON_GAS_STATION_URL, {method: 'GET'})
            const data = await response.json()
            maxFeePerGas = ethers.utils.parseUnits(
                Math.ceil(data.fast.maxFee * gasBuffer) + '',
                'gwei'
            )
            maxPriorityFeePerGas = ethers.utils.parseUnits(
                Math.ceil(data.fast.maxPriorityFee * gasBuffer) + '',
                'gwei'
            )
        }else{
            const feeData = await providerNetwork.getFeeData()
            maxFeePerGas = Math.ceil(Number(feeData.maxFeePerGas) * gasBuffer)
            maxPriorityFeePerGas = Math.ceil(Number(feeData.maxPriorityFeePerGas) * gasBuffer)
        }
        const contract = new ethers.Contract(contractAddress, abi, providerNetwork)
        const contractSigned = contract.connect(wallet)
        console.log(contractSigned)
        return {
            contractSigned,
            maxFeePerGas,
            maxPriorityFeePerGas
        }
    } catch (error) {
        console.log(error)
        throw new Error(ERRORS.TRANSACTION_SIGN_FAILED.CODE)
    }
}

const sendStake = async (contract, maxFeePerGas, maxPriorityFeePerGas, signature, nonce, stakeAmount, matchId, decimals) => {
    try {
        const amountWei = ethers.utils.parseUnits(stakeAmount.toString(), decimals);
        const tx = await contract.setStake(signature, nonce, matchId, 0, {
            maxFeePerGas: maxFeePerGas,
            maxPriorityFeePerGas: maxPriorityFeePerGas,
            value: amountWei
        })
        const receipt = await tx.wait()
        return {status:true, tx, receipt}
    } catch (error) {
        if(error.code == ERRORS.UNPREDICTABLE_GAS_LIMIT.CODE){
            throw new Error(error.error.reason)
        }
        console.log(error)
        throw new Error(error.code)
    }
}

const revokeStake = async (contract, maxFeePerGas, maxPriorityFeePerGas, signature, nonce, matchId) => {
    try {
        const tx = await contract.revokeStake(signature, nonce, matchId, 0, {
            maxFeePerGas: maxFeePerGas,
            maxPriorityFeePerGas: maxPriorityFeePerGas
        })
        const receipt = await tx.wait()
        return {status:true, tx, receipt}
    } catch (error) {
        if(error.code == ERRORS.UNPREDICTABLE_GAS_LIMIT.CODE){
            throw new Error(error.error.reason)
        }
        console.log(error)
        throw new Error(error.code)
    }
}

const sendCustomFee = async (contract, maxFeePerGas, maxPriorityFeePerGas, signature, nonce, tourFee, matchId, decimals) => {
    try {
        const amountWei = ethers.utils.parseUnits(tourFee.toString(), decimals);
        const tx = await contract.setStake(signature, nonce, matchId, 0, {
            maxFeePerGas: maxFeePerGas,
            maxPriorityFeePerGas: maxPriorityFeePerGas,
            value: amountWei
        })
        const receipt = await tx.wait()
        return {status:true, tx, receipt}
    } catch (error) {
        if(error.code == ERRORS.UNPREDICTABLE_GAS_LIMIT.CODE){
            throw new Error(error.error.reason)
        }
        console.log(error)
        throw new Error(error.code)
    }
}

const sendWinningsAll = async (contract, maxFeePerGas, maxPriorityFeePerGas, matchId, addresses, percentages, flagged) => {
    try {
        const tx = await contract.sendAllWinnings(matchId, addresses, percentages, flagged, 0, {
            maxFeePerGas: maxFeePerGas,
            maxPriorityFeePerGas: maxPriorityFeePerGas
        })
        const receipt = await tx.wait()
        return {status:true, tx, receipt}
    } catch (error) {
        if(error.code == ERRORS.UNPREDICTABLE_GAS_LIMIT.CODE){
            throw new Error(error.error.reason)
        }
        console.log(error)
        throw new Error(error.code)
    }
}

const retrySendWinningsAll = async (contract, maxFeePerGas, maxPriorityFeePerGas, matchId) => {
    try {
        const tx = await contract.retrySendAllWinnings(matchId, 0, {
            maxFeePerGas: maxFeePerGas,
            maxPriorityFeePerGas: maxPriorityFeePerGas
        })
        const receipt = await tx.wait()
        return {status:true, tx, receipt}
    } catch (error) {
        if(error.code == ERRORS.UNPREDICTABLE_GAS_LIMIT.CODE){
            throw new Error(error.error.reason)
        }
        console.log(error)
        throw new Error(error.code)
    }
}

const mintNFT = async (contract, maxFeePerGas, maxPriorityFeePerGas, signature, nonce, amount, tokenUri) => {
    try {
        const tx = await contract.mint(signature, nonce, amount, tokenUri, {
            maxFeePerGas: maxFeePerGas,
            maxPriorityFeePerGas: maxPriorityFeePerGas
        })
        const receipt = await tx.wait()
        return {status:true, tx, receipt}
    } catch (error) {
        if(error.code == ERRORS.UNPREDICTABLE_GAS_LIMIT.CODE){
            throw new Error(error.error.reason)
        }
        console.log(error)
        throw new Error(error.code)
    }
}

const createAuction = async (contract, maxFeePerGas, maxPriorityFeePerGas, signature, nonce, tokenId, quantity, auctionPrice, duration) => {
    try {
        const tx = await contract.createAuction(signature, nonce, tokenId, quantity, auctionPrice, duration, {
            maxFeePerGas: maxFeePerGas,
            maxPriorityFeePerGas: maxPriorityFeePerGas
        })
        const receipt = await tx.wait()
        return {status:true, tx, receipt}
    } catch (error) {
        if(error.code == ERRORS.UNPREDICTABLE_GAS_LIMIT.CODE){
            throw new Error(error.error.reason)
        }
        throw new Error(error.code)
        
    }
}

const placeBid = async (contract, maxFeePerGas, maxPriorityFeePerGas, signature, nonce, auctionId, bidPrice) => {
    try {
        const tx = await contract.placeBid(signature, nonce, auctionId, {
            maxFeePerGas: maxFeePerGas,
            maxPriorityFeePerGas: maxPriorityFeePerGas,
            value: bidPrice
        })
        const receipt = await tx.wait()
        return {status:true, tx, receipt}
    } catch (error) {
        if(error.code == ERRORS.UNPREDICTABLE_GAS_LIMIT.CODE){
            throw new Error(error.error.reason)
        }
        throw new Error(error.code)
    }
}

const endAuction = async (contract, maxFeePerGas, maxPriorityFeePerGas, signature, nonce, auctionId) => {
    try {
        const tx = await contract.endAuction(signature, nonce, auctionId, {
            maxFeePerGas: maxFeePerGas,
            maxPriorityFeePerGas: maxPriorityFeePerGas
        })
        const receipt = await tx.wait()
        return {status:true, tx, receipt}
    } catch (error) {
        if(error.code == ERRORS.UNPREDICTABLE_GAS_LIMIT.CODE){
            throw new Error(error.error.reason)
        }
        throw new Error(error.code)
    }
}

const withdrawBid = async (contract, maxFeePerGas, maxPriorityFeePerGas, signature, nonce, auctionId) => {
    try {
        const tx = await contract.withdrawBid(signature, nonce, auctionId, {
            maxFeePerGas: maxFeePerGas,
            maxPriorityFeePerGas: maxPriorityFeePerGas
        })
        const receipt = await tx.wait()
        return {status:true, tx, receipt}
    } catch (error) {
        if(error.code == ERRORS.UNPREDICTABLE_GAS_LIMIT.CODE){
            throw new Error(error.error.reason)
        }
        throw new Error(error.code)
    }
}

const cancelAuction = async (contract, maxFeePerGas, maxPriorityFeePerGas, signature, nonce, auctionId) => {
    try {
        const tx = await contract.cancelAuction(signature, nonce, auctionId, {
            maxFeePerGas: maxFeePerGas,
            maxPriorityFeePerGas: maxPriorityFeePerGas
        })
        const receipt = await tx.wait()
        return {status:true, tx, receipt}
    } catch (error) {
        if(error.code == ERRORS.UNPREDICTABLE_GAS_LIMIT.CODE){
            throw new Error(error.error.reason)
        }
        throw new Error(error.code)
    }
}

const getAuctionBCDetails = async (contract, auctionId) => {
    try {
        const auctionBC = await contract.auctions(auctionId)
        return auctionBC
    } catch (error) {
        if(error.code == ERRORS.UNPREDICTABLE_GAS_LIMIT.CODE){
            throw new Error(error.error.reason)
        }
        throw new Error(error.code)
    }
}

const pendingReturns = async (contract, auctionId, address) => {
    try {
        const auctionBC = await contract.pendingReturns(auctionId, address)
        return auctionBC
    } catch (error) {
        if(error.code == ERRORS.UNPREDICTABLE_GAS_LIMIT.CODE){
            throw new Error(error.error.reason)
        }
        throw new Error(error.code)
    }
}

const sendSimpleTransaction = async (network, senderPrivateKey, toAddress, amount) => {
    try 
    {
		const { INFURA_KEY } = await getSecret(AWS_SECRET);
        const networkUrls = {
            [CONSTANTS.ETH_NETWORK] : RPC_ETH,
            [CONSTANTS.POLYGON_NETWORK]: RPC_POLYGON,
            [CONSTANTS.POLYGON_NETWORK_TEST]: RPC_POLYGON_TEST
        };

        const infuraUrl = networkUrls[network] + INFURA_KEY;
        const web3 = new Web3(infuraUrl);
        const fromAddress = web3.eth.accounts.privateKeyToAccount(senderPrivateKey).address;
        const nonce = await web3.eth.getTransactionCount(fromAddress, 'pending');
    
        const tx = {
            from: fromAddress,
            to: toAddress,
            value: web3.utils.toWei(amount.toString(), 'ether'),
            gas: 21000,
            gasPrice: await web3.eth.getGasPrice(),
            nonce: nonce
        };
    
        const signedTx = await web3.eth.accounts.signTransaction(tx, senderPrivateKey);
        const receipt = await web3.eth.sendSignedTransaction(signedTx.rawTransaction);
        //console.log('Transaction receipt:', receipt);
        return {"success": !!receipt.status, "txHash": receipt.transactionHash, "error": ""};
    } 
    catch (error) 
    {
        return {"success": false, "txHash": "", "error": error.message};
    }

};

module.exports = {
    getSignature,
    signTransaction,
    sendStake,
    revokeStake,
    sendCustomFee,
    sendWinningsAll,
    retrySendWinningsAll,
    mintNFT,
    getBalance,
    createAuction,
    placeBid,
    getAuctionBCDetails,
    endAuction,
    pendingReturns,
    withdrawBid,
    cancelAuction,
    sendSimpleTransaction
}
