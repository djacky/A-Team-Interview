const mongoose = require('mongoose')
const { Schema } = mongoose

const dailyAchievementSchema = new Schema({
  date: {
    type: String, // YYYY-MM-DD format
    required: true
  },
  dtRatio: {
    type: Number,
    default: 0
  },
  kdRatio: {
    type: Number,
    default: 0
  },
  accPercent: {
    type: Number,
    default: 0
  },
  matchType: {
    type: String,
    default: ""
  }
}, { _id: false });

const statsSchema = new Schema({
    userId: {
        type: String,
        required: true,
        default: null
    },
    dailyAchievements: {
      type: [dailyAchievementSchema],
      default: []
    },

    stats: {

        userName: {
            type: String,
            required: false,
            default: ""
        },

        numKills: {
            type: Number,
            required: true,
            default: 0
        },

        numDeaths: {
            type: Number,
            required: true,
            default: 0
        },

        numHit: {
            type: Number,
            required: true,
            default: 0
        },

        numMiss: {
            type: Number,
            required: true,
            default: 0
        },

        numGamesPlayed: {
            type: Number,
            required: true,
            default: 0
        },

        totalMinutesPlayed: {
            type: Number,
            required: true,
            default: 0
        },

        totalDamageDealt: {
            type: Number,
            required: true,
            default: 0
        },

        totalDamageTaken: {
            type: Number,
            required: true,
            default: 0
        },

        rating: {
            type: Number,
            required: true,
            default: 0
        }
    },
    stakeStats: {

        userName: {
            type: String,
            required: false,
            default: ""
        },

        numKills: {
            type: Number,
            required: false,
            default: 0
        },

        numDeaths: {
            type: Number,
            required: false,
            default: 0
        },

        numHit: {
            type: Number,
            required: false,
            default: 0
        },

        numMiss: {
            type: Number,
            required: false,
            default: 0
        },

        numGamesPlayed: {
            type: Number,
            required: false,
            default: 0
        },

        totalMinutesPlayed: {
            type: Number,
            required: false,
            default: 0
        },

        totalDamageDealt: {
            type: Number,
            required: false,
            default: 0
        },

        totalDamageTaken: {
            type: Number,
            required: false,
            default: 0
        },

        rating: {
            type: Number,
            required: false,
            default: 0
        }
    },
    currentLevel: {
        type: Number,
        default: 0
    },
    totalXP: {
        type: Number,
        default: 0
    },
    metrics: {
        type: Object,
        default: {}
    },
    badges: {
        type: [String],
        default: []
    },
    unlocks: {
        type: [String],
        default: []
    },
    inventory: {
        type: Object,
        default: {}
    },
    latestMatchData: {
        type: Object,
        default: {
            stats: {
                kd: [],
                dt: []
            },
            stakeStats: {
                kd: [],
                dt: [],
                stakePL: {
                    polygon: [],
                    ethereum: []
                }
            }
        }
    }
},
{ timestamps: true }
)

module.exports = statsSchema