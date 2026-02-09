const uploadRouter = require('express').Router()
const uploadController = require('../../controllers/uploadController.js')
const rateLimit = require('express-rate-limit');
const { validate, uploadNFTMetadataRules, validateUpload, validateS3sign, validateScan } = require('../../middlewares/request.validators')

const uploadLimiter = rateLimit({
    windowMs: 15 * 60 * 1000, // 15 minutes
    max: 10, // limit each IP to 10 requests per 15 minutes
  });

uploadRouter.post('/nft-metadata', uploadNFTMetadataRules(), validate, uploadController.uploadJSONToIPFS)
uploadRouter.post('/nft-image', uploadController.uploadFileToIPFS)
uploadRouter.post('/s3', validateUpload(), validate, uploadLimiter, uploadController.uploadFileToS3)
uploadRouter.post('/s3-delete', uploadController.deleteFileFromS3)
uploadRouter.post('/scan', validateScan(), validate, uploadLimiter, uploadController.scanFile)
uploadRouter.post('/getSignedUrl', validateS3sign(), validate, uploadLimiter, uploadController.getSignedURL)

module.exports = uploadRouter
