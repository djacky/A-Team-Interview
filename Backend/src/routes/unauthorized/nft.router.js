const nftRouter = require('express').Router()
const nftController = require('../../controllers/nftController')
const rateLimit = require('express-rate-limit');

const { validateMintRequest, validateRegeneratedMintRequest, validateGetAllNFTsUser, validateGetNFT, validateMintNFT, validateAssetsURI, validateWeaponName, userSignInRules, validate } = require('../../middlewares/request.validators')

const transactionLimiter = rateLimit({
    windowMs: 15 * 60 * 1000, // 15 minutes
    max: 10, // limit each IP to 10 requests per 15 minutes
  });

nftRouter.post('/request', validateMintRequest(), validate, nftController.createRequest)
nftRouter.put('/regenerate-request/:requestId', validateRegeneratedMintRequest(), validate, nftController.regenerateRequest)
nftRouter.get('/all', validateGetAllNFTsUser(), validate, nftController.getNFTsByAccountId)
nftRouter.get('/:id', validateGetNFT(), validate, nftController.getNFTById)
nftRouter.post('/mint/:id', validateMintNFT(), validate, transactionLimiter, nftController.mintNFT)
nftRouter.put('/assets-uri/:requestId', validateAssetsURI(), validate, nftController.updateAssetsURI)

module.exports = nftRouter