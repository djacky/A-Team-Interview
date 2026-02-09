const mongoose = require('mongoose');
const { tournamentSchema } = require('./schema');
const CONSTANTS = require('../constants')
const { fetchWithRetry, getSecret } = require('../utils/helperFunctions');
const { AWS_SECRET, AWS_REGION } = require('../config');
const { DateTime } = require('luxon');
const stakingHelper = require('../helpers/staking.helper')

let SchedTasksModel;
tournamentSchema.static('startTasks', async function (TaskModel, UserModel) {
    try {
        const tasks = await TaskModel.find();
        for (const task of tasks) {
          const timeToRun = task.runAt - Date.now();
          if (timeToRun <= 0) {
            await updateTournament(task.taskId, UserModel, task.createdBy, false);
            await TaskModel.deleteOne({ taskId: task.taskId });
          } else {
            // Set up a timeout to run the task in the future
            setTimeout(async () => {
              await updateTournament(task.taskId, UserModel, task.createdBy, true);
              await TaskModel.deleteOne({ taskId: task.taskId });
            }, timeToRun);
            console.log(`Task for tournament ${task.taskId} scheduled to run in ${timeToRun} ms`);
          }
        } 
        SchedTasksModel = TaskModel;
        
    } catch (error) {
        console.log(error);
    }
})

tournamentSchema.static('clearOldEntries', async function (TaskModel, days) {
    try {
        const clearAllBeforeThisTime = Math.floor(Date.now() / 1000) - days * 24 * 60 * 60;

        await this.deleteMany({
            timeUnixSeconds: { $lt: clearAllBeforeThisTime }
        });

        await TaskModel.deleteMany({
            runAt: { $lt: 1000 * clearAllBeforeThisTime }
        });

    } catch (error) {
        console.log(error);
    }
})

tournamentSchema.static('setTournament', async function (reqBody, TaskModel) {

    try {
        const tournament = await addTournament(reqBody, TaskModel, null);
        return tournament;
    } catch (error) {
        console.log(error);
    }
})

tournamentSchema.static('userSetTournament', async function (Id, reqBody, TaskModel, UserModel) {

    try {
        let feasibleRegions = ["us-east-1", "eu-central-1"];
        let feasibleModes = ["solo", "team"];
        let feasibleNetworks = ["ethereum", "polygon", "polygonTest"];
        const time = reqBody.time;

        const user = await UserModel.findOne({ accountId: Id });
        if (!user) return {status: false, message: "Your user information was not found in the database."};
        if (user.isBanned) return {status: false, message: "You don't have access to create custom tournaments."};
        if (user.tournamentsCreated && user.tournamentsCreated.length >= 3) return {status: false, message: "You've reached your limit in creating custom tournaments."};
        
        // If requested time is more than 7 days
        if (typeof time !== 'number' || time > Date.now()/1000 + 604800) return {status: false, message: "Chosen time is incorrect. Please choose a new time."};
        const existingTours = await this.findOne({type: "customStake", timeUnixSeconds: time});
        if (existingTours) return {status: false, message: "Time slot is already taken. Please specify another time slot."};

        // Check is the sent time is correct (at invervals of 0 or 30, and doesn't conflict with current tournament times)
        const date = new Date(time * 1000);
        const minutes = date.getMinutes();
        const currentTourTimes = await getTourTimes();
        const bTimeConflict = currentTourTimes.includes(time);
        if ((minutes !== 0 && minutes !== 30) || bTimeConflict) return {status: false, message: "There is a conflict with the time you selected."};

        // If requested time is within a 2 hour period
        //if (time < Date.now()/1000 + 7200) return {status: false, message: "Please choose a new time slot for the tournament."};
        if (time < Date.now()/1000 + 1800) return {status: false, message: "Please choose a new time slot for the tournament."};
        if (typeof reqBody.minPlayers !== 'number' || typeof reqBody.limit !== 'number' || reqBody.minPlayers < 2 || 
            reqBody.minPlayers > 30 || reqBody.limit < 2 || reqBody.limit > 30 || reqBody.minPlayers > reqBody.limit)
        {
            return {status: false, message: "Inconsistency with minimum and maximum players selected. Check these parameters."};
        }
        
        let stakeLimits = [parseFloat(reqBody.stakeInfoMin), parseFloat(reqBody.stakeInfoMax)]
        if (typeof stakeLimits[0] !== 'number' || typeof stakeLimits[1] !== 'number' ||
            stakeLimits[0] == 0 || stakeLimits[1] == 0 || stakeLimits[0] > stakeLimits[1])
        {
            return {status: false, message: "Inconsistency with minimum and maximum stake. Check these parameters. They both must be greater than 0."};
        }
        
        if (!feasibleRegions.includes(reqBody.region)) return {status: false, message: "Selected region is not feasible."};
        if (!feasibleModes.includes(reqBody.mode)) return {status: false, message: "Selected mode is not feasible."};
        if (!feasibleNetworks.includes(reqBody.network)) return {status: false, message: "Selected network is not feasible."};

        reqBody.stakeInfo = {limits: stakeLimits, network: reqBody.network, rake: 0.07, winningsTxHash: ""};
        reqBody.active = true;
        reqBody.type = "customStake";
        reqBody.reward = "N/A";
        reqBody.createdBy = Id;
        const createdTournament = await addTournament(reqBody, TaskModel, UserModel);

        await UserModel.findOneAndUpdate(
            { accountId: Id },
            { $push: { tournamentsCreated: createdTournament._id.toString() } },
            { upsert: true }
        );

        if (createdTournament) return {status: true, message: "Tournament created successfully!"};

        /*
        const fee = reqBody.minPlayers * stakeLimits[0] * 0.02;
        // NEED TO FIX THIS
        const { success, log, txHash } = await stakingHelper.sendCustomTourFee(
            user,
            reqBody,
            createdTournament._id.toString(),
            fee
            );
        if (!success) return {status: false, message: log};
        if (txHash) return {status: true, message: txHash};
        return {status: false, message: "Something went wrong. Could not process the transaction. Please try again."};
        */
        
        
    } catch (error) {
        console.log(error);
        return {status: false, message: "An error occured...Please try again later"};
    }
})

async function addNewStakeTournament(net, tourRegion)
{
    let body = {};
    let maxTimeRegion = {};
    let stakeLimits = {};
    let name = {};
    let description = {};

    const regions = ['us-east-1', 'eu-central-1'];
    const modes = ['solo', 'team'];
    const mode = modes[Math.floor(Math.random() * modes.length)];
    const networks = ['polygon', 'ethereum', 'polygonTest'];

    body.type = 'stake';
    body.createdBy = '';
    body.userPassword = '';
    body.mode = mode;
    body.active = true;
    body.minPlayers = 2;
    body.limit = 30;
    body.reward = '200';
    body.userPassword = '';
    body.matchStarted = false;

    stakeLimits["ethereum"] = [0.002, 0.1];
    stakeLimits["polygon"] = [10, 500];
    stakeLimits["polygonTest"] = [0.5, 500];
    name["solo"] = "Solo Mayhem";
    name["team"] = "Team Rampage";
    description["solo"] = `Solo tournament with prize pool split among all players (performance-based). Minimum of ${body.minPlayers} players needed to kick it off.`;
    description["team"] = `Team tournament with prize pool split among all players (performance-based). Minimum of ${body.minPlayers} players needed to kick it off.`;

    const maxTimeTournament = await Tournament.find({ 
        region: tourRegion,
        'stakeInfo.network': net
    }).sort({ timeUnixSeconds: -1 }).limit(1);

    if (maxTimeTournament && maxTimeTournament.length !== 0)
    {
        maxTimeRegion[tourRegion] = maxTimeTournament[0]?.timeUnixSeconds + 24 * 3600;
    }
    else
    {
        maxTimeRegion[tourRegion] = Date.now()/1000 + 24 * 3600;
    }

    body.name = name[mode];
    body.description = description[mode];
    body.region = tourRegion;
    body.time = maxTimeRegion[tourRegion];
    body.stakeInfo = {
            "network": net,
            "limits": stakeLimits[net],
            "winningsTxHash": "",
            "rake": 0.07
        }
    await addTournament(body, SchedTasksModel, null);
}

async function addTournament(reqBody, TaskModel, UserModel)
{
    let time;
    let ext;
    const tourRegion = reqBody.region;
    const userCreatedId = reqBody.createdBy;
    time = DateTime.fromSeconds(reqBody.time, { zone: 'utc' });
    ext = "(UTC)";
    
    const formattedDate = `${time.day}.${time.month}.${time.year}`;
    const formattedTime = time.toFormat('HH:mm') + ` ${ext}`;

    const tournamentDetails = {
        name: reqBody.name,
        mode: reqBody.mode,
        timeUnixSeconds: reqBody.time,
        date: formattedDate,
        time: formattedTime,
        active: reqBody.active,
        description: reqBody.description,
        reward: reqBody.reward,
        limit: reqBody.limit,
        region: tourRegion,
        minPlayers: reqBody.minPlayers,
        stakeInfo: reqBody.stakeInfo,
        type: reqBody.type,
        userPassword: reqBody.userPassword,
        createdBy: userCreatedId,
        matchStarted: false
    };
    let newTournament = await Tournament.create(tournamentDetails);
    const tourId = newTournament._id.toString();
    
    const runAtMs = 1000 * (reqBody.time - 300);
    await TaskModel.findOneAndUpdate(
        { taskId: tourId },
        { taskId: tourId, runAt: runAtMs, createdBy: userCreatedId},
        { upsert: true }
    );
    
    setTimeout(async () => {
        await updateTournament(tourId, UserModel, userCreatedId, true);
        await TaskModel.deleteOne({ taskId: tourId });
      }, runAtMs - Date.now());

    return newTournament;
}

async function updateTournament(tournamentId, UserModel, userCreated, createNewTour)
{
    const tournament = await Tournament.findOne({ _id: tournamentId, "active": true });
    if (tournament)
    {
        const playersRegistered = Object.keys(tournament.players).length;
        if (playersRegistered == 0 || (tournament.type === 'free' && playersRegistered < tournament.minPlayers))
        {
            console.log("playersRegistered = 0");
            if (UserModel && userCreated)
            {
                await UserModel.findOneAndUpdate(
                    { accountId: userCreated },
                    { $pull: { tournamentsCreated: tournamentId } },
                    { returnDocument: 'after' }
                );
            }
            if (tournament.type === 'stake' && createNewTour)
            {
                const currentUnixTime = Math.floor(Date.now() / 1000);
                const count = await Tournament.countDocuments({ 
                    region: tournament.region,
                    'stakeInfo.network': tournament.stakeInfo.network,
                    timeUnixSeconds: { $gt: currentUnixTime }
                });
                if (count <= 5)
                {
                    await addNewStakeTournament(tournament.stakeInfo.network, tournament.region);
                }
            }
            await Tournament.deleteOne({ _id: tournamentId });
            console.log("Tournament deleted");
            return;
        }
        console.log("Tournament NOT deleted");
    }
}

tournamentSchema.static('addPlayer', async function (playerId, body, UserModel, StatsModel) {

    let inventoryInfo = {};
    const stakeMultiplier = body.multiplier;
    const playerRegistered = await checkPlayer(body.tournamentId, playerId);
    if (playerRegistered) return {status: false, message: "You are already registered for this tournament.",  tournament: null};

    const selectedTournament = await this.findOne({ _id: body.tournamentId, "active": true });
    if (!selectedTournament) return {status: false, message: "Tournament has been canceled.",  tournament: null};
    if (Object.keys(selectedTournament.players).length >= selectedTournament.limit) return {status: false, message: "This tournament is already full.",  tournament: null};
    
    const items = [];

    if (selectedTournament.needsStakeTicket) {
        items.push({
            key: 'TicketStake',
            errorMessage: "You need a KZ stake ticket to participate"
        });
    }

    if (stakeMultiplier && stakeMultiplier !== 'None') {
        items.push({
            key: stakeMultiplier,
            errorMessage: "You don't have this type of multiplier. Choose another multiplier that you own, or None"
        });
    }

    if (items.length > 0) {
        const invResult = await processInventoryItems(playerId, StatsModel, items, true);
        if (!invResult.status) return invResult;
        
        // invResult.inventory will be like: { "TicketStake": 2, "Multiplier_5": 6 }
        inventoryInfo = invResult.inventory;
    }

    const unixTimestamp = Math.floor(Date.now() / 1000);
    if (unixTimestamp >= selectedTournament.timeUnixSeconds - CONSTANTS.CLOSE_TOURNAMENT_SECONDS) return {status: false, message: "It's too late to register for this tournament.",  tournament: null};

    let playerInfo = {name: body.playerName};
    if (["stake", "customStake"].includes(selectedTournament.type.toString()))
    {
        if (selectedTournament.type === "customStake" && selectedTournament.userPassword &&
            ((body.password && body.password !== selectedTournament.userPassword) || !body.password))
        {
            return {status: false, message: "You did not provide the correct password to register.",  tournament: null};
        }
        const stakeAmount = parseFloat(body.stakeAmount);
        if (stakeAmount < selectedTournament.stakeInfo.limits[0] || stakeAmount > selectedTournament.stakeInfo.limits[1])
        {
            return {status: false, message: `This tournament requires a stake between ${selectedTournament.stakeInfo.limits[0]} and ${selectedTournament.stakeInfo.limits[1]} ${selectedTournament.stakeInfo.network}`,  tournament: null}
        }
        try 
        {
            const { success, log, txHash } = await stakingHelper.sendTourStake(
                playerId,
                body,
                selectedTournament,
                UserModel);
            if (!success)
            {
                return {status: false, message: log, tournament: null}
            }
            if (txHash)
            {
                playerInfo.stakeAmount = stakeAmount;
                playerInfo.stakeTxHash = txHash;
                if (stakeMultiplier && stakeMultiplier !== 'None') playerInfo.multiplierType = stakeMultiplier;
            }
        } catch (error) {
            return {status: false, message: `Transaction failed. Could not register for the tournament.`,  tournament: null}
        }
    }
    else
    {
        playerInfo.paypalEmail = body.playerEmail;
    }

    let result = await this.findOneAndUpdate(
        { _id: body.tournamentId },    // Query to find the document
        { $set: { [`players.${playerId}`]: playerInfo } },
        { returnDocument: 'after' }  // Options: 'after' returns the updated document
    );
    let tourObj = result.toObject();
    delete tourObj.players;
    if (playerInfo.stakeTxHash) tourObj.txHash = playerInfo.stakeTxHash;
    tourObj.inventoryInfo = inventoryInfo;
    return {status: true, message: "", tournament: tourObj};
})

tournamentSchema.static('removePlayer', async function (body, playerId, UserModel, StatsModel) {

    let inventoryInfo = {};
    const tournamentId = body.tournamentId;
    const playerRegistered = await checkPlayer(tournamentId, playerId);
    if (!playerRegistered) return {status: false, message: "You are not registered for this tournament.", tournament: null};

    const selectedTournament = await this.findOne({ _id: tournamentId, "active": true });
    const unixTimestamp = Math.floor(Date.now() / 1000);

    if (selectedTournament.active && unixTimestamp >= selectedTournament.timeUnixSeconds - CONSTANTS.CLOSE_TOURNAMENT_SECONDS)
    {
        if (Object.keys(selectedTournament.players).length >= selectedTournament.minPlayers)
        {
            return {status: false, message: "Tournament is about to start or has already started. Can't unregister.", tournament: null};
        }
    }

    let txHash;
    try
    {
        if (["stake", "customStake"].includes(selectedTournament.type.toString()))
        {
            txHash = await stakingHelper.revokeTourStake(
                playerId,
                body,
                selectedTournament,
                UserModel);
        }
    } catch (error) {
        return {status: false, message: "Failed to unregister...", tournament: null};
    }

    const player = selectedTournament.players[playerId];
    if (player)
    {
        const items = [];

        if (selectedTournament.needsStakeTicket) {
            items.push({
                key: 'TicketStake',
                errorMessage: "You need a KZ stake ticket to participate"
            });
        }

        if (player.multiplierType) {
            items.push({
                key: player.multiplierType,
                errorMessage: "You don't have this type of multiplier. Choose another multiplier that you own, or None"
            });
        }

        if (items.length > 0) {
            const invResult = await processInventoryItems(playerId, StatsModel, items, false);
            // invResult.inventory will be like: { "TicketStake": 2, "Multiplier_5": 6 }
            inventoryInfo = invResult.inventory;
        }
    }
    
    let result = await this.findOneAndUpdate(
        { _id: tournamentId },
        { $unset: { [`players.${playerId}`]: "" } },  // Unset the field for the given playerId
        { returnDocument: 'after' }
    );

    if (result && selectedTournament.active && unixTimestamp >= selectedTournament.timeUnixSeconds - CONSTANTS.CLOSE_TOURNAMENT_SECONDS)
    {
        if (Object.keys(result.players).length < selectedTournament.minPlayers)
        {
            if (result.createdBy)
            {
                console.log("removePlayer: removing player from tournamentsCreated");
                await UserModel.findOneAndUpdate(
                    { accountId: result.createdBy }, 
                    { $pull: { tournamentsCreated: result._id.toString() } },
                    { returnDocument: 'after' }
                    );
            }
            console.log("removePlayer: updateTournament called");
            await updateTournament(result._id.toString(), UserModel, "", true);
        }
    }

    let tourObj = result.toObject();
    delete tourObj.players;
    if (txHash) tourObj.txHash = txHash;
    tourObj.inventoryInfo = inventoryInfo;
    return {status: true, message: "", tournament: tourObj};
})

async function checkPlayer(tournamentId, playerId) {

    const result = await Tournament.findOne(
        { 
            _id: tournamentId,
            [`players.${playerId}`]: { $exists: true }},
            { _id: 1 }
        );
    return !!result;
}

async function processInventoryItems(playerId, StatsModel, items, isRemoval = true) {
    try {
        const query = { userId: playerId };
        const updateOps = { $inc: {} };

        if (isRemoval) {
            // DECREMENT: Build query conditions to check availability
            for (const item of items) {
                query[`inventory.${item.key}`] = { $gte: 1 };
                updateOps.$inc[`inventory.${item.key}`] = -1;
            }

            // Single atomic operation with conditions
            const updateResult = await StatsModel.findOneAndUpdate(
                query,
                updateOps,
                { returnDocument: 'after' }
            );

            if (!updateResult) {
                // Determine which item(s) were insufficient
                const userDoc = await StatsModel.findOne({ userId: playerId });
                for (const item of items) {
                    if (!userDoc?.inventory?.[item.key] || userDoc.inventory[item.key] < 1) {
                        return { 
                            status: false, 
                            message: item.errorMessage || `Insufficient ${item.key}`,
                            inventory: {}
                        };
                    }
                }
                return { status: false, message: "Insufficient inventory items", inventory: {} };
            }

            // Build inventory object with updated values
            const inventory = {};
            for (const item of items) {
                inventory[item.key] = updateResult.inventory[item.key];
            }

            return { status: true, inventory };

        } else {
            // INCREMENT: No conditions needed, just increment
            for (const item of items) {
                updateOps.$inc[`inventory.${item.key}`] = 1;
            }

            const updateResult = await StatsModel.findOneAndUpdate(
                { userId: playerId },
                updateOps,
                { returnDocument: 'after', upsert: true }
            );

            // Build inventory object with updated values
            const inventory = {};
            for (const item of items) {
                inventory[item.key] = updateResult?.inventory?.[item.key] || 1;
            }

            return { status: true, inventory };
        }
    } catch (error) {
        console.error(`Inventory process error for user ${playerId}:`, error);
        return { status: false, message: "Error processing inventory", inventory: {} };
    }
}

tournamentSchema.static('getAllTournaments', async function (playerId, playerRegion, type) {

    const queryConditions = {
        $or: [
            {
                active: true,
                type: type
                //...(type !== 'customStake' && { region: playerRegion }) // Conditionally add 'region' only if 'type' is not 'custom'
            },
            {
                active: true,
                [`players.${playerId}`]: { $exists: true },
                type: type
            }
        ]
    };

    /*
    await this.deleteMany({
        timeUnixSeconds: { $lt: Date.now()/1000},
        active: true,
        $expr: { $eq: [{ $size: { $objectToArray: "$players" } }, 0] }
    });
    */

    let tourArray = await this.find(queryConditions).sort({ timeUnixSeconds: 1 });
    const unixTimestamp = Math.floor(Date.now() / 1000);
    let tournaments = [];
    if (tourArray)
    {
        for (const tour of tourArray)
        {
            let tourObj = tour.toObject();
            tourObj.multiplier = (tourObj.players[playerId] && tourObj.players[playerId].multiplierType) ? tourObj.players[playerId].multiplierType : 'None';
            tourObj.isPlayerRegistered = await checkPlayer(tourObj._id.toString(), playerId);
            const totalStake = await getTotalStakes(tourObj);
            const numPlayers = Object.keys(tourObj.players).length;
            tourObj.isFull = numPlayers >= tourObj.limit || unixTimestamp >= tourObj.timeUnixSeconds - CONSTANTS.CLOSE_TOURNAMENT_SECONDS;
            tourObj.numPlayers = numPlayers;
            //tourObj.totalStake = totalStake * (1 - tourObj.stakeInfo.rake);
            tourObj.totalStake = totalStake;
            tourObj.hasPassword = tourObj.userPassword === "" ? false : true;
            tourObj.needsStakeTicket = tourObj.needsStakeTicket;
            delete tourObj.userPassword;
            delete tourObj.players;
            tournaments.push(tourObj);
        }
        return tournaments;
    }
})

tournamentSchema.static('getSingleTournament', async function (tournamentId, playerId, region) {

    let tour = await this.findOne({ _id: tournamentId });
    const unixTimestamp = Math.floor(Date.now() / 1000);
    //if (tour && (tour.type === "customStake" || tour.region === region))
    if (tour)
    {
        let tourObj = tour.toObject();
        if (tourObj.active)
        {
            tourObj.multiplier = (tourObj.players[playerId] && tourObj.players[playerId].multiplierType) ? tourObj.players[playerId].multiplierType : 'None';
            tourObj.isPlayerRegistered = await checkPlayer(tourObj._id.toString(), playerId);
            const totalStake = await getTotalStakes(tourObj);
            const numPlayers = Object.keys(tourObj.players).length;
            tourObj.isFull = numPlayers >= tourObj.limit || unixTimestamp >= tourObj.timeUnixSeconds - CONSTANTS.CLOSE_TOURNAMENT_SECONDS;
            tourObj.numPlayers = numPlayers;
            //tourObj.totalStake = totalStake * (1 - tourObj.stakeInfo.rake);
            tourObj.totalStake = totalStake;
            tourObj.hasPassword = tourObj.userPassword === "" ? false : true;
            tourObj.needsStakeTicket = tourObj.needsStakeTicket;
            delete tourObj.userPassword;
            delete tourObj.players;

            return tourObj;
        }
        return null;
    }
    return null;
})

tournamentSchema.static('getRegisteredTimes', async function () {

    const timeValues = await getTourTimes();
    return timeValues;
})

async function getTourTimes()
{
    const tournamentTimeValues = await Tournament.find(
        { type: { $in: ["customStake", "customStakePending"] }, active: true }, // Match documents with type in the specified array
        { timeUnixSeconds: 1, _id: 0 } // Project only the timeUnixSeconds field and exclude _id
      )

    return tournamentTimeValues.map(doc => doc.timeUnixSeconds);
}

tournamentSchema.static('saveScores', async function (scoreBody, UserModel, StatsModel) {

    const tournamentId = scoreBody.tournamentId;
    try 
    {
        delete scoreBody.tournamentId;
        var result;

        const updateFields = {};
        let i = 1;
        for (const [id, scoreValue] of Object.entries(scoreBody)) {
            // scoreValue[0] is ranking score, [1] is KDR, [2] is DTR
            updateFields[`players.${id}.score`] = [i, scoreValue[0], scoreValue[1], scoreValue[2]];
            updateFields[`players.${id}.teamId`] = Math.round(scoreValue[3]);
            i++;
        }
        updateFields['active'] = false;
        const updatedResult = await this.findOneAndUpdate(
            { _id: tournamentId },
            { $set: updateFields },
            { returnDocument: 'after' }
        );
        
        result = updatedResult.toObject();
        
        if (["stake", "customStake"].includes(result.type.toString()))
        {
            const {status, message, txHash} = await stakingHelper.sendTourWinningsAll(this, result, UserModel);
            const updatedResult = await this.findOneAndUpdate(
                { _id: tournamentId },
                { $set: { 'stakeInfo.winningsTxHash': txHash } },
                { returnDocument: 'after' }
            );
            result = updatedResult.toObject();
            var prizeObj = {};
            const totalStake = result.stakeInfo.totalStake * (1 - result.stakeInfo.rake);
            const Ids = Object.keys(result.players);
            for (const Id of Ids)
            {
                result.players[Id].prizePercent = (result.players[Id].prizePercent ?? 0) / 100;
                prizeObj[Id] = {
                    percent: result.players[Id].prizePercent,
                    totalStake: totalStake,
                    network: result.stakeInfo.network,
                    name: result.name
                };
            }
            result.prizeInfo = prizeObj;
            await postMatchUpdate(result, Ids, totalStake, UserModel, StatsModel);
        }
    } catch (error) {
        console.log(error);
        return null;
    }

    // Give top players the full version
    const Ids = Object.keys(scoreBody);
    const selectedIds = Ids.slice(0, 3);
    //const selectedIds = Ids;
    const topPlayers = [];
    for (const Id of selectedIds) {
        //topPlayers.push(key);
        await UserModel.findOneAndUpdate(
			{ accountId: Id }, 
			{ $set: { 'accessInfo.reviewCompleted': true } },
			{ returnDocument: 'after' }
			);
    }

    const { WEB_KEY } = await getSecret(AWS_SECRET);
    const fetchOptions = {
        method: 'POST',
        headers: {'Content-Type': 'application/json', "auth": WEB_KEY},
        body: JSON.stringify({"tournamentId": tournamentId})
    };

    console.log("Score Result: ", result);
    try {
        await fetchWithRetry(CONSTANTS.TOURNAMENT_TOP_PLAYERS_URL, fetchOptions);
        return result;
    } catch (error) {
        return result;
    }
})

async function postMatchUpdate(tournamentObj, Ids, totalStake, UserModel, StatsModel) {

    const network = tournamentObj.stakeInfo.network;

    if (tournamentObj.createdBy)
    {
        await UserModel.findOneAndUpdate(
            { accountId: tournamentObj.createdBy }, 
            { $pull: { tournamentsCreated: tournamentObj._id.toString() } },
            { returnDocument: 'after' }
            );
    }

    const stats = await StatsModel.find({ userId: { $in: Ids } });
    const statsMap = stats.reduce((map, stat) => {
        map[stat.userId] = stat;
        return map;
    }, {});

    const users = await UserModel.find({ accountId: { $in: Ids } });
    const userStakeMap = users.reduce((map, user) => {
        map[user.accountId] = user.stakeInfo[network] || {};
        map[user.accountId].privilege = user.accessInfo.privilege || 0;
        return map;
    }, {});

    const bulkOperations = Ids.map((Id) => {

        const networkField = `stakeInfo.${network}`;
        const userStat = statsMap[Id];
        const userInfo = userStakeMap[Id] || {};
        const numTimesStaked = userInfo.numTimesStaked || 0;

        let privilege = userStat ? calculatePrivilege(userStat.stakeStats, numTimesStaked) : 0;
        if (userInfo.privilege > privilege) privilege = userInfo.privilege;

        const updateObj = {
            $inc: {
                [`${networkField}.total`]: tournamentObj.players[Id].stakeAmount,
                [`${networkField}.earned`]: (tournamentObj.players[Id].prizePercent / 100) * totalStake,
                [`${networkField}.numTimesStaked`]: 1,
            },
            $set: { 'accessInfo.privilege': privilege },
        };

        return {
            updateOne: {
                filter: { accountId: Id },
                update: updateObj,
                upsert: true,
            },
        };
        });

    const validOperations = bulkOperations.filter(op => op !== null);
    if (validOperations.length > 0) {
        UserModel.bulkWrite(validOperations).catch(err => {
            console.error("Error during updating user's stake info:", err);
        });
    }

    const bulkStatsOperations = [];
    Ids.forEach((Id) => {
        let statsDoc = statsMap[Id];
        if (!statsDoc) {
            // Create new if not found (with defaults)
            statsDoc = new StatsModel({ userId: Id });
            statsMap[Id] = statsDoc;  // Add to map for future reference
        }

        // Ensure structure (schema defaults should handle, but safeguard)
        if (!statsDoc.latestMatchData) {
            statsDoc.latestMatchData = {
                stats: { kd: [], dt: [] },
                stakeStats: { kd: [], dt: [], stakePL: { polygon: [], ethereum: [] } }
            };
        }
        if (!statsDoc.latestMatchData.stakeStats.stakePL[network]) {
            statsDoc.latestMatchData.stakeStats.stakePL[network] = [];
        }

        // Compute P/L
        const stakeAmount = tournamentObj.players[Id].stakeAmount || 0;
        const earned = ((tournamentObj.players[Id].prizePercent || 0) / 100) * totalStake;
        const pl = earned - stakeAmount;

        // Append and limit to last 5
        const plArray = statsDoc.latestMatchData.stakeStats.stakePL[network];
        plArray.push(pl);
        if (plArray.length > 5) {
            plArray.shift();
        }

        // Prepare bulk update (set the entire array)
        bulkStatsOperations.push({
            updateOne: {
                filter: { userId: Id },
                update: {
                    $set: { [`latestMatchData.stakeStats.stakePL.${network}`]: plArray }
                },
                upsert: true,
            },
        });
    });


    if (bulkStatsOperations.length > 0) {
        await StatsModel.bulkWrite(bulkStatsOperations).catch(err => {
            console.error("Error during updating stats stakePL:", err);
        });
    }
}

function calculatePrivilege(stats, numTimesStaked) {
    if (!stats) return 0;

    const initVar = 3;
    if (numTimesStaked + 1 >= 2**(initVar + 3) && stats.numGamesPlayed >= 2**(initVar + 4)) {
        return 4;
    }
    if (numTimesStaked + 1 >= 2**(initVar + 2) && stats.numGamesPlayed >= 2**(initVar + 3)) {
        return 3;
    }
    if (numTimesStaked + 1 >= 2**(initVar + 1) && stats.numGamesPlayed >= 2**(initVar + 2)) {
        return 2;
    }
    if (numTimesStaked + 1 >= 2**initVar && stats.numGamesPlayed >= 2**(initVar + 1)) {
        return 1;
    }
    return 0;
}

tournamentSchema.static('getScores', async function () {

    try {
        const tours = {};
        const tournaments = await this.find({
            active: false,
            matchStarted: true,
            type: { $in: ["stake", "customStake"] }
        }).sort({ timeUnixSeconds: -1 }).limit(15);

        for (const tour of tournaments)
        {
            const tourObj = {};
            const tourScoreObj = {};
            const tourPercentObj = {};
    
            let Ids;
            const players = tour.players;
            const playersWithScore = Object.fromEntries(
                Object.entries(players).filter(([key, value]) => value.hasOwnProperty('score'))
            );
            const playersArray = Object.entries(playersWithScore);
            // sort the players based on their ranking position
            playersArray.sort(([, a], [, b]) => a.score[0] - b.score[0]);
            const sortedPlayers = Object.fromEntries(playersArray);
            Ids = Object.keys(sortedPlayers);

            for (Id of Ids)
            {
                let player = tour.players[Id];
                if (player)
                {
                    const playerName = player.name;
                    tourScoreObj[playerName] = player.score;
                    tourPercentObj[playerName] = (player.prizePercent / 100).toString() + "%";
                }
            }
            tourObj.Scores = tourScoreObj;
            tourObj.Stakes = tourPercentObj;
            tourObj.network = tour.stakeInfo.network;

            tourObj.type = tour.type;
            tourObj.active = tour.active;
            tourObj.name = tour.name;
            tourObj.date = tour.date;
            tourObj.reward = isNaN(parseInt(tour.reward)) ? tour.reward : parseInt(tour.reward);
            tours[tour._id.toString()] = tourObj;
        }
        return tours;
    } catch (error) {
        console.log(error);
    }

})

tournamentSchema.static('updateStatus', async function (tournamentId, status) {

    const result = await this.findOneAndUpdate(
        { _id: tournamentId },
        { $set: { active: status } },
        { returnDocument: 'after' }
    );
})

tournamentSchema.static('reportPlayer', async function (body, reporter) {

    try {
        await this.updateOne(
            { _id: body.tournamentId, [`stakeInfo.flaggedPlayers.${body.reportedId}`]: { $exists: false } },
            {
                $set: {
                    [`stakeInfo.flaggedPlayers.${body.reportedId}`]: {
                      numTimesFlagged: 0,
                      description: [],
                      flaggedBy: []
                    }
                }
            }
        );

        const result2 = await this.updateOne(
            { _id: body.tournamentId, [`stakeInfo.flaggedPlayers.${body.reportedId}.flaggedBy`]: { $ne: reporter } },
            {
              $push: {
                [`stakeInfo.flaggedPlayers.${body.reportedId}.description`]: body.description,
                [`stakeInfo.flaggedPlayers.${body.reportedId}.flaggedBy`]: reporter
              },
              $inc: {
                [`stakeInfo.flaggedPlayers.${body.reportedId}.numTimesFlagged`]: 1
              }
            },
            {
                returnDocument: 'after' // Return the modified document
            }
          );

        return result2;
    } catch (error) {
        console.log(error);
        return null;
    }
})

tournamentSchema.static('getPlayers', async function (body) {

    const tournament = await this.findOne({ _id: body.tournamentId, "active": true });
    return tournament ? Object.keys(tournament.players) : null;
})

tournamentSchema.static('checkMatchRegister', async function (body, tournamentId, User) {

    try {
        const user = await User.findOne({
            'accessInfo.email': body.accounts.email
        });
        if (user)
        {
            const playerRegistered = await checkPlayer(tournamentId, user.accountId);
            return playerRegistered;
        }
        return null;
    } catch (error) {
        console.log(error);
        return null;
    }
})

tournamentSchema.static('checkMatchCompletion', async function (body, tournamentId, User) {

    try {
        const user = await User.findOne({
            'accessInfo.email': body.accounts.email
        });
        if (user)
        {
            const result = await Tournament.findOne(
                { 
                    _id: tournamentId,
                    [`players.${user.accountId}`]: { $exists: true },
                    [`players.${user.accountId}.prizePercent`]: { $exists: true }
                },
                    { _id: 1 }
                );
            return result;
        }
        return null;
    } catch (error) {
        console.log(error);
        return null;
    }
})

async function getTotalStakes(tournament)
{
    if (["stake", "customStake"].includes(tournament.type.toString()))
    {
        const playerIds = Object.keys(tournament.players);
        if (playerIds.length > 0)
        {
            let sumStake = 0;
            for (const Id of playerIds)
            {
                sumStake += tournament.players[Id].stakeAmount;
            }
            return sumStake;
        }
        return 0;
    }
    else
    {
        return 0;
    }

}

function delay(seconds) {
	return new Promise(resolve => setTimeout(resolve, seconds * 1000));
  }

const Tournament = mongoose.model('tournament', tournamentSchema);
module.exports = Tournament;
