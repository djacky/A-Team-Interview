const { responseSuccess, responseBadRequest, responseUnauthorized, responseServerSideError } = require('../utils/responseTypes')
const { filterEmptyKeys, generateUpdateObject, getEnabledPaginationOption } = require('../utils/helperFunctions')
const userHelper = require('../helpers/user.helper')
const adminHelper = require('../helpers/admin.helper')
const nftHelper = require('../helpers/nft.helper')
const ERRORS = require('../utils/errorTypes')
const CONSTANTS = require('../constants')
const fetch = require('node-fetch')


const createRequest = async (req, res) => {
    try {
        //const response = await fetch(CONSTANTS.AUTH_INFO_URL, {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: `token=${req.headers.token}`})
        //const data = await response.json()
        const options = { asResponse: true, paginationOptions: { enabled: false }, populate: false }
        const user = await userHelper.getUserByAccountId(req.tokenData.account_id, options)
        const creator = user._id
        const { pinataUri, s3ImageUri, s3ModelUri, name, description, amount, type, onChainMetadata, offChainMetadata, network} = req.body
		const nftRequestDetails = filterEmptyKeys({ pinataUri, s3ImageUri, s3ModelUri, name, description, amount, type, creator, onChainMetadata, offChainMetadata, network })
        const nft = await nftHelper.createNewNFTRequest(nftRequestDetails, options)
        return responseSuccess(res, { nft })
    } catch (error) {
        console.log(error)
        if (ERRORS[error.message]) {
            return responseBadRequest(res, ERRORS[error.message])
        }
        return responseServerSideError(res, error.message)
    }
}

const updateAssetsURI = async (req, res) => {
    try {
        const { requestId } = req.params
        const fieldsToUpdate = [
            "s3ImageUri",
            "s3ModelUri"
        ]
        let requestUpdates = generateUpdateObject(fieldsToUpdate, req.body)
        const options = { asResponse: true, paginationOptions: { enabled: false }, populate: false }
        requestUpdates.status = CONSTANTS.NFT_REQUEST_STATUS_PENDING
        const nft = await nftHelper.updateRequestDetailsById(requestId, requestUpdates, options)

        return responseSuccess(res, { nft })
    } catch (error) {
        if (ERRORS[error.message]) {
            return responseBadRequest(res, ERRORS[error.message])
        }
        return responseServerSideError(res, error)
    }
}

const getNFTsByAccountId = async (req, res) => {
    try {
        //const response = await fetch(CONSTANTS.AUTH_INFO_URL, {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: `token=${req.headers.token}`})
        //const data = await response.json()
        const { page, limit, status, type, network } = req.query
		const paginationOptions = getEnabledPaginationOption(page, limit)
        
        const options = { asResponse: true, paginationOptions, populate: false }
        const user = await userHelper.getUserByAccountId(req.tokenData.account_id, options)
        let query
        if(status != CONSTANTS.NFT_REQUEST_STATUS_MINTED){
            query = { creator: user._id };
        }else{
            query = { 'owners.user': user._id };
        }
        
        if (status) {
            query.status = status;
        }
        if (type) {
            query.type = type;
        }
        if (network) {
            query.network = network;
        }
        let nfts = await nftHelper.fetchNFTsByQuery(query, options)
        nfts.docs = nfts.docs.map(nft => {
            // Filter the owners array to include only the specified userId
            nft.owners = nft.owners.filter(owner => owner.user.toString() === user._id.toString());
            return nft;
        });
        return responseSuccess(res, { nfts })
    } catch (error) {
        console.log(error)
        if (ERRORS[error.message]) {
            return responseBadRequest(res, ERRORS[error.message])
        }
        return responseServerSideError(res, error.message)
    }
}

const getNFTById = async (req, res) => {
    try {
        //const response = await fetch(CONSTANTS.AUTH_INFO_URL, {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: `token=${req.headers.token}`})
        //const data = await response.json()
        const options = { asResponse: true, paginationOptions: { enabled: false }, populate: false }
        const user = await userHelper.getUserByAccountId(req.tokenData.account_id, options)
        const { id } = req.params
        const query = {_id : id}
        let nft = await nftHelper.getNFTById(query, options)
        nft.owners = nft.owners.filter(owner => owner.user.toString() === user._id.toString());
        return responseSuccess(res, { nft })
    } catch (error) {
        console.log(error)
        if (ERRORS[error.message]) {
            return responseBadRequest(res, ERRORS[error.message])
        }
        return responseServerSideError(res, error.message)
    }
}

const regenerateRequest = async (req, res) => {
    try {
        const { requestId } = req.params
        const fieldsToUpdate = [
            "s3ImageUri",
            "s3ModelUri",
            "name",
            "description",
            "amount",
            "offChainMetadata",
            "type"
        ]
        let requestUpdates = generateUpdateObject(fieldsToUpdate, req.body)
        const options = { asResponse: true, paginationOptions: { enabled: false }, populate: false }
        requestUpdates.status = CONSTANTS.NFT_REQUEST_STATUS_PENDING
        const nft = await nftHelper.updateRequestDetailsById(requestId, requestUpdates, options)

        return responseSuccess(res, { nft })
    } catch (error) {
        if (ERRORS[error.message]) {
            return responseBadRequest(res, ERRORS[error.message])
        }
        return responseServerSideError(res, error)
    }
}

const updateNFT = async (req, res) => {
    try {
        const { nftId } = req.params
        const fieldsToUpdate = [
            "s3ImageUri",
            "s3ModelUri",
            "name",
            "description",
            "amount",
            "offChainMetadata",
            "status",
            "feedback",
            "type"
        ]
        let nftUpdates = generateUpdateObject(fieldsToUpdate, req.body)
        const options = { asResponse: true, paginationOptions: { enabled: false }, populate: false }
        const nft = await nftHelper.updateRequestDetailsById(nftId, nftUpdates, options)

        return responseSuccess(res, { nft })
    } catch (error) {
        if (ERRORS[error.message]) {
            return responseBadRequest(res, ERRORS[error.message])
        }
        return responseServerSideError(res, error)
    }
}

const mintNFT = async (req, res) => {
    try {
        const options = { asResponse: true, paginationOptions: { enabled: false }, populate: false }
        const nft = await nftHelper.mintNFTHelper(req, options)
        return responseSuccess(res, { nft })
    } catch (error) {
        console.log(error)
        if (ERRORS[error.message]) {
            return responseBadRequest(res, ERRORS[error.message])
        }
        return responseServerSideError(res, {MESSAGE: error.message})
    }
}

const getNFTsForAdmin = async (req, res) => {
    try {
        const { page, limit, status, type } = req.query
		const paginationOptions = getEnabledPaginationOption(page, limit)

        const options = { asResponse: true, paginationOptions, populate: false }
        let query = {}
        if (status) {
            query.status = status;
        }
        if (type) {
            query.type = type;
        }
        const nfts = await nftHelper.fetchNFTsByQuery(query, options)
        return responseSuccess(res, { nfts })
    } catch (error) {
        console.log(error)
        if (ERRORS[error.message]) {
            return responseBadRequest(res, ERRORS[error.message])
        }
        return responseServerSideError(res, error.message)
    }
}

module.exports = {
    createRequest,
    updateAssetsURI,
    getNFTsByAccountId,
    getNFTById,
    regenerateRequest,
    updateNFT,
    mintNFT,
    getNFTsForAdmin

}