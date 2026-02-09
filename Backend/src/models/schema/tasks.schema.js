const mongoose = require('mongoose')
const { Schema } = mongoose

const CONSTANTS = require('../../constants')

const taskSchema = new Schema({
    taskId: {
        type: String,
        default: "",
        required: true
    },
    runAt: {
        type: Number,
        default: 0,
        required: true
    },
    createdBy: {
        type: String,
        default: "",
        required: false
    }
},
{ timestamps: true }
)

module.exports = taskSchema