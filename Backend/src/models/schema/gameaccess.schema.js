const mongoose = require('mongoose')
const { Schema } = mongoose

const gameAccessSchema = new Schema({
    userId: {
        type: String,
        required: true,
        default: null
    },
    postedVideo: {
        type: Boolean,
        required: true,
        default: false
    },

    reviewCompleted: {
        type: Boolean,
        required: true,
        default: false
    },

    boughtGame: {
        type: Boolean,
        required: true,
        default: false
    },

    passedKyc: {
        type: Boolean,
        required: true,
        default: false
    },

    publicWallet: {
        type: String,
        required: true,
        default: ""
    }

},
{ timestamps: true }
)

module.exports = gameAccessSchema