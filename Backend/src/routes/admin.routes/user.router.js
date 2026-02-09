const userRouter = require('express').Router()
const userController = require('../../controllers/userController')

const { banRules, validateGetAllUsersAdmin, validate, validateAdmin } = require('../../middlewares/request.validators')

userRouter.post('/ban/:userId', banRules(), validateAdmin(), validate, userController.banUserByAdmin)

userRouter.get('/all', validateGetAllUsersAdmin(), validateAdmin(), validate, userController.getUsersForAdmin)

module.exports = userRouter
