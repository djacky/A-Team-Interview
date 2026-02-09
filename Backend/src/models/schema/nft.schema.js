const mongoose = require('mongoose')
const { Schema } = mongoose

const CONSTANTS = require('../../constants')

const nftSchema = new Schema({
    pinataUri: {
      type: String
    },
    txHash: {
      type: String
    },
    tokenId: {
      type: Number
    },
    s3ImageUri: {
      type: String
    },
    s3ModelUri: {
      type: String
    },
    type: {
      type: String,
      enum: CONSTANTS.TOOL_TYPES,
    },
    status: {
      type: String,
      enum: CONSTANTS.NFT_REQUEST_STATUSES,
      default: CONSTANTS.NFT_REQUEST_STATUS_PENDING
    },
    network: {
      type: String,
      enum: CONSTANTS.NETWORKS,
    },
    feedback: {
      type: String
    },
    name: {
      type: String,
      required: true,
      maxlength: CONSTANTS.NFT_NAME_LENGTH
    },
    description: {
      type: String,
    },
    price: {
      type: Number
    },
    amount: {
        type: Number,
        required: true,
        min: CONSTANTS.NFT_MIN_AMOUNT,
        max: CONSTANTS.NFT_MAX_AMOUNT
    },
    creator: {
      type: Schema.Types.ObjectId,
      ref: 'User',
      required: true
    },
    onChainMetadata: {
      type: Map,
      of: Schema.Types.Mixed
    },
    offChainMetadata: {
      type: Map,
      of: Schema.Types.Mixed
    },
    owners: [{
      user: {
          type: Schema.Types.ObjectId,
          ref: 'User',
          required: true
      },
      balance: {
          type: Number,
          required: true,
          default: 0
      },
      amountAuctioned: {
        type: Number,
        default: 0
      },
      _id: false
  }]
  }, 
  {
    timestamps: true 
  });

module.exports = nftSchema
