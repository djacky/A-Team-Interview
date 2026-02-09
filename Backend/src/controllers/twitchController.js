const { responseSuccess, responseBadRequest, responseUnauthorized, responseServerSideError } = require('../utils/responseTypes')



const test = async (req, res) => {

  return responseSuccess(res, {authData});
}



module.exports = {
  test
}