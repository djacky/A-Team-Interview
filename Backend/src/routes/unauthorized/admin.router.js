const adminRouter = require('express').Router()
const adminController = require('../../controllers/adminController')
const rateLimit = require('express-rate-limit');
const { adminSignUpRules, adminSignInRules, validate, validateAdmin } = require('../../middlewares/request.validators')

const loginLimiter = rateLimit({
    windowMs: 15 * 60 * 1000, // 15 minutes
    max: 15, // limit each IP to 15 requests per 15 minutes
  });

//Dummy API - Not to be used for production
//adminRouter.post('/signup', adminSignUpRules(), validate, adminController.signUp)
adminRouter.post('/signin', adminSignInRules(), validate, loginLimiter, adminController.signIn)

module.exports = adminRouter
