const CONSTANTS = require('../constants')
const { getSecret, getWalletAddressFromPrivateKey, convertEtherToWei, convertUsdToWei, decrypt } = require('../utils/helperFunctions');
const ERRORS = require('../utils/errorTypes');
const { sendStake, revokeStake, sendWinningsAll, retrySendWinningsAll, getSignature, signTransaction } = require('../services/blockchain.service')
const { AWS_SECRET, STAKE_CONTRACT_ADDRESS_POLYGON, STAKE_CONTRACT_ADDRESS_POLYGON_TEST, STAKE_CONTRACT_ADDRESS_ETH, RPC_POLYGON, RPC_POLYGON_TEST, RPC_ETH } = require('../config');
const stakeABI = require('../../abis/stake.json')

async function sendTourStake(playerId, body, tournament, UserModel) {

	const { INFURA_KEY, PRIVATE_PEM } = await getSecret(AWS_SECRET);
    
	const privKey = await decrypt(body.key, body.iv, body.cipher, PRIVATE_PEM);
    //const privKey = "0xcb8619bff8b5cba51be1987ae2835ee8efc0efc139dc11115b6a5a01d4b05967";
	const walletAddress = getWalletAddressFromPrivateKey(privKey);
	const user = await UserModel.findOne({ accountId: playerId });
	if (!user) return { success: false, log: ERRORS.NO_USER_FOUND.MESSAGE, txHash: "" };
	if (user.walletAddress != walletAddress) return { success: false, log: ERRORS.INVALID_WALLET_ADDRESS.MESSAGE, txHash: "" };
	if (user.isBanned) return { success: false, log: ERRORS.BANNED_USER.MESSAGE, txHash: "" };
	//if (!user.accessInfo.passedKyc) return { success: false, log: ERRORS.KYC_NOT_PASSED.MESSAGE, txHash: "" };

	//console.log(body.stakeAmount);
	const {transactionSigned, signature, decimals} = await signatures(tournament.stakeInfo.network, privKey, walletAddress, INFURA_KEY);

    const {status, tx, receipt} = await sendStake(
		transactionSigned.contractSigned,
		transactionSigned.maxFeePerGas,
		transactionSigned.maxPriorityFeePerGas,
        signature.signature, signature.nonce, body.stakeAmount, tournament._id.toString(), decimals);
    return { success: true, log: "", txHash: receipt.transactionHash };
}

async function revokeTourStake(playerId, body, tournament, UserModel) {

	const { INFURA_KEY, PRIVATE_PEM } = await getSecret(AWS_SECRET);
    
	const privKey = await decrypt(body.key, body.iv, body.cipher, PRIVATE_PEM);
    //const privKey = "0xed97d13226c887c04974d4b00d4b36aca243cc13322f8be7f1dfa69ee4e54211";
	const walletAddress = getWalletAddressFromPrivateKey(privKey);
	const user = await UserModel.findOne({ accountId: playerId });
	if(user && user.walletAddress != walletAddress){
		throw new Error(ERRORS.INVALID_WALLET_ADDRESS.CODE)
	}

	const {transactionSigned, signature} = await signatures(tournament.stakeInfo.network, privKey, walletAddress, INFURA_KEY);

    const {status, tx, receipt} = await revokeStake(
		transactionSigned.contractSigned,
		transactionSigned.maxFeePerGas,
		transactionSigned.maxPriorityFeePerGas,
        signature.signature, signature.nonce, tournament._id.toString());
    return receipt.transactionHash;
}

async function sendCustomTourFee(user, body, matchId, tourFee) {
	const { INFURA_KEY, PRIVATE_PEM } = await getSecret(AWS_SECRET);
    
	const privKey = await decrypt(body.key, body.iv, body.cipher, PRIVATE_PEM);
    //const privKey = "0xcb8619bff8b5cba51be1987ae2835ee8efc0efc139dc11115b6a5a01d4b05967";
	const walletAddress = getWalletAddressFromPrivateKey(privKey);
	if (!user) return { success: false, log: ERRORS.NO_USER_FOUND.MESSAGE, txHash: "" };
	if (user.walletAddress != walletAddress) return { success: false, log: ERRORS.INVALID_WALLET_ADDRESS.MESSAGE, txHash: "" };
	//if (!user.accessInfo.passedKyc) return { success: false, log: ERRORS.KYC_NOT_PASSED.MESSAGE, txHash: "" };

	const {transactionSigned, signature, decimals} = await signatures(body.network, privKey, walletAddress, INFURA_KEY);

    const {status, tx, receipt} = await sendCustomFee(
		transactionSigned.contractSigned,
		transactionSigned.maxFeePerGas,
		transactionSigned.maxPriorityFeePerGas,
        signature.signature, signature.nonce, tourFee, matchId, decimals);
    return { success: true, log: "", txHash: receipt.transactionHash };
}

async function startSendingWinningsAll(tourModel, tournament, UserModel) {
	// Wait 5 mins for player reports (if any)
	await delay(300000);
	const tourResult = await tourModel.findOne({ _id: tournament._id.toString() });
	const Ids = Object.keys(tourResult.players);
	const numPlayers = Ids.length;
	if (tourResult.stakeInfo.flaggedPlayers && numPlayers >= 12)
	{
		const flaggedIds = Object.keys(tourResult.stakeInfo.flaggedPlayers);
		if (flaggedIds.length > 0)
		{
			let bets = [], scores = [];
			for (const Id of Ids)
			{
				let player = tourResult.players[Id];
				if (player.stakeAmount) bets.push(player.stakeAmount);
				if (player.score[1]) scores.push(player.score[1]);
			}
			const [betMean, betStdDev] = await getMeanStd(bets);
			const [scoreMean, scoreStdDev] = await getMeanStd(scores);

			var numTimesFlagged;
			let flaggedUsers = [];
			for (const flaggedId of flaggedIds)
			{
				numTimesFlagged = tourResult.stakeInfo.flaggedPlayers[flaggedId].numTimesFlagged;
				const flagCondition = numTimesFlagged / numPlayers >= 0.75 &&
					tourResult.players[flaggedId].stakeAmount > betMean + 1.5 * betStdDev &&
					tourResult.players[flaggedId].score[1] > scoreMean + 1.5 * scoreStdDev;
				if (flagCondition)
				{
					flaggedUsers.push(flaggedId);
				}
			}
			await sendTourWinningsAll(tourModel, tournament, UserModel, flaggedUsers);
		}
	}
	else
	{
		await sendTourWinningsAll(tourModel, tournament, UserModel);
	}
}

async function sendTourWinningsAll(tourModel, tournament, UserModel, flaggedUsers = []) {
	try 
	{
		const { INFURA_KEY, STAKE_PRIVATE_KEY } = await getSecret(AWS_SECRET);
		const privKey = "0x" + STAKE_PRIVATE_KEY;
		const walletAddress = getWalletAddressFromPrivateKey(privKey);
		const {finalAddresses, finalPercentages, flaggedArray} = await calculateSplit(tourModel, tournament, UserModel, flaggedUsers);
		const {transactionSigned} = await signatures(tournament.stakeInfo.network, privKey, walletAddress, INFURA_KEY, 1.05);

		const {status, tx, receipt} = await sendWinningsAll(
			transactionSigned.contractSigned,
			transactionSigned.maxFeePerGas,
			transactionSigned.maxPriorityFeePerGas,
			tournament._id.toString(), finalAddresses, finalPercentages, flaggedArray);
		
		for (const event of receipt.events) {
			if (event.event === 'SendPrizePoolFailed' && event.args.failedAddresses.length > 0) {
				// Retry to send the winnings again if some transactions failed
				const {transactionSignedRetry} = await signatures(tournament.stakeInfo.network, privKey, walletAddress, INFURA_KEY, 1.05);
				const {statusRetry, txRetry, receiptRetry} = await retrySendWinningsAll(
					transactionSignedRetry.contractSigned,
					transactionSignedRetry.maxFeePerGas,
					transactionSignedRetry.maxPriorityFeePerGas,
					tournament._id.toString());

				for (const eventRetry of receiptRetry.events) {
					if (eventRetry.event === 'SendPrizePoolFailed' && eventRetry.args.failedAddresses.length > 0) 
					{
						const resultWithFailed = await tourModel.findOneAndUpdate(
							{ _id: tournament._id.toString() },
							{ $set: 
								{ 
									'stakeInfo.failedAddresses': eventRetry.args.failedAddresses,
									'stakeInfo.failedAmounts': eventRetry.args.failedAmounts} 
								},
							{ returnDocument: 'after' }
						);
						break;
					}
				}
				return {status: true, message: "", txHash: receiptRetry.transactionHash};
			}
		}
		return {status: true, message: "", txHash: receipt.transactionHash};
	} catch (error) {
		console.log(error)
		return {status: false, message: "FAILED TO CALCULATE WINNING SPLIT", txHash: "Failed"};
	}
}

async function getSortedPlayers(players, numIds = 0) {
	const playersWithScoreAndStake = Object.fromEntries(
		Object.entries(players).filter(
			([key, value]) => 
				value.hasOwnProperty('score') && 
				value.hasOwnProperty('stakeAmount') && 
				value.stakeAmount > 0
		)
	);
	const playersArray = Object.entries(playersWithScoreAndStake);
	// Sort the players based on their ranking position
	playersArray.sort(([, a], [, b]) => a.score[0] - b.score[0]);
	const topPlayers = numIds == 0 ? playersArray : playersArray.slice(0, numIds);
	return Object.fromEntries(topPlayers);
}

async function calculateSplit(tourModel, tournament, UserModel, flaggedUsers) {

    // Values corresponding the the UE data table name of multiplierName
    const multiplierMap = {
        'Multiplier_5': 0.05,
        'Multiplier_10': 0.1,
        'Multiplier_20': 0.2,
        'Multiplier_30': 0.3,
        'Multiplier_40': 0.4,
        'Multiplier_50': 0.5,
    };

	const players = tournament.players;
	const sumStakes = Object.values(tournament.players).reduce(
		(sum, player) => sum + player.stakeAmount,
		0
	);

	const initialSortedPlayers = await getSortedPlayers(players);
	const initialIds = Object.keys(initialSortedPlayers);

	const userAccounts = await UserModel.find({ accountId: { $in: initialIds } }).lean();
	const userMap = Object.fromEntries(userAccounts.map(user => [user.accountId, user]));
	const validIds = new Set(userAccounts.map(user => user.accountId));
	const sortedPlayers = Object.fromEntries(
		Object.entries(initialSortedPlayers).filter(([key]) => validIds.has(key))
	);
	const Ids = Object.keys(sortedPlayers);

	// Condition to check if scores are all non-zero or single player in game 
	const scoreCondition = Object.values(sortedPlayers).every(checkPlayer => checkPlayer.score[1] === 0) || Ids.length == 1;

	let betArray = [], originalBetArray = [], scoreArray = [], addressArray = [], isFlagged, flaggedArray = [];
	const userBulkOperations = [];
	let betWeights = new Array(Ids.length).fill(1);
	if (Ids.length > 0) betWeights[0] = 2;
	if (Ids.length > 1) betWeights[1] = 1.5;
	if (Ids.length > 2) betWeights[2] = 1.25;
	let i = 0, userMultiplier = 0;

	for (const [Id, player] of Object.entries(sortedPlayers))
	{
		userMultiplier = (player.multiplierType && multiplierMap[player.multiplierType]) ? multiplierMap[player.multiplierType] : 0;
		originalBetArray.push(player.stakeAmount);
		betArray.push(player.stakeAmount * (betWeights[i] + userMultiplier));
		scoreArray.push(scoreCondition ? 100 : player.score[1]);

		isFlagged = flaggedUsers.some(element => element === Id);
		flaggedArray.push(isFlagged);

		const user = userMap[Id];
		if (user) {
			addressArray.push(user.walletAddress);
			userBulkOperations.push({
				updateOne: {
					filter: { accountId: Id },
					update: { $set: { isBanned: isFlagged } }
				}
			});
		}
		i++;
	}

	if (userBulkOperations.length > 0) {
		await UserModel.bulkWrite(userBulkOperations);
	}

	let combinedWeights;
	if (tournament.mode == 'solo')
	{
		combinedWeights = scoreArray.map((rank, index) => rank * betArray[index]);
	}
	else if (tournament.mode == 'team')
	{
		const teamSplitObj = calcTeamSplit(sortedPlayers, Ids, scoreCondition);
		combinedWeights = Object.values(teamSplitObj);
	}
	const percentageSplitArray = getPercents(combinedWeights);

	const [mean, stdDev] = await getMeanStd(originalBetArray);
	let sumStakeTour = await tourModel.findOneAndUpdate(
		{ _id: tournament._id.toString() },
		{ $set: {
			[`stakeInfo.totalStake`]: sumStakes,
			[`stakeInfo.averageStake`]: mean,
			[`stakeInfo.stdDevStake`]: stdDev
			} 
		},
		{ returnDocument: 'after' }
	);

	const bulkOperations = Ids.map((Id, index) => {
		return {
			updateOne: {
				filter: { _id: tournament._id.toString() },
				update: {
					$set: {
						[`players.${Id}.prizePercent`]: percentageSplitArray[index],
						[`players.${Id}.wallet`]: addressArray[index],
						[`players.${Id}.flagged`]: flaggedArray[index]
					}
				}
			}
		};
	});

	await tourModel.bulkWrite(bulkOperations);

	const finalAddresses = addressArray;
	const finalPercentages = percentageSplitArray;
	/*
	let finalAddresses = [], finalPercentages = [];
	for (let i = 0; i < flaggedArray.length; i++)
	{
		if (!flaggedArray[i])
		{
			finalAddresses.push(addressArray[i]);
			finalPercentages.push(percentageSplitArray[i]);
		}
	}
		*/
	return {finalAddresses, finalPercentages, flaggedArray};
}

function calcTeamSplit(players, sortedIds, scoreCondition)
{
  const teamData = Object.entries(players).reduce((acc, [playerId, { score, stakeAmount, teamId }]) => {
    if (!acc[teamId]) {
      acc[teamId] = { totalScore: 0, totalStake: 0, count: 0, playerProducts: {}, playerProductSum: 0 };
    }
    const playerProduct = score[1] * stakeAmount;
  
    acc[teamId].totalScore += score[1];
    acc[teamId].totalStake += stakeAmount;
    acc[teamId].count += 1;
    acc[teamId].playerProducts[playerId] = playerProduct;
    acc[teamId].playerProductSum += playerProduct;
  
    return acc;
  }, {});
  
  // Step 2: Calculate averages and identify the team with the highest average score
  let maxAverage = 0;
  let maxAverageTeamId = null;
  
  const teamProducts = Object.keys(teamData).map((teamId) => {
    const team = teamData[teamId];
    const averageScore = team.totalScore / team.count;
    const averageStake = team.totalStake / team.count;
    const teamProduct = averageScore * averageStake;
  
    if (averageScore > maxAverage) {
      maxAverage = averageScore;
      maxAverageTeamId = teamId;
    }
  
    return { teamId, teamProduct };
  });
  
  // Step 3: Boost the highest team's product and calculate the total sum
  const totalProductSum = teamProducts.reduce((sum, { teamId, teamProduct }) => {
    const boostedProduct = teamId === maxAverageTeamId ? teamProduct * 2 : teamProduct;
    teamData[teamId].boostedProduct = boostedProduct;
    return sum + boostedProduct;
  }, 0);
  
  // Step 4: Calculate normalized products and distribute to players
  const playerDistributions = Object.entries(teamData).reduce((acc, [teamId, team]) => {
    const normalizedProduct = team.boostedProduct / totalProductSum;
  
    Object.entries(team.playerProducts).forEach(([playerId, playerProduct]) => {
      const playerShare = (playerProduct / team.playerProductSum) * normalizedProduct;
      acc[playerId] = playerShare;
    });
  
    return acc;
  }, {});

  const rearrangedDistributions = {};
  for (const id of sortedIds) {
    if (playerDistributions[id] !== undefined) {
      rearrangedDistributions[id] = scoreCondition ? 100 : playerDistributions[id] * 100;
    }
  }

  return rearrangedDistributions;
}

async function resolveSuspects(tourModel, tournamentId, suspectObj) {
	// suspectObj = {"Id1": true, "Id2": false...}

	const tour = await tourModel.findOne({ _id: tournamentId });
	const sortedPlayers = await getSortedPlayers(tour.players);
	const Ids = Object.keys(sortedPlayers);

	let betArray = [], addressArray = [], penalizedArray = [];
	for (const Id of Ids) {
		let player = tour.players[Id];
		if (player.flagged)
		{
			if (suspectObj[Id] === true)
			{
				betArray.push(0);
				penalizedArray.push(99);
			}
			else
			{
				betArray.push(player.prizePercent / 100);
				penalizedArray.push(100);
			}
		}
		else
		{
			betArray.push(player.prizePercent / 100);
			penalizedArray.push(100);
		}
		addressArray.push(player.wallet);
	  }

	  const resolvedPercentages = getPercents(betArray);
	  return { addressArray, resolvedPercentages, penalizedArray };
}

function getPercents(array)
{
	const sumArray = array.reduce((accumulator, currentValue) => accumulator + currentValue, 0);
	const normalizedArray = array.map(num => (num / sumArray) * 100);

	let splitRounded = normalizedArray.map(num => Math.round(num * 100) / 100);
	const sumSplitRounded = splitRounded.reduce((accumulator, currentValue) => accumulator + currentValue, 0);

	const difference = 100 - sumSplitRounded;
	if (difference < 0)
	{
		for (let i = splitRounded.length - 1; i >= 0; i--)
		{
			if (splitRounded[i] > 0)
			{
				splitRounded[i] = splitRounded[i] + difference;
				break;
			}
		}
	}
	else if (difference > 0)
	{
		for (let i = 0; i < splitRounded.length - 1; i++)
		{
			if (splitRounded[i] > 0)
			{
				splitRounded[i] = splitRounded[i] + difference;
				break;
			}
		}
	}
	const percentageSplitArray = splitRounded.map(num => Math.round(num * 100));
	return percentageSplitArray;
}

async function signatures(network, privKey, walletAddress, INFURA_KEY, GasBuffer = 1) {
	let signature, transactionSigned;
    let decimals;
    switch (network) {
        case CONSTANTS.ETH_NETWORK:
            signature = await getSignature(STAKE_CONTRACT_ADDRESS_ETH, RPC_ETH + INFURA_KEY, walletAddress);
            transactionSigned = await signTransaction(STAKE_CONTRACT_ADDRESS_ETH, stakeABI, RPC_ETH + INFURA_KEY, privKey, CONSTANTS.ETH_NETWORK, GasBuffer);
            decimals = 18;
            break;

        case CONSTANTS.POLYGON_NETWORK:
            signature = await getSignature(STAKE_CONTRACT_ADDRESS_POLYGON, RPC_POLYGON + INFURA_KEY, walletAddress);
            transactionSigned = await signTransaction(STAKE_CONTRACT_ADDRESS_POLYGON, stakeABI, RPC_POLYGON + INFURA_KEY, privKey, CONSTANTS.POLYGON_NETWORK, GasBuffer);
            decimals = 18;
            break;

        case CONSTANTS.POLYGON_NETWORK_TEST:
            signature = await getSignature(STAKE_CONTRACT_ADDRESS_POLYGON_TEST, RPC_POLYGON_TEST + INFURA_KEY, walletAddress);
            transactionSigned = await signTransaction(STAKE_CONTRACT_ADDRESS_POLYGON_TEST, stakeABI, RPC_POLYGON_TEST + INFURA_KEY, privKey, CONSTANTS.POLYGON_NETWORK, GasBuffer);
            decimals = 18;
            break;

        default:
            console.log("Unknown network for staking");
    }
	return {transactionSigned, signature, decimals};
}

function delay(ms) {
	return new Promise(resolve => setTimeout(resolve, ms));
  }

async function getMeanStd (array) {
	const n = array.length;
	const mean = array.reduce((a, b) => a + b) / n;
	const stdDev = Math.sqrt(array.map(x => Math.pow(x - mean, 2)).reduce((a, b) => a + b) / n);
	return [mean, stdDev];
  }


module.exports = {
	sendTourStake,
	revokeTourStake,
	sendCustomTourFee,
	sendTourWinningsAll
}
