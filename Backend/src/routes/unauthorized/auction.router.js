const auctionRouter = require('express').Router()
const auctionController = require('../../controllers/auctionController')

const { validatecreateAuction, validateUser, validateGetAuction, validatePlaceBid, validateEndAuction, validatePendingRefunds, validate } = require('../../middlewares/request.validators')

auctionRouter.get('/listed', auctionController.getAuctionedNFTs)
auctionRouter.get('/filter', validateUser(), validate, auctionController.getFilterAuctions)
auctionRouter.post('/create/:nftId', validatecreateAuction(), validate, auctionController.createAuction)
auctionRouter.get('/all', validateUser(), validate, auctionController.getAllAuctions)
auctionRouter.get('/bids', validateUser(), validate, auctionController.getAllBids)
auctionRouter.get('/pending-refunds', validatePendingRefunds(), validate, auctionController.getPendingRefunds)
auctionRouter.get('/:id', validateGetAuction(), validate, auctionController.getAuctionDetails)
auctionRouter.post('/bid/:auctionId', validatePlaceBid(), validate, auctionController.placeBid)
auctionRouter.post('/refund/:auctionId', validatePendingRefunds(), validate, auctionController.refund)
auctionRouter.post('/cancel/:auctionId', validatePendingRefunds(), validate, auctionController.cancelAuction)
auctionRouter.post('/end/:auctionId', validateEndAuction(), validate, auctionController.endAuction)

module.exports = auctionRouter