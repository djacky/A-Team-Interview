const mongoose = require('mongoose')
const { Schema } = mongoose

const tournamentSchema = new Schema({
    name: {
        type: String,
        default: "",
        required: true
    },
    mode: {
        type: String,
        default: "solo",
        required: true
    },
    type: {
        type: String,
        default: "free",
        required: true
    },
    time: {
        type: String,
        default: "",
        required: true
    },
    timeUnixSeconds: {
        type: Number,
        default: 0,
        required: true
    },
    date: {
        type: String,
        default: "",
        required: true
    },
    active: {
        type: Boolean,
        default: false,
        required: true
    },
    matchStarted: {
        type: Boolean,
        default: false,
        required: true
    },
    description: {
        type: String,
        default: "",
        required: true
    },
    reward: {
        type: String,
        default: "",
        required: true
    },
    players: {
        type: Object,
        default: {},
        required: true
    },
    limit: {
        type: Number,
        default: 10,
        required: true
    },
    minPlayers: {
        type: Number,
        default: 0,
        required: true
    },
    region: {
        type: String,
        default: "us-east-1",
        required: true
    },
    userPassword: {
        type: String,
        default: "",
        required: false
    },
    needsStakeTicket: {
        type: Boolean,
        default: false,
        required: true
    },
    createdBy: {
        type: String,
        default: "",
        required: false
    },
    stakeInfo: {
        type: Object,
        default: {
            network: "",
            limits: [],
            winningsTxHash: "",
            flaggedPlayers: {},
            rake: 0.05
        },
        required: true
    }
},
{ 
    timestamps: true,
    minimize: false
 }
)

module.exports = tournamentSchema