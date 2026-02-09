const ERRORS = require('./errorTypes')

const _send = (res, statusCode, success, data, error) => { 
    let response = {success}
    response.result = error ? error : data
    res.status(statusCode).json(response)
}

const responseSuccess = (res, data) => {
    _send(res, 200, true, data, null)
}

const responseBadRequest = (res, details) => {
    const error = {
        ...ERRORS.BAD_REQUEST,
        details
    }
    _send(res, ERRORS.BAD_REQUEST.STATUS, false, null, error)
}

const responseUnauthorized = (res, details) => {
    const error = {
		...ERRORS.CLIENT_UNAUTHORIZED,
		details,
	}
	_send(res, ERRORS.CLIENT_UNAUTHORIZED.STATUS, false, null, error)
}

const responseServerSideError = (res, details) => {
    const error = {
		...ERRORS.SERVER_SIDE_ERROR,
		details,
	}
	_send(res, ERRORS.SERVER_SIDE_ERROR.STATUS, false, null, error)
}

const responses = {
	responseSuccess,
	responseBadRequest,
	responseUnauthorized,
	responseServerSideError,
}

module.exports = responses