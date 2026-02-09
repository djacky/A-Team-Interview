const mongoose = require('mongoose')
const { auctionSchema } = require('./schema')

const { removeKeysFromObject } = require('../utils/helperFunctions')
const { AUCTION_AS_RESPONSE_PROJECTION_JSON, NFT_AS_RESPONSE_PROJECTION_JSON_POPULATE, USER_OBJECT_AS_RESPONSE_PROJECTION_JSON_POPULATE } = require('../mongooseOptions/mongooseProjections')
const { AUCTION_POPULATE_ONLY_NFT, AUCTION_POPULATE_ONLY_USER } = require('../mongooseOptions/mongoosePopulate')


const mongoosePaginate = require('mongoose-paginate-v2')
auctionSchema.plugin(mongoosePaginate)

auctionSchema.static('createNewAuction', async function (auctionDetails, options = { asResponse: false }) {
    let auction = await this.create(auctionDetails)

    if (options.asResponse) {
        auction = auction.toObject()
        delete auction.createdAt
        delete auction.updatedAt
        delete auction.__v
    }

    return auction
})


// Fetch Requests
auctionSchema.static('fetchAuctionsByQuery', async function (findQuery, options = { asResponse: false, paginationOptions: { enabled: false } }) {
    let result = {}

    // Check if pagination is enabled
    if (options.paginationOptions?.enabled) {
        const { page, limit } = options.paginationOptions

        // Perform paginated query using the pagination options
        const paginationResult = await this.paginate(findQuery, {
            page, limit,
            select: options.asResponse ? AUCTION_AS_RESPONSE_PROJECTION_JSON : '', // By passing an empty string ('') as the argument to select, all fields will be selected in the query
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
            .select(options.asResponse ? AUCTION_AS_RESPONSE_PROJECTION_JSON : '')
            .sort({ createdAt: -1 })
            .exec()
    }
    return result
})

auctionSchema.static('fetchAuctionByUniqueKeys', async function (findQuery, options = { asResponse: false, populate: false }) {
    let query = this.findOne(findQuery)
		.select(options.asResponse ? AUCTION_AS_RESPONSE_PROJECTION_JSON : '')

	if (options.populate) {
		const populateConfig = []

		// Add populate configurations for each model you want to populate
		if (options.populate.nft) {
			populateConfig.push({
				path: AUCTION_POPULATE_ONLY_NFT,
				select: options.asResponse ? NFT_AS_RESPONSE_PROJECTION_JSON_POPULATE : ''
			})
		}

		if (options.populate.seller) {
			populateConfig.push({
				path: AUCTION_POPULATE_ONLY_USER,
				select: options.asResponse ? USER_OBJECT_AS_RESPONSE_PROJECTION_JSON_POPULATE : ''
			})
		}

        if (options.populate.highestBidder) {
			populateConfig.push({
				path: AUCTION_POPULATE_ONLY_USER,
				select: options.asResponse ? USER_OBJECT_AS_RESPONSE_PROJECTION_JSON_POPULATE : ''
			})
		}

		// Apply the populated configurations to the query
		query = query.populate(populateConfig)
	}

	return query.exec()
})

// Update requests
auctionSchema.static('updateAuctionByUniqueKeys', async function (findQuery, updation, options = { asResponse: false }) {

    // Cannot allow updation of these keys
    const keysToRemove = ['_id']
    removeKeysFromObject(updation, keysToRemove)

    // let query = this.findByIdAndUpdate(_id, updation, { new: true, runValidators: true })
    // 	.select(options.asResponse ? AUCTION_AS_RESPONSE_PROJECTION_JSON : '')
    // return query.exec()
    // Why it was not working?
    // findByIdAndUpdate is a method provided by Mongoose that performs a direct update in the database without retrieving the document.
    // It avoids the Mongoose middleware, including the validation checks defined in your schema. This means that if you use findByIdAndUpdate,
    // the validation defined in your schema will not be triggered.

    // Get the auction instance using findOne
    const auction = await this.findOne(findQuery)

    // Update the fields
    Object.assign(auction, updation) // Assign the updation to the auction object
    const updatedAuction = await auction.save()

    return options.asResponse ? this.findOne(findQuery).select(AUCTION_AS_RESPONSE_PROJECTION_JSON) : updatedAuction
})

auctionSchema.static('deleteAuctionById', function (auctionId, options = { asResponse: false }) {
    let query = this.findByIdAndDelete(auctionId).select(options.asResponse ? AUCTION_AS_RESPONSE_PROJECTION_JSON : '')
    return query.exec()
})

const Auction = mongoose.model('Auction', auctionSchema)
module.exports = Auction
