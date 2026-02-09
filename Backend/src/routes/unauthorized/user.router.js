const userRouter = require('express').Router()
const userController = require('../../controllers/userController')
const rateLimit = require('express-rate-limit');
const { userSignUpRules, userSignInRules, validateUser, validateSimpleSignin, validateGetStats, validateZealyKey, validate } = require('../../middlewares/request.validators')

const loginLimiter = rateLimit({
    windowMs: 15 * 60 * 1000, // 15 minutes
    max: 15, // limit each IP to 15 requests per 15 minutes
  });

userRouter.post('/signUp', userSignUpRules(), validate, loginLimiter, userController.signUp)
userRouter.post('/signin', userSignInRules(), validate, userController.signIn)
userRouter.post('/email', validateGetStats(), validate, loginLimiter, userController.setEmail)
userRouter.post('/checkEmail', validateZealyKey(), validate, loginLimiter, userController.checkEmail)

userRouter.post('/address', validateUser(), validate, loginLimiter, userController.getWalletAddress)
userRouter.post('/reportPlayer', validateSimpleSignin(), validate, loginLimiter, userController.reportPlayer)

userRouter.get('/discordRedirect', loginLimiter, userController.discordRedirect)

// unrelated to game
userRouter.post('/aiSessionId', loginLimiter, userController.aiSessionId)
userRouter.post('/askAiAboutMe', loginLimiter, userController.askAiAboutMe)

module.exports = userRouter