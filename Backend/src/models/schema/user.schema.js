const mongoose = require('mongoose')
const { Schema } = mongoose

const CONSTANTS = require('../../constants')

const userSchema = new Schema({
    role: {
        type: String,
        default: CONSTANTS.USER_ROLE_USER,
    },
    accountId: {
        type: String,
        default: null,
        required: true
    },
    accountName: {
        type: String,
        default: null,
        required: true
    },
    discordId: {
        type: String,
        default: "",
        required: true
    },
    discordName: {
        type: String,
        default: "",
        required: true
    },
    walletAddress: {
        type: String,
        default: "",
        required: true
    },
    accessInfo: {
        type: Object,
        default: {
            postedVideo: false,
            reviewCompleted: false,
            boughtGame: false,
            passedKyc: false,
            privilege: 0,
            flaggedInfo: {},
            email: ''
        },
        required: true
    },
    isBanned: {
        type: Boolean,
        default: false,
    },
    banReason: {
        type: String,
        default: ''
    },
    banStrikes: {
        type: Number,
        default: 0
    },
    nftFeeId: {
        type: String,
        default: ''
    },
    nftUploadStatus: {
        type: String,
        default: ''
    },
    tournamentsCreated: {
        type: Array,
        default: []
    },
    stakeInfo: {
        type: Object,
        default: {
            ethereum: {total: 0, earned: 0, numTimesStaked: 0},
            polygon: {total: 0, earned: 0, numTimesStaked: 0},
            polygonTest: {total: 0, earned: 0, numTimesStaked: 0}
        }
    }
},
{ timestamps: true }
)

module.exports = userSchema