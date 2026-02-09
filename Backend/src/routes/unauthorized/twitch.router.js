const twitchRouter = require('express').Router()
const twitchController = require('../../controllers/twitchController')
const rateLimit = require('express-rate-limit');
const { userSignUpRules, userSignInRules, validateUser, validate } = require('../../middlewares/request.validators')

const loginLimiter = rateLimit({
    windowMs: 15 * 60 * 1000, // 15 minutes
    max: 15, // limit each IP to 15 requests per 15 minutes
  });

twitchRouter.get('', loginLimiter, twitchController.test)

module.exports = twitchRouter