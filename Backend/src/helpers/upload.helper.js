const { Upload } = require('../models');
const { User } = require('../models');

const { VIRUSTOTAL_API_KEY } = require('../config')
const FormData = require('form-data');
const fetch = require('node-fetch');
const CONSTANTS = require('../constants')

const { LambdaClient, InvokeCommand } = require('@aws-sdk/client-lambda');


// Function to scan file with VirusTotal
async function scanFileWithVirusTotal(req) {

    var formData = new FormData();
    formData.append('file', req.file.buffer, req.file.originalname);

    const url = CONSTANTS.VIRUS_TOTAL_MAIN_URL + 'files';
    const options = {
        method: 'POST',
        headers: {'x-apikey': VIRUSTOTAL_API_KEY, accept: 'application/json', 'content-type': 'multipart/form-data', ...formData.getHeaders()},
        body: formData
    };

    try {
        const virusResponse = await fetch(url, options);
        const virusResponseJson = await virusResponse.json();
        const scanId = virusResponseJson.data.id;
        // Get scan results
        const resultUrl = CONSTANTS.VIRUS_TOTAL_MAIN_URL + `analyses/${scanId}`;
        const resultResponse = await fetch(resultUrl, {
            headers: {'x-apikey': VIRUSTOTAL_API_KEY}
        });
        
        mainScanResult = await resultResponse.json();
		if (mainScanResult.data.attributes.stats.malicious > 0)
		{
			const scanDetails = {
				ip: req.ip,
				accountId: req.tokenData.account_id,
				fileName: req.file.originalname,
				info: mainScanResult.data.attributes.stats
			};
			await logScanData(scanDetails, User);
			return false;
		}
        return true;
    } catch (error) {
        throw new Error('Error scanning file with VirusTotal: ' + error.message);
    }
  }

const logScanData = async (scanDetails, userModel) => {
	return Upload.logScanAndBan(scanDetails, userModel)
}

async function processGLB(req) {
	// Step 1: Parse the GLB file
	const glbBuffer = req.file.buffer; // .glb file as a buffer from Unreal Engine

    const gltf = await compressFiles(glbBuffer);
    
    // Compress all textures
    //await compressTextures(gltf);
    
    // Reconstruct the GLB with compressed textures
    //const compressedGlbBuffer = await reconstructGLB(gltf);
	return false;
  }

  async function compressFiles(glbBuffer) {

	try {
	const client = new LambdaClient({ region: 'eu-central-1' });
	const params = {
		FunctionName: 'ConvertGLB', // Replace with your Lambda function name or ARN
		InvocationType: 'RequestResponse', // Use 'Event' for async invocation
		Payload: JSON.stringify({
			glb: "Test" // Convert buffer to base64 string
		  })
	  };
	  const command = new InvokeCommand(params);
	  const { Payload } = await client.send(command);
	  const resultString = Buffer.from(Payload).toString('utf-8');
	  const result = JSON.parse(resultString);
	  console.log(result);
	  return false;
	}
	catch (error)
	{
		console.error("Error invoking Lambda function:", error);
		throw error;
	}
}



module.exports = {
	scanFileWithVirusTotal,
	processGLB
}
