const unauthorized = require('express').Router()

const adminRouter = require('./admin.router')
const userRouter = require('./user.router')
const nftRouter = require('./nft.router')
const uploadRouter = require('./upload.router')
const auctionRouter = require('./auction.router')
const gameliftRouter = require('./gamelift.router')
const twitchRouter = require('./twitch.router')

unauthorized.use('/admin', adminRouter)
unauthorized.use('/user', userRouter)
unauthorized.use('/nft', nftRouter)
unauthorized.use('/auction', auctionRouter)
unauthorized.use('/upload', uploadRouter)
unauthorized.use('/gamelift', gameliftRouter)
unauthorized.use('/twitch', twitchRouter)

module.exports = unauthorized
