const mongoose = require('mongoose')
const { Schema } = mongoose

const CONSTANTS = require('../../constants')

const adminSchema = new Schema(
    {
        accountId: {
            type: String,
            default: null,
            required: true
        },
        role: {
            type: String,
            default: CONSTANTS.USER_ROLE_ADMIN,
            required: true
        },
        walletAddress: {
            type: String,
            default: null,
            required: true
        },
        jwt: {
            type: String,
            default: null,
            required: true
        }
    },
    { timestamps: true },
)

module.exports = adminSchema
