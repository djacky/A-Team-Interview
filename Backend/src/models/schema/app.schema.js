const mongoose = require('mongoose')
const { Schema } = mongoose


const appSchema = new Schema({
    pause: {
        type: Boolean,
        default: false,
    },
    pauseReason: {
        type: String,
        default: ''
    }
},
{ timestamps: true }
)

module.exports = appSchema