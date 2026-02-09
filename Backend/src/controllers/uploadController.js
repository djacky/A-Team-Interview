const uploadService = require('../services/upload.service')
const uploadHelper = require('../helpers/upload.helper')

const { responseSuccess, responseBadRequest, responseUnauthorized, responseServerSideError } = require('../utils/responseTypes')
const ERRORS = require('../utils/errorTypes')

const uploadFileToIPFS = async (req, res) => {
    try {
        // const { fileBase64, fileName } = req.body
        const file = req.file
        const response = await uploadService.uploadFileToIPFS(file)

        return responseSuccess(res, { imgLink: response })
    } catch (error) {
        if (ERRORS[error.message]) {
            return responseBadRequest(res, ERRORS[error.message])
        }
        return responseServerSideError(res, error)
    }
}

const uploadJSONToIPFS = async (req, res) => {
    try {
        const { metadata, fileName } = req.body

        const response = await uploadService.uploadJsonToIPFS(metadata, fileName)

        return responseSuccess(res, { metadataLink: response.metadataLink, ipfsHash: response.ipfsHash })
    } catch (error) {
        if (ERRORS[error.message]) {
            return responseBadRequest(res, ERRORS[error.message])
        }
        return responseServerSideError(res, error)
    }
}

const uploadFileToS3 = async (req, res) => {
    try {
        const file = req.file
        const response = await uploadService.uploadFileToS3(file)
        console.log(response)
        return responseSuccess(res, { fileLink: response })
    } catch (error) {
        if (ERRORS[error.message]) {
            return responseBadRequest(res, ERRORS[error.message])
        }
        return responseServerSideError(res, error)
    }
}

const scanFile = async (req, res) => {
    try {
        
        //await uploadHelper.processGLB(req);
        //return responseSuccess(res, { });
        const scanResult = await uploadHelper.scanFileWithVirusTotal(req);
        return scanResult ? responseSuccess(res, { scanResult }) : responseBadRequest(res, ERRORS.FAILED_FILE_SCAN);
    } catch (error) {
        if (ERRORS[error.message]) {
            return responseBadRequest(res, ERRORS[error.message])
        }
        return responseServerSideError(res, error)
    }
}

const deleteFileFromS3 = async (req, res) => {
    try {
        const { url } = req.body
        await uploadService.deleteFileFromS3(url)
        return responseSuccess(res, { response: `File deleted` })
    } catch (error) {
        if (ERRORS[error.message]) {
            return responseBadRequest(res, ERRORS[error.message])
        }
        return responseServerSideError(res, error)
    }
}

const getSignedURL = async (req, res) => {

    try 
    {
        const { url, fileSize } = await uploadService.generatePresignedUrl(req.body.uri);
        return responseSuccess(res, { url, fileSize });
    } 
    catch (error) 
    {
        return responseServerSideError(res, error)
    }
}


module.exports = {
    uploadFileToIPFS,
    uploadJSONToIPFS,
    uploadFileToS3,
    deleteFileFromS3,
    getSignedURL,
    scanFile
}
