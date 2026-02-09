const statsRouter = require('express').Router()
const statsController = require('../../controllers/statsController')
const { validateGetAllStats, validate, validateStatAuth, validateWebAuth, validateZealyKey, validateGetStats } = require('../../middlewares/request.validators')

const rateLimit = require('express-rate-limit');
const statsLimiter = rateLimit({
    windowMs: 15 * 60 * 1000, // 15 minutes
    max: 15, // limit each IP to 10 requests per 15 minutes
  });

const statsLimiter2 = rateLimit({
    windowMs: 15 * 60 * 1000, // 15 minutes
    max: 30, // limit each IP to 30 requests per 15 minutes
  });

statsRouter.post('/updateStats', validateStatAuth(), validate, statsController.updateStats)
statsRouter.get('/getStats', validateGetStats(), validate, statsLimiter, statsController.getStats)
statsRouter.get('/getTopPlayersStats', validateGetAllStats(), validate, statsController.getTopPlayersStats)
statsRouter.get('/getPlayerMetrics', validateGetStats(), validate, statsLimiter2, statsController.getLatestMetrics)

statsRouter.post('/buyAsset', validateGetStats(), validate, statsLimiter2, statsController.buyAsset)
statsRouter.post('/checkStat', validate, statsLimiter, statsController.checkStat)

module.exports = statsRouter
/*


userRouter.post('/signup', userSignUpRules(), validate, userController.signUp)
userRouter.post('/signin', userSignInRules(), validate, userController.signIn)

userRouter.post('/address', validateUser(), validate, userController.getWalletAddress)

module.exports = userRouter
*/
