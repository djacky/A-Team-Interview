const { User, NFT, Admin, App, Auction } = require('../models')
const fetch = require('node-fetch')
const { header, body, param, validationResult, query } = require('express-validator')
const { responseBadRequest } = require('../utils/responseTypes')
const { getSecret, verifyHMACSignature, formHMACFromStats } = require('../utils/helperFunctions');
const { AWS_SECRET, CLIENT_GAME_TOKEN_PASS, GET_ALL_STATS_KEY, ZEALY_KEY } = require('../config');
var jwt = require('jsonwebtoken');

const { isValidateS3Url, isValidWalletAddress } = require('../validations')

const ERRORS = require('../utils/errorTypes')
const CONSTANTS = require('../constants')

const updateStatusRules = () => {
	return [
        header('token')
            .trim()
            .notEmpty()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE)
            .isString()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE)
            .custom( async (token) => {
                const response = await fetch(CONSTANTS.AUTH_INFO_URL, {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: `token=${token}`})
                const data = await response.json()
                if(!("active" in data) || data.active != true){
                    return Promise.reject(ERRORS.CLIENT_UNAUTHORIZED)
                }
				if (!await Admin.findOne({ accountId: data.account_id })) {
					return Promise.reject(ERRORS.CLIENT_UNAUTHORIZED)
				}
                return true
            }),
        body('pause')
            .isBoolean()
            .withMessage(ERRORS.INVALID_PAUSE_TYPE),
        body('pauseReason')
            .notEmpty()
            .withMessage(ERRORS.INVALID_PAUSE_REASON_TYPE)
            .isString()
            .withMessage(ERRORS.INVALID_PAUSE_REASON_TYPE)
	]
}

const banRules = () => {
	return [
        header('token')
            .trim()
            .notEmpty()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE)
            .isString()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE)
            .custom( async (token) => {
                const response = await fetch(CONSTANTS.AUTH_INFO_URL, {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: `token=${token}`})
                const data = await response.json()
                if(!("active" in data) || data.active != true){
                    return Promise.reject(ERRORS.CLIENT_UNAUTHORIZED)
                }
				if (!await Admin.findOne({ accountId: data.account_id })) {
					return Promise.reject(ERRORS.CLIENT_UNAUTHORIZED)
				}
                return true
            }),
        param('userId')
			.isMongoId()
			.withMessage(ERRORS.INVALID_OBJECT_ID_FORMAT)
			.custom(async (userId) => {
                const user = await User.findById(userId)
				if (!user) {
					return Promise.reject(ERRORS.NO_USER_FOUND)
				}
				return true
			}),
        body('isBanned')
        .isBoolean()
        .withMessage(ERRORS.INVALID_BAN_TYPE)
	]
}

const userSignUpRules = () => {
	return [
        header('auth')
            .trim()
            .notEmpty()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE)
            .isString()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE)
            .custom( async (token, {req}) => {
                const { WEB_KEY } = await getSecret(AWS_SECRET);
                const tokenData = await jwt.verify(token, WEB_KEY);
                if(!tokenData){
                    return Promise.reject(ERRORS.CLIENT_UNAUTHORIZED)
                }
				if (!isValidWalletAddress(tokenData.walletAddress)) {
					return Promise.reject(ERRORS.INVALID_WALLET_ADDRESS)
				}
                req.tokenData = tokenData;
                return true
            })
	]
}

const userSignInRules = () => {
	return [
        header('token')
            .trim()
            .notEmpty()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE)
            .isString()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE)
            .custom( async (token, {req}) => {
                const response = await fetch(CONSTANTS.AUTH_INFO_URL, {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: `token=${token}`})
                const data = await response.json()
                
                if(!("active" in data) || data.active != true){
                    return Promise.reject(ERRORS.CLIENT_UNAUTHORIZED)
                }
				const user = await User.findOne({ accountId: data.account_id })
                if (!user) {
					return Promise.reject(ERRORS.CLIENT_UNAUTHORIZED)
				}
                if (user && user.isBanned == true) {
					return Promise.reject(ERRORS.BANNED_USER)
				}
                req.tokenData = data;
                return true
            }),
        body('walletAddress')
            .custom(async (walletAddress, {req}) => {
                if (!isValidWalletAddress(walletAddress)) {
                    return Promise.reject(ERRORS.INVALID_WALLET_ADDRESS)
                }
                if (!await User.findOne({ walletAddress })) {
                	return Promise.reject(ERRORS.WALLET_ADDRESS_NOT_REGISTERED)
                }
                return true
            })
			
	]
}

const validateScan = () => {
	return [
        header('token')
            .trim()
            .notEmpty()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE)
            .isString()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE)
            .custom( async (token, {req}) => {
                const response = await fetch(CONSTANTS.AUTH_INFO_URL, {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: `token=${token}`})
                const data = await response.json()
                
                if(!("active" in data) || data.active != true){
                    return Promise.reject(ERRORS.CLIENT_UNAUTHORIZED)
                }
				if (!await User.findOne({ accountId: data.account_id })) {
                    return Promise.reject(ERRORS.NO_USER_FOUND)
				}
                req.tokenData = data;
                return true
            })
	]
}

const validateS3sign = () => {
	return [
        header('token')
            .trim()
            .notEmpty()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE)
            .isString()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE)
            .custom( async (token, {req}) => {
                const response = await fetch(CONSTANTS.AUTH_INFO_URL, {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: `token=${token}`})
                const data = await response.json()
                
                if(!("active" in data) || data.active != true){
                    return Promise.reject(ERRORS.CLIENT_UNAUTHORIZED)
                }
				if (!await User.findOne({ accountId: data.account_id }) && !await Admin.findOne({ accountId: data.account_id })) {
                    return Promise.reject(ERRORS.NO_USER_FOUND)
				}
                return true
            }),
        body('walletAddress')
            .custom(async (walletAddress, {req}) => {
                if (!isValidWalletAddress(walletAddress)) {
                    return Promise.reject(ERRORS.INVALID_WALLET_ADDRESS)
                }
                if (!await User.findOne({ walletAddress }) && !await Admin.findOne({ walletAddress })) {
                	return Promise.reject(ERRORS.WALLET_ADDRESS_NOT_REGISTERED)
                }
                return true
            })
			
	]
}

// Auction Rules
const validatecreateAuction = () => {
	return [
        body().custom(async () => {
            const app = await App.findOne({});
            if (!app) {
                return Promise.reject(ERRORS.APP_CONFIG_MISSING);
            }else if(app && app.pause === true){
                return Promise.reject(ERRORS.APP_IS_PAUSED);
            }
            return true;
        }),
        header('token')
            .trim()
            .notEmpty()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE)
            .isString()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE)
            .custom( async (token, {req}) => {
                const response = await fetch(CONSTANTS.AUTH_INFO_URL, {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: `token=${token}`})
                const data = await response.json()
                if(!("active" in data) || data.active != true){
                    return Promise.reject(ERRORS.CLIENT_UNAUTHORIZED)
                }
                const user = await User.findOne({ accountId: data.account_id })
                if (!user) {
					return Promise.reject(ERRORS.CLIENT_UNAUTHORIZED)
				}
                if (user && user.isBanned == true) {
					return Promise.reject(ERRORS.BANNED_USER)
				}
                req.tokenData = data;
                return true
            }),
        param('nftId')
			.isMongoId()
			.withMessage(ERRORS.INVALID_OBJECT_ID_FORMAT)
			.custom(async (id, {req}) => {
                const mintRequest = await NFT.findById(id)
				if (!mintRequest) {
					return Promise.reject(ERRORS.NO_NFT_FOUND)
				}else if (mintRequest.status != CONSTANTS.NFT_REQUEST_STATUS_MINTED){
					return Promise.reject(ERRORS.UNABLE_TO_CREATE_AUCTION)
                }else if (!mintRequest.txHash){
					return Promise.reject(ERRORS.UNABLE_TO_CREATE_AUCTION)
                }
                // else if(mintRequest.activeInAuction){
				// 	return Promise.reject(ERRORS.AUCTION_ALREADY_ACTIVE)
                // }
				return true
			}),
        body('auctionPrice')
            .notEmpty()
            .withMessage(ERRORS.INVALID_AUCTION_PRICE)
            .custom(auctionPrice => {
                const floatValue = parseFloat(auctionPrice);
                if (isNaN(floatValue)) {
                    return Promise.reject(ERRORS.INVALID_AUCTION_PRICE)
                }
                if (floatValue <= CONSTANTS.NFT_MIN_PRICE || floatValue >= CONSTANTS.NFT_MAX_PRICE) {
                    return Promise.reject(ERRORS.INVALID_AUCTION_PRICE_RANGE)
                }
                return true;
            }),
        body('quantity')
            .notEmpty()
            .withMessage(ERRORS.INVALID_NFT_AMOUNT_TYPE)
            .isInt({gt: 0})
            .withMessage(ERRORS.INVALID_NFT_AMOUNT_TYPE),
        body('duration')
            .notEmpty()
            .withMessage(ERRORS.INVALID_DURATION)
            .isInt({gt: CONSTANTS.MIN_AUCTION_DURATION})
            .withMessage(ERRORS.INVALID_DURATION),
        
	]
}

const validateGetAuction = () => {
	return [
        header('token')
            .trim()
            .notEmpty()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE)
            .isString()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE)
            .custom( async (token) => {
                const response = await fetch(CONSTANTS.AUTH_INFO_URL, {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: `token=${token}`})
                const data = await response.json()
                if(!("active" in data) || data.active != true){
                    return Promise.reject(ERRORS.CLIENT_UNAUTHORIZED)
                }
				const user = await User.findOne({ accountId: data.account_id })
                if (!user) {
					return Promise.reject(ERRORS.CLIENT_UNAUTHORIZED)
				}
                if (user && user.isBanned == true) {
					return Promise.reject(ERRORS.BANNED_USER)
				}
                return true
            }),
        param('id')
			.isMongoId()
			.withMessage(ERRORS.INVALID_OBJECT_ID_FORMAT)
			.custom(async (id) => {
                const auction = await Auction.findById(id)
				if (!auction) {
					return Promise.reject(ERRORS.NO_AUCTION_FOUND)
				}
				return true
			}),
	]
}

const validatePlaceBid = () => {
	return [
        body().custom(async () => {
            const app = await App.findOne({});
            if (!app) {
                return Promise.reject(ERRORS.APP_CONFIG_MISSING);
            }else if(app && app.pause === true){
                return Promise.reject(ERRORS.APP_IS_PAUSED);
            }
            return true;
        }),
        header('token')
            .trim()
            .notEmpty()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE)
            .isString()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE)
            .custom( async (token) => {
                const response = await fetch(CONSTANTS.AUTH_INFO_URL, {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: `token=${token}`})
                const data = await response.json()
                if(!("active" in data) || data.active != true){
                    return Promise.reject(ERRORS.CLIENT_UNAUTHORIZED)
                }
                const user = await User.findOne({ accountId: data.account_id })
                if (!user) {
					return Promise.reject(ERRORS.CLIENT_UNAUTHORIZED)
				}
                if (user && user.isBanned == true) {
					return Promise.reject(ERRORS.BANNED_USER)
				}
                return true
            }),
        param('auctionId')
			.isMongoId()
			.withMessage(ERRORS.INVALID_OBJECT_ID_FORMAT)
			.custom(async (auctionId, {req}) => {
                const auction = await Auction.findById(auctionId)
				if (!auction) {
					return Promise.reject(ERRORS.NO_AUCTION_FOUND)
				}
				return true
			}),
        body('bidPrice')
            .notEmpty()
            .withMessage(ERRORS.INVALID_BID_PRICE)
            .isDecimal()
            .withMessage(ERRORS.INVALID_BID_PRICE),
        
	]
}

const validateEndAuction = () => {
	return [
        body().custom(async () => {
            const app = await App.findOne({});
            if (!app) {
                return Promise.reject(ERRORS.APP_CONFIG_MISSING);
            }else if(app && app.pause === true){
                return Promise.reject(ERRORS.APP_IS_PAUSED);
            }
            return true;
        }),
        header('token')
            .trim()
            .notEmpty()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE)
            .isString()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE)
            .custom( async (token) => {
                const response = await fetch(CONSTANTS.AUTH_INFO_URL, {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: `token=${token}`})
                const data = await response.json()
                if(!("active" in data) || data.active != true){
                    return Promise.reject(ERRORS.CLIENT_UNAUTHORIZED)
                }
                const user = await User.findOne({ accountId: data.account_id })
                if (!user) {
					return Promise.reject(ERRORS.CLIENT_UNAUTHORIZED)
				}
                if (user && user.isBanned == true) {
					return Promise.reject(ERRORS.BANNED_USER)
				}
                return true
            }),
        param('auctionId')
			.isMongoId()
			.withMessage(ERRORS.INVALID_OBJECT_ID_FORMAT)
			.custom(async (auctionId, {req}) => {
                const auction = await Auction.findById(auctionId)
				if (!auction) {
					return Promise.reject(ERRORS.NO_AUCTION_FOUND)
                }else if(auction.ended == false){
					return Promise.reject(ERRORS.AUCTION_NOT_ENDED_YET)
                }else if(auction.txHash){
                    return Promise.reject(ERRORS.AUCTION_ALREADY_ENDED)
                }
				return true
			}),
	]
}

const validatePendingRefunds = () => {
	return [
        body().custom(async () => {
            const app = await App.findOne({});
            if (!app) {
                return Promise.reject(ERRORS.APP_CONFIG_MISSING);
            }else if(app && app.pause === true){
                return Promise.reject(ERRORS.APP_IS_PAUSED);
            }

            return true;
        }),
        header('token')
            .trim()
            .notEmpty()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE)
            .isString()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE)
            .custom( async (token, {req}) => {
                const response = await fetch(CONSTANTS.AUTH_INFO_URL, {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: `token=${token}`})
                const data = await response.json()
                if(!("active" in data) || data.active != true){
                    return Promise.reject(ERRORS.CLIENT_UNAUTHORIZED)
                }
                const user = await User.findOne({ accountId: data.account_id })
                if (!user) {
					return Promise.reject(ERRORS.CLIENT_UNAUTHORIZED)
				}
                if (user && user.isBanned == true) {
					return Promise.reject(ERRORS.BANNED_USER)
				}
                req.tokenData = data;
                return true
            })
	]
}

// NFT Rules
const validateMintRequest = () => {
	return [
        body().custom(async () => {
            const app = await App.findOne({});
            if (!app) {
                return Promise.reject(ERRORS.APP_CONFIG_MISSING);
            }else if(app && app.pause === true){
                return Promise.reject(ERRORS.APP_IS_PAUSED);
            }
            return true;
        }),
        header('token')
            .trim()
            .notEmpty()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE)
            .isString()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE)
            .custom( async (token, {req}) => {
                const response = await fetch(CONSTANTS.AUTH_INFO_URL, {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: `token=${token}`})
                const data = await response.json()
                if(!("active" in data) || data.active != true){
                    return Promise.reject(ERRORS.CLIENT_UNAUTHORIZED)
                }
                const user = await User.findOne({ accountId: data.account_id })
                if (!user) {
					return Promise.reject(ERRORS.CLIENT_UNAUTHORIZED)
				}
                if (user && user.isBanned == true) {
					return Promise.reject(ERRORS.BANNED_USER)
				}
                req.tokenData = data;
                return true
            }),
        // body('s3ImageUri')
        //     .optional()
        //     .trim()
        //     .notEmpty()
        //     .withMessage(ERRORS.INVALID_S3_IMAGE_URI_TYPE)
        //     .isString()
        //     .withMessage(ERRORS.INVALID_S3_IMAGE_URI_TYPE)
        //     .custom(async (s3ImageUri) => {
        //         if (!isValidateS3Url(s3ImageUri)) {
		// 			return Promise.reject(ERRORS.INVALID_S3_IMAGE_URI)
		// 		}
        //         if(await NFT.findOne({s3ImageUri})){
        //             return Promise.reject(ERRORS.S3_IMAGE_URI_NOT_UNIQUE)
        //         }
        //         return true
        //     }),
        // body('s3ModelUri')
        //     .optional()
        //     .trim()
        //     .notEmpty()
        //     .withMessage(ERRORS.INVALID_S3_MODEL_URI_TYPE)
        //     .isString()
        //     .withMessage(ERRORS.INVALID_S3_MODEL_URI_TYPE)
        //     .custom(async (s3ModelUri) => {
        //         if (!isValidateS3Url(s3ModelUri)) {
		// 			return Promise.reject(ERRORS.INVALID_S3_MODEL_URI)
		// 		}
        //         if(await NFT.findOne({s3ModelUri})){
        //             return Promise.reject(ERRORS.S3_MODEL_URI_NOT_UNIQUE)
        //         }
        //         return true
        //     }),
        body('name')
            .trim()
            .notEmpty()
            .withMessage(ERRORS.INVALID_NFT_NAME_TYPE)
            .isString()
            .withMessage(ERRORS.INVALID_NFT_NAME_TYPE)
            .custom(async (name) => {
            
                // if (!isValidName(name)) {
				// 	return Promise.reject(ERRORS.INVALID_NFT_NAME)
				// }
                if(await NFT.findOne({name: { $regex: new RegExp(`^${name}$`, 'i') }})){
                    return Promise.reject(ERRORS.NFT_NAME_NOT_UNIQUE)
                }
                return true
            }),
        body('description')
            .optional()
            .trim()
            .notEmpty()
            .withMessage(ERRORS.INVALID_NFT_DESCRIPTION_TYPE)
            .isString()
            .withMessage(ERRORS.INVALID_NFT_DESCRIPTION_TYPE),
        body('amount')
            .notEmpty()
            .withMessage(ERRORS.INVALID_NFT_AMOUNT_TYPE)
            .isNumeric({gt: CONSTANTS.NFT_MIN_AMOUNT, lt: CONSTANTS.NFT_MAX_AMOUNT})
            .withMessage(ERRORS.INVALID_NFT_AMOUNT_RANGE),
        body('network')
            .notEmpty()
            .withMessage(ERRORS.INVALID_NETWORK_TYPE)
			.isIn(CONSTANTS.NETWORKS)
            .withMessage(ERRORS.INVALID_NETWORK_TYPE),
        
	]
}

const validateAssetsURI = () => {
	return [
        body().custom(async () => {
            const app = await App.findOne({});
            if (!app) {
                return Promise.reject(ERRORS.APP_CONFIG_MISSING);
            }else if(app && app.pause === true){
                return Promise.reject(ERRORS.APP_IS_PAUSED);
            }
            return true;
        }),
        header('token')
            .trim()
            .notEmpty()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE)
            .isString()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE)
            .custom( async (token) => {
                const response = await fetch(CONSTANTS.AUTH_INFO_URL, {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: `token=${token}`})
                const data = await response.json()
                if(!("active" in data) || data.active != true){
                    return Promise.reject(ERRORS.CLIENT_UNAUTHORIZED)
                }
                const user = await User.findOne({ accountId: data.account_id })
                if (!user) {
					return Promise.reject(ERRORS.CLIENT_UNAUTHORIZED)
				}
                if (user && user.isBanned == true) {
					return Promise.reject(ERRORS.BANNED_USER)
				}
                return true
            }),
        param('requestId')
			.isMongoId()
			.withMessage(ERRORS.INVALID_OBJECT_ID_FORMAT)
			.custom(async (requestId) => {
                const mintRequest = await NFT.findById(requestId)
				if (!mintRequest) {
					return Promise.reject(ERRORS.NO_MINT_REQUEST_FOUND)
				}
				return true
			}),
        body('s3ImageUri')
            .trim()
            .notEmpty()
            .withMessage(ERRORS.INVALID_S3_IMAGE_URI_TYPE)
            .isString()
            .withMessage(ERRORS.INVALID_S3_IMAGE_URI_TYPE),
            // .custom(async (s3ImageUri) => {
            //     if (!isValidateS3Url(s3ImageUri)) {
			// 		return Promise.reject(ERRORS.INVALID_S3_IMAGE_URI)
			// 	}
            //     if(await NFT.findOne({s3ImageUri})){
            //         return Promise.reject(ERRORS.S3_IMAGE_URI_NOT_UNIQUE)
            //     }
            //     return true
            // }),
        body('s3ModelUri')
            .optional()
            .trim()
            .notEmpty()
            .withMessage(ERRORS.INVALID_S3_MODEL_URI_TYPE)
            .isString()
            .withMessage(ERRORS.INVALID_S3_MODEL_URI_TYPE),
            // .custom(async (s3ModelUri) => {
            //     if (!isValidateS3Url(s3ModelUri)) {
			// 		return Promise.reject(ERRORS.INVALID_S3_MODEL_URI)
			// 	}
            //     if(await NFT.findOne({s3ModelUri})){
            //         return Promise.reject(ERRORS.S3_MODEL_URI_NOT_UNIQUE)
            //     }
            //     return true
            // }),
	]
}

const validateRegeneratedMintRequest = () => {
	return [
        body().custom(async () => {
            const app = await App.findOne({});
            if (!app) {
                return Promise.reject(ERRORS.APP_CONFIG_MISSING);
            }else if(app && app.pause === true){
                return Promise.reject(ERRORS.APP_IS_PAUSED);
            }
            return true;
        }),
        header('token')
            .trim()
            .notEmpty()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE)
            .isString()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE)
            .custom( async (token) => {
                const response = await fetch(CONSTANTS.AUTH_INFO_URL, {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: `token=${token}`})
                const data = await response.json()
                if(!("active" in data) || data.active != true){
                    return Promise.reject(ERRORS.CLIENT_UNAUTHORIZED)
                }
                const user = await User.findOne({ accountId: data.account_id })
                if (!user) {
					return Promise.reject(ERRORS.CLIENT_UNAUTHORIZED)
				}
                if (user && user.isBanned == true) {
					return Promise.reject(ERRORS.BANNED_USER)
				}
                return true
            }),
        param('requestId')
			.isMongoId()
			.withMessage(ERRORS.INVALID_OBJECT_ID_FORMAT)
			.custom(async (requestId) => {
                const mintRequest = await NFT.findById(requestId)
				if (!mintRequest) {
					return Promise.reject(ERRORS.NO_MINT_REQUEST_FOUND)
				}else if (mintRequest.status != CONSTANTS.NFT_REQUEST_STATUS_REJECTED){
					return Promise.reject(ERRORS.UNABLE_TO_REGENERATE_REQUEST)
                }
				return true
			}),
        // body('s3ImageUri')
        //     .optional()
        //     .trim()
        //     .notEmpty()
        //     .withMessage(ERRORS.INVALID_S3_IMAGE_URI_TYPE)
        //     .isString()
        //     .withMessage(ERRORS.INVALID_S3_IMAGE_URI_TYPE)
        //     .custom(async (s3ImageUri) => {
        //         if (!isValidateS3Url(s3ImageUri)) {
		// 			return Promise.reject(ERRORS.INVALID_S3_IMAGE_URI)
		// 		}
        //         // if(await NFT.findOne({s3ImageUri})){
        //         //     return Promise.reject(ERRORS.S3_IMAGE_URI_NOT_UNIQUE)
        //         // }
        //         return true
        //     }),
        // body('s3ModelUri')
        //     .trim()
        //     .notEmpty()
        //     .withMessage(ERRORS.INVALID_S3_MODEL_URI_TYPE)
        //     .isString()
        //     .withMessage(ERRORS.INVALID_S3_MODEL_URI_TYPE)
        //     .custom(async (s3ModelUri) => {
        //         if (!isValidateS3Url(s3ModelUri)) {
		// 			return Promise.reject(ERRORS.INVALID_S3_MODEL_URI)
		// 		}
        //         // if(await NFT.findOne({s3ModelUri})){
        //         //     return Promise.reject(ERRORS.S3_MODEL_URI_NOT_UNIQUE)
        //         // }
        //         return true
        //     }),
        body('name')
            .trim()
            .notEmpty()
            .withMessage(ERRORS.INVALID_NFT_NAME_TYPE)
            .isString()
            .withMessage(ERRORS.INVALID_NFT_NAME_TYPE)
            .custom(async (name, {req}) => {
            
                // if (!isValidName(name)) {
				// 	return Promise.reject(ERRORS.INVALID_NFT_NAME)
				// }
                const nft = await NFT.findOne({name: { $regex: new RegExp(`^${name}$`, 'i') }})
                if(nft._id != req.params.requestId){
                    return Promise.reject(ERRORS.NFT_NAME_NOT_UNIQUE)
                }
                return true
            }),
        body('description')
            .optional()
            .trim()
            .notEmpty()
            .withMessage(ERRORS.INVALID_NFT_DESCRIPTION_TYPE)
            .isString()
            .withMessage(ERRORS.INVALID_NFT_DESCRIPTION_TYPE),
        body('amount')
            .notEmpty()
            .withMessage(ERRORS.INVALID_NFT_AMOUNT_TYPE)
            .isNumeric({gt: CONSTANTS.NFT_MIN_AMOUNT, lt: CONSTANTS.NFT_MAX_AMOUNT})
            .withMessage(ERRORS.INVALID_NFT_AMOUNT_RANGE),
        body('network')
            .notEmpty()
            .withMessage(ERRORS.INVALID_NETWORK_TYPE)
			.isIn(CONSTANTS.NETWORKS)
            .withMessage(ERRORS.INVALID_NETWORK_TYPE),
        
	]
}

const validateUser = () => {
	return [
        header('token')
            .trim()
            .notEmpty()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE)
            .isString()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE)
            .custom( async (token, {req}) => {
                const response = await fetch(CONSTANTS.AUTH_INFO_URL, {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: `token=${token}`})
                const data = await response.json()
                if(!("active" in data) || data.active != true){
                    return Promise.reject(ERRORS.CLIENT_UNAUTHORIZED)
                }
				const user = await User.findOne({ accountId: data.account_id })
                if (!user) {
					return Promise.reject(ERRORS.CLIENT_UNAUTHORIZED)
				}
                if (user && user.isBanned == true) {
					return Promise.reject(ERRORS.BANNED_USER)
				}
                req.tokenData = data;
                return true
            }),
	]
}

const validateAdminFee = () => {
	return [
        header('token')
            .trim()
            .notEmpty()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE)
            .isString()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE)
            .custom( async (token, {req}) => {
                const response = await fetch(CONSTANTS.AUTH_INFO_URL, {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: `token=${token}`})
                const data = await response.json()
                if(!("active" in data) || data.active != true){
                    return Promise.reject(ERRORS.CLIENT_UNAUTHORIZED)
                }
				const user = await User.findOne({ accountId: data.account_id })
                if (!user) {
					return Promise.reject(ERRORS.CLIENT_UNAUTHORIZED)
				}
                req.tokenData = data;
                return true
            })
	]
}

const validateGetAllNFTsUser = () => {
	return [
        header('token')
            .trim()
            .notEmpty()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE)
            .isString()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE)
            .custom( async (token, {req}) => {
                const response = await fetch(CONSTANTS.AUTH_INFO_URL, {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: `token=${token}`})
                const data = await response.json()
                if(!("active" in data) || data.active != true){
                    return Promise.reject(ERRORS.CLIENT_UNAUTHORIZED)
                }
				const user = await User.findOne({ accountId: data.account_id })
                if (!user) {
					return Promise.reject(ERRORS.CLIENT_UNAUTHORIZED)
				}
                if (user && user.isBanned == true) {
					return Promise.reject(ERRORS.BANNED_USER)
				}
                req.tokenData = data;
                return true
            }),
        query('status')
            .optional()
            .notEmpty()
            .withMessage(ERRORS.INVALID_STATUS_TYPE)
			.isIn(CONSTANTS.NFT_REQUEST_STATUSES)
            .withMessage(ERRORS.INVALID_STATUS_TYPE),
        query('type')
            .optional()
            .notEmpty()
            .withMessage(ERRORS.INVALID_TOOL_TYPE)
			.isIn(CONSTANTS.TOOL_TYPES)
            .withMessage(ERRORS.INVALID_TOOL_TYPE),
        query('network')
            .optional()
            .notEmpty()
            .withMessage(ERRORS.INVALID_NETWORK_TYPE)
			.isIn(CONSTANTS.NETWORKS)
            .withMessage(ERRORS.INVALID_NETWORK_TYPE),
        
	]
}

const validateWeaponName = () => {
	return [
        header('token')
            .trim()
            .notEmpty()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE)
            .isString()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE)
            .custom( async (token) => {
                const response = await fetch(CONSTANTS.AUTH_INFO_URL, {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: `token=${token}`})
                const data = await response.json()
                if(!("active" in data) || data.active != true){
                    return Promise.reject(ERRORS.CLIENT_UNAUTHORIZED)
                }
				const user = await User.findOne({ accountId: data.account_id })
                if (!user) {
					return Promise.reject(ERRORS.CLIENT_UNAUTHORIZED)
				}
                if (user && user.isBanned == true) {
					return Promise.reject(ERRORS.BANNED_USER)
				}
                return true
            }),
        body('name')
            .notEmpty()
            .withMessage(ERRORS.BAD_WEAPON_NAME)
			.isString()
            .withMessage(ERRORS.BAD_WEAPON_NAME)
	]
}

const validateGetNFT = () => {
	return [
        header('token')
            .trim()
            .notEmpty()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE)
            .isString()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE)
            .custom( async (token, {req}) => {
                const response = await fetch(CONSTANTS.AUTH_INFO_URL, {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: `token=${token}`})
                const data = await response.json()
                if(!("active" in data) || data.active != true){
                    return Promise.reject(ERRORS.CLIENT_UNAUTHORIZED)
                }
				const user = await User.findOne({ accountId: data.account_id })
                if (!user) {
					return Promise.reject(ERRORS.CLIENT_UNAUTHORIZED)
				}
                if (user && user.isBanned == true) {
					return Promise.reject(ERRORS.BANNED_USER)
				}
                req.tokenData = data;
                return true
            }),
        param('id')
			.isMongoId()
			.withMessage(ERRORS.INVALID_OBJECT_ID_FORMAT)
			.custom(async (id) => {
                const mintRequest = await NFT.findById(id)
				if (!mintRequest) {
					return Promise.reject(ERRORS.NO_NFT_FOUND)
				}
				return true
			}),
	]
}

const validateMintNFT = () => {
	return [
        body().custom(async () => {
            const app = await App.findOne({});
            if (!app) {
                return Promise.reject(ERRORS.APP_CONFIG_MISSING);
            }else if(app && app.pause === true){
                return Promise.reject(ERRORS.APP_IS_PAUSED);
            }
            return true;
        }),
        header('token')
            .trim()
            .notEmpty()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE)
            .isString()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE)
            .custom( async (token) => {
                const response = await fetch(CONSTANTS.AUTH_INFO_URL, {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: `token=${token}`})
                const data = await response.json()
                if(!("active" in data) || data.active != true){
                    return Promise.reject(ERRORS.CLIENT_UNAUTHORIZED)
                }
				const user = await User.findOne({ accountId: data.account_id })
                if (!user) {
					return Promise.reject(ERRORS.CLIENT_UNAUTHORIZED)
				}
                if (user && user.isBanned == true) {
					return Promise.reject(ERRORS.BANNED_USER)
				}
                return true
            }),
        param('id')
			.isMongoId()
			.withMessage(ERRORS.INVALID_OBJECT_ID_FORMAT)
			.custom(async (id) => {
                const mintRequest = await NFT.findById(id)
				if (!mintRequest) {
					return Promise.reject(ERRORS.NO_NFT_FOUND)
				}else if (mintRequest.status != CONSTANTS.NFT_REQUEST_STATUS_APPROVED){
					return Promise.reject(ERRORS.UNABLE_TO_MINT)
                }else if (mintRequest.txHash){
					return Promise.reject(ERRORS.ALREADY_MINTED)
                }
				return true
			}),
	]
}

const uploadNFTMetadataRules = () => {
	return [
        body().custom(async () => {
            const app = await App.findOne({});
            if (!app) {
                return Promise.reject(ERRORS.APP_CONFIG_MISSING);
            }else if(app && app.pause === true){
                return Promise.reject(ERRORS.APP_IS_PAUSED);
            }
            return true;
        }),
		body('metadata')
			.isObject()
			.withMessage(ERRORS.INVALID_NFT_METADATA_TYPE)
			.custom((metadata) => {

				// Check if metadata is not empty (at least one key-value pair)
				if (Object.keys(metadata).length === 0) {
					return Promise.reject('metadata is required')
				}
				// Add custom validation logic for individual attributes inside the metadata here
				// For example, check if companyName, description, or otherAttributes are strings, etc.
				// If the validation fails, throw an error or return false
				// If the validation passes, return true
				return true
			}),
		body('fileName')
			.notEmpty()
			.withMessage('fileName is required')
			.isString()
			.withMessage(ERRORS.INVALID_NFT_METADATA_FILENAME_TYPE)
	]
}

const validateUpload = () => {
    return [
        header('token')
            .trim()
            .notEmpty()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE)
            .isString()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE),
        query('id')
            .notEmpty()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE),
        // Custom validator that validates both token and id together
        (req, res, next) => {
            const errors = validationResult(req);
            if (!errors.isEmpty()) {
                return res.status(400).json({ errors: errors.array() });
            }

            const { admintoken } = req.headers;
            const { token } = req.headers;
            const { id } = req.query;

            // Perform combined validation logic here
            fetch(CONSTANTS.AUTH_INFO_URL, {
                method: 'POST',
                headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                body: `token=${token}`
            })
            .then(response => response.json())
            .then(async (data) => {
                if (!("active" in data) || data.active !== true) {
                    return Promise.reject(ERRORS.CLIENT_UNAUTHORIZED);
                }

                const admin = await Admin.exists({ accountId: data.account_id });
                if (!!admin)
                {
                    const { ADMIN_SIGN_KEY } = await getSecret(AWS_SECRET);
                    if(!await jwt.verify(admintoken, ADMIN_SIGN_KEY)){
                        console.log(admin)
                        return Promise.reject(ERRORS.CLIENT_UNAUTHORIZED)
                    }
                    return next();
                }

                const user = await User.findOne({ accountId: data.account_id });
                if (!user || user.nftFeeId !== id) {
                    return Promise.reject(ERRORS.CLIENT_UNAUTHORIZED);
                }

                // If validation passed
                next();
            })
            .catch(err => {
                return res.status(401).json({ error: err });
            });
        }
    ];
};

// Admin Routes Rules
const adminSignUpRules = () => {
	return [
        header('token')
            .trim()
            .notEmpty()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE)
            .isString()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE)
            .custom( async (token, {req}) => {
                const response = await fetch(CONSTANTS.AUTH_INFO_URL, {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: `token=${token}`})
                const data = await response.json()
                if(!("active" in data) || data.active != true){
                    return Promise.reject(ERRORS.CLIENT_UNAUTHORIZED)
                }
				if (await Admin.findOne({ accountId: data.account_id })) {
					return Promise.reject(ERRORS.ACCOUNT_ALREADY_EXISTS)
				}
                req.tokenData = data;
                return true
            }),
            body('walletAddress')
			.trim()
			.notEmpty()
			.withMessage(ERRORS.INVALID_WALLET_ADDRESS)
			.isString()
			.withMessage(ERRORS.INVALID_WALLET_ADDRESS)
			.custom(async (walletAddress) => {
				if (!isValidWalletAddress(walletAddress)) {
					return Promise.reject(ERRORS.INVALID_WALLET_ADDRESS)
				}
                if (await Admin.findOne({ walletAddress })) {
					return Promise.reject(ERRORS.WALLET_LINKED_TO_SOME_OTHER_ACCOUNT)
				}
				return true
			}),
	]
}

const adminSignInRules = () => {
	return [
        header('token')
            .trim()
            .notEmpty()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE)
            .isString()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE)
            .custom( async (token, {req}) => {
                const response = await fetch(CONSTANTS.AUTH_INFO_URL, {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: `token=${token}`})
                const data = await response.json()
                
                if(!("active" in data) || data.active != true){
                    return Promise.reject(ERRORS.CLIENT_UNAUTHORIZED)
                }
				if (!await Admin.findOne({ accountId: data.account_id })) {
                    return Promise.reject(ERRORS.NO_USER_FOUND)
				}
                req.tokenData = data;
                return true
            }),
        body('walletAddress')
            .custom(async (walletAddress, {req}) => {
                if (!isValidWalletAddress(walletAddress)) {
                    return Promise.reject(ERRORS.INVALID_WALLET_ADDRESS)
                }
                if (!await Admin.findOne({ walletAddress })) {
                	return Promise.reject(ERRORS.WALLET_ADDRESS_NOT_REGISTERED)
                }
                return true
            })
			
	]
}

const validateUpdateNFT = () => {
	return [
        header('token')
            .trim()
            .notEmpty()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE)
            .isString()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE)
            .custom( async (token) => {
                const response = await fetch(CONSTANTS.AUTH_INFO_URL, {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: `token=${token}`})
                const data = await response.json()
                if(!("active" in data) || data.active != true){
                    return Promise.reject(ERRORS.CLIENT_UNAUTHORIZED)
                }
				if (!await Admin.findOne({ accountId: data.account_id })) {
					return Promise.reject(ERRORS.CLIENT_UNAUTHORIZED)
				}
                return true
            }),
        param('nftId')
			.isMongoId()
			.withMessage(ERRORS.INVALID_OBJECT_ID_FORMAT)
			.custom(async (nftId) => {
                const nft = await NFT.findById(nftId)
				if (!nft) {
					return Promise.reject(ERRORS.NO_NFT_FOUND)
				}
                if(nft.status == CONSTANTS.NFT_REQUEST_STATUS_MINTED){
					return Promise.reject(ERRORS.UNABLE_TO_UPDATE)
                }
				return true
			}),
        // body('s3ImageUri')
        //     .optional()
        //     .trim()
        //     .notEmpty()
        //     .withMessage(ERRORS.INVALID_S3_IMAGE_URI_TYPE)
        //     .isString()
        //     .withMessage(ERRORS.INVALID_S3_IMAGE_URI_TYPE)
        //     .custom(async (s3ImageUri) => {
        //         if (!isValidateS3Url(s3ImageUri)) {
		// 			return Promise.reject(ERRORS.INVALID_S3_IMAGE_URI)
		// 		}
        //         // if(await NFT.findOne({s3ImageUri})){
        //         //     return Promise.reject(ERRORS.S3_IMAGE_URI_NOT_UNIQUE)
        //         // }
        //         return true
        //     }),
        // body('s3ModelUri')
        //     .optional()
        //     .trim()
        //     .notEmpty()
        //     .withMessage(ERRORS.INVALID_S3_MODEL_URI_TYPE)
        //     .isString()
        //     .withMessage(ERRORS.INVALID_S3_MODEL_URI_TYPE)
        //     .custom(async (s3ModelUri) => {
        //         if (!isValidateS3Url(s3ModelUri)) {
		// 			return Promise.reject(ERRORS.INVALID_S3_MODEL_URI)
		// 		}
        //         // if(await NFT.findOne({s3ModelUri})){
        //         //     return Promise.reject(ERRORS.S3_MODEL_URI_NOT_UNIQUE)
        //         // }
        //         return true
        //     }),
        body('name')
            .optional()
            .trim()
            .notEmpty()
            .withMessage(ERRORS.INVALID_NFT_NAME_TYPE)
            .isString()
            .withMessage(ERRORS.INVALID_NFT_NAME_TYPE)
            .custom(async (name, {req}) => {
            
                // if (!isValidName(name)) {
				// 	return Promise.reject(ERRORS.INVALID_NFT_NAME)
				// }
                const nft = await NFT.findOne({name: { $regex: new RegExp(`^${name}$`, 'i') }})
                if(nft._id != req.params.nftId){
                    return Promise.reject(ERRORS.NFT_NAME_NOT_UNIQUE)
                }
                return true
            }),
        body('description')
            .optional()
            .trim()
            .notEmpty()
            .withMessage(ERRORS.INVALID_NFT_DESCRIPTION_TYPE)
            .isString()
            .withMessage(ERRORS.INVALID_NFT_DESCRIPTION_TYPE),
        body('amount')
            .optional()
            .notEmpty()
            .withMessage(ERRORS.INVALID_NFT_AMOUNT_TYPE)
            .isNumeric({gt: CONSTANTS.NFT_MIN_AMOUNT, lt: CONSTANTS.NFT_MAX_AMOUNT})
            .withMessage(ERRORS.INVALID_NFT_AMOUNT_RANGE),
        body('status')
            .optional()
            .notEmpty()
            .withMessage(ERRORS.INVALID_STATUS_TYPE)
			.isIn(CONSTANTS.NFT_REQUEST_STATUSES)
            .withMessage(ERRORS.INVALID_STATUS_TYPE),
        body('feedback')
            .optional()
            .notEmpty()
            .withMessage(ERRORS.INVALID_FEEDBACK_TYPE)
			.isString()
            .withMessage(ERRORS.INVALID_FEEDBACK_TYPE),
        
	]
}

const validateGetAllNFTsAdmin = () => {
	return [
        header('token')
            .trim()
            .notEmpty()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE)
            .isString()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE)
            .custom( async (token) => {
                const response = await fetch(CONSTANTS.AUTH_INFO_URL, {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: `token=${token}`})
                const data = await response.json()
                if(!("active" in data) || data.active != true){
                    return Promise.reject(ERRORS.CLIENT_UNAUTHORIZED)
                }
				if (!await Admin.findOne({ accountId: data.account_id })) {
					return Promise.reject(ERRORS.CLIENT_UNAUTHORIZED)
				}
                return true
            }),
        param('status')
            .optional()
            .notEmpty()
            .withMessage(ERRORS.INVALID_STATUS_TYPE)
			.isIn(CONSTANTS.NFT_REQUEST_STATUSES)
            .withMessage(ERRORS.INVALID_STATUS_TYPE),
        
	]
}

const validateGetAllUsersAdmin = () => {
	return [
        header('token')
            .trim()
            .notEmpty()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE)
            .isString()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE)
            .custom( async (token) => {
                const response = await fetch(CONSTANTS.AUTH_INFO_URL, {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: `token=${token}`})
                const data = await response.json()
                if(!("active" in data) || data.active != true){
                    return Promise.reject(ERRORS.CLIENT_UNAUTHORIZED)
                }
				if (!await Admin.findOne({ accountId: data.account_id })) {
					return Promise.reject(ERRORS.CLIENT_UNAUTHORIZED)
				}
                return true
            }),
        param('isBanned')
            .optional()
            .notEmpty()
            .withMessage(ERRORS.INVALID_BAN_TYPE)
			.isBoolean()
            .withMessage(ERRORS.INVALID_BAN_TYPE),
        
	]
}

const validateGetNFTAdmin = () => {
	return [
        header('token')
            .trim()
            .notEmpty()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE)
            .isString()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE)
            .custom( async (token) => {
                const response = await fetch(CONSTANTS.AUTH_INFO_URL, {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: `token=${token}`})
                const data = await response.json()
                if(!("active" in data) || data.active != true){
                    return Promise.reject(ERRORS.CLIENT_UNAUTHORIZED)
                }
				if (!await Admin.findOne({ accountId: data.account_id })) {
					return Promise.reject(ERRORS.CLIENT_UNAUTHORIZED)
				}
                return true
            }),
        param('id')
			.isMongoId()
			.withMessage(ERRORS.INVALID_OBJECT_ID_FORMAT)
			.custom(async (id) => {
				if (!await NFT.findById(id)) {
					return Promise.reject(ERRORS.NO_NFT_FOUND)
				}
				return true
			}),
	]
}

const validateWebAuth = () => {

	return [
        header('auth')
            .trim()
            .notEmpty()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE)
            .isString()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE)
            .custom( async (pass) => {
                const { WEB_KEY } = await getSecret(AWS_SECRET);
                if(pass != WEB_KEY){
                    return Promise.reject(ERRORS.CLIENT_UNAUTHORIZED)
                }
                return true
            })
        ]
}

const validateGetAllStats = () => {

	return [
        header('auth')
            .trim()
            .notEmpty()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE)
            .isString()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE)
            .custom( async (pass) => {
                if(pass != GET_ALL_STATS_KEY){
                    return Promise.reject(ERRORS.CLIENT_UNAUTHORIZED)
                }
                return true
            })
        ]
}

const validateStatAuth = () => {

	return [
        header('auth')
            .trim()
            .notEmpty()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE)
            .isString()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE)
            .custom( async (signature, {req}) => {
                const { UPDATE_STATS_KEY } = await getSecret(AWS_SECRET);
                // HMAC is used here because it's easier to implement than JWT on UE side.
                //const formedData = Array.isArray(req.body) ? formHMACFromStats(req.body[0]) : formHMACFromStats(req.body);
                console.log(req.body[0]);
                const formedData = formHMACFromStats(req.body[0]);
                const verifySig = verifyHMACSignature(formedData, signature, UPDATE_STATS_KEY);
                if(!verifySig){
                    console.log("Signature verify failed");
                    return Promise.reject(ERRORS.CLIENT_UNAUTHORIZED)
                }
                return true
            })
        ]
}

const validateGetStats = () => {

	return [
        header('clientToken')
            .trim()
            .notEmpty()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE)
            .isString()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE)
            .custom( async (token, {req}) => {
                const tokenData = await jwt.verify(token, CLIENT_GAME_TOKEN_PASS);
                if(!tokenData){
                    return Promise.reject(ERRORS.CLIENT_UNAUTHORIZED)
                }
                //console.log(tokenData.result);
                req.tokenData = tokenData;
                return true
            })
        ]
}

const validateZealyKey = () => {

	return [
        header('x-api-key')
            .trim()
            .notEmpty()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE)
            .isString()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE)
            .custom( async (key, {req}) => {
                
                if (key !== ZEALY_KEY)
                {
                    return Promise.reject(ERRORS.CLIENT_UNAUTHORIZED)
                }
                return true
            })
        ]
}

const validateSimpleSignin = () => {

	return [
        header('token')
            .trim()
            .notEmpty()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE)
            .isString()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE)
            .custom( async (token, {req}) => {
                const response = await fetch(CONSTANTS.AUTH_INFO_URL, {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: `token=${token}`})
                const data = await response.json()
                
                if(!("active" in data) || data.active != true){
                    return Promise.reject(ERRORS.CLIENT_UNAUTHORIZED)
                }
				//if (!await User.findOne({ accountId: data.account_id })) {
                //    return Promise.reject(ERRORS.NO_USER_FOUND)
				//}
                req.tokenData = data;
                return true;
            })
	]
}

const validateAdmin = () => {

	return [
        header('admintoken')
            .trim()
            .notEmpty()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE)
            .isString()
            .withMessage(ERRORS.INVALID_TOKEN_TYPE)
            .custom( async (token) => {
                const { ADMIN_SIGN_KEY } = await getSecret(AWS_SECRET);
                if(!await jwt.verify(token, ADMIN_SIGN_KEY)){
                    return Promise.reject(ERRORS.CLIENT_UNAUTHORIZED)
                }
                return true
            })
        ]
}

const validate = (req, res, next) => {

	const errors = validationResult(req)
	if (errors.isEmpty()) {
		return next()
	}
	const errorMessages = errors.array()[0].msg //Only the first error
	responseBadRequest(res, errorMessages)
}

module.exports = {
	validate,
    banRules,
	userSignUpRules,
    userSignInRules,
    validateMintRequest,
    validateUser,
    uploadNFTMetadataRules,
    validateRegeneratedMintRequest,
    adminSignUpRules,
    adminSignInRules,
    validateUpdateNFT,
    validateGetNFT,
    validateMintNFT,
    validateGetAllNFTsAdmin,
    validateGetNFTAdmin,
    validateGetAllNFTsUser,
    validateAssetsURI,
    validateGetAllUsersAdmin,
    updateStatusRules,
    validatecreateAuction,
    validateGetAuction,
    validatePlaceBid,
    validateEndAuction,
    validatePendingRefunds,
    validateStatAuth,
    validateWebAuth,
    validateAdmin,
    validateWeaponName,
    validateUpload,
    validateS3sign,
    validateAdminFee,
    validateScan,
    validateGetStats,
    validateGetAllStats,
    validateSimpleSignin,
    validateZealyKey
}