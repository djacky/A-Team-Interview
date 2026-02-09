const { getSignature, signTransaction, mintNFT, getBalance } = require('../services/blockchain.service')
const { getWalletAddressFromPrivateKey, convertUsdToWei, decrypt, getSecret } = require('../utils/helperFunctions')
const { NFT_CONTRACT_ADDRESS_ETH, NFT_CONTRACT_ADDRESS_POLYGON, RPC_ETH, RPC_POLYGON, AWS_SECRET } = require('../config')
const nftAbi = require('../../abis/nft.json')
const ERRORS = require('../utils/errorTypes')
const { NFT, User } = require('../models')
const CONSTANTS = require('../constants')


const createNewNFTRequest = (nftRequestDetails, options) => {
	return NFT.createNewNFTRequest(nftRequestDetails, options)
}

const fetchNFTsByQuery = (findQuery, options) => {

	return NFT.fetchNFTsByQuery(findQuery, options)
}

const getNFTById = (findQuery, options) => {
	return NFT.fetchNFTByUniqueKeys(findQuery, options)
}

const checkNFTName = (name) => {
	return NFT.checkNFTName(name)
}

const updateRequestDetailsById = (requestId, requestUpdates, options) => {
	return NFT.updateNFTByUniqueKeys({ _id: requestId }, requestUpdates, options)
}

const mintNFTHelper = async (req, options) => {
	const { id } = req.params
	const { INFURA_KEY, PRIVATE_PEM } = await getSecret(AWS_SECRET);
    const mintRequest = await NFT.fetchNFTByUniqueKeys({_id : id}, options)
	const privKey = await decrypt(req.body.key, req.body.iv, req.body.cipher, PRIVATE_PEM)
	const walletAddress = getWalletAddressFromPrivateKey(privKey)
	const user = await User.fetchUserByUniqueKeys({_id: mintRequest.creator}, options)
	if(user && user.walletAddress != walletAddress){
		throw new Error(ERRORS.INVALID_WALLET_ADDRESS.CODE)
	}

	let signature, transactionSigned
	if(mintRequest.network === CONSTANTS.ETH_NETWORK){
		signature = await getSignature(NFT_CONTRACT_ADDRESS_ETH, RPC_ETH + INFURA_KEY, walletAddress)
		transactionSigned = await signTransaction(NFT_CONTRACT_ADDRESS_ETH, nftAbi, RPC_ETH + INFURA_KEY, privKey, CONSTANTS.ETH_NETWORK)
	}else if(mintRequest.network === CONSTANTS.POLYGON_NETWORK){
		signature = await getSignature(NFT_CONTRACT_ADDRESS_POLYGON, RPC_POLYGON + INFURA_KEY, walletAddress)
		transactionSigned = await signTransaction(NFT_CONTRACT_ADDRESS_POLYGON, nftAbi, RPC_POLYGON + INFURA_KEY, privKey, CONSTANTS.POLYGON_NETWORK)
	}
	const {tx, receipt} = await mintNFT(transactionSigned.contractSigned, transactionSigned.maxFeePerGas, transactionSigned.maxPriorityFeePerGas, signature.signature, signature.nonce, mintRequest.amount, req.body.pinataUri)
	let tokenId
	for (const event of receipt.events) {
		if (event.event === 'TokenMinted') {
			tokenId = event.args.id;
		}
	}
	return NFT.updateNFTByUniqueKeys({ _id: id }, {pinataUri: req.body.pinataUri, txHash:tx.hash, tokenId: Number(tokenId), status: CONSTANTS.NFT_REQUEST_STATUS_MINTED, owners: [{user:mintRequest.creator, balance:mintRequest.amount}]}, options)
}

async function checkWeaponName(req)
{
    try {
        const duplicate = await checkNFTName(req.body.name);
        return duplicate;
    } catch (error) {
        console.log(error);
        return false;
    }
}

module.exports = {
	createNewNFTRequest,
	fetchNFTsByQuery,
	getNFTById,
	updateRequestDetailsById,
	mintNFTHelper,
	checkNFTName,
	checkWeaponName
}
