const game = require('express').Router()

const statsRouter = require('./stats.router')
const gameAccessRouter = require('./gameAccess.router')
const blockchainActionRouter = require('./blockchainAction.router')
const tournamentRouter = require('./tournament.router')
const devRouter = require('./dev.router')

game.use('/stats', statsRouter)
game.use('/gameAccess', gameAccessRouter)
game.use('/blockchainAction', blockchainActionRouter)
game.use('/tournament', tournamentRouter)
game.use('/dev', devRouter)

module.exports = game
