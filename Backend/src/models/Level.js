const mongoose = require('mongoose')
const { levelSchema } = require('./schema')


const Level = mongoose.model('Level', levelSchema)
module.exports = Level
