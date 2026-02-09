const { App } = require('../models')

const updateAppDetailsById = (appId, appUpdates, options) => {
	return App.updateAppByUniqueKeys({ _id: appId }, appUpdates, options)
}

const getApp = (options) => {
	return App.fetchAppsByQuery({}, options)
}



module.exports = {
	updateAppDetailsById,
	getApp
}
