const { responseSuccess, responseBadRequest, responseUnauthorized, responseServerSideError } = require('../utils/responseTypes')
const ERRORS = require('../utils/errorTypes')
const CONSTANTS = require('../constants')
const { AWS_ACCESS_KEY, COGNITO_POOL_ID, AWS_REGION, TEST_MODE } = require('../config');
const { CognitoIdentityClient, GetIdCommand, GetCredentialsForIdentityCommand } = require('@aws-sdk/client-cognito-identity');
const { GameLiftClient, CreateGameSessionCommand, CreatePlayerSessionCommand, SearchGameSessionsCommand, DescribeFleetAttributesCommand, DescribeGameSessionsCommand, DescribeRuntimeConfigurationCommand, DescribeInstancesCommand } = require("@aws-sdk/client-gamelift");
const fs = require('fs')
const path = require('path');
const { Tournament } = require('../models');

const getCognitoCredentials = async (req, res) => {
    try
    {
        const client = new CognitoIdentityClient({ region: AWS_REGION });
        const logins = {
          [CONSTANTS.AUTH_LOGIN_IDENTITY]: req.headers.token  // Replace with your region, User Pool ID, and ID token from login
        };

        const getIdParams = {
            AccountId: AWS_ACCESS_KEY,
            IdentityPoolId: COGNITO_POOL_ID,
            Logins: logins
          };
          
          const getIdCommand = new GetIdCommand(getIdParams);
          const identityResponse = await client.send(getIdCommand);
          
          //console.log('Identity ID:', identityResponse.IdentityId);
      
          const getCredentialsParams = {
            IdentityId: identityResponse.IdentityId,
            Logins: logins
          };
          
          const getCredentialsCommand = new GetCredentialsForIdentityCommand(getCredentialsParams);
          const credentialsResponse = await client.send(getCredentialsCommand);
          let creds = credentialsResponse.Credentials;
          creds.timeLeft = (new Date(creds.Expiration).getTime() - Date.now()) / 1000;

          const publicKeyPath = path.resolve(__dirname, '../../public.pem');
          const publicKey = fs.readFileSync(publicKeyPath, 'utf8');
          //console.log('Credentials:', creds);

        return responseSuccess(res, {creds, publicKey})
    } 
    catch (error) {
        if (ERRORS[error.message]) {
            return responseBadRequest(res, ERRORS[error.message])
        }
        return responseServerSideError(res, {MESSAGE: error.message})
    }
}

let gameLiftFleetId = "";
let gameLiftFleetIdTest = "";
let gameLiftNumProcesses = 0;
async function getGameLiftFleetId() {
  const client = new GameLiftClient({region: AWS_REGION});
  const input = {};
  const command = new DescribeFleetAttributesCommand(input);
  const response = await client.send(command);
  if (response.FleetAttributes.length > 0)
  {
    for (const fleet of response.FleetAttributes)
    {
      if (fleet.Name.toLowerCase().includes("test"))
      {
        gameLiftFleetIdTest = fleet.FleetId;
        console.log("Test Fleet ID: ", gameLiftFleetIdTest);
      }
      else
      {
        gameLiftFleetId = fleet.FleetId;
        console.log("Production Fleet ID: ", gameLiftFleetId);
      }
    }
    //gameLiftFleetId = response.FleetIds[0];
    //gameLiftFleetIdTest = gameLiftFleetId;
    const runtimeInput = { // DescribeRuntimeConfigurationInput
      FleetId: TEST_MODE === 'true' ? gameLiftFleetIdTest : gameLiftFleetId, 
    };
    const runtimeCommand = new DescribeRuntimeConfigurationCommand(runtimeInput);
    const runtimeResponse = await client.send(runtimeCommand);
    
    const instanceInput = { // DescribeInstancesInput
      FleetId: TEST_MODE === 'true' ? gameLiftFleetIdTest : gameLiftFleetId, // required
      Location: "eu-central-1"
    };
    const instanceCommand = new DescribeInstancesCommand(instanceInput);
    const instanceResponse = await client.send(instanceCommand);

    // Total number of game processes (assuming each region has the same number of instances and server processes running)
    gameLiftNumProcesses = (instanceResponse.Instances.length) * (runtimeResponse.RuntimeConfiguration.ServerProcesses.length);
  }
}

let requestQueue = [];
let isProcessing = false;

const startSessionQueue = async (req, res) => {
  requestQueue.push({ req, res });
  if (!isProcessing) {
    processQueue(); // Only start a new loop if none is running
  }
}

let createdGameSessions = [];
let tournamentQueue = {}; //this should be an object, not an array
const pendingGameSessions = new Set();

async function processQueue() {
    isProcessing = true;
    while (requestQueue.length > 0) {
      const { req, res } = requestQueue.shift();
      await checkGameSession(req, res);
  
      // Wait for 100ms before processing the next request
      await new Promise(resolve => setTimeout(resolve, 100));
    }
    isProcessing = false; // Let future requests start a new loop
}

const checkGameSession = async (req, res) => {

    const config = {
      region: AWS_REGION,  // Specify your AWS region here
    };
    const client = new GameLiftClient(config)
  
    // mode in [solo, team] ; type in [freetoplay, skilltoearn]
    const tournamentId = req.body.tournamentId;
    let serverInfo;
    let region = req.body.region;
    let filter = `gameSessionProperties.mode = '${req.body.mode}' AND gameSessionProperties.type = '${req.body.type}' AND hasAvailablePlayerSessions = true`;
    let name = `${req.body.type}-${req.body.mode}`;
    let tournament;
    let lobby = {isFromLobby: req.body.isFromLobby || false, teamId: req.body.teamGUID || "", numLobbyPlayers: req.body.numLobbyPlayers || 0};

    try 
    {
      const isTournamentMode = tournamentId && req.body.type == "tournament";
      if (isTournamentMode)
      {
          tournament = await Tournament.findOne({
            _id: tournamentId,
            "active": true,
            [`players.${req.tokenData.accountId}`]: { $exists: true }
          });
          if (!tournament || (tournament && Object.keys(tournament.players).length < tournament.minPlayers))
          {
            return responseServerSideError(res, {MESSAGE: "You are not registered for this tournament"});
          }
          filter += ` AND gameSessionProperties.tournamentId = '${tournamentId}' AND gameSessionProperties.tourType = '${tournament.type}'`;
          name += `-${tournamentId}-${tournament.type}`;
          region = tournament.region;

          if (!tournament.matchStarted)
          {
            addUniqueId(tournamentId, req.tokenData.accountId);
            if (tournament.type == "free" && tournamentQueue[tournamentId].length < tournament.minPlayers)
            {
              serverInfo = {"ip": "", "port": "", "gameSessionId": "", "status": "Queued"};
              return responseSuccess(res, {serverInfo});
            }
          }
      }

      if (req.body.gameSessionId)
      {
        serverInfo = await onJoinFromLobby(client, req.body.gameSessionId, req.body.playerName, lobby.teamId);
        if (serverInfo)
        {
          return responseSuccess(res, {serverInfo});
        }
      }
      if (region) name += `-${region}`;

      serverInfo = await searchSession(client, filter, region, req.body.playerName, isTournamentMode, lobby);
      if (serverInfo) return responseSuccess(res, {serverInfo});
    
      if (createdGameSessions.includes(name) || pendingGameSessions.has(name))
      {
        serverInfo = {"ip": "", "port": "", "gameSessionId": "", "status": "Placing"};
        return responseSuccess(res, {serverInfo});
      }

      const isTournamentStarting = await checkOpenGameSessions(client, isTournamentMode);
      if (isTournamentStarting)
      {
        serverInfo = {"ip": "", "port": "", "gameSessionId": "", "status": "Failed"};
        return responseSuccess(res, {serverInfo});
      }

      const inputCreate = { // CreateGameSessionInput
        FleetId: TEST_MODE === 'true' ? gameLiftFleetIdTest : gameLiftFleetId,
        MaximumPlayerSessionCount: 30 //required
      };
      let GameProps = [
        {Key: "mode", Value: req.body.mode},
        {Key: "type", Value: req.body.type}
      ];
      if (isTournamentMode && tournament)
      {
        if (tournament.matchStarted)
        {
          serverInfo = {"ip": "", "port": "", "gameSessionId": "", "status": "Failed"};
          return responseSuccess(res, {serverInfo});
        }
        GameProps.push({Key: "tournamentId", Value: tournamentId});
        GameProps.push({Key: "tourType", Value: tournament.type});
        inputCreate.MaximumPlayerSessionCount = tournament.limit;
      }
      inputCreate.GameProperties = GameProps;

      pendingGameSessions.add(name);
      if (region) inputCreate.Location = region;
      const command = new CreateGameSessionCommand(inputCreate);
      const response = await client.send(command);

      console.log("Creating Session!");
      if (isTournamentMode)
      {
        await Tournament.findOneAndUpdate(
          { _id: tournamentId },
          { $set: { 'matchStarted': true } },
          { returnDocument: 'after' }
        );
        if (tournamentQueue[tournamentId]) delete tournamentQueue[tournamentId];
      }
      createdGameSessions.push(name);
      pendingGameSessions.delete(name);
    
      setTimeout(async () => {
        await updateGameSessionArray();
      }, 150000);

      serverInfo = await searchSessionWithRetry(client, filter, region, req.body.playerName, isTournamentMode, maxRetries = 35, interval = 5000, lobby);
      return responseSuccess(res, {serverInfo});

  } catch (error) {
    console.log(error);
    if (pendingGameSessions.has(name)) pendingGameSessions.delete(name);
    serverInfo = {"ip": "", "port": "", "gameSessionId": "", "status": "Failed"};
    return responseSuccess(res, {serverInfo});
  }
}

async function searchSessionWithRetry(client, filter, region, playerName, isTournamentMode, maxRetries = 40, interval = 5000, lobbyInfo) {
  for (let attempt = 1; attempt <= maxRetries; attempt++) {
    //console.log(`Attempt ${attempt}: Searching for an ACTIVE game session...`);
    const session = await searchSession(client, filter, region, playerName, isTournamentMode, lobbyInfo);

    if (session) {
      console.log("Game session found:", session);
      return session;
    }

    if (attempt < maxRetries) {
      //console.log(`No ACTIVE game session found. Retrying in ${interval / 1000} seconds...`);
      await new Promise(res => setTimeout(res, interval)); // Wait for the specified interval
    }
  }

  console.log("Max retries reached. No ACTIVE game session found.");
  return {"ip": "", "port": "", "gameSessionId": "", "status": "Failed"}; // Return null if no session is found after retries
}

async function searchSession(client, filter, region, playerName, isTournamentMode, lobbyInfo)
{
  const input = { // SearchGameSessionsInput
    FleetId: TEST_MODE === 'true' ? gameLiftFleetIdTest : gameLiftFleetId,
    FilterExpression: filter
  };
  if (isTournamentMode) input.Location = region;
  const command = new SearchGameSessionsCommand(input);
  const response = await client.send(command);
  if (response.GameSessions.length == 0) return null;

  for (const session of response.GameSessions)
  {
    switch (session.Status) {
      case "ACTIVE":
        {
          if (session.PlayerSessionCreationPolicy == "ACCEPT_ALL")
          {
            if (lobbyInfo.isFromLobby)
            {
              // Can probably add a buffer for the openSlots in the future
              const openSlots = session.MaximumPlayerSessionCount - session.CurrentPlayerSessionCount;
              if (openSlots < lobbyInfo.numLobbyPlayers) continue;
            }
            const { playerSessionId, playerId } = await createPlayerSessionWithRetry(client, playerName, session.GameSessionId, { teamId: lobbyInfo.teamId });
            return {
              "status": "Completed",
              "gameSessionId": session.GameSessionId,
              "ip": session.IpAddress,
              "port": session.Port,
              "playerSessionId": playerSessionId,
              "playerId": playerId
            };
          }
        }

      default:
        break;
    }
  }
  return null;
}

async function createPlayerSessionWithRetry(client, playerName, gameSessionId, options = {})
{
  const {
    maxRetries = 5,
    interval = 2000,
    teamId = ""
  } = options;

  for (let attempt = 1; attempt <= maxRetries; attempt++) {
    const playerSession = await createPlayerSession(client, playerName, gameSessionId, teamId);

    if (playerSession) {
      return playerSession;
    }

    if (attempt < maxRetries) {
      console.log("Could not create player session...trying again");
      await new Promise(res => setTimeout(res, interval));
    }
  }
}

async function createPlayerSession(client, playerName, gameSessionId, teamId)
{
  const input = {
    GameSessionId: gameSessionId,
    PlayerId: playerName,
    PlayerData: JSON.stringify({ groupId: teamId })
  };
  const command = new CreatePlayerSessionCommand(input);
  const response = await client.send(command);
  if (response.PlayerSession)
  {
    return {"playerSessionId": response.PlayerSession.PlayerSessionId, "playerId": response.PlayerSession.PlayerId};
  }
  return null;
}

async function updateGameSessionArray()
{
  createdGameSessions.shift();
  return;
}

function addUniqueId(tourId, playerId) {

  if (!tournamentQueue.hasOwnProperty(tourId)) {  // Check if the key does not exist in the object
    tournamentQueue[tourId] = [playerId];
    //console.log("New tournament");
  }
  else
  {
    if (!tournamentQueue[tourId].includes(playerId)) {
      tournamentQueue[tourId].push(playerId);
      //console.log("New player");
    }
  }
}

async function checkOpenGameSessions(client, bIsTournament)
{
  if (!bIsTournament)
  {
    const currentUnixTime = Math.floor(Date.now() / 1000);
    const tourResult = await Tournament.find(
      {
          active: true, // Match only active documents
          timeUnixSeconds: { $gt: currentUnixTime }, // Match where timeUnixSeconds > current time
      },
      {
          timeUnixSeconds: 1, // Include the timeUnixSeconds field in the output
          region: 1, // Include the region field in the output
          _id: 0, // Exclude the _id field
      }
    ).sort({ timeUnixSeconds: 1 }).limit(3);

    if (tourResult[0] && tourResult[0].timeUnixSeconds - currentUnixTime <= 1380)
    {
      const input = {
        FleetId: TEST_MODE === 'true' ? gameLiftFleetIdTest : gameLiftFleetId,
        Location: tourResult[0].region,
        StatusFilter: "ACTIVE" || "ACTIVATING"
      };
      const command = new DescribeGameSessionsCommand(input);
      const response = await client.send(command);
      if (gameLiftNumProcesses - response.GameSessions.length <= 1)
      {
        return true;
      }
    }
  }
  return false;
}

async function onJoinFromLobby(client, gameSessionId, playerName, teamGUID)
{
  const input = {
    GameSessionId: gameSessionId,
    StatusFilter: "ACTIVE" || "ACTIVATING"
  };

  const command = new DescribeGameSessionsCommand(input);
  const response = await client.send(command);
  for (const session of response.GameSessions)
  {
    if (session.Status == "ACTIVE")
    {
      const { playerSessionId, playerId } = await createPlayerSessionWithRetry(client, playerName, session.GameSessionId, { teamId: teamGUID });
      const modeValue = (session.GameProperties.find(item => item.Key === "mode")?.Value) || "solo";
      return {
        "status": "Completed",
        "gameSessionId": session.GameSessionId,
        "ip": session.IpAddress,
        "port": session.Port,
        "playerSessionId": playerSessionId,
        "playerId": playerId,
        "mode": modeValue
      };
    }
  }
  return {
    "status": "Failed",
    "gameSessionId": gameSessionId,
    "ip": "",
    "port": 0,
    "playerSessionId": "",
    "playerId": ""
  };
}

module.exports = {
    getCognitoCredentials,
    startSessionQueue,
    getGameLiftFleetId
}