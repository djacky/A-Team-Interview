const { Stats, User, Tournament, Level } = require('../models')

const updateUserStats = (statsBody, type, tourId) => {
	return Stats.updateUserStat(statsBody, type, tourId, Tournament, Level)
}

const getUserStats = (userId) => {
	return Stats.getStats(userId, User)
}

const getBestPlayers = (num) => {
	return Stats.getTopPlayers(num)
}

const checkAchievement = (body, params) => {
	return Stats.checkAchievement(body, params, User)
}

const getLatestMetrics = (tourId) => {
	return Stats.getLatestMetrics(tourId, Tournament)
}

const buyAsset = (userId, body) => {
	return Stats.buyAsset(userId, body)
}

const getLevelRequirements = () => {
	return Stats.getLevelRequirements(Level)
}

module.exports = {
	updateUserStats,
	getUserStats,
	getBestPlayers,
	checkAchievement,
	getLatestMetrics,
	buyAsset,
	getLevelRequirements
}
