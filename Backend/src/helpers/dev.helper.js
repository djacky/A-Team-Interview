const { Dev } = require('../models');
//const CONSTANTS = require('../constants')

const getIsDelete = async () => {
	return Dev.isDeleteGame()
}

const confirmDelete = async () => {
	return Dev.confirmDelete()
}


module.exports = {
	getIsDelete,
	confirmDelete
}
