const blockchainActionRouter = require('express').Router()
const blockchainActionController = require('../../controllers/blockchainActionController')
const { WEB_PASS, UPDATE_STATS_PASS } = require('../../config')
const { userSignUpRules, userSignInRules, validateUser, validate, validateGetStats, validateAdminFee } = require('../../middlewares/request.validators')

const rateLimit = require('express-rate-limit');
const transactionLimiter = rateLimit({
    windowMs: 15 * 60 * 1000, // 15 minutes
    max: 15, // limit each IP to 15 requests per 15 minutes
  });

blockchainActionRouter.post('/sendTransaction', transactionLimiter, validateGetStats(), validate, blockchainActionController.sendTokens);
blockchainActionRouter.post('/sendAdminFee', validateAdminFee(), validate, blockchainActionController.sendNFTAdminFee);
blockchainActionRouter.post('/nftFee', userSignInRules(), validate, transactionLimiter, blockchainActionController.NFTFeeNotify);
blockchainActionRouter.get('/updateId', transactionLimiter, blockchainActionController.updateTransactionId);

module.exports = blockchainActionRouter
/*


userRouter.post('/signup', userSignUpRules(), validate, userController.signUp)
userRouter.post('/signin', userSignInRules(), validate, userController.signIn)

userRouter.post('/address', validateUser(), validate, userController.getWalletAddress)

module.exports = userRouter
*/