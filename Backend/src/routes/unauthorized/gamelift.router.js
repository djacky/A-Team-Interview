const gameliftRouter = require('express').Router()
const gameliftController = require('../../controllers/gameLiftController')
const rateLimit = require('express-rate-limit');
const { userSignUpRules, userSignInRules, validateUser, validateGetStats, validate } = require('../../middlewares/request.validators')

const loginLimiter = rateLimit({
    windowMs: 15 * 60 * 1000, // 15 minutes
    max: 15, // limit each IP to 15 requests per 15 minutes
  });

gameliftRouter.get('/creds', loginLimiter, gameliftController.getCognitoCredentials)
gameliftRouter.post('/createSession', validateGetStats(), validate, gameliftController.startSessionQueue)
//gameliftRouter.post('/createSession', loginLimiter, gameliftController.startSessionQueue)

module.exports = gameliftRouter