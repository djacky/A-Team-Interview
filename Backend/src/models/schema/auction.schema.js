const mongoose = require('mongoose')
const { Schema } = mongoose
const CONSTANTS = require('../../constants')

const auctionSchema = new Schema({
    nftData: {
        type: Object,
        ref: 'data',
        required: true
    },
    nft: {
        type: Schema.Types.ObjectId,
        ref: 'NFT',
        required: true
    },
    auctionNumber: {
      type: Number,
      required: true
    },
    seller: {
        type: Schema.Types.ObjectId,
        ref: 'user',
        required: true
    },
    quantity: {
      type: Number,
      required: true
    },
    startingPrice: {
      type: Number,
      required: true
    },
    duration: {
      type: Number,
      required: true
    },
    endTime: {
      type: Number
    },
    ended: {
      type: Boolean,
      default: false
    },
    highestBidder: {
      type: Schema.Types.ObjectId,
      ref: 'user'
    },
    highestBid: {
      type: Number,
      default: 0
    },
    txHash: {
      type: String
    },
    network: {
      type: String,
      enum: CONSTANTS.NETWORKS,
    },
    bids: [{
      user: {
          type: Schema.Types.ObjectId,
          ref: 'User',
          required: true
      },
      bidAmount: {
          type: Number,
          required: true,
      },
      _id: false
  }]
  }, 
  {
    timestamps: true 
  });

module.exports = auctionSchema
