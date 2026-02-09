const mongoose = require('mongoose')
const { Schema } = mongoose

const uploadSchema = new Schema({
    ip: {
        type: String,
        default: null,
        required: true
    },
    accountId: {
        type: String,
        default: null,
        required: true
    },
    fileName: {
        type: String,
        default: null,
        required: true
    },
    info: {
        type: Object,
        default: null,
        required: true
    }
},
{ timestamps: true }
)

module.exports = uploadSchema