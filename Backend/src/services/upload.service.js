const pinataSDK = require('@pinata/sdk')
const { getSecret } = require('../utils/helperFunctions')
const { AWS_S3_BUCKET, AWS_REGION, ACL_ACCESS, AWS_SECRET } = require('../config')
const { S3Client, PutObjectCommand, DeleteObjectCommand, GetObjectCommand, HeadObjectCommand } = require('@aws-sdk/client-s3');
const { getSignedUrl } = require("@aws-sdk/s3-request-presigner");

const streamifier = require('streamifier')

const ERRORS = require('../utils/errorTypes')

const uploadJsonToIPFS = async (jsonMetadata, name) => {
    try {
        const sec = await getSecret(AWS_SECRET);
        const pinata = new pinataSDK(sec.PINATA_API_KEY, sec.PINATA_API_SECRET_KEY);
        let response = {}
        const options = {
            pinataMetadata: {
                name: name // Provide the filename here
            }
        }
        const result = await pinata.pinJSONToIPFS(jsonMetadata, options)
        console.log('JSON pinned successfully!')
        console.log('IPFS Hash:', result.IpfsHash)
        response.metadataLink = `https://gateway.pinata.cloud/ipfs/${result.IpfsHash}`
        response.ipfsHash = result.IpfsHash
        return response
    } catch (error) {
        console.error('Error pinning JSON to IPFS:', error)
        throw new Error(ERRORS.JSON_UPLOAD_FAILED.CODE)
    }
}

const uploadFileToIPFS = async file => {
    try {
        // const fileBuffer = Buffer.from(file, 'base64')
        const sec = await getSecret(AWS_SECRET);
        const pinata = new pinataSDK(sec.PINATA_API_KEY, sec.PINATA_API_SECRET_KEY);
        
        const readableStream = streamifier.createReadStream(file.buffer)

        const uniqueSuffix = `${Date.now()}-${Math.round(Math.random() * 1e9)}`
        const name = `${file.originalname}-${uniqueSuffix}`

        const options = {
            pinataMetadata: {
                name: name // Provide the filename here
            }
        }

        const result = await pinata.pinFileToIPFS(readableStream, options)
        console.log('File pinned successfully!')
        console.log('IPFS Hash:', result.IpfsHash)

        return `https://gateway.pinata.cloud/ipfs/${result.IpfsHash}`
    } catch (error) {
        console.error('Error uploading file to Pinata:', error)
        throw new Error(ERRORS.FILE_UPLOAD_FAILED.CODE)
    }
}

const uploadFileToS3 = async file => {
    try {
        // const fileBuffer = Buffer.from(file, 'base64')

        const readableStream = streamifier.createReadStream(file.buffer)

        const uniqueSuffix = `${Date.now()}-${Math.round(Math.random() * 1e9)}`
        const name = `${uniqueSuffix}-${file.originalname}`

        const s3Client = new S3Client({
            region: AWS_REGION,
          });

        // Set the parameters for the file you want to upload
        const params = {
            Bucket: AWS_S3_BUCKET,
            Key: name,
            Body: file.buffer,
            ACL: ACL_ACCESS,
            ContentType: file.mimetype
        };

        // Upload the file to S3
        return new Promise((resolve, reject) => {
            const command = new PutObjectCommand(params);
            s3Client.send(command)
                .then((data) => {
                    console.log({ data });
                    const fileUrl = `https://${params.Bucket}.s3.${AWS_REGION}.amazonaws.com/${params.Key}`;
                    resolve(fileUrl);
                })
                .catch((err) => {
                    console.error('Error uploading file from S3:', err);
                    reject(new Error(ERRORS.S3_FILE_UPLOAD_FAILED.CODE));
                });
        });

    } catch (error) {
        console.error('Error uploading file to S3:', error)
        throw new Error(ERRORS.S3_FILE_UPLOAD_FAILED.CODE)
    }
}

const deleteFileFromS3 = async fileUrl => {
    try {

        // Set the region and access keys
        const s3Client = new S3Client({
            region: AWS_REGION,
          });

        const parsedUrl = new URL(fileUrl);
        const key = decodeURIComponent(parsedUrl.pathname.substring(1));

        const params = {
            Bucket: AWS_S3_BUCKET,
            Key: key,
        };

        return new Promise((resolve, reject) => {
            const command = new DeleteObjectCommand(params);
            s3Client.send(command)
                .then((data) => {
                    console.log({ data });
                    resolve(data);
                })
                .catch((err) => {
                    console.error('Error deleting file from S3:', err);
                    reject(new Error(ERRORS.DELETE_S3_FILE_FAILED.CODE));
                });
        });
    } catch (error) {
        console.error('Error deleting file from S3:', error)
        throw new Error(ERRORS.DELETE_S3_FILE_FAILED.CODE)
    }
}

// Function to generate pre-signed URL
const generatePresignedUrl = async (uri, expiresIn = 300) => {

    const s3Client = new S3Client({
        region: AWS_REGION,
      });

    const objectKey = uri.split('/').at(-1);
    const headObjectCommand = new HeadObjectCommand({
    Bucket: AWS_S3_BUCKET,
    Key: objectKey,
    });
    const headObjectResponse = await s3Client.send(headObjectCommand);

    // Get the file size in bytes
    const fileSize = headObjectResponse.ContentLength;
    
    const command = new GetObjectCommand({
        Bucket: AWS_S3_BUCKET,
        Key: objectKey,
    });

    // Generate a pre-signed URL
    const url = await getSignedUrl(s3Client, command, { expiresIn });
    return { url, fileSize };
};


module.exports = {
    uploadJsonToIPFS,
    uploadFileToIPFS,
    uploadFileToS3,
    deleteFileFromS3,
    generatePresignedUrl
}
