const gameAccessRouter = require('express').Router()
const gameAccessController = require('../../controllers/gameAccessController')
const { WEB_PASS, UPDATE_STATS_PASS } = require('../../config')
const { userSignUpRules, userSignInRules, validate, validateSimpleSignin, validateGetStats, validateWebAuth } = require('../../middlewares/request.validators')

gameAccessRouter.get('/loginGameToken', validateSimpleSignin(), validate, gameAccessController.getLoginGameToken) // add epic games validation
gameAccessRouter.get('/updateGameToken', validateSimpleSignin(), validate, gameAccessController.updateGameToken) // add epic games validation
gameAccessRouter.get('/verifyAccessData', validateGetStats(), validate, gameAccessController.verifyGameToken)
gameAccessRouter.post('/adminUpdateData', validateWebAuth(), validate, gameAccessController.adminUpdate)

module.exports = gameAccessRouter
/*


userRouter.post('/signup', userSignUpRules(), validate, userController.signUp)
userRouter.post('/signin', userSignInRules(), validate, userController.signIn)

userRouter.post('/address', validateUser(), validate, userController.getWalletAddress)

module.exports = userRouter
*/