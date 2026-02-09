const { getSignature, signTransaction, createAuction, placeBid, getAuctionBCDetails, endAuction, withdrawBid, cancelAuction, pendingReturns, getBalance } = require('../services/blockchain.service')
const { getWalletAddressFromPrivateKey, convertEtherToWei, convertUsdToWei, decrypt, getSecret } = require('../utils/helperFunctions')
const { MARKETPLACE_CONTRACT_ADDRESS_ETH, MARKETPLACE_CONTRACT_ADDRESS_POLYGON, RPC_ETH, RPC_POLYGON, AWS_SECRET } = require('../config')
const marketAbi = require('../../abis/market.json')
const ERRORS = require('../utils/errorTypes')
const CONSTANTS = require('../constants')
const { ethers, BigNumber } = require('ethers');
const { Auction, NFT, User } = require('../models')


const createAuctionHelper = async (req, options) => {
	const { nftId } = req.params
    let auctionPrice, priceinEth
    const { INFURA_KEY, PRIVATE_PEM } = await getSecret(AWS_SECRET);

    const nft = await NFT.fetchNFTByUniqueKeys({_id : nftId}, options)
    
    const privKey = await decrypt(req.body.key, req.body.iv, req.body.cipher, PRIVATE_PEM)
	const walletAddress = getWalletAddressFromPrivateKey(privKey)
    //const response = await fetch(CONSTANTS.AUTH_INFO_URL, {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: `token=${req.headers.token}`})
    //const data = await response.json()
    
    let user = await User.fetchUserByUniqueKeys({accountId: req.tokenData.account_id}, options)
    if(user && user.walletAddress != walletAddress){
        throw new Error(ERRORS.WALLET_ADDRESS_NOT_REGISTERED.CODE)
	}
    
    priceinEth = await convertUsdToWei(req.body.auctionPrice, nft.network)//req.body.auctionPrice
    auctionPrice = await convertEtherToWei(priceinEth)

    const fromOwner = nft.owners.find(owner => owner.user.toString() === user._id.toString());
    if (!fromOwner) {
        throw new Error(ERRORS.INSUFFICIENT_NFT_BALANCE.CODE);
    }
    if(fromOwner.balance - fromOwner.amountAuctioned < req.body.quantity){
        throw new Error(ERRORS.INSUFFICIENT_NFT_BALANCE.CODE)
    }

    let signature, transactionSigned
	if(nft.network === CONSTANTS.ETH_NETWORK){
        signature = await getSignature(MARKETPLACE_CONTRACT_ADDRESS_ETH, RPC_ETH + INFURA_KEY, walletAddress)
        transactionSigned = await signTransaction(MARKETPLACE_CONTRACT_ADDRESS_ETH, marketAbi, RPC_ETH + INFURA_KEY, privKey, CONSTANTS.ETH_NETWORK)
    }else if(nft.network === CONSTANTS.POLYGON_NETWORK){
        signature = await getSignature(MARKETPLACE_CONTRACT_ADDRESS_POLYGON, RPC_POLYGON + INFURA_KEY, walletAddress)
        transactionSigned = await signTransaction(MARKETPLACE_CONTRACT_ADDRESS_POLYGON, marketAbi, RPC_POLYGON + INFURA_KEY, privKey, CONSTANTS.POLYGON_NETWORK)
    }
    // tokenId, quantity, auctionPrice, duration
	const {tx, receipt} = await createAuction(transactionSigned.contractSigned, transactionSigned.maxFeePerGas, transactionSigned.maxPriorityFeePerGas, signature.signature, signature.nonce, 
        nft.tokenId, req.body.quantity, auctionPrice, req.body.duration)
	let auctionNumber
	for (const event of receipt.events) {
		if (event.event === 'AuctionCreated') {
			auctionNumber = event.args.auctionId;
		}
	}
    
    // Update the owner's amountAuctioned
    const ownerIndex = nft.owners.findIndex(owner => owner.user.toString() === user._id.toString());
    if (ownerIndex > -1) {
        nft.owners[ownerIndex].amountAuctioned += req.body.quantity;
    }
	const nftUpdated = await NFT.updateNFTByUniqueKeys({ _id: nftId }, {owners: nft.owners}, options)
    const newAuctionDetails = {
        nftData: nftUpdated.toObject(),
        nft: nftId, 
        auctionNumber: auctionNumber,
        seller: user._id,
        quantity: req.body.quantity,
        startingPrice: priceinEth,
        duration: req.body.duration,
        network: nft.network
    }

    return Auction.createNewAuction(newAuctionDetails, options)
}

const fetchAuctionsByQuery = (findQuery, options) => {
	return Auction.fetchAuctionsByQuery(findQuery, options)
}

const getAuctionById = async(auctionId, options) => {
	let findQuery = { _id: auctionId }
	return Auction.fetchAuctionByUniqueKeys(findQuery, options)
}

const placeBidHelper = async (req, options) => {
	const { auctionId } = req.params
    const { INFURA_KEY, PRIVATE_PEM } = await getSecret(AWS_SECRET);
    const auction = await Auction.fetchAuctionByUniqueKeys({_id : auctionId}, options)
    const privKey = await decrypt(req.body.key, req.body.iv, req.body.cipher, PRIVATE_PEM)
    const walletAddress = getWalletAddressFromPrivateKey(privKey)
    const user = await User.fetchUserByUniqueKeys({walletAddress: walletAddress}, options)
    if(!user){
        throw new Error(ERRORS.WALLET_ADDRESS_NOT_REGISTERED.CODE)
    }
    
    let signature, transactionSigned
	if(auction.network === CONSTANTS.ETH_NETWORK){
        signature = await getSignature(MARKETPLACE_CONTRACT_ADDRESS_ETH, RPC_ETH + INFURA_KEY, walletAddress)
        transactionSigned = await signTransaction(MARKETPLACE_CONTRACT_ADDRESS_ETH, marketAbi, RPC_ETH + INFURA_KEY, privKey, CONSTANTS.ETH_NETWORK)
    }else if(auction.network === CONSTANTS.POLYGON_NETWORK){
        signature = await getSignature(MARKETPLACE_CONTRACT_ADDRESS_POLYGON, RPC_POLYGON + INFURA_KEY, walletAddress)
        transactionSigned = await signTransaction(MARKETPLACE_CONTRACT_ADDRESS_POLYGON, marketAbi, RPC_POLYGON + INFURA_KEY, privKey, CONSTANTS.POLYGON_NETWORK)
    }

  
    const priceinEth = await convertUsdToWei(req.body.bidPrice, auction.network)//req.body.auctionPrice
    const bidPrice = await convertEtherToWei(priceinEth)
    
    const currentTimestamp = Math.floor(Date.now() / 1000);
    if(auction.endTime && auction.endTime < currentTimestamp){
        throw new Error(ERRORS.AUCTION_ALREADY_ENDED.CODE)
    }
    
    if(Number(auction.highestBid) >= Number(priceinEth) || Number(auction.startingPrice) > Number(priceinEth)){
        throw new Error(ERRORS.LOW_BID_PRICE.CODE)
    }
    
    const {tx, receipt} = await placeBid(transactionSigned.contractSigned, transactionSigned.maxFeePerGas, transactionSigned.maxPriorityFeePerGas, signature.signature, signature.nonce, 
        auction.auctionNumber, bidPrice)
    const aucBc = await getAuctionBCDetails(transactionSigned.contractSigned, auction.auctionNumber)

    auction.bids.push({ user: user._id, bidAmount: priceinEth });
    
    return await Auction.updateAuctionByUniqueKeys({ _id: auctionId }, {highestBid: priceinEth, 
        highestBidder: user._id,
        endTime: Number(aucBc.endTime),
        bids: auction.bids
    }, options)
}

const endAuctionHelper = async (req, options) => {
	const { auctionId } = req.params
    const { INFURA_KEY, PRIVATE_PEM } = await getSecret(AWS_SECRET);
    const auction = await Auction.fetchAuctionByUniqueKeys({_id : auctionId}, options)
    const privKey = await decrypt(req.body.key, req.body.iv, req.body.cipher, PRIVATE_PEM)
    const walletAddress = getWalletAddressFromPrivateKey(privKey)
    const user = await User.fetchUserByUniqueKeys({walletAddress: walletAddress}, options)

    if(!user){
        throw new Error(ERRORS.WALLET_ADDRESS_NOT_REGISTERED.CODE)
    }else if(user && !user._id.equals(auction.seller)){
        throw new Error(ERRORS.NOT_ALLOWED_TO_END_AUCTION.CODE)
    }

    let signature, transactionSigned
	if(auction.network === CONSTANTS.ETH_NETWORK){
        signature = await getSignature(MARKETPLACE_CONTRACT_ADDRESS_ETH, RPC_ETH + INFURA_KEY, walletAddress)
        transactionSigned = await signTransaction(MARKETPLACE_CONTRACT_ADDRESS_ETH, marketAbi, RPC_ETH + INFURA_KEY, privKey, CONSTANTS.ETH_NETWORK)
    }else if(auction.network === CONSTANTS.POLYGON_NETWORK){
        signature = await getSignature(MARKETPLACE_CONTRACT_ADDRESS_POLYGON, RPC_POLYGON + INFURA_KEY, walletAddress)
        transactionSigned = await signTransaction(MARKETPLACE_CONTRACT_ADDRESS_POLYGON, marketAbi, RPC_POLYGON + INFURA_KEY, privKey, CONSTANTS.POLYGON_NETWORK)
    }
    
    const {tx, receipt} = await endAuction(transactionSigned.contractSigned, transactionSigned.maxFeePerGas, transactionSigned.maxPriorityFeePerGas, signature.signature, signature.nonce, 
        auction.auctionNumber)
    
    const nft = await NFT.findById(auction.nft);
    // Update the owner's balance
    const ownerIndex = nft.owners.findIndex(owner => owner.user.toString() === user._id.toString());
    if (ownerIndex > -1) {
        nft.owners[ownerIndex].amountAuctioned -= auction.quantity;
        nft.owners[ownerIndex].balance -= auction.quantity;

        // Remove the sender from the owners list if their balance is 0
        if (nft.owners[ownerIndex].balance === 0) {
            nft.owners = nft.owners.filter(owner => owner.user.toString() !== nft.owners[ownerIndex].user.toString());
        }
    }
    // Find the recipient in the owners list
    const toOwner = nft.owners.find(owner => owner.user.toString() === auction.highestBidder.toString());

    if (toOwner) {
        // If the recipient already owns some of the NFT, add to their balance
        toOwner.balance += auction.quantity;
    } else {
        // If the recipient is a new owner, add them to the owners list
        nft.owners.push({
            user: auction.highestBidder,
            balance: auction.quantity,
            amountAuctioned: 0 // New owner has no auctioned amount initially
        });
    }


    await NFT.updateNFTByUniqueKeys({ _id: auction.nft }, {owners: nft.owners}, options)
    return await Auction.updateAuctionByUniqueKeys({ _id: auctionId }, { txHash: tx.hash }, options)
}

const getPendingRefundsHelper = async (req, options) => {
    //const response = await fetch(CONSTANTS.AUTH_INFO_URL, {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: `token=${req.headers.token}`})
    //const data = await response.json()
    
    let user = await User.fetchUserByUniqueKeys({accountId: req.tokenData.account_id}, options)

    const { network } = req.query
    let auctions = await Auction.fetchAuctionsByQuery({network: network}, options)
    
    auctions = auctions.map(auction => {
        const userBids = auction.bids.filter(bid => bid.user.toString() === user._id.toString());
        const highestBid = auction.highestBidder ? auction.highestBid : 0;
        const highestBidAmount = auction.bids.reduce((max, bid) => bid.bidAmount > max ? bid.bidAmount : max, 0);
        const userIsHighestBidder = auction.highestBidder && auction.highestBidder._id.toString() === user._id.toString();

        let pendingRefunds = 0;
        if (userIsHighestBidder) {
            // Scenario: User is the highest bidder
            if (userBids.length > 1) {
            // Scenario: User is the highest bidder but has multiple bids
            pendingRefunds = userBids.reduce((total, bid) => bid.bidAmount !== highestBidAmount ? total + bid.bidAmount : total, 0);
            }
        } else {
            // Scenario: User is not the highest bidder
            pendingRefunds = userBids.reduce((total, bid) => total + bid.bidAmount, 0);
        }
        
        const auctionResponse = auction.toObject();
        delete auctionResponse.bids;
        delete auctionResponse.duration;
        delete auctionResponse.txHash;
        if (pendingRefunds > 0) {
            return {
              ...auctionResponse,
              pendingRefunds,
            };
        }else {
            return null;
        }
    }).filter(auction => auction !== null)
  
    return auctions

}

const refundHelper = async (req, options) => {
	const { auctionId } = req.params
    const { INFURA_KEY, PRIVATE_PEM } = await getSecret(AWS_SECRET);
    const auction = await Auction.fetchAuctionByUniqueKeys({_id : auctionId}, options)

    const privKey = await decrypt(req.body.key, req.body.iv, req.body.cipher, PRIVATE_PEM)
    const walletAddress = getWalletAddressFromPrivateKey(privKey)
    const user = await User.fetchUserByUniqueKeys({walletAddress: walletAddress}, options)

    if(!user){
        throw new Error(ERRORS.WALLET_ADDRESS_NOT_REGISTERED.CODE)
    }

    
    let signature, transactionSigned
	if(auction.network === CONSTANTS.ETH_NETWORK){
        signature = await getSignature(MARKETPLACE_CONTRACT_ADDRESS_ETH, RPC_ETH + INFURA_KEY, walletAddress)
        transactionSigned = await signTransaction(MARKETPLACE_CONTRACT_ADDRESS_ETH, marketAbi, RPC_ETH + INFURA_KEY, privKey, CONSTANTS.ETH_NETWORK)
    }else if(auction.network === CONSTANTS.POLYGON_NETWORK){
        signature = await getSignature(MARKETPLACE_CONTRACT_ADDRESS_POLYGON, RPC_POLYGON + INFURA_KEY, walletAddress)
        transactionSigned = await signTransaction(MARKETPLACE_CONTRACT_ADDRESS_POLYGON, marketAbi, RPC_POLYGON + INFURA_KEY, privKey, CONSTANTS.POLYGON_NETWORK)
    }
    
    
    const userBids = auction.bids.filter(bid => bid.user._id.toString() === user._id.toString());
    if (userBids.length === 0) {
        throw new Error(ERRORS.NO_BID_FOUND.CODE);
    }

    let totalRefund = 0;
    let highestBid
    if (user._id.toString() === auction.highestBidder._id.toString() && userBids.length === 1) {
        totalRefund = 0;
    } else if (user._id.toString() !== auction.highestBidder._id.toString()) {
        totalRefund = userBids.reduce((sum, bid) => sum + bid.bidAmount, 0);
    } else if (user._id.toString() === auction.highestBidder._id.toString() && userBids.length > 1) {
        highestBid = Math.max(...userBids.map(bid => bid.bidAmount));
        totalRefund = userBids.reduce((sum, bid) => sum + bid.bidAmount, 0) - highestBid;
    }
    
    if (totalRefund > 0) {
        const {tx, receipt} = await withdrawBid(transactionSigned.contractSigned, transactionSigned.maxFeePerGas, transactionSigned.maxPriorityFeePerGas, signature.signature, signature.nonce, 
            auction.auctionNumber)
        auction.bids = auction.bids.filter(bid => bid.user._id.toString() !== user._id.toString() || (user._id.toString() === auction.highestBidder._id.toString() && bid.bidAmount === highestBid));
        await Auction.updateAuctionByUniqueKeys({ _id: auctionId }, { bids: auction.bids }, options)
        return { auction };
    }else{
        throw new Error(ERRORS.NO_REFUND_FOUND.CODE);
    }



}

const cancelAuctionHelper = async (req, options) => {
	const { auctionId } = req.params
    const { INFURA_KEY, PRIVATE_PEM } = await getSecret(AWS_SECRET);
    const auction = await Auction.fetchAuctionByUniqueKeys({_id : auctionId}, options)

    const privKey = await decrypt(req.body.key, req.body.iv, req.body.cipher, PRIVATE_PEM)
    const walletAddress = getWalletAddressFromPrivateKey(privKey)
    const user = await User.fetchUserByUniqueKeys({walletAddress: walletAddress}, options)

    if(!user){
        throw new Error(ERRORS.WALLET_ADDRESS_NOT_REGISTERED.CODE)
    }else if(user && !user._id.equals(auction.seller)){
        throw new Error(ERRORS.NOT_ALLOWED_TO_CANCEL_AUCTION.CODE)
    }

    if(auction.bids.length > 0){
        throw new Error(ERRORS.UNABLE_TO_CANCEL_AUCTION.CODE)
    }

    let signature, transactionSigned
	if(auction.network === CONSTANTS.ETH_NETWORK){
        signature = await getSignature(MARKETPLACE_CONTRACT_ADDRESS_ETH, RPC_ETH + INFURA_KEY, walletAddress)
        transactionSigned = await signTransaction(MARKETPLACE_CONTRACT_ADDRESS_ETH, marketAbi, RPC_ETH + INFURA_KEY, privKey, CONSTANTS.ETH_NETWORK)
    }else if(auction.network === CONSTANTS.POLYGON_NETWORK){
        signature = await getSignature(MARKETPLACE_CONTRACT_ADDRESS_POLYGON, RPC_POLYGON + INFURA_KEY, walletAddress)
        transactionSigned = await signTransaction(MARKETPLACE_CONTRACT_ADDRESS_POLYGON, marketAbi, RPC_POLYGON + INFURA_KEY, privKey, CONSTANTS.POLYGON_NETWORK)
    }
    
    const {tx, receipt} = await cancelAuction(transactionSigned.contractSigned, transactionSigned.maxFeePerGas, transactionSigned.maxPriorityFeePerGas, signature.signature, signature.nonce, 
        auction.auctionNumber)
    
    const nft = await NFT.findById(auction.nft);
    // Update the owner's balance
    const ownerIndex = nft.owners.findIndex(owner => owner.user.toString() === user._id.toString());
    if (ownerIndex > -1) {
        nft.owners[ownerIndex].amountAuctioned -= auction.quantity;
    }
    await NFT.updateNFTByUniqueKeys({ _id: auction.nft }, {owners: nft.owners}, options)

    return await Auction.updateAuctionByUniqueKeys({ _id: auctionId }, { txHash: tx.hash, ended: true }, options)
}

const getFilterAuctions = async (req, usdRate, options) => {
    const { type, isAutomatic, minFireRate, maxFireRate, minDamage, maxDamage, minProjRadius, maxProjRadius, minProjSpeed, maxProjSpeed, proj_Drop, proj_Bounce, minCapacity, maxCapacity, minPrice, maxPrice, network } = req.query
    let nftFilter = {}

    if (type) nftFilter['offChainMetadata.general.type'] = type;
    if (isAutomatic !== undefined) nftFilter['offChainMetadata.stats.isAutomatic'] = isAutomatic === 'true';  // Convert to boolean
    // FireRate filtering
    if (minFireRate || maxFireRate) {
        nftFilter['offChainMetadata.stats.fireRate'] = {};
        if (minFireRate) nftFilter['offChainMetadata.stats.fireRate'].$gt = parseFloat(minFireRate);
        if (maxFireRate) nftFilter['offChainMetadata.stats.fireRate'].$lt = parseFloat(maxFireRate);
    }

    // Damage filtering
    if (minDamage || maxDamage) {
        nftFilter['offChainMetadata.stats.damage'] = {};
        if (minDamage) nftFilter['offChainMetadata.stats.damage'].$gt = parseFloat(minDamage);
        if (maxDamage) nftFilter['offChainMetadata.stats.damage'].$lt = parseFloat(maxDamage);
    }

    // Projectile Radius filtering
    if (minProjRadius || maxProjRadius) {
        nftFilter['offChainMetadata.stats.proj_Radius'] = {};
        if (minProjRadius) nftFilter['offChainMetadata.stats.proj_Radius'].$gt = parseFloat(minProjRadius);
        if (maxProjRadius) nftFilter['offChainMetadata.stats.proj_Radius'].$lt = parseFloat(maxProjRadius);
    }
    
    // Projectile Speed filtering
    if (minProjSpeed || maxProjSpeed) {
        nftFilter['offChainMetadata.stats.proj_Speed'] = {};
        if (minProjSpeed) nftFilter['offChainMetadata.stats.proj_Speed'].$gt = parseFloat(minProjSpeed);
        if (maxProjSpeed) nftFilter['offChainMetadata.stats.proj_Speed'].$lt = parseFloat(maxProjSpeed);
    }

    // Capacity filtering
    if (minCapacity || maxCapacity) {
        nftFilter['offChainMetadata.stats.capacity'] = {};
        if (minCapacity) nftFilter['offChainMetadata.stats.capacity'].$gt = parseInt(minCapacity, 10);
        if (maxCapacity) nftFilter['offChainMetadata.stats.capacity'].$lt = parseInt(maxCapacity, 10);
    }
    
    if (proj_Drop !== undefined) nftFilter['offChainMetadata.stats.proj_Drop'] = proj_Drop === 'true';  // Convert to boolean
    if (proj_Bounce !== undefined) nftFilter['offChainMetadata.stats.proj_Bounce'] = proj_Bounce === 'true';  // Convert to boolean

    let auctionFilter = {ended: false };
    if (minPrice){
        const priceinEth =  (minPrice / usdRate).toFixed(4); //await convertUsdToWei(minPrice, network)//req.body.auctionPrice
        auctionFilter['startingPrice'] = { ...auctionFilter['startingPrice'], $gt: parseFloat(priceinEth) };  // Greater than filter 
    } 
    if (maxPrice){
        const priceinEth = (maxPrice / usdRate).toFixed(4);//await convertUsdToWei(maxPrice, network)//req.body.auctionPrice
        auctionFilter['startingPrice'] = { ...auctionFilter['startingPrice'], $lt: parseFloat(priceinEth) };  // Less than filter
    } 

    // Query to find auctions based on the NFT's offChainMetadata
    const auctions = await Auction.find(auctionFilter)
    .populate({
        path: 'nft',
        match: nftFilter
    })
    .exec();

    const filteredAuctions = auctions.filter(auction => auction.nft);
    return filteredAuctions

}

module.exports = {
	createAuctionHelper,
    fetchAuctionsByQuery,
    getAuctionById,
    placeBidHelper,
    endAuctionHelper,
    getPendingRefundsHelper,
    refundHelper,
    cancelAuctionHelper,
    cancelAuctionHelper,
    getFilterAuctions
}
