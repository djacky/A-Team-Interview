const appHelper = require('../helpers/app.helper')

const { responseSuccess, responseBadRequest, responseUnauthorized, responseServerSideError } = require('../utils/responseTypes')


const { generateUpdateObject } = require('../utils/helperFunctions')

const ERRORS = require('../utils/errorTypes')

const updateAppStatusByAdmin = async (req, res) => {
    try {
        const fieldsToUpdate = [
            "pause",
            "pauseReason"
        ]
        let appUpdates = generateUpdateObject(fieldsToUpdate, req.body)
        const options = { asResponse: true, paginationOptions: { enabled: false }, populate: false }
        const appsConfig = await appHelper.getApp(options)
        const appConfig = await appHelper.updateAppDetailsById(appsConfig[0]._id, appUpdates, options)
        return responseSuccess(res, { appConfig })
    } catch (error) {
        console.error(error)
        if (ERRORS[error.message]) {
            return responseBadRequest(res, ERRORS[error.message])
        }
        return responseServerSideError(res, {MESSAGE: error.message})
    }
}

const getAppStatus = async (req, res) => {
    try {
        const options = { asResponse: true, paginationOptions: { enabled: false }, populate: false }
        const appsConfig = await appHelper.getApp(options)
        return responseSuccess(res, { appsConfig: appsConfig[0]})
    } catch (error) {
        console.log(error)
        if (ERRORS[error.message]) {
            return responseBadRequest(res, ERRORS[error.message])
        }
        return responseServerSideError(res, error.message)
    }
}

module.exports = {
    updateAppStatusByAdmin,
    getAppStatus
}