const nftRouter = require('express').Router()
const nftController = require('../../controllers/nftController')

const { validateUpdateNFT, validateGetAllNFTsAdmin, validateGetNFTAdmin, validateAdmin, validate } = require('../../middlewares/request.validators')

nftRouter.put('/:nftId', validateUpdateNFT(), validateAdmin(), validate, nftController.updateNFT)

nftRouter.get('/all', validateGetAllNFTsAdmin(), validateAdmin(), validate, nftController.getNFTsForAdmin)

nftRouter.get('/:id', validateGetNFTAdmin(), validateAdmin(), validate, nftController.getNFTById)

module.exports = nftRouter
