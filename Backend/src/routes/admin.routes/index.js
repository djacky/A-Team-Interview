const adminRouter = require('express').Router()

const nftRouter = require('./nft.router')
const userRouter = require('./user.router')
const appRouter = require('./app.router')

adminRouter.use('/nft', nftRouter)
adminRouter.use('/user', userRouter)
adminRouter.use('/app', appRouter)

module.exports = adminRouter
