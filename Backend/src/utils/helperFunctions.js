const ethers = require('ethers')
const fetch = require('node-fetch')
const CONSTANTS = require('../constants')
const crypto = require('crypto')
//const fs = require('fs')
//const path = require('path');
const { SecretsManagerClient, GetSecretValueCommand } = require("@aws-sdk/client-secrets-manager");
const { COINMARKET_API, AWS_SECRET } = require('../config')

async function decrypt(encodedAesKey, encodedIv, encodedCiphertext, privatePemRaw) {
    // Load the private key from the specified path

    //const privateKeyPath = path.resolve(__dirname, '../../private.pem');
    //const privateKey = fs.readFileSync(privateKeyPath);
//     // Convert encrypted data from hex to a buffer
    const privatePem = reformatPrivateKey(privatePemRaw);
    const encryptedAesKeyBuffer = Buffer.from(encodedAesKey, 'base64');
    const decodedIv = Buffer.from(encodedIv, 'base64');
    const decodedCipher = Buffer.from(encodedCiphertext, 'base64');
    
    // Decrypt the data using the private key
    const decryptedAesKey = crypto.privateDecrypt(
        {
            key: privatePem, // The hash function for OAEP padding
            padding: crypto.constants.RSA_PKCS1_OAEP_PADDING
        },
        encryptedAesKeyBuffer
    );

    // Decrypt the ciphertext using the AES key and IV
    const decipher = crypto.createDecipheriv('aes-256-cbc', Buffer.from(decryptedAesKey, 'binary'), decodedIv);
    let decryptedData = decipher.update(decodedCipher);
    decryptedData = Buffer.concat([decryptedData, decipher.final()]);

    // Remove padding (PKCS7 padding)
    const padLen = decryptedData[decryptedData.length - 1];
    // decryptedData = decryptedData.slice(0, -padLen);
    //console.log(decryptedData.toString('utf-8'))
    return decryptedData.toString('utf-8');
}

function reformatPrivateKey(privateKeyString) {
    // Remove all spaces
    let key = privateKeyString.replace(/ /g, '');
    
    // Add line breaks every 64 characters for the key body
    const formattedKey = key.match(/.{1,64}/g).join('\n');
    
    // Reconstruct the full PEM key
    return `-----BEGIN RSA PRIVATE KEY-----\n${formattedKey}\n-----END RSA PRIVATE KEY-----\n`;
}

async function convertEtherToWei(etherAmount) {
    try {
        const weiAmount = ethers.utils.parseEther(etherAmount.toString());
        return weiAmount;
    } catch (error) {
        console.error('Error converting Ether to Wei:', error);
        throw error;
    }
}

async function getPriceInUSD(network) {
    try {
        const { COINMARKET_API_KEY } = await getSecret(AWS_SECRET);
        let tokenPriceInUSD
        if(network == CONSTANTS.ETH_NETWORK){
            const response = await (await fetch(COINMARKET_API.replace('[symbol]', 'ETH'), {method: 'GET', headers: {accept: 'application/json', 'X-CMC_PRO_API_KEY': COINMARKET_API_KEY}})).json();
            tokenPriceInUSD = response.data[0].quote.USD.price;
        }else if(network == CONSTANTS.POLYGON_NETWORK){
            const response = await (await fetch(COINMARKET_API.replace('[symbol]', 'MATIC'), {method: 'GET', headers: {accept: 'application/json', 'X-CMC_PRO_API_KEY': COINMARKET_API_KEY}})).json();
            tokenPriceInUSD = response.data[0].quote.USD.price;
        }
      return tokenPriceInUSD;
    } catch (error) {
      console.error('Error fetching ETH price:', error);
      throw new Error('Error in Coinmarket API');
    }
  }

  async function convertPriceToUsd(tokenAmount, network) {
    try {
        const tokenPriceInUSD = await getPriceInUSD(network);
        const usdAmount = (tokenAmount * tokenPriceInUSD).toFixed(2);
        return usdAmount;
    } catch (error) {
        console.error('Error converting Wei to USD:', error);
        throw error;
    }
}

async function convertUsdToWei(usdAmount, network) {
    try {
        const tokenPriceInUSD = await getPriceInUSD(network);
        const ethAmount = (usdAmount / tokenPriceInUSD).toFixed(4);
        // const weiAmount = ethers.utils.parseEther(ethAmount.toString());
        return ethAmount;
      } catch (error) {
        console.error('Error converting USD to ETH:', error);
        throw error;
      }
  }

async function convertWeiToEth(weiAmount) {
    try {
        const ethAmount = ethers.utils.formatEther(weiAmount);
        return ethAmount;
    } catch (error) {
        console.error('Error converting Wei to USD:', error);
        throw error;
    }
}

function getWalletAddressFromPrivateKey(privateKey) {
    const wallet = new ethers.Wallet(privateKey);
    return wallet.address;
  }

const getEnabledPaginationOption = (page, limit) => {
    let result = { enabled: false }

    if (page && limit) {
        result = { enabled: true, page, limit }
    }
    return result
}

const generateUpdateObject = (fieldsArray, dataArray) => {
    const updateObject = {}
    for (let item in fieldsArray) {
        if (dataArray[fieldsArray[item]] !== undefined) {
            updateObject[fieldsArray[item]] = dataArray[fieldsArray[item]]
        }
    }
    return updateObject
}

const generateNonce = () => {
    // Time-Based OTP algorithm
    const nonce = Math.floor(100000 + Math.random() * 900000)
    return nonce
}

const removeKeysFromObject = (object, keysToRemoveArray) => {
    const objectKeys = Object.keys(object)
    keysToRemoveArray.forEach((key) => {
        if (objectKeys.includes(key)) {
            delete object[key]
        }
    })
}

function filterEmptyKeys(object) {
    const filteredObj = {}

    for (const key in object) {
        const value = object[key]
        if (value !== '' && value !== null && value !== undefined) {
            filteredObj[key] = value
        }
    }
    return filteredObj
}

async function getSecret(secretName) {
    try {
      const client = new SecretsManagerClient({
        region: "eu-central-1", 
        });
      // Create the command to get the secret value
      const command = new GetSecretValueCommand({ SecretId: secretName });
  
      // Send the command to the Secrets Manager client
      const response = await client.send(command);
      const secret = response.SecretString;
  
      if (secret) {
        return JSON.parse(secret); // this returns an object of the secret: { test: "12345"}
      } else {
        throw new Error("Secret is empty or undefined");
      }
    } catch (error) {
      console.error(`Error retrieving secret: ${error.message}`);
      throw error;
    }
  }

function verifyHMACSignature(data, receivedSignature, secretKey) {
    const hmac = crypto.createHmac('sha1', secretKey);
    hmac.update(data);
    const expectedSignature = hmac.digest('base64');
    return expectedSignature === receivedSignature;
  }

  function formHMACFromStats(dataObj) {
    let formedStatString = '';
    for (const [key, value] of Object.entries(dataObj)) 
    {
        formedStatString += key;
        if (Array.isArray(value)) {
            value.forEach(element => {
                formedStatString += typeof element === 'number' ? element.toFixed(2) : element;
            });
        } else if (typeof value === 'number') {
            formedStatString += value.toFixed(10);
        } else if (typeof value === 'string') {
            formedStatString += value;
        }
    }
    return formedStatString;
}

async function fetchWithRetry(url, options = {}, retries = 3, delay = 1000) {
    for (let i = 0; i < retries; i++) {
        try {
            const response = await fetch(url, options);
            if (!response.ok) {
                throw new Error(`HTTP error! status: ${response.status}`);
            }
            return response;
        } catch (error) {
            console.error(`Attempt ${i + 1} failed: ${error.message}`);
            if (i < retries - 1) {
                console.log(`Retrying in ${delay}ms...`);
                await new Promise((resolve) => setTimeout(resolve, delay));
            } else {
                throw error;
            }
        }
    }
}
  

module.exports = {
    getEnabledPaginationOption,
    generateUpdateObject,
    generateNonce,
    removeKeysFromObject,
    filterEmptyKeys,
    getWalletAddressFromPrivateKey,
    convertUsdToWei,
    convertPriceToUsd,
    convertEtherToWei,
    convertWeiToEth,
    decrypt,
    getPriceInUSD,
    getSecret,
    verifyHMACSignature,
    formHMACFromStats,
    fetchWithRetry
}
