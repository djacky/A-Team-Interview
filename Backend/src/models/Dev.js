const mongoose = require('mongoose')
const { devSchema } = require('./schema')


devSchema.static('isDeleteGame', async function () {

	let devResult = await this.findOne();
    if (devResult)
    {
        return devResult.deleteGame;
    }
    return false;
})

devSchema.static('confirmDelete', async function () {

	await this.updateOne({}, { deleteGame: false });
    return true;
})


const Dev = mongoose.model('dev', devSchema)
module.exports = Dev
