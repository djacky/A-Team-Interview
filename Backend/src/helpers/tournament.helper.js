const { Tournament, User, ScheduledTasks, Stats } = require('../models');

const startTournamentTasks = () => {
	return Tournament.startTasks(ScheduledTasks, User)
}

const clearOldEntries = (days) => {
	return Tournament.clearOldEntries(ScheduledTasks, days)
}

const setTournament = (reqBody) => {
	return Tournament.setTournament(reqBody, ScheduledTasks)
}

const userSetTournament = (Id, reqBody) => {
	return Tournament.userSetTournament(Id, reqBody, ScheduledTasks, User)
}

const getTournaments = (playerId, region, type) => {
	return Tournament.getAllTournaments(playerId, region, type)
}

const getSingleTournament = (tournamentId, playerId, region) => {
	return Tournament.getSingleTournament(tournamentId, playerId, region)
}

const getTimes = () => {
	return Tournament.getRegisteredTimes()
}

const registerPlayer = (playerId, body) => {
	return Tournament.addPlayer(playerId, body, User, Stats)
}

const unregisterPlayer = (body, playerId) => {
	return Tournament.removePlayer(body, playerId, User, Stats)
}

const setScores = (reqBody) => {
	return Tournament.saveScores(reqBody, User, Stats)
}

const getScores = () => {
	return Tournament.getScores()
}

const reportPlayer = (body, reporter) => {
	return Tournament.reportPlayer(body, reporter)
}

const getPlayers = (body) => {
	return Tournament.getPlayers(body)
}

const checkMatchRegister = (body, tournamentId) => {
	return Tournament.checkMatchRegister(body, tournamentId, User)
}

const checkMatchCompletion = (body, tournamentId) => {
	return Tournament.checkMatchCompletion(body, tournamentId, User)
}

module.exports = {
	setTournament,
	clearOldEntries,
	userSetTournament,
	getTournaments,
	getSingleTournament,
	registerPlayer,
	unregisterPlayer,
	setScores,
	getScores,
	reportPlayer,
	startTournamentTasks,
	getPlayers,
	getTimes,
	checkMatchRegister,
	checkMatchCompletion
}
