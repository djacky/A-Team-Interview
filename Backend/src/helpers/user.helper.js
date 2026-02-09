const { User } = require('../models')

const createNewUser = (userDetails, options) => {
	return User.createNewUser(userDetails, options)
}

const getUserByAccountId = async (accountId, options) => {
	return User.fetchUserByUniqueKeys({ accountId: accountId }, options)
}

const getUserByQuery = async (findQuery, options) => {
	return User.fetchUserByUniqueKeys(findQuery, options)
}

const fetchUsersByQuery = (findQuery, options) => {
	return User.fetchUsersByQuery(findQuery, options)
}

const updateUserDetailsById = (userId, userUpdates, options) => {
	return User.updateUserByUniqueKeys({ _id: userId }, userUpdates, options)
}

const reportPlayer = (reporter, body) => {
	return User.reportAPlayer(reporter, body)
}

const setEmail = (userId, body) => {
	return User.setEmail(userId, body)
}

const checkEmail = (body) => {
	return User.checkEmail(body)
}

const getAllUsers = (options) => {
	return User.fetchUsersByQuery({}, options)
}

const setDiscordId = (queryParams) => {
	return User.setDiscordId(queryParams)
}



module.exports = {
	createNewUser,
	getUserByAccountId,
	getUserByQuery,
	updateUserDetailsById,
	fetchUsersByQuery,
	reportPlayer,
	setEmail,
	checkEmail,
	setDiscordId
}
