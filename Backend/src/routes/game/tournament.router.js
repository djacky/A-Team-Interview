const tournamentRouter = require('express').Router()
const tournamentController = require('../../controllers/tournamentController')
const createQueueProcessor = require('../queueProcessor')
const { validateSimpleSignin, validateStatAuth, validateWebAuth, validateGetStats, userSignInRules, validateZealyKey, validate } = require('../../middlewares/request.validators')

const rateLimit = require('express-rate-limit');
const limiter = rateLimit({
    windowMs: 15 * 60 * 1000, // 15 minutes
    max: 70, // limit each IP to 70 requests per 15 minutes
  });

  const reportLimiter = rateLimit({
    windowMs: 10 * 60 * 1000, // 10 minutes
    max: 2, 
  });

const registerPlayerQueue = createQueueProcessor(tournamentController.registerPlayer);
const unregisterPlayerQueue = createQueueProcessor(tournamentController.unregisterPlayer);
const userCreateTournamentQueue = createQueueProcessor(tournamentController.userStartTournament);

tournamentRouter.post('/startTournament', validateWebAuth(), validate, tournamentController.startTournament)
tournamentRouter.post('/userCreateTournament', limiter, validateGetStats(), validate, userCreateTournamentQueue)
tournamentRouter.get('/getTimes', limiter, validateGetStats(), validate, tournamentController.getTimes)
tournamentRouter.get('/getTournaments', limiter, validateGetStats(), validate, tournamentController.getTournaments)
tournamentRouter.get('/getSingleTournament', limiter, validateGetStats(), validate, tournamentController.getSingleTournament)


tournamentRouter.post('/register', limiter, validateGetStats(), validate, registerPlayerQueue)
tournamentRouter.post('/unregister', limiter, validateGetStats(), validate, unregisterPlayerQueue)
tournamentRouter.post('/setScores', limiter, validateStatAuth(), validate, tournamentController.setScores)
tournamentRouter.get('/getScores', limiter, validateWebAuth(), validate, tournamentController.getScores)
tournamentRouter.post('/getPlayers', limiter, validateStatAuth(), validate, tournamentController.getPlayers)

tournamentRouter.post('/report', reportLimiter, validateSimpleSignin(), validate, tournamentController.report)
tournamentRouter.post('/checkStakeAccess', limiter, userSignInRules(), validate, tournamentController.checkStakeAccess)

tournamentRouter.post('/checkMatchReg', limiter, validateZealyKey(), validate, tournamentController.checkMatchRegister)
tournamentRouter.post('/checkMatchComplete', limiter, validateZealyKey(), validate, tournamentController.checkMatchCompletion)

module.exports = tournamentRouter
