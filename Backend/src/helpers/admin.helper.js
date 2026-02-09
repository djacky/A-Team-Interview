const { Admin } = require('../models')

const createNewAdmin = (adminDetails, options) => {
	return Admin.createNewAdmin(adminDetails, options)
}

const getAdminByAccountId = async (accountId, options) => {
	return Admin.fetchAdminByUniqueKeys({ accountId: accountId }, options, loginKey)
}

const getAdminByQuery = async (findQuery, options, loginKey) => {
	return Admin.fetchAdminByUniqueKeys(findQuery, options, loginKey)
}

module.exports = {
	createNewAdmin,
	getAdminByAccountId,
	getAdminByQuery
}
