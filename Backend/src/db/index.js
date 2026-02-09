const mongoose = require('mongoose')
const { startApp } = require('../helpers/startApp.helper')
const { startTournamentTasks } = require('../helpers/tournament.helper')
const { getGameLiftFleetId } = require('../controllers/gameLiftController')

const { DB_HOST, DB_PORT, DB_NAME, DB_USER, DB_PASSWORD, DB_AUTH_SOURCE, TEST_MODE } = require('../config')

let uri = TEST_MODE === 'true' ?
            `mongodb://${DB_HOST}:${DB_PORT}/${DB_NAME}` : 
            `mongodb://${DB_USER}:${DB_PASSWORD}@${DB_HOST}:${DB_PORT}/${DB_NAME}?authSource=${DB_AUTH_SOURCE}`;

mongoose.connect(uri)

mongoose.connection.on('connected', async function () {
    console.log('Mongoose connected')
    startApp();
    getGameLiftFleetId();
    startTournamentTasks();
})
mongoose.connection.on('error', function (error) {
    console.log('Mongoose connection error', error)
})
mongoose.connection.on('disconnected', function () {
    console.log('Mongoose disconnected')
})
