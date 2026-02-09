const jwt = require('jsonwebtoken')
const { JWT_SECRET } = require('../config')

const { responseUnauthorized } = require('../utils/responseTypes')

const { getUserById } = require('../helpers/user.helper')
const { getAdminDetailsById } = require('../helpers/admin.helper')

const CONSTANTS = require('../constants')

const authenticate = (req, res, next) => {
	let authorization = req.header('Authorization')
	if (!authorization) {
		const errorMessage = 'Authorization header not found'
		return responseUnauthorized(res, errorMessage)
	}

	let token = authorization.split(' ')
	jwt.verify(token[1], JWT_SECRET, async function (error, token_decoded) {
		if (error) {
			return responseUnauthorized(res, error)
		} else {
			if (token_decoded.authToken) {

				if (token_decoded.role === CONSTANTS.USER_ROLE_PLAYER) {
					const userObject = await getUserById((token_decoded)._id)

					if (!userObject) {
						const errorMessage = 'Invalid authorization token'
						return responseUnauthorized(res, errorMessage)
					}
				}

				req.auth = token_decoded
				next()
			} else {
				const errorMessage = 'Invalid authorization token'
				return responseUnauthorized(res, errorMessage)
			}
		}
	})
}

const adminAuthenticate = (req, res, next) => {
	let authorization = req.header('Authorization')
	if (!authorization) {
		const errorMessage = 'Authorization header not found'
		return responseUnauthorized(res, errorMessage)
	}

	let token = authorization.split(' ')
	jwt.verify(token[1], JWT_SECRET, async function (error, token_decoded) {
		if (error) {
			console.log('Error in adminAuthenticate', error)
			return responseUnauthorized(res, error)
		} else {
			if (token_decoded.authToken && token_decoded.role === CONSTANTS.USER_ROLE_ADMIN) {
				const adminUserObject = await getAdminDetailsById((token_decoded)._id)

				if (!adminUserObject) {
					const errorMessage = 'Invalid authorization token'
					return responseUnauthorized(res, errorMessage)
				}

				req.auth = token_decoded
				next()
			} else {
				const errorMessage = 'Invalid authorization token'
				console.log('Error in adminAuthenticate', errorMessage)
				return responseUnauthorized(res, errorMessage)
			}
		}
	})
}

module.exports = {
	authenticate,
	adminAuthenticate
}
