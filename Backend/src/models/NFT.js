const mongoose = require('mongoose')
const { nftSchema } = require('./schema')

const { removeKeysFromObject } = require('../utils/helperFunctions')
const { NFT_AS_RESPONSE_PROJECTION_JSON } = require('../mongooseOptions/mongooseProjections')

const CONSTANTS = require('../constants')

const mongoosePaginate = require('mongoose-paginate-v2')
nftSchema.plugin(mongoosePaginate)

nftSchema.static('createNewNFTRequest', async function (nftDetails, options = { asResponse: false }) {
    let nft = await this.create(nftDetails)

    if (options.asResponse) {
        nft = nft.toObject()
        delete nft.createdAt
        delete nft.updatedAt
        delete nft.__v
    }

    return nft
})


// Fetch Requests
nftSchema.static('fetchNFTsByQuery', async function (findQuery, options = { asResponse: false, paginationOptions: { enabled: false } }) {
    let result = {}

    // Check if pagination is enabled
    if (options.paginationOptions?.enabled) {
        const { page, limit } = options.paginationOptions

        // Perform paginated query using the pagination options
        const paginationResult = await this.paginate(findQuery, {
            page, limit,
            select: options.asResponse ? NFT_AS_RESPONSE_PROJECTION_JSON : '', // By passing an empty string ('') as the argument to select, all fields will be selected in the query
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
            .select(options.asResponse ? NFT_AS_RESPONSE_PROJECTION_JSON : '')
            .sort({ createdAt: -1 })
            .exec()
    }
    return result
})

nftSchema.static('fetchNFTByUniqueKeys', async function (findQuery, options = { asResponse: false }) {
    let query = this.findOne(findQuery).select(options.asResponse ? NFT_AS_RESPONSE_PROJECTION_JSON : '')
    return query.exec()
})

// Update requests
nftSchema.static('updateNFTByUniqueKeys', async function (findQuery, updation, options = { asResponse: false }) {

    // Cannot allow updation of these keys
    const keysToRemove = ['_id']
    removeKeysFromObject(updation, keysToRemove)

    // let query = this.findByIdAndUpdate(_id, updation, { new: true, runValidators: true })
    // 	.select(options.asResponse ? NFT_AS_RESPONSE_PROJECTION_JSON : '')
    // return query.exec()
    // Why it was not working?
    // findByIdAndUpdate is a method provided by Mongoose that performs a direct update in the database without retrieving the document.
    // It avoids the Mongoose middleware, including the validation checks defined in your schema. This means that if you use findByIdAndUpdate,
    // the validation defined in your schema will not be triggered.

    // Get the nft instance using findOne
    const nft = await this.findOne(findQuery)

    // Update the fields
    Object.assign(nft, updation) // Assign the updation to the nft object
    const updatedNFT = await nft.save()

    return options.asResponse ? this.findOne(findQuery).select(NFT_AS_RESPONSE_PROJECTION_JSON) : updatedNFT
})

nftSchema.static('deleteNFTById', function (nftId, options = { asResponse: false }) {
    let query = this.findByIdAndDelete(nftId).select(options.asResponse ? NFT_AS_RESPONSE_PROJECTION_JSON : '')
    return query.exec()
})

nftSchema.static('checkNFTName', async function (weaponName) {

    const exists = await this.exists({name: weaponName});
    return !!exists;
})

const NFT = mongoose.model('NFT', nftSchema)
module.exports = NFT
