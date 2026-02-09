const tournamentHelper = require('../helpers/tournament.helper')

const { responseSuccess, responseBadRequest, responseUnauthorized, responseServerSideError } = require('../utils/responseTypes')
const ERRORS = require('../utils/errorTypes')

const startTournament = async (req, res) => {
    try 
    {
        const response = await tournamentHelper.setTournament(req.body);
        return responseSuccess(res, {})
    } catch (error) 
    {
        return responseServerSideError(res, error)
    }
}

const clearTournaments = async (req, res) => {
    try 
    {
        const response = await tournamentHelper.clearOldEntries(req.query.days);
        return responseSuccess(res, {})
    } catch (error) 
    {
        return responseServerSideError(res, error)
    }
}

const userStartTournament = async (req, res) => {
    try 
    {
        const {status, message} = await tournamentHelper.userSetTournament(req.tokenData.accountId, req.body);
        if (status)
        {
            return responseSuccess(res, {message})
        }
        else
        {
            return responseServerSideError(res, message);
        }
        
    } catch (error) 
    {
        return responseServerSideError(res, error)
    }
}

const getTournaments = async (req, res) => {
    try 
    {
        const tournaments = await tournamentHelper.getTournaments(req.tokenData.accountId, req.query.region, req.query.type);
        return responseSuccess(res, {tournaments})
    } catch (error) 
    {
        return responseServerSideError(res, error)
    }
}

const getSingleTournament = async (req, res) => {
    try 
    {
        const tournament = await tournamentHelper.getSingleTournament(req.query.tournament, req.tokenData.accountId, req.query.region);
        return responseSuccess(res, {tournament})
    } catch (error) 
    {
        return responseServerSideError(res, error)
    }
}

const getTimes = async (req, res) => {
    try 
    {
        const times = await tournamentHelper.getTimes();
        return responseSuccess(res, {times})
    } catch (error) 
    {
        return responseServerSideError(res, error)
    }
}

const registerPlayer = async (req, res) => {
    try 
    {
        const { status, message, tournament } = await tournamentHelper.registerPlayer(req.tokenData.accountId, req.body);
        //const { status, message, tournament } = await tournamentHelper.registerPlayer("df2424bba23f4c7f8226220b5626f353", req.body);
        //const status = await tournamentHelper.registerPlayer(req.body.tournamentId, req.body.playerId, req.body.playerName, req.body.playerEmail);
        if (status)
        {
            return responseSuccess(res, {tournament});
        }
        else
        {
            return responseServerSideError(res, message);
        }
        
    } catch (error) 
    {
        return responseServerSideError(res, error);
    }
}

const unregisterPlayer = async (req, res) => {
    try 
    {
        const { status, message, tournament } = await tournamentHelper.unregisterPlayer(req.body, req.tokenData.accountId);
        //const { status, message } = await tournamentHelper.unregisterPlayer(req.query.tournamentId, "df2424bba23f4c7f8226220b5626f353");
        if (status)
        {
            return responseSuccess(res, {tournament});
        }
        else
        {
            return responseServerSideError(res, message);
        }
        
    } catch (error) 
    {
        return responseServerSideError(res, error);
    }
}

const setScores = async (req, res) => {
    try 
    {
        const tourData = await tournamentHelper.setScores(req.body[0]);
        if (tourData)
        {
            return responseSuccess(res, {tourData});
        }
        else
        {
            return responseServerSideError(res, "Error with saving scores.");
        }
    } catch (error) 
    {
        return responseServerSideError(res, error)
    }
}

const getScores = async (req, res) => {
    try 
    {
        const scores = await tournamentHelper.getScores();
        return responseSuccess(res, {scores});
    } catch (error) 
    {
        return responseServerSideError(res, error)
    }
}

const report = async (req, res) => {
    try 
    {
        const tour = await tournamentHelper.reportPlayer(req.body, req.tokenData.account_id);
        if (tour)
        {
            return responseSuccess(res, {tour});
        }
        else
        {
            return responseServerSideError(res, error);
        }
        
    } catch (error) 
    {
        return responseServerSideError(res, error)
    }
}

const getPlayers = async (req, res) => {
    try 
    {
        const players = await tournamentHelper.getPlayers(req.body[0]);
        if (players)
        {
            return responseSuccess(res, {players});
        }
        else
        {
            return responseServerSideError(res, "Error with getting players.");
        }
    } catch (error) 
    {
        return responseServerSideError(res, error)
    }
}

const checkMatchRegister = async (req, res) => {
    try 
    {
        const bIsRegistered = await tournamentHelper.checkMatchRegister(req.body, req.query.tourId);
        if (bIsRegistered)
        {
            return responseSuccess(res, {});
        }
        else
        {
            return res.status(400).send({message: 'You are not registered for this match. Please follow the instructions in the quest.'});
        }
    } catch (error) 
    {
        return res.status(400).send({message: error.message});
    }
}

const checkMatchCompletion = async (req, res) => {
    try 
    {
        const tournament = await tournamentHelper.checkMatchCompletion(req.body, req.query.tourId);
        if (tournament)
        {
            return responseSuccess(res, {});
        }
        else
        {
            return res.status(400).send({message: 'You did not compete in this tournament.'});
        }
    } catch (error) 
    {
        return res.status(400).send({message: error.message});
    }
}

const checkStakeAccess = async (req, res) => {

    return responseSuccess(res, {});
}

module.exports = {
    startTournament,
    userStartTournament,
    getTournaments,
    getSingleTournament,
    registerPlayer,
    unregisterPlayer,
    setScores,
    getScores,
    report,
    checkStakeAccess,
    getPlayers,
    getTimes,
    checkMatchRegister,
    checkMatchCompletion
}
