const mongoose = require('mongoose')
const { statsSchema } = require('./schema')
const { sendSimpleTransaction } = require('../services/blockchain.service');   
const { AWS_SECRET  } = require('../config');
const { decrypt, getSecret } = require('../utils/helperFunctions');


var bCanUpdate = true
function calcRating(jsonBody,
    meanVolume, stdVolume, 
    meanEfficiency, stdEfficiency,
    meanKDR, stdKDR,
    meanAccuracy, stdAccuracy,
    meanRisk, stdRisk,
    maxGamesPlayed) {

    // 1. Volume Score (z-normalized total damage dealt)
    const totalDamageDealt = jsonBody["totalDamageDealt"];
    const zVolume = stdVolume > 0 ? (totalDamageDealt - meanVolume) / stdVolume : 0;
    const normVolume = 0.5 * Math.tanh(zVolume) + 0.5;

    // 2. Efficiency Score (z-normalized damage efficiency) (modified DTR)
    const totalDamage = jsonBody["totalDamageDealt"] + jsonBody["totalDamageTaken"];
    const efficiency = totalDamage > 0 ? jsonBody["totalDamageDealt"] / totalDamage : 0;
    const zEfficiency = stdEfficiency > 0 ? (efficiency - meanEfficiency) / stdEfficiency : 0;
    const normEfficiency = 0.5 * Math.tanh(zEfficiency) + 0.5;

    // 3. Risk Score (NEW)
    const risk = jsonBody["numGamesPlayed"] > 0 ? jsonBody["totalDamageTaken"] / jsonBody["numGamesPlayed"] : 0;
    const zRisk = stdRisk > 0 ? (risk - meanRisk) / stdRisk : 0;
    const normRisk = 0.5 * Math.tanh(zRisk) + 0.5;

    // 4. Risk-Adjusted Efficiency (COMBINED)
    // High risk + good efficiency = brave and skilled
    // Low risk + good efficiency = safe but less impactful
    const riskAdjustedEfficiency = normEfficiency * (0.7 + 0.6 * normRisk);
    // This creates a multiplier range of 0.7x to 1.3x on efficiency
    // Low risk (normRisk=0): 0.7x multiplier
    // Average risk (normRisk=0.5): 1.0x multiplier  
    // High risk (normRisk=1): 1.3x multiplier

    // 5. K/D Score
    const adjustedKDR = jsonBody["numKills"] / Math.max(1, jsonBody["numDeaths"]);
    const zKDR = stdKDR > 0 ? (adjustedKDR - meanKDR) / stdKDR : 0;
    const normKDR = 0.5 * Math.tanh(zKDR) + 0.5;

    // 6. Accuracy Score
    const totalShots = jsonBody["numHit"] + jsonBody["numMiss"];
    const accuracy = totalShots > 0 ? jsonBody["numHit"] / totalShots : 0;
    const zAccuracy = stdAccuracy > 0 ? (accuracy - meanAccuracy) / stdAccuracy : 0;
    const normAccuracy = 0.5 * Math.tanh(zAccuracy) + 0.5;

    let baseScore = 
        0.35 * normVolume +
        0.30 * normKDR +
        0.25 * riskAdjustedEfficiency +
        0.10 * normAccuracy;
    
    /*
    let cumulativeShotAccuracy = 0;
    if (jsonBody["numHit"] + jsonBody["numMiss"] > 0) {
        cumulativeShotAccuracy = jsonBody["numHit"] / (jsonBody["numHit"] + jsonBody["numMiss"]);
    } // Else 0: Fair for inactive shooters
    */

    /*
    const risk = jsonBody["numGamesPlayed"] > 0 ? jsonBody["totalDamageTaken"] / jsonBody["numGamesPlayed"] : 0;

    let adjustedDTR = jsonBody["totalDamageDealt"] / Math.max(1, jsonBody["totalDamageTaken"]);
    let zDTR = stdDTR > 0 ? (adjustedDTR - meanDTR) / stdDTR : 0;
    let normalizedDTR = 0.5 * Math.tanh(zDTR) + 0.5;  // Shifted to [0,1]

    let adjustedKDR = jsonBody["numKills"] / Math.max(1, jsonBody["numDeaths"]);
    let zKDR = stdKDR > 0 ? (adjustedKDR - meanKDR) / stdKDR : 0;
    let normalizedKDR = 0.5 * Math.tanh(zKDR) + 0.5;  // Shifted to [0,1]

    const normRisk = stdRisk > 0 ? 0.5 * Math.tanh((risk - meanRisk) / stdRisk) + 0.5 : 0.5 * Math.tanh(risk) + 0.5;
    // Risk-Adjusted DTR: Multiply DTR by normalized risk for "brave efficiency"
    const riskAdjustedDTR = normalizedDTR * Math.max(0.2, normRisk); // Penalize low-risk slightly

    // Balanced weights: Slightly boost accuracy for precision players
    let baseScore = 0.15 * normAccuracy + 0.35 * riskAdjustedDTR + 0.50 * normalizedKDR;
    */

    // Keep games factor, but cap at 1 for veterans
    //let longevityFactor = Math.min(1, Math.sqrt(jsonBody["numGamesPlayed"] / maxGamesPlayed));
    const targetGames = 20; // Reach 95% by 50 games (tune this)
    const longevityFactor = 1 - Math.exp(-jsonBody["numGamesPlayed"] / targetGames);

    return baseScore * longevityFactor * 100; // Scale to 100 for readability
}

async function refreshStats(type) {
    // Get all user IDs and recalculate the stats
    if (!bCanUpdate) return;

    const statType = type.toLowerCase().includes("stake") ? 'stakeStats' : 'stats';
    const prefix = `$${statType}.`; // For dynamic field access in aggregation

    const volumeAgg = await Stats.aggregate([
        {
            $project: {
                totalDamage: `${prefix}totalDamageDealt`
            }
        },
        {
            $group: {
                _id: null,
                meanVolume: { $avg: "$totalDamage" },
                stdVolume: { $stdDevPop: "$totalDamage" }
            }
        }
    ]);
    const meanVolume = volumeAgg[0]?.meanVolume || 0;
    const stdVolume = volumeAgg[0]?.stdVolume || 0;

    // Compute mean and std for adjusted KDR
    const kdrAgg = await Stats.aggregate([
        {
            $project: {
                adjustedKDR: {
                    $divide: [
                        `${prefix}numKills`,
                        { $max: [1, `${prefix}numDeaths`] }
                    ]
                }
            }
        },
        {
            $group: {
                _id: null,
                meanKDR: { $avg: "$adjustedKDR" },
                stdKDR: { $stdDevPop: "$adjustedKDR" } // Population std dev for full dataset
            }
        }
    ]);
    const meanKDR = kdrAgg[0]?.meanKDR || 0;
    const stdKDR = kdrAgg[0]?.stdKDR || 0;

    // Compute mean and std for adjusted DTR
    const efficiencyAgg = await Stats.aggregate([
        {
            $project: {
                efficiency: {
                    $cond: {
                        if: { $gt: [{ $add: [`${prefix}totalDamageDealt`, `${prefix}totalDamageTaken`] }, 0] },
                        then: {
                            $divide: [
                                `${prefix}totalDamageDealt`,
                                { $add: [`${prefix}totalDamageDealt`, `${prefix}totalDamageTaken`] }
                            ]
                        },
                        else: 0
                    }
                }
            }
        },
        {
            $group: {
                _id: null,
                meanEfficiency: { $avg: "$efficiency" },
                stdEfficiency: { $stdDevPop: "$efficiency" }
            }
        }
    ]);
    const meanEfficiency = efficiencyAgg[0]?.meanEfficiency || 0;
    const stdEfficiency = efficiencyAgg[0]?.stdEfficiency || 0;

    const riskAgg = await Stats.aggregate([
        {
            $project: {
                risk: {
                    $cond: {
                        if: { $gt: [`${prefix}numGamesPlayed`, 0] },
                        then: { $divide: [`${prefix}totalDamageTaken`, `${prefix}numGamesPlayed`] },
                        else: 0
                    }
                }
            }
        },
        {
            $group: {
                _id: null,
                meanRisk: { $avg: "$risk" },
                stdRisk: { $stdDevPop: "$risk" }
            }
        }
    ]);
    const meanRisk = riskAgg[0]?.meanRisk || 0;
    const stdRisk = riskAgg[0]?.stdRisk || 0;

    const accuracyAgg = await Stats.aggregate([
        {
            $project: {
                adjustedAccuracy: {
                    $cond: {
                        if: { $gt: [{ $add: [`${prefix}numHit`, `${prefix}numMiss`] }, 0] },
                        then: { $divide: [`${prefix}numHit`, { $add: [`${prefix}numHit`, `${prefix}numMiss`] }] },
                        else: 0
                    }
                }
            }
        },
        {
            $group: {
                _id: null,
                meanAccuracy: { $avg: "$adjustedAccuracy" },
                stdAccuracy: { $stdDevPop: "$adjustedAccuracy" } // Population std dev for full dataset
            }
        }
    ]);
    const meanAccuracy = accuracyAgg[0]?.meanAccuracy || 0;
    const stdAccuracy = accuracyAgg[0]?.stdAccuracy || 0;

    const maxGamesPlayed = await getMaxValue(`${prefix}numGamesPlayed`);

    // Fetch all users (lean for efficiency)
    const allUsers = await Stats.find({}).lean();

    const bulkOps = allUsers.map(userObj => {
        const playerRating = calcRating(
            userObj[statType], 
            meanVolume, stdVolume,
            meanEfficiency, stdEfficiency,
            meanKDR, stdKDR,
            meanAccuracy, stdAccuracy,
            meanRisk, stdRisk,
            maxGamesPlayed
        );
        // Skip invalid ratings
        if (playerRating === undefined || Number.isNaN(playerRating)) return null;
        return {
            updateOne: {
                filter: { userId: userObj.userId },
                update: { $set: { [`${statType}.rating`]: playerRating } }
            }
        };
    }).filter(op => op !== null); // Remove null entries

    if (bulkOps.length > 0) {
        await Stats.bulkWrite(bulkOps);
    }
}

async function getMaxRatioValues(numName = '$stats.numKills', denName = '$stats.numDeaths', statType)
{
    const result = await Stats.aggregate([
        {
            $addFields: {
                [`${statType}.maxField`]: {
                    $cond: {
                        if: { $eq: [denName, 0] },
                        then: 1, // or any other value you prefer for zero deaths
                        else: { $divide: [numName, denName] }
                    }
                }
            }
        },
        {
            $group: {
              _id: null,
              maxValue: { $max: `$${statType}.maxField` }
            }
        }
    ]);

    if (result.length > 0) {
        return result[0].maxValue;
    } else {
        return 10;
    }
}

async function getMaxValue(statName = '$stats.numGamesPlayed')
{
    const result = await Stats.aggregate([
        { $group: { _id: null, maxValue: { $max: statName } } }
      ]);
    
    if (result.length > 0) {
        return result[0].maxValue;
    } else {
        return 1;
    }
}

function convertToCumulativeRequirements(levelRequirements) {
    const sorted = [...levelRequirements].sort((a, b) => a.level - b.level);
    const cumulativeMetrics = {};
    
    return sorted.map(req => {
        const cumulativeReq = { ...req.toObject() }; // Convert Mongoose doc to plain object
        
        cumulativeReq.requiredMetrics = req.requiredMetrics.map(metric => {
            if (!cumulativeMetrics[metric.type]) {
                cumulativeMetrics[metric.type] = 0;
            }
            
            cumulativeMetrics[metric.type] += metric.value;
            
            return {
                type: metric.type,
                value: cumulativeMetrics[metric.type], // Cumulative value
                xpPerUnit: metric.xpPerUnit
            };
        });
        
        return cumulativeReq;
    });
}

async function computeMatchXP(player, deltas, startLevel, levelRequirements) {
  let computedXP = 0;
  let simulatedLevel = startLevel;
  const deltaMap = { ...deltas };
  
  // Track simulated cumulative values as if we're adding deltas progressively
  const simulatedMetrics = { ...player.metrics };

  while (true) {
    const nextReq = levelRequirements.find(req => req.level === simulatedLevel + 1);
    if (!nextReq) break;

    let canLevelUp = true;
    
    // Change: iterate over array instead of object entries
    for (const metric of (nextReq.requiredMetrics || [])) {
      const key = metric.type; // Get the metric type (e.g., "EPM_AIHeadshots")
      const oldValue = simulatedMetrics[key] || 0;
      const deltaForThis = deltaMap[key] || 0;
      const simulatedNew = oldValue + deltaForThis;

      if (simulatedNew < metric.value) { 
        canLevelUp = false; 
      }

      const remainingToCap = metric.value - oldValue;
      if (remainingToCap > 0 && deltaForThis > 0) {
        const awardedDelta = Math.min(deltaForThis, remainingToCap);
        computedXP += awardedDelta * (metric.xpPerUnit || 0);
        deltaMap[key] = deltaForThis - awardedDelta;
        simulatedMetrics[key] = oldValue + awardedDelta;
      }
    }

    if (!canLevelUp) break;
    
    simulatedLevel++;
  }

  if (simulatedLevel > startLevel)
  {
    const finalNextReq = levelRequirements.find(req => req.level === simulatedLevel + 1);
    if (finalNextReq) {
        for (const metric of (finalNextReq.requiredMetrics || [])) {
        const key = metric.type;
        const residual = deltaMap[key] || 0;
        if (residual > 0) {
            computedXP += residual * (metric.xpPerUnit || 0);
        }
        }
    }
  }

  return computedXP;
}

statsSchema.static('processLevelUps', async function(userId, levelRequirements) {
    try {
        const player = await this.findOne({ userId });
        if (!player) return null;

        let newLevel = player.currentLevel;
        const newBadges = [];
        const newUnlocks = [];
        const inventoryUpdates = {};

        while (true) {
            const nextReq = levelRequirements.find(req => req.level === newLevel + 1);
            if (!nextReq) break;

            // Change: iterate over array and check every metric
            const canLevelUp = (nextReq.requiredMetrics || []).every(metric => {
                const key = metric.type;
                return (player.metrics[key] || 0) >= metric.value;
            });

            if (!canLevelUp) break;
            
            newLevel++;
            if (nextReq.badgeID) newBadges.push(nextReq.badgeID);
            if (nextReq.unlockID) {
                if (nextReq.isInventory) {
                    const quantity = nextReq.quantity || 1;
                    inventoryUpdates[nextReq.unlockID] = (inventoryUpdates[nextReq.unlockID] || 0) + quantity;
                } else {
                    newUnlocks.push(nextReq.unlockID);
                }
            }
        }

        if (newLevel > player.currentLevel) {
            const updateObject = {
                $set: { currentLevel: newLevel }
            };

            // Add $addToSet only if there are items to add (ensures uniqueness)
            if (newBadges.length > 0 || newUnlocks.length > 0) {
                updateObject.$addToSet = {};
                if (newBadges.length > 0) {
                    updateObject.$addToSet.badges = { $each: newBadges };
                }
                if (newUnlocks.length > 0) {
                    updateObject.$addToSet.unlocks = { $each: newUnlocks };
                }
            }

            // Add inventory $inc
            if (Object.keys(inventoryUpdates).length > 0) {
                updateObject.$inc = {};
                for (const [key, value] of Object.entries(inventoryUpdates)) {
                    updateObject.$inc[`inventory.${key}`] = value;
                }
            }

            await Stats.updateOne({ userId }, updateObject);
        }
        
        return { newLevel, newUnlocks, newBadges, inventoryUpdates };
    } catch (error) {
        console.error(`Level up error for user ${userId}:`, error);
        return null;
    }
});

async function updateStat(statsBody, bRefresh = false, type, LevelModel){
    const propertiesToUpdate = [
        'numKills',
        'numDeaths',
        'numHit',
        'numMiss',
        'numGamesPlayed',
        'totalMinutesPlayed',
        'totalDamageDealt',
        'totalDamageTaken'
    ];

    const statType = type.toLowerCase().includes("stake") ? 'stakeStats': 'stats';
    let levelXP = 0;
    let player = await Stats.findOne({ userId: statsBody.userId });
    if (!player) {
        player = new Stats({ userId: statsBody.userId });  // New player fallback
    }

    // Compute matchXP using old + deltas (statsBody.metrics = deltas object)
    if (statsBody.metrics && typeof statsBody.metrics === 'object') {
        levelXP = await computeMatchXP(player, statsBody.metrics, player.currentLevel, LevelModel);
    }
    
    let tempStats;
    const updateFields = {};
    propertiesToUpdate.forEach(prop => {
        if (statsBody[prop] !== undefined) {
            updateFields[`${statType}.${prop}`] = statsBody[prop];
        }
    });

    

    if (statsBody.metrics && typeof statsBody.metrics === 'object') {
        console.log(statsBody.metrics);
        for (const [key, value] of Object.entries(statsBody.metrics)) {
            if (typeof value === 'number') {
                updateFields[`metrics.${key}`] = value;
            }
        }
    }
    updateFields['totalXP'] = levelXP;

    // Construct the MongoDB $inc operator dynamically
    const updateObject = { $inc: updateFields, $set: { [`${statType}.userName`]: statsBody['userName'] } };
    
    // Find the user and update their stats in one atomic operation
    tempStats = await Stats.findOneAndUpdate(
        { userId: statsBody.userId },
        updateObject,
        { returnDocument: 'after', upsert: true }  // Upsert will create a new document if no matching document is found
    );
    await updateAchievements(statsBody, tempStats, statType);
    const nextLevelInfo = await Stats.processLevelUps(statsBody.userId, LevelModel);

    const xpInfo = {'newLevel': player.currentLevel, 'newXp': player.totalXP, 'newUnlocks': []};
    xpInfo.newXp = tempStats.totalXP;
    if (nextLevelInfo)
    {
        xpInfo.newLevel = nextLevelInfo.newLevel;
        xpInfo.newUnlocks = nextLevelInfo.newUnlocks;
    }

    return xpInfo;
}


statsSchema.static('updateUserStat', async function (statsBody, type, tourId, TourModel, LevelModel) {

    try
    {
        if (type.toLowerCase().includes("stake"))
        {
            const tour = await TourModel.findOne({ _id: tourId });
            if (tour && tour.stakeInfo.network.toLowerCase().includes("test"))
            {
                return null;
            }
        }
        bCanUpdate = false

        const requirements = await LevelModel.find().sort({ level: 1 });
        const cumulativeRequirements = convertToCumulativeRequirements(requirements);
        let response = {};

        console.log(statsBody);
        console.log("---------------------------------------------------------");
        console.log(requirements);
        console.log("---------------------------------------------------------");
        console.log(cumulativeRequirements);
        console.log("---------------------------------------------------------");

        for (const userStat of statsBody)
        {
            response[userStat.userId] = await updateStat(userStat, false, type, cumulativeRequirements);
        }
        console.log(response);
        bCanUpdate = true;
        await refreshStats(type);
        return response;
    }
    catch (error)
    {
        console.log(error)
        return null;
    }
})

statsSchema.static('getStats', async function (userID, User) {

    try
    {
        const stats = await this.findOne({userId: `${userID}`});
        if (stats)
        {
            const userStats = stats.toObject();
            const user = await User.findOne(
                { accountId: `${userID}` }, 
                { 'stakeInfo.ethereum.earned': 1, 'stakeInfo.polygon.earned': 1, 'accessInfo.privilege': 1 } // Projection
              );
              
            if (user?.stakeInfo) {
                userStats.stakeStats.ethEarned = user.stakeInfo.ethereum?.earned || 0; // Default to 0 if undefined
                userStats.stakeStats.polEarned = user.stakeInfo.polygon?.earned || 0;
            }
            userStats.stakeStats.privilege = user.accessInfo.privilege;
            return userStats;
        }
        else
        {
            //let userStat = await this.create({userId: `${userID}`});
            return null;
        }
    }
    catch (error)
    {
        console.log(error);
        return null;
    }
})

statsSchema.static('getTopPlayers', async function (numPlayers = 50) {

    const topEntries = await this.find().sort({ 'stats.rating': -1 }).limit(numPlayers);
    const topStakingEntries = await this.find().sort({ 'stakeStats.rating': -1 }).limit(numPlayers);
    return {"genStats": topEntries, "stakeStats": topStakingEntries};
})

async function updateAchievements(statsBody, statsDoc, matchType) {
    // Update dailyAchievements based on this match
    const now = new Date();
    const utcDate = now.toISOString().split('T')[0]; // YYYY-MM-DD in UTC

    // Find the index of the existing entry for this date and matchType
    let dayEntryIndex = statsDoc.dailyAchievements.findIndex(
        entry => entry.date === utcDate && entry.matchType === matchType
    );

    if (dayEntryIndex === -1) {
        // Create and push a new entry if none exists, then set the index
        statsDoc.dailyAchievements.push({
            date: utcDate,
            kdRatio: 0,
            dtRatio: 0,
            accPercent: 0,
            matchType: matchType
        });
        dayEntryIndex = statsDoc.dailyAchievements.length - 1;
    }

    // Reference the subdocument at the index
    const dayEntry = statsDoc.dailyAchievements[dayEntryIndex];

    // Calculate ratios/accuracy for this match (with NaN safeguards)
    const kd = statsBody.numKills / (statsBody.numDeaths || 1) || 0;
    const totalDamage = (statsBody.totalDamageDealt + statsBody.totalDamageTaken) || 1;
    const dt = (statsBody.totalDamageDealt / totalDamage) * 100 || 0;
    const totalShots = (statsBody.numHit + statsBody.numMiss) || 1;
    const acc = (statsBody.numHit / totalShots) * 100 || 0;

    // Update to the max value seen today
    dayEntry.kdRatio = Math.max(dayEntry.kdRatio, kd);
    dayEntry.dtRatio = Math.max(dayEntry.dtRatio, dt);
    dayEntry.accPercent = Math.max(dayEntry.accPercent, acc);

    // Mark the specific array index as modified for nested changes
    statsDoc.markModified(`dailyAchievements.${dayEntryIndex}`);

    // Limit the array to the 5 most recent entries
    if (statsDoc.dailyAchievements.length > 5) {
        // Sort by date descending (most recent first)
        statsDoc.dailyAchievements.sort((a, b) => b.date.localeCompare(a.date));
        // Keep only the first 5
        statsDoc.dailyAchievements = statsDoc.dailyAchievements.slice(0, 5);
        // Mark the entire array as modified after reassignment
        statsDoc.markModified('dailyAchievements');
    }

    if (!statsDoc.latestMatchData) {
        statsDoc.latestMatchData = {
            stats: { kd: [], dt: [] },
            stakeStats: { kd: [], dt: [], stakePL: { polygon: [], ethereum: [] } }
        };
    }

    statsDoc.latestMatchData[matchType].kd.push(kd);
    statsDoc.latestMatchData[matchType].dt.push(dt);

    // Limit to last 5 entries (remove oldest if exceeded)
    if (statsDoc.latestMatchData[matchType].kd.length > 5) {
        statsDoc.latestMatchData[matchType].kd.shift();
        statsDoc.latestMatchData[matchType].dt.shift();
    }

    // Mark as modified
    statsDoc.markModified('latestMatchData');

    // Save the updated document
    await statsDoc.save();
}

statsSchema.static('checkAchievement', async function (body, params, User) {

    try {
        const user = await User.findOne({
            'accessInfo.email': body.accounts.email
        });
        if (user)
        {
            const result = await this.findOne({ userId: user.accountId });
            if (!result) {
                return null;
            }
            const now = new Date();
            const today = now.toISOString().split('T')[0];

            const { type, threshold } = params;
            const thresholdNum = parseFloat(threshold);

            const dayEntry = result.dailyAchievements.find(
                entry => entry.date === today
            );

            // type here can be dtRatio, kdRatio, or accPercent
            if (dayEntry && dayEntry[type] > thresholdNum) {
                return true;
            }
        }
        return null;
    } catch (error) {
        console.log(error);
        return null;
    }
})

statsSchema.static('getLatestMetrics', async function (tournamentID, TournamentModel) {
    try {
        const tournamentObj = await TournamentModel.findOne({ _id: tournamentID });
        if (!tournamentObj) {
            return { error: 'Tournament not found' };
        }

        const matchType = tournamentObj.type.toLowerCase().includes("stake") ? 'stakeStats' : 'stats';
        const network = tournamentObj.stakeInfo.network || null;
        const Ids = Object.keys(tournamentObj.players);

        const stats = await this.find({ userId: { $in: Ids } });
        const statsMap = stats.reduce((map, stat) => {
            map[stat.userId] = stat;
            return map;
        }, {});

        const playersData = Ids.map(Id => {
            const statsDoc = statsMap[Id] || null;
            let kd = [], dt = [], pl = [], stake = 0, multiplier = "None";

            if (statsDoc && statsDoc.latestMatchData) {
                const typeData = statsDoc.latestMatchData[matchType] || { kd: [], dt: [] };
                kd = typeData.kd || [];
                dt = typeData.dt || [];

                if (matchType === 'stakeStats' && network) {
                    const stakePL = statsDoc.latestMatchData.stakeStats.stakePL || {};
                    pl = stakePL[network] || [];
                    if (tournamentObj.players[Id].multiplierType)
                    {
                        multiplier = tournamentObj.players[Id].multiplierType;
                    }
                }
            }
            stake = tournamentObj.players[Id].stakeAmount || 0;
            return { userId: Id, kd, dt, pl, stake, multiplier };
        });
        return playersData;
    } catch (err) {
        console.error("Error fetching player metrics:", err);
        return { error: 'Internal server error' };
    }
})

statsSchema.static('buyAsset', async function (userId, body) {

    const user = await this.findOne({ userId: userId });
    if (user)
    {
        if (user.unlocks.includes(body.unlockId))
        {
            return {status: false, message: "You already own this item!", txHash: ""};
        }
        const userXP = user.totalXP;
        const paymentType = body.paymentType;
        if (paymentType == "kodes")
        {
            const XPInt = Math.round(body.price);
            if (userXP >= XPInt)
            {
                await this.updateOne({ userId: userId }, {
                    $set: { totalXP: userXP - XPInt}, 
                    $push: { 
                        unlocks: body.unlockId
                }
                });
                return {status: true, paymentType: paymentType, message: "Success!", txHash: ""};
            }
            else
            {
                return {status: false, message: "You don't have enough Kodes to get this item. Complete some XP tasks to earn Kodes!", txHash: ""};
            }
        }
        else if (paymentType == "crypto")
        {
            const { PRIVATE_PEM } = await getSecret(AWS_SECRET);
            const privateKey = await decrypt(body.key, body.iv, body.cipher, PRIVATE_PEM);
            const transaction = await sendSimpleTransaction("polygon", privateKey, "0x8abde6c55ca701cceed8fa47896c66919c59372c", body.price);
            if (transaction.success)
            {
                const txHash = transaction.txHash;
                await this.updateOne({ userId: userId }, {
                    $push: { 
                        unlocks: body.unlockId
                }
                });
                return {status: true, paymentType: paymentType, message: "Success!", txHash: txHash};
            }
            else
            {
                return {status: false, message: "Transaction failed. Please check if you have enough funds to get this item.", txHash: ""};
            }

        }
    }
    return {status: false, message: "You are not registered in the game. Please login and try again.", txHash: ""};
})

statsSchema.static('getLevelRequirements', async function (LevelModel) {
    try {
        const requirements = await LevelModel.find().sort({ level: 1 });
        const cumulativeRequirements = convertToCumulativeRequirements(requirements);
        return cumulativeRequirements;
    } catch (error) {
        console.log(error);
        return [];
    }

})


const Stats = mongoose.model('stats', statsSchema)
module.exports = Stats