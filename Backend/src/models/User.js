const mongoose = require('mongoose')
const { userSchema } = require('./schema')
const validator = require('validator');
const fetch = require('node-fetch');

const { removeKeysFromObject } = require('../utils/helperFunctions')
const mongoosePaginate = require('mongoose-paginate-v2')
const { USER_OBJECT_AS_RESPONSE_PROJECTION_JSON } = require('../mongooseOptions/mongooseProjections')
const { CLIENT_GAME_TOKEN_PASS, DISCORD_CLIENT, DISCORD_SECRET } = require('../config');
var jwt = require('jsonwebtoken');
userSchema.plugin(mongoosePaginate)


userSchema.static('createNewUser', async function (userDetails, options = { asResponse: false }) {

	let user = await this.create(userDetails)

	if (options.asResponse) {
		user = user.toObject()
		delete user.createdAt
		delete user.updatedAt
		delete user.__v
	}

	return user
})

// Fetch Requests
userSchema.static('fetchUsersByQuery', async function (findQuery, options = { asResponse: false, paginationOptions: { enabled: false } }) {
    let result = {}

    // Check if pagination is enabled
    if (options.paginationOptions?.enabled) {
        const { page, limit } = options.paginationOptions

        // Perform paginated query using the pagination options
        const paginationResult = await this.paginate(findQuery, {
            page, limit,
            select: options.asResponse ? USER_OBJECT_AS_RESPONSE_PROJECTION_JSON : '', // By passing an empty string ('') as the argument to select, all fields will be selected in the query
            sort: { createdAt: -1 }
        })

        // Extract pagination information from the result
        const { docs, totalDocs, totalPages, page: currentPage, limit: itemsPerPage } = paginationResult

        // Return the paginated result along with additional pagination information
        result = {
            docs,
            totalDocs,
            totalPages,
            currentPage,
            itemsPerPage
        }
    } else {
        // Perform non-paginated query
        result = await this.find(findQuery)
            .select(options.asResponse ? USER_OBJECT_AS_RESPONSE_PROJECTION_JSON : '')
            .sort({ createdAt: -1 })
            .exec()
    }

    return result
})

userSchema.static('fetchUserByUniqueKeys', async function (findQuery, options = { asResponse: false }) {
    let query = this.findOne(findQuery).select(options.asResponse ? USER_OBJECT_AS_RESPONSE_PROJECTION_JSON : '')
    return query.exec()
})

// Update requests
userSchema.static('updateUserByUniqueKeys', async function (findQuery, updation, options = { asResponse: false }) {

    // Cannot allow updation of these keys
    const keysToRemove = ['_id']
    removeKeysFromObject(updation, keysToRemove)

    // let query = this.findByIdAndUpdate(_id, updation, { new: true, runValidators: true })
    // 	.select(options.asResponse ? USER_OBJECT_AS_RESPONSE_PROJECTION_JSON : '')
    // return query.exec()
    // Why it was not working?
    // findByIdAndUpdate is a method provided by Mongoose that performs a direct update in the database without retrieving the document.
    // It avoids the Mongoose middleware, including the validation checks defined in your schema. This means that if you use findByIdAndUpdate,
    // the validation defined in your schema will not be triggered.

    // Get the contract instance using findOne
    const contract = await this.findOne(findQuery)

    // Update the fields
    Object.assign(contract, updation) // Assign the updation to the contract object
    const updatedContract = await contract.save()

    return options.asResponse ? this.findOne(findQuery).select(USER_OBJECT_AS_RESPONSE_PROJECTION_JSON) : updatedContract
})

userSchema.static('deleteContractById', function (contractId, options = { asResponse: false }) {
    let query = this.findByIdAndDelete(contractId).select(options.asResponse ? USER_OBJECT_AS_RESPONSE_PROJECTION_JSON : '')
    return query.exec()
})

userSchema.static('loginAccessData', async function (userId, userName) {
    try {
        const userData = await this.findOneAndUpdate(
            { accountId: `${userId}` },
            { accountName: `${userName}` },
            {
                upsert: true, // Upsert will create a new document if no matching document is found
                projection: { _id: 0, __v: 0}, // Exclude _id and __v
                returnDocument: 'after'
            }
        );
        const {data} = await generateGameToken(userData);
        return {data};
    } catch (error) {
        console.log(error);
    }
})

userSchema.static('updateAccessData', async function (userId, queries) {
    const userData = await updateParams(userId, this, queries);
    const {data} = await generateGameToken(userData);
    return {data};
})

// Not being used because done in middleware
userSchema.static('verifyGameToken', async function (token) {

    try {
        const userData = await verifyToken(token)
        return userData ? userData : null;
    } catch (error) {
        console.log(error);
        return null;
    }

})

userSchema.static('adminUpdateAccessData', async function (queries) {

    const userData = await updateParams(queries.userId, this, queries, true);
    const {data} = await generateGameToken(userData);
    return {data};
})

async function updateParams(userId, accessSchema, queries, bisAdmin = false)
{
    // queries should contain boughtGame
    const updateObject = {
        ...(queries.boughtGame && bisAdmin && { 'accessInfo.boughtGame': queries.boughtGame }),
        ...(queries.reviewCompleted && bisAdmin && { 'accessInfo.reviewCompleted': queries.reviewCompleted }),
        ...(queries.passedKYC && bisAdmin && { 'accessInfo.passedKYC': queries.passedKYC }),
        ...(queries.postedVideo && bisAdmin && { 'accessInfo.postedVideo': queries.postedVideo }),
        ...(queries.publickey && { walletAddress: queries.publickey })
      };

    const userData = await accessSchema.findOneAndUpdate(
        { accountId: userId },
        updateObject,
        { returnDocument: 'after'}  // Upsert will create a new document if no matching document is found
    );

    return userData;
}

async function generateGameToken(userData)
{
    try 
    {
        const data = userData.toObject();
        const token = await jwt.sign(data, CLIENT_GAME_TOKEN_PASS, { expiresIn: '2h' });
        data.token = token;
        return {data};
    } 
    catch (error) 
    {
        console.log(error);
    }
}

async function verifyToken(token)
{
    try 
    {
        const jwtVerified  = await jwt.verify(token, CLIENT_GAME_TOKEN_PASS);
        return jwtVerified;
    } 
    catch (error) 
    {
        return null;
    }
}

userSchema.static('reportAPlayer', async function (reporter, body)
{
    try {
        const result = await this.findOneAndUpdate(
            { accountId: body.reportedId },
            {
                $set: {
                    [`accessInfo.flaggedInfo.${reporter}`]: body.description
                }
            }
        );

        return result;
    } catch (error) {
        console.log(error);
        return null;
    }
})

userSchema.static('setEmail', async function (userId, body)
{
    try {
        if (validator.isEmail(body.email))
        {
            const result = await this.findOneAndUpdate(
                { accountId: userId },
                {
                    $set: {
                        [`accessInfo.email`]: body.email
                    }
                }
            );
            return result;
        }
        return null;

    } catch (error) {
        console.log(error);
        return null;
    }
})

userSchema.static('checkEmail', async function (body)
{
    try {
        const user = await User.findOne({
            'accessInfo.email': body.accounts.email
        });
        return user;

    } catch (error) {
        console.log(error);
        return null;
    }
})

userSchema.static('setDiscordId', async function (queryParams)
{
    try {
        const code = queryParams.code;
        const state = queryParams.state;  // This is your client game JWT
        const tokenData = await jwt.verify(state, CLIENT_GAME_TOKEN_PASS);

        if (!code || !state || !tokenData) {
            return null;
        }

        const accessToken = await handleDiscordCallback(code, state);
        const userData = await getDiscordUserId(accessToken);

        // Now update MongoDB using the state as playerGameId
        const userDoc = await this.updateOne({ accountId: tokenData.accountId }, { discordId: userData.userId, discordName: userData.userName });
        return userDoc;

    } catch (error) {
        console.log(error);
        return null;
    }
})

async function getDiscordUserId(accessToken) {
  const userUrl = 'https://discord.com/api/users/@me';
  const response = await fetch(userUrl, {
    headers: { Authorization: `Bearer ${accessToken}` },
  });

  if (!response.ok) {
    throw new Error('Failed to fetch user info');
  }

  const userData = await response.json();
  //console.log("Discord USER data: ", userData);
  return {'userId': userData.id, 'userName': userData.global_name};  // This is the Discord user ID (a string, e.g., '123456789012345678')
}

async function handleDiscordCallback(code, state) {
  const tokenUrl = 'https://discord.com/api/oauth2/token';
  const redirectUrl = 'https://apis.disruptive-labs.io/user/discordRedirect';
  const params = new URLSearchParams({
    client_id: DISCORD_CLIENT,
    client_secret: DISCORD_SECRET,
    grant_type: 'authorization_code',
    code: code,
    redirect_uri: redirectUrl
  });

  const response = await fetch(tokenUrl, {
    method: 'POST',
    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
    body: params,
  });

  if (!response.ok) {
    throw new Error('Failed to get access token');
  }

  const data = await response.json();
  return data.access_token;
}

const User = mongoose.model('user', userSchema)
module.exports = User
