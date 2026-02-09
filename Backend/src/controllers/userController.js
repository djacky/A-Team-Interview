const userHelper = require('../helpers/user.helper')
const fs = require('fs')
const path = require('path');

const OpenAI = require('openai');
const { v4: uuidv4 } = require('uuid');

const { responseSuccess, responseBadRequest, responseUnauthorized, responseServerSideError } = require('../utils/responseTypes')


const { filterEmptyKeys, getWalletAddressFromPrivateKey, generateUpdateObject, getEnabledPaginationOption } = require('../utils/helperFunctions')

const ERRORS = require('../utils/errorTypes')
const CONSTANTS = require('../constants')

const fetch = require('node-fetch')

const openai = new OpenAI({
  apiKey: CONSTANTS.OPENAIKEY
});

const sessions = new Map();

setInterval(() => {
  const now = Date.now();
  const maxAge = 24 * 60 * 60 * 1000; // 24 hours
  
  for (const [sessionId, session] of sessions.entries()) {
    if (now - session.createdAt > maxAge) {
      sessions.delete(sessionId);
    }
  }
}, 60 * 60 * 1000);


const signUp = async (req, res) => {
    try {
        const walletAddress = req.tokenData.walletAddress;
        const options = { asResponse: true, paginationOptions: { enabled: false }, populate: false }
        //const response = await fetch(CONSTANTS.AUTH_INFO_URL, {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: `token=${req.headers.token}`})
        //const data = await response.json()
        const userDetails = filterEmptyKeys({ walletAddress, accountId: req.tokenData.userId })
        let user;
        user = await userHelper.getUserByQuery({accountId: req.tokenData.userId}, options)
        if(user){
            return responseBadRequest(res, ERRORS.ACCOUNT_ALREADY_EXISTS)
        }
        user = await userHelper.createNewUser(userDetails, options)
        return responseSuccess(res, {user })
    } catch (error) {
        if (ERRORS[error.message]) {
            return responseBadRequest(res, ERRORS[error.message])
        }
        return responseServerSideError(res, {MESSAGE: error.message})
    }
}

const signIn = async (req, res) => {
    try {
        const { walletAddress} = req.body
        const options = { asResponse: true, paginationOptions: { enabled: false }, populate: false }

        const publicKeyPath = path.resolve(__dirname, '../../public.pem');
        const publicKey = fs.readFileSync(publicKeyPath, 'utf8');
        
        let user
        user = await userHelper.getUserByQuery({accountId: req.tokenData.account_id}, options)
        if(!user){
            return responseBadRequest(res, ERRORS.NO_USER_FOUND)
        }else if (user.isBanned == true){
            return responseBadRequest(res,  {
                ...ERRORS.BANNED_USER,
                REASON: `${user.banReason}`
            } )
        }else if (user.role == CONSTANTS.USER_ROLE_USER && user.walletAddress != walletAddress){
            return responseBadRequest(res, ERRORS.WALLET_ADDRESS_NOT_REGISTERED)
        }
        return responseSuccess(res, {user, publicKey })
    } catch (error) {
        if (ERRORS[error.message]) {
            return responseBadRequest(res, ERRORS[error.message])
        }
        return responseServerSideError(res, {MESSAGE: error.message})
    }
}

const getWalletAddress = async (req, res) => {
    try {
        const { privateKey} = req.body
        const address = getWalletAddressFromPrivateKey(privateKey)
        return responseSuccess(res, {address })
    } catch (error) {
        if (ERRORS[error.message]) {
            return responseBadRequest(res, ERRORS[error.message])
        }
        return responseServerSideError(res, {MESSAGE: error.message})
    }
}

const banUserByAdmin = async (req, res) => {
    try {
        const { userId } = req.params
        const fieldsToUpdate = [
            "isBanned",
            "banReason"
        ]
        let userUpdates = generateUpdateObject(fieldsToUpdate, req.body)
        const options = { asResponse: true, paginationOptions: { enabled: false }, populate: false }
        const user = await userHelper.updateUserDetailsById(userId, userUpdates, options)
        return responseSuccess(res, { user })
    } catch (error) {
        console.error(error)
        if (ERRORS[error.message]) {
            return responseBadRequest(res, ERRORS[error.message])
        }
        return responseServerSideError(res, {MESSAGE: error.message})
    }
}

const getUsersForAdmin = async (req, res) => {
    try {
        const { page, limit, isBanned } = req.query
		const paginationOptions = getEnabledPaginationOption(page, limit)

        const options = { asResponse: true, paginationOptions, populate: false }
        let query = {}
        if (isBanned) {
            query.isBanned = isBanned;
        }
        const users = await userHelper.fetchUsersByQuery(query, options)
        return responseSuccess(res, { users })
    } catch (error) {
        console.log(error)
        if (ERRORS[error.message]) {
            return responseBadRequest(res, ERRORS[error.message])
        }
        return responseServerSideError(res, error.message)
    }
}

const reportPlayer = async (req, res) => {
    try {
        const users = await userHelper.reportPlayer(req.tokenData.account_id, req.body)
        return responseSuccess(res, { users })
    } catch (error) {
        console.log(error)
        if (ERRORS[error.message]) {
            return responseBadRequest(res, ERRORS[error.message])
        }
        return responseServerSideError(res, error.message)
    }
}

const setEmail = async (req, res) => {
    try {
        const userDoc = await userHelper.setEmail(req.tokenData.accountId, req.body);
        return userDoc ? responseSuccess(res, { userDoc }) : responseServerSideError(res, "Error with setting email");
    } catch (error) {
        console.log(error);
        if (ERRORS[error.message]) {
            return responseBadRequest(res, ERRORS[error.message]);
        }
        return responseServerSideError(res, error.message);
    }
}

const checkEmail = async (req, res) => {
    try {
        console.log(req.body);
        const userDoc = await userHelper.checkEmail(req.body);
        return userDoc ? responseSuccess(res, { userDoc }) : 
            res.status(400).send({
                message: `We could not find a user with email ${req.body.accounts.email}. Make sure to enter your Zealy email as described in the quest.`
            });
    } catch (error) {
        console.log(error);
        return res.status(400).send({message: error.message});
    }
}

const discordRedirect = async (req, res) => {    
    try {
        const userDoc = await userHelper.setDiscordId(req.query);
        const successMessage = "Successfully linked Discord account!";
        return userDoc ? responseSuccess(res, { successMessage }) : responseServerSideError(res, "Error with setting email");
    } catch (error) {
        console.log(error);
        if (ERRORS[error.message]) {
            return responseBadRequest(res, ERRORS[error.message]);
        }
        return responseServerSideError(res, error.message);
    }
    
}

const aiSessionId = async (req, res) => {    
    const sessionId = uuidv4();
    sessions.set(sessionId, {
    createdAt: Date.now(),
    lastResponseId: null,
    messageCount: 0
    });

    res.json({ sessionId });
}

const askAiAboutMe = async (req, res) => {    
  try {
    const { sessionId, message } = req.body;

    if (!sessionId) {
      return res.status(400).json({ 
        error: 'Session ID is required.' 
      });
    }

    if (!message || typeof message !== 'string') {
      return res.status(400).json({ 
        error: 'Message is required and must be a string.' 
      });
    }

    // Get or create session
    let session = sessions.get(sessionId);
    if (!session) {
      session = {
        createdAt: Date.now(),
        lastResponseId: null,
        messageCount: 0
      };
      sessions.set(sessionId, session);
    }

    // Prepare request parameters
    const requestParams = {
      model: 'gpt-4o', // Can also use 'gpt-4o-mini' for lower cost
      input: message,
      instructions: CONSTANTS.ABOUTMEINSTRUCTIONS,
      max_output_tokens: 500,
      temperature: 0.7
    };

    // If this is a continuation of a conversation, include previous response ID
    if (session.lastResponseId) {
      requestParams.previous_response_id = session.lastResponseId;
    }

    // Call OpenAI Responses API
    const response = await openai.responses.create(requestParams);

    // Update session with new response ID
    session.lastResponseId = response.id;
    session.messageCount += 1;
    sessions.set(sessionId, session);

    // Extract the text response
    const assistantMessage = response.output_text || 
      response.output
        .filter(item => item.type === 'message' && item.role === 'assistant')
        .map(item => item.content
          .filter(c => c.type === 'output_text')
          .map(c => c.text)
          .join('\n')
        )
        .join('\n');

    // Return the response
    res.json({
      message: assistantMessage,
      responseId: response.id,
      sessionId: sessionId,
      messageCount: session.messageCount
    });

  } catch (error) {
    console.error('Error calling OpenAI Responses API:', error);
    
    // Handle specific OpenAI errors
    if (error.status === 401) {
      return res.status(500).json({ 
        error: 'API key is invalid or missing.' 
      });
    }
    
    if (error.status === 429) {
      return res.status(429).json({ 
        error: 'Rate limit exceeded. Please try again later.' 
      });
    }

    res.status(500).json({ 
      error: 'Failed to process your request. Please try again.' 
    });
  }
}

module.exports = {
    signUp,
    signIn,
    getWalletAddress,
    banUserByAdmin,
    getUsersForAdmin,
    reportPlayer,
    setEmail,
    checkEmail,
    discordRedirect,
    askAiAboutMe,
    aiSessionId
}