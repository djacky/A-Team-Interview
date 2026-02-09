const { User } = require('../models')

const getLoginToken = (userID, userName) => {
	return User.loginAccessData(userID, userName)
}

const updateToken = (userId, query) => {
	return User.updateAccessData(userId, query)
}

const verifyToken = (token) => {
	return User.verifyGameToken(token)
}

const adminUpdateData = (query) => {
	return User.adminUpdateAccessData(query)
}


module.exports = {
	getLoginToken,
	updateToken,
	verifyToken,
	adminUpdateData
}
