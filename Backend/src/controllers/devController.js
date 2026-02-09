const devHelper = require('../helpers/dev.helper')

const { responseSuccess, responseBadRequest, responseUnauthorized, responseServerSideError } = require('../utils/responseTypes')
const ERRORS = require('../utils/errorTypes')

const getIsDelete = async (req, res) => {
    try {
        const toDelete = await devHelper.getIsDelete();
        return responseSuccess(res, { toDelete });
    } catch (error) {
        if (ERRORS[error.message]) {
            return responseBadRequest(res, ERRORS[error.message])
        }
        return responseServerSideError(res, error)
    }
}

const confirmDelete = async (req, res) => {
    try {
        const confirm = await devHelper.confirmDelete();
        return responseSuccess(res, { confirm });
    } catch (error) {
        if (ERRORS[error.message]) {
            return responseBadRequest(res, ERRORS[error.message])
        }
        return responseServerSideError(res, error)
    }
}


module.exports = {
    getIsDelete,
    confirmDelete
}
