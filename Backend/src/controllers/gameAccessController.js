
const gameAccessHelper = require('../helpers/gameAccess.helper')
const { responseSuccess, responseBadRequest, responseUnauthorized, responseServerSideError } = require('../utils/responseTypes')

//const ERRORS = require('../utils/errorTypes')
//const CONSTANTS = require('../constants')


const getLoginGameToken = async (req, res) => {
    try 
    {
        //statsHelper.updateUserStats(req.body)
        const { data } = await gameAccessHelper.getLoginToken(req.tokenData.account_id, req.query.userName);

        return responseSuccess(res, {data})
    } 
    catch (error) 
    {
        return responseServerSideError(res, {MESSAGE: error.message})
    }
}


const updateGameToken = async (req, res) => {
    try 
    {
        //statsHelper.updateUserStats(req.body)
        const { data } = await gameAccessHelper.updateToken(req.tokenData.account_id, req.query);
        return responseSuccess(res, {data});
    } 
    catch (error) 
    {
        return responseServerSideError(res, {MESSAGE: error.message})
    }
}

const verifyGameToken = async (req, res) => {
    try 
    {
        //const data = await gameAccessHelper.verifyToken(req.tokenData)
        const data = req.tokenData;
        return data.accountId ? responseSuccess(res, {data}) : responseServerSideError(res, {MESSAGE: error.message});
    } 
    catch (error) 
    {
        console.log(error);
        return responseServerSideError(res, {MESSAGE: error.message})
    }
}

const adminUpdate = async (req, res) => {
    try 
    {
        //statsHelper.updateUserStats(req.body)
        const { data } = await gameAccessHelper.adminUpdateData(req.body);
        return responseSuccess(res, {data});
    } 
    catch (error) 
    {
        return responseServerSideError(res, {MESSAGE: error.message})
    }
}

module.exports = {
    getLoginGameToken,
    updateGameToken,
    verifyGameToken,
    adminUpdate
}