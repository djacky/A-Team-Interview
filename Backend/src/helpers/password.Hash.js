const bcryptJS = require('bcryptjs')

exports.hash = password => bcryptJS.hashSync(password, bcryptJS.genSaltSync(10))
exports.match = (password, hash) => bcryptJS.compare(password, hash)
