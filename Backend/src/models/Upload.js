const mongoose = require('mongoose')
const { uploadSchema } = require('./schema')


uploadSchema.static('logScanAndBan', async function (scanDetails, userModel) {

	let uploadResult = await this.create(scanDetails);
    await userModel.updateOne(
        { accountId: scanDetails.accountId },
        { 
            $set: { 
                'isBanned': true,
                'banReason': "Suspicious files were uploaded into the server when you submitted your asset"
            },
            $inc: {
                'banStrikes': 1
            }
        }
      );

    uploadResult = uploadResult.toObject()
    delete uploadResult.createdAt
    delete uploadResult.updatedAt
    delete uploadResult.__v

	return uploadResult;
})


const Upload = mongoose.model('upload', uploadSchema)
module.exports = Upload
