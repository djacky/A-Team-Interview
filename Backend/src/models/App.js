const mongoose = require('mongoose')
const { appSchema } = require('./schema')

const { removeKeysFromObject } = require('../utils/helperFunctions')
const mongoosePaginate = require('mongoose-paginate-v2')
const { USER_OBJECT_AS_RESPONSE_PROJECTION_JSON } = require('../mongooseOptions/mongooseProjections')
appSchema.plugin(mongoosePaginate)


appSchema.static('startApp', async function () {
    if (!await this.findOne({}))
    {
        const app = await this.create({});
    }
})

appSchema.static('createNewApp', async function (appDetails, options = { asResponse: false }) {

	let app = await this.create(appDetails)

	if (options.asResponse) {
		app = app.toObject()
		delete app.createdAt
		delete app.updatedAt
		delete app.__v
	}

	return app
})

// Fetch Requests
appSchema.static('fetchAppsByQuery', async function (findQuery, options = { asResponse: false, paginationOptions: { enabled: false } }) {
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

appSchema.static('fetchAppByUniqueKeys', async function (findQuery, options = { asResponse: false }) {
    let query = this.findOne(findQuery).select(options.asResponse ? USER_OBJECT_AS_RESPONSE_PROJECTION_JSON : '')
    return query.exec()
})

// Update requests
appSchema.static('updateAppByUniqueKeys', async function (findQuery, updation, options = { asResponse: false }) {

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

    // Get the app instance using findOne
    const app = await this.findOne(findQuery)

    // Update the fields
    Object.assign(app, updation) // Assign the updation to the app object
    const updatedApp = await app.save()

    return options.asResponse ? this.findOne(findQuery).select(USER_OBJECT_AS_RESPONSE_PROJECTION_JSON) : updatedApp
})

const App = mongoose.model('app', appSchema)
module.exports = App
