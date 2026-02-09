const { responseSuccess, responseBadRequest, responseUnauthorized, responseServerSideError } = require('../utils/responseTypes')
const { filterEmptyKeys, generateUpdateObject, getEnabledPaginationOption, convertPriceToUsd } = require('../utils/helperFunctions')
const auctionHelper = require('../helpers/auction.helper')
const userHelper = require('../helpers/user.helper')
const ERRORS = require('../utils/errorTypes')
const fetch = require('node-fetch')
const CONSTANTS = require('../constants')


const createAuction = async (req, res) => {
    try {
        const options = { asResponse: true, paginationOptions: { enabled: false }, populate: false }
        let auction = await auctionHelper.createAuctionHelper(req, options)
        const usdRate = await convertPriceToUsd(1, auction.network);

        auction.startingPrice = (auction.startingPrice * usdRate).toFixed(2);//await convertPriceToUsd(auction.startingPrice, auction.network);
        auction.highestBid = (auction.highestBid * usdRate).toFixed(2);//await convertPriceToUsd(auction.highestBid, auction.network);
        if (auction.bids && auction.bids.length > 0) {
            auction.bids = await Promise.all(auction.bids.map(async (bid) => {
                bid.bidAmount = (bid.bidAmount * usdRate).toFixed(2)//await convertPriceToUsd(bid.bidAmount, auction.network);
                return bid;
            }));
        }
        return responseSuccess(res, { auction })
    } catch (error) {
        console.log(error)
        if (ERRORS[error.message]) {
            return responseBadRequest(res, ERRORS[error.message])
        }
        return responseServerSideError(res, {MESSAGE: error.message})
    }
}

const getAllAuctions = async (req, res) => {
    try {
        const { page, limit, network } = req.query
		const paginationOptions = getEnabledPaginationOption(page, limit)
        const options = { asResponse: true, paginationOptions, populate: false }
        let query = {}
        if (network) {
            query.network = network;
        }
        const usdRate = await convertPriceToUsd(1, req.query.network);

        let auctions = await auctionHelper.fetchAuctionsByQuery(query, options)
        for (let i=0; i<auctions.docs.length; i++){
            auctions.docs[i].startingPrice = (auctions.docs[i].startingPrice * usdRate).toFixed(2);//await convertPriceToUsd(auctions.docs[i].startingPrice, auctions.docs[i].network);
            auctions.docs[i].highestBid = (auctions.docs[i].highestBid * usdRate).toFixed(2)//await convertPriceToUsd(auctions.docs[i].highestBid, auctions.docs[i].network);
            if (auctions.docs[i].bids && auctions.docs[i].bids.length > 0) {
                auctions.docs[i].bids = await Promise.all(auctions.docs[i].bids.map(async (bid) => {
                    bid.bidAmount = (bid.bidAmount * usdRate).toFixed(2)//await convertPriceToUsd(bid.bidAmount, auctions.docs[i].network);
                    return bid;
                }));
            }
        }
        return responseSuccess(res, { auctions })
    } catch (error) {
        console.log(error)
        if (ERRORS[error.message]) {
            return responseBadRequest(res, ERRORS[error.message])
        }
        return responseServerSideError(res, error.message)
    }
}

const getAllBids = async (req, res) => {
    try {
        //const response = await fetch(CONSTANTS.AUTH_INFO_URL, {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: `token=${req.headers.token}`})
        //const data = await response.json()

        const { page, limit, network } = req.query
		const paginationOptions = getEnabledPaginationOption(page, limit)
        const options = { asResponse: true, paginationOptions, populate: false }
        const user = await userHelper.getUserByAccountId(req.tokenData.account_id, options)
        let query = { 'bids.user': user._id };
        if (network) {
            query.network = network;
        }
        const usdRate = await convertPriceToUsd(1, req.query.network);

        let auctions = await auctionHelper.fetchAuctionsByQuery(query, options)
        auctions.docs = auctions.docs.map(auction => {
            auction.startingPrice = (auction.startingPrice * usdRate).toFixed(2);
            auction.highestBid = (auction.highestBid * usdRate).toFixed(2);
            // Filter the owners array to include only the specified userId
            auction.bids = auction.bids.filter(bid => bid.user.toString() === user._id.toString())
            .map(bid => {
                // Convert bidAmount to USD and keep it in the same field
                bid.bidAmount = (bid.bidAmount * usdRate).toFixed(2);
                return bid;
            });;
            return auction;
        });
        return responseSuccess(res, { auctions })
    } catch (error) {
        console.log(error)
        if (ERRORS[error.message]) {
            return responseBadRequest(res, ERRORS[error.message])
        }
        return responseServerSideError(res, error.message)
    }
}

const getAuctionDetails = async (req, res) => {
	try {
		const { id } = req.params
		const options = { asResponse: true, paginationOptions: { enabled: false }, populate: false }
        options.populate = { nft: true, seller: true, highestBidder: true }
		let auction = await auctionHelper.getAuctionById(id, options)
        auction.startingPrice = await convertPriceToUsd(auction.startingPrice, auction.network);
        auction.highestBid = await convertPriceToUsd(auction.highestBid, auction.network);
        if (auction.bids && auction.bids.length > 0) {
            auction.bids = await Promise.all(auction.bids.map(async (bid) => {
                bid.bidAmount = await convertPriceToUsd(bid.bidAmount, auction.network);
                return bid;
            }));
        }
		return responseSuccess(res, { auction })
	} catch (error) {
		if (ERRORS[error.message]) {
			return responseBadRequest(res, ERRORS[error.message])
		}
		return responseServerSideError(res, {MESSAGE: error.message})
	}
}

const placeBid = async (req, res) => {
    try {
        const options = { asResponse: true, paginationOptions: { enabled: false }, populate: false }
        const bid = await auctionHelper.placeBidHelper(req, options)
        return responseSuccess(res, { bid })
    } catch (error) {
        console.log(error)
        if (ERRORS[error.message]) {
            return responseBadRequest(res, ERRORS[error.message])
        }
        return responseServerSideError(res, {MESSAGE: error.message})
    }
}

const endAuction = async (req, res) => {
    try {
        const options = { asResponse: true, paginationOptions: { enabled: false }, populate: false }
        const auction = await auctionHelper.endAuctionHelper(req, options)
        return responseSuccess(res, { auction })
    } catch (error) {
        console.log(error)
        if (ERRORS[error.message]) {
            return responseBadRequest(res, ERRORS[error.message])
        }
        return responseServerSideError(res, {MESSAGE: error.message})
    }
}

const getPendingRefunds = async (req, res) => {
    try {
        const options = { asResponse: true, paginationOptions: { enabled: false }, populate: false }
        const refunds = await auctionHelper.getPendingRefundsHelper(req, options)

        const usdRate = await convertPriceToUsd(1, req.query.network);
        for (let i=0; i<refunds.length; i++){
            refunds[i].startingPrice = (refunds[i].startingPrice * usdRate).toFixed(2)
            refunds[i].highestBid = (refunds[i].highestBid * usdRate).toFixed(2)
            refunds[i].pendingRefunds = (refunds[i].pendingRefunds * usdRate).toFixed(2)
        }

        return responseSuccess(res, {refunds: refunds} )
    } catch (error) {
        console.log(error)
        if (ERRORS[error.message]) {
            return responseBadRequest(res, ERRORS[error.message])
        }
        return responseServerSideError(res, {MESSAGE: error.message})
    }
}

const refund = async (req, res) => {
    try {
        const options = { asResponse: true, paginationOptions: { enabled: false }, populate: false }
        const refund = await auctionHelper.refundHelper(req, options)
        return responseSuccess(res, refund )
    } catch (error) {
        console.log(error)
        if (ERRORS[error.message]) {
            return responseBadRequest(res, ERRORS[error.message])
        }
        return responseServerSideError(res, {MESSAGE: error.message})
    }
}

const cancelAuction = async (req, res) => {
    try {
        const options = { asResponse: true, paginationOptions: { enabled: false }, populate: false }
        const auction = await auctionHelper.cancelAuctionHelper(req, options)
        return responseSuccess(res, { auction })
    } catch (error) {
        console.log(error)
        if (ERRORS[error.message]) {
            return responseBadRequest(res, ERRORS[error.message])
        }
        return responseServerSideError(res, {MESSAGE: error.message})
    }
}

const getAuctionedNFTs = async (req, res) => {
	try {
        const response = await fetch(CONSTANTS.AUTH_INFO_URL, {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: `token=${req.headers.token}`})
        const data = await response.json()

        const { page, limit, network } = req.query
        const paginationOptions = getEnabledPaginationOption(page, limit)
        const options = { asResponse: true, paginationOptions, populate: false }
       
        let user
        user = await userHelper.getUserByQuery({accountId: data.account_id}, options)
        if(!user){
            return responseBadRequest(res, ERRORS.NO_USER_FOUND)
        }else if (user.isBanned == true){
            return responseBadRequest(res,  {
                ...ERRORS.BANNED_USER,
                REASON: `${user.banReason}`
            } )
        }

        let query = {seller: user._id}
        if (network) {
            query.network = network;
        }

        const usdRate = await convertPriceToUsd(1, req.query.network);

        let auctions = await auctionHelper.fetchAuctionsByQuery(query, options)
        for (let i=0; i<auctions.docs.length; i++){
            auctions.docs[i].startingPrice = (auctions.docs[i].startingPrice * usdRate).toFixed(2);//await convertPriceToUsd(auctions.docs[i].startingPrice, auctions.docs[i].network);
            auctions.docs[i].highestBid = (auctions.docs[i].highestBid * usdRate).toFixed(2)//await convertPriceToUsd(auctions.docs[i].highestBid, auctions.docs[i].network);
            if (auctions.docs[i].bids && auctions.docs[i].bids.length > 0) {
                auctions.docs[i].bids = await Promise.all(auctions.docs[i].bids.map(async (bid) => {
                    bid.bidAmount = (bid.bidAmount * usdRate).toFixed(2)//await convertPriceToUsd(bid.bidAmount, auctions.docs[i].network);
                    return bid;
                }));
            }
        }
		return responseSuccess(res, { auctions })
	} catch (error) {
        console.error(error)
		if (ERRORS[error.message]) {
			return responseBadRequest(res, ERRORS[error.message])
		}
		return responseServerSideError(res, {MESSAGE: error.message})
	}
}

const getFilterAuctions = async (req, res) => {
    try {
        const { page, limit, network } = req.query
        const paginationOptions = getEnabledPaginationOption(page, limit)
        const options = { asResponse: true, paginationOptions, populate: false }
        const usdRate = await convertPriceToUsd(1, req.query.network);

        const auctions = await auctionHelper.getFilterAuctions(req, usdRate, options)
        for (let i=0; i<auctions.length; i++){
            auctions[i].startingPrice = (auctions[i].startingPrice * usdRate).toFixed(2);//await convertPriceToUsd(auctions[i].startingPrice, auctions[i].network);
            auctions[i].highestBid = (auctions[i].highestBid * usdRate).toFixed(2)//await convertPriceToUsd(auctions[i].highestBid, auctions[i].network)
            auctions[i].nft = auctions[i].nft._id 
            if (auctions[i].bids && auctions[i].bids.length > 0) {
                auctions[i].bids = await Promise.all(auctions[i].bids.map(async (bid) => {
                    bid.bidAmount = (bid.bidAmount * usdRate).toFixed(2)//await convertPriceToUsd(bid.bidAmount, auctions[i].network);
                    return bid;
                }));
            }
        }
        return responseSuccess(res, { auctions })
    } catch (error) {
        console.log(error)
        if (ERRORS[error.message]) {
            return responseBadRequest(res, ERRORS[error.message])
        }
        return responseServerSideError(res, {MESSAGE: error.message})
    }
}

module.exports = {
    createAuction,
    getAllAuctions,
    getAuctionDetails,
    placeBid,
    endAuction,
    getPendingRefunds,
    getAuctionedNFTs,
    getAllBids,
    refund,
    cancelAuction,
    getFilterAuctions
}