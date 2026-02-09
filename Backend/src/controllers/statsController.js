
const statsHelper = require('../helpers/stats.helper')
const { responseSuccess, responseBadRequest, responseUnauthorized, responseServerSideError } = require('../utils/responseTypes')

//const ERRORS = require('../utils/errorTypes')
//const CONSTANTS = require('../constants')


const updateStats = async (req, res) => {
    try 
    {
        const playerData = await statsHelper.updateUserStats(req.body, req.query.type, req.query.tourId);
        if (playerData)
        {
            responseSuccess(res, {playerData});
        }
        else
        {
            console.log("updateStats: playerData was null");
            return responseBadRequest(res, ERRORS.NO_USER_FOUND);
        }
        
    } 
    catch (error) 
    {
        return responseServerSideError(res, {MESSAGE: error.message});
    }
}

const getStats = async (req, res) => {
    try 
    {
        let userStats = await statsHelper.getUserStats(req.tokenData.accountId)
        if (userStats)
        {
            let levelReqs = await statsHelper.getLevelRequirements();
            let stats = userStats;
            return responseSuccess(res, {stats, levelReqs});
        }
        else
        {
            return responseBadRequest(res, ERRORS.NO_USER_FOUND);
        }
        
    } 
    catch (error) 
    {
        return responseServerSideError(res, {MESSAGE: error.message})
    }
}

const getTopPlayersStats = async (req, res) => {
    try 
    {
        let numPlayers = req.query.numPlayers ? req.query.numPlayers : 50
        let bestPlayers = await statsHelper.getBestPlayers(numPlayers)
        responseSuccess(res, {bestPlayers})
    } 
    catch (error) 
    {
        return responseServerSideError(res, {MESSAGE: error.message})
    }
}

const checkStat = async (req, res) => {
    try 
    {
        const result = await statsHelper.checkAchievement(req.body, req.query);
        if (result)
        {
            responseSuccess(res, {});
        }
        else
        {
            return res.status(400).send({message: `You did not compete today, or you did not meet the quest of getting a ${req.query.type} > ${req.query.threshold}`});
        }
    } 
    catch (error) 
    {
        return res.status(400).send({message: error.message});
    }
}

const getLatestMetrics = async (req, res) => {
    try 
    {
        const playerMetrics = await statsHelper.getLatestMetrics(req.query.tourId);
        if (playerMetrics)
        {
            responseSuccess(res, {playerMetrics});
        }
        else
        {
            return responseBadRequest(res, ERRORS.NO_USER_FOUND);
        }
    } 
    catch (error) 
    {
        return responseServerSideError(res, {MESSAGE: error.message})
    }
}

const buyAsset = async (req, res) => {
    try 
    {
        const data = await statsHelper.buyAsset(req.tokenData.accountId, req.body);
        if (data)
        {
            responseSuccess(res, {data});
        }
        else
        {
            return responseBadRequest(res, ERRORS.NO_USER_FOUND);
        }
    } 
    catch (error) 
    {
        return responseServerSideError(res, {MESSAGE: error.message})
    }
}

module.exports = {
    updateStats,
    getStats,
    getTopPlayersStats,
    checkStat,
    getLatestMetrics,
    buyAsset
}