const appRouter = require('express').Router()
const appController = require('../../controllers/appController')

const { updateStatusRules, validate, validateAdmin } = require('../../middlewares/request.validators')

appRouter.post('/update-status', updateStatusRules(), validateAdmin(), validate, appController.updateAppStatusByAdmin)
appRouter.get('/get-status', appController.getAppStatus)

module.exports = appRouter
