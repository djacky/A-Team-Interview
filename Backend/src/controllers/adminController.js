const adminHelper = require('../helpers/admin.helper')
const fs = require('fs');
const path = require('path');

const { responseSuccess, responseBadRequest, responseUnauthorized, responseServerSideError } = require('../utils/responseTypes')

const { generateAdminAuthToken } = require('../utils/jwt')
const { generateUpdateObject, filterEmptyKeys } = require('../utils/helperFunctions')

const ERRORS = require('../utils/errorTypes')
const CONSTANTS = require('../constants')

const signUp = async (req, res) => {
	try {
        const { walletAddress} = req.body
        const options = { asResponse: true, paginationOptions: { enabled: false }, populate: false }
        //const response = await fetch(CONSTANTS.AUTH_INFO_URL, {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: `token=${req.headers.token}`})
        //const data = await response.json()
        const adminDetails = filterEmptyKeys({ accountId: req.tokenData.account_id, walletAddress, jwt: "reserved" })
		await adminHelper.createNewAdmin(adminDetails, options)
      	return responseSuccess(res, { message: 'Dummy API, Not to be used for production' })
	} catch (error) {
		if (ERRORS[error.message]) {
			return responseBadRequest(res, ERRORS[error.message])
		}
		return responseServerSideError(res, {MESSAGE: error.message})
	}
}

const signIn = async (req, res) => {
    try {
        const { walletAddress } = req.body
        const options = { asResponse: true, paginationOptions: { enabled: false }, populate: false }
        //const response = await fetch(CONSTANTS.AUTH_INFO_URL, {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: `token=${req.headers.token}`})
        //const data = await response.json()

        const publicKeyPath = path.resolve(__dirname, '../../public.pem');
        const publicKey = fs.readFileSync(publicKeyPath, 'utf8');
        
        let admin
        admin = await adminHelper.getAdminByQuery({accountId: req.tokenData.account_id}, options, req.headers.adminkey)
        if(!admin){
            return responseBadRequest(res, ERRORS.NO_USER_FOUND)
        }else if (admin.role == CONSTANTS.USER_ROLE_ADMIN && admin.walletAddress != walletAddress){
            return responseBadRequest(res, ERRORS.WALLET_ADDRESS_NOT_REGISTERED)
        }
        return responseSuccess(res, { admin, publicKey })
    } catch (error) {
        if (ERRORS[error.message]) {
            return responseBadRequest(res, ERRORS[error.message])
        }
        return responseServerSideError(res, {MESSAGE: error.message})
    }
}

module.exports = {
	signUp,
	signIn
}
