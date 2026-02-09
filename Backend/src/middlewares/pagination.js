const CONSTANTS = require('../constants')

const setPagination = (req, res, next) => {
    const { page, limit } = req.query

    if (!page) {
        req.query.page = CONSTANTS.PAGE_DEFAULT
    }
    if (!limit) {
        req.query.limit = CONSTANTS.LIMIT_DEFAULT
    }
    next()
}

module.exports = {
    setPagination
}
