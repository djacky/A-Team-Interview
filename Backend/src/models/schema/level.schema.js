const mongoose = require('mongoose')
const { Schema } = mongoose

const levelSchema = new Schema({
    level: {
        type: Number,
        required: true,
        unique: true
    },
    requiredMetrics: [{
        type: {
            type: String,
            required: true
        },
        value: {
            type: Number,
            required: true
        },
        xpPerUnit: {
            type: Number,
            required: true
        }
    }],
    badgeID: {
        type: String,
        default: ""
    },
    unlockID: {
        type: String,
        default: ""
    },
    quantity: {
        type: Number,
        default: 1
    },
    isInventory: {
        type: Boolean,
        default: false
    }
});

module.exports = levelSchema;