const jwt = require('jsonwebtoken')
const { JWT_SECRET, JWT_EXPIRY } = require('../config')

const _createJWT = (data, expiry) => {
	return (jwt.sign(data, JWT_SECRET, { expiresIn: expiry || JWT_EXPIRY }))
}

/* In a JWT, you should generally include information that is necessary for authentication and authorization purposes. This typically 
includes a unique identifier for the user, such as the object ID or username, along with any relevant roles or permissions.
It's important to note that the JWT itself is not meant to store sensitive information, such as passwords */

const generateUserAuthToken = (data) => {
	return _createJWT({
		_id: data._id,
		role: data.role,
		email: data.email,
		nickName: data.nickName,
		walletAddress: (data.walletAddress).toLowerCase(),
		authToken: true
	})
}

const generateAdminAuthToken = (data) => {
	return _createJWT({
		_id: data._id,
		email: data.email,
		role: data.role,
		authToken: true
	})
}

module.exports = {
	generateUserAuthToken,
	generateAdminAuthToken
}