const devRouter = require('express').Router()
const devController = require('../../controllers/devController')
//const { validateGetAllStats, validate, validateStatAuth, validateWebAuth, validateZealyKey, validateGetStats } = require('../../middlewares/request.validators')

const rateLimit = require('express-rate-limit');
const devLimiter = rateLimit({
    windowMs: 15 * 60 * 1000, // 15 minutes
    max: 8, // limit each IP to 8 requests per 15 minutes
  });


devRouter.get('/getDevRefresh', devLimiter, devController.getIsDelete)
devRouter.post('/confirmRefresh', devLimiter, devController.confirmDelete)

module.exports = devRouter