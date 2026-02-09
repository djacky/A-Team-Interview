const mongoose = require('mongoose')
const { adminSchema } = require('./schema')
var jwt = require('jsonwebtoken');

const passwordHash = require('../helpers/password.Hash')

const { removeKeysFromObject, getSecret } = require('../utils/helperFunctions')
const { AWS_SECRET } = require('../config')
const CONSTANTS = require('../constants')
const { ADMIN_OBJECT_AS_RESPONSE_PROJECTION_JSON } = require('../mongooseOptions/mongooseProjections')

adminSchema.static('createNewAdmin', async function (adminDetails, options = { asResponse: false }) {

	let admin = await this.create(adminDetails)

	if (options.asResponse) {
		admin = admin.toObject()
		delete admin.createdAt
		delete admin.updatedAt
		delete admin.__v
	}

	return admin
})

// Fetch Requests 
adminSchema.static('fetchAdminByUniqueKeys', async function (findQuery, options = { asResponse: false }, loginKey) {

	const { ADMIN_LOGIN_KEY, ADMIN_SIGN_KEY } = await getSecret(AWS_SECRET);
	if (ADMIN_LOGIN_KEY !== loginKey) return null;

	const payload = {"userId": findQuery.accountId, "timestamp": Date.now(), "role": CONSTANTS.USER_ROLE_ADMIN};
	const adminToken = await generateToken(payload, ADMIN_SIGN_KEY);
	const updateObject = { "jwt": adminToken };
	let query = await this.findOneAndUpdate(
		findQuery,
		updateObject,
		{ new: true }
	).select(options.asResponse ? ADMIN_OBJECT_AS_RESPONSE_PROJECTION_JSON : '')
	
	//let query = this.findOne(findQuery).select(options.asResponse ? ADMIN_OBJECT_AS_RESPONSE_PROJECTION_JSON : '')
	return query
})

// Password related requests 
adminSchema.static('setPassword', async function (adminId, password, options = { asResponse: false }) {

	let admin = await this.findByIdAndUpdate(
		adminId,
		{ password, forgotPasswordNonce: null },
		{ new: true, runValidators: true })
		.exec()

	if (options.asResponse) {
		admin = admin.toObject()
		delete admin.password
		delete admin.emailVerificationCode
		delete admin.forgotPasswordNonce
		delete admin.createdAt
		delete admin.updatedAt
		delete admin.__v
	}

	return admin
})

adminSchema.method('matchPassword', function (password) {

	if (password && password.length && this.password) {
		return passwordHash.match(password, this.password)
	}
	return false
})

// Update requests
adminSchema.static('updateAdminByUniqueKeys', async function (findQuery, updation, options = { asResponse: false }) {

	// Cannot allow updation of these keys 	
	const keysToRemove = ['_id', 'role', 'email', 'password']
	removeKeysFromObject(updation, keysToRemove)

	// let query = this.findOneAndUpdate(findQuery, updation, { new: true, runValidators: true })
	// 	.select(options.asResponse ? ADMIN_OBJECT_AS_RESPONSE_PROJECTION_JSON : '')
	// return query.exec()
	// Why it was not working?
	// findByIdAndUpdate is a method provided by Mongoose that performs a direct update in the database without retrieving the document. 
	// It avoids the Mongoose middleware, including the validation checks defined in your schema. This means that if you use findByIdAndUpdate, 
	// the validation defined in your schema will not be triggered.

	// Get the admin instance using findOne
	const admin = await this.findOne(findQuery)

	// Update the fields
	Object.assign(admin, updation) // Assign the updation to the admin object
	const updatedAdmin = await admin.save()

	return options.asResponse ? this.findOne(findQuery).select(ADMIN_OBJECT_AS_RESPONSE_PROJECTION_JSON) : updatedAdmin
})

async function generateToken(data, signKey)
{
    try 
    {
        const jwtToken = await jwt.sign(data, signKey, { expiresIn: '2h' });
        return jwtToken;
    } 
    catch (error) 
    {
        console.log(error);
    }
}

const Admin = mongoose.model('admin', adminSchema)
module.exports = Admin