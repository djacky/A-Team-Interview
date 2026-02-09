const mongoose = require('mongoose')
const { Schema } = mongoose

const devSchema = new Schema({
    deleteGame: {
        type: Boolean,
        required: true,
        default: false
    }
});

module.exports = devSchema;