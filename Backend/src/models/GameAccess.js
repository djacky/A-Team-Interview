const mongoose = require('mongoose');
const { gameAccessSchema } = require('./schema');
var jwt = require('jsonwebtoken');
const { CLIENT_GAME_TOKEN_PASS } = require('../config');

gameAccessSchema.static('loginAccessData', async function (userId) {
    const userData = await this.findOneAndUpdate(
        { userId: `${userId}` },
        {},
        {
            new: true,
            upsert: true, // Upsert will create a new document if no matching document is found
            projection: { _id: 0, __v: 0} // Exclude _id and __v
        }
    );
    const token = await generateGameToken(userData);
    return token;
})

gameAccessSchema.static('updateAccessData', async function (queries) {
    
    const userData = await updateParams(this, queries);
    const token = await generateGameToken(userData);
    return token;
})

gameAccessSchema.static('verifyGameToken', async function (token) {

    const userData = await verifyToken(token)
    return userData ? userData : null;
})

gameAccessSchema.static('adminUpdateAccessData', async function (queries) {

    const userData = await updateParams(this, queries, true);
    return userData;
})

async function updateParams(accessSchema, queries, bisAdmin = false)
{
    // queries should contain boughtGame
    const updateObject = {
        ...(queries.boughtGame && bisAdmin && { boughtGame: queries.boughtGame }),
        ...(queries.reviewCompleted && bisAdmin && { reviewCompleted: queries.reviewCompleted }),
        ...(queries.passedKYC && bisAdmin && { passedKYC: queries.passedKYC }),
        ...(queries.postedVideo && bisAdmin && { postedVideo: queries.postedVideo }),
        ...(queries.publicWallet && { publicWallet: queries.publicWallet })
      };

    const userData = await accessSchema.findOneAndUpdate(
        { userId: queries.userId },
        updateObject,
        { new: true, upsert: true }  // Upsert will create a new document if no matching document is found
    );

    return userData;
}

async function generateGameToken(data)
{
    try 
    {
        const jwtToken = await jwt.sign(data.toObject(), CLIENT_GAME_TOKEN_PASS, { expiresIn: '2h' });
        return jwtToken;
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


const GameAccess = mongoose.model('gameAccess', gameAccessSchema)
module.exports = GameAccess