let dotenvConfig = require('dotenv').config()

if (dotenvConfig.error) {
    console.log('.env file not found')
}

const config = {
    // NODE_ENV : process.env.NODE_ENV || 'development',
    PORT: process.env.PORT || 3000,
    JWT_SECRET: process.env.JWT_SECRET || 'secret',// Change jwt_secret before production --- TODO
    JWT_EXPIRY: process.env.JWT_EXPIRY || '1w',// Reduce the expiry time before production --- TODO
    DB_HOST: process.env.DB_HOST,
    DB_PORT: process.env.DB_PORT,
    DB_USER: process.env.DB_USER,
    DB_PASSWORD: process.env.DB_PASSWORD,
    DB_AUTH_SOURCE: process.env.DB_AUTH_SOURCE,
    DB_NAME: process.env.DB_NAME,

    AWS_S3_BUCKET: process.env.AWS_S3_BUCKET,
    AWS_REGION: process.env.AWS_REGION,
    ACL_ACCESS: process.env.ACL_ACCESS,
    VERIFIER_PRIVATE_KEY: process.env.VERIFIER_PRIVATE_KEY,
    NFT_CONTRACT_ADDRESS_ETH: process.env.NFT_CONTRACT_ADDRESS_ETH,
    MARKETPLACE_CONTRACT_ADDRESS_ETH: process.env.MARKETPLACE_CONTRACT_ADDRESS_ETH,
    STAKE_CONTRACT_ADDRESS_ETH: process.env.STAKE_CONTRACT_ADDRESS_ETH,
    RPC_ETH: process.env.RPC_ETH,
    NFT_CONTRACT_ADDRESS_POLYGON: process.env.NFT_CONTRACT_ADDRESS_POLYGON,
    MARKETPLACE_CONTRACT_ADDRESS_POLYGON: process.env.MARKETPLACE_CONTRACT_ADDRESS_POLYGON,
    STAKE_CONTRACT_ADDRESS_POLYGON: process.env.STAKE_CONTRACT_ADDRESS_POLYGON,
    STAKE_CONTRACT_ADDRESS_POLYGON_TEST: process.env.STAKE_CONTRACT_ADDRESS_POLYGON_TEST,
    RPC_POLYGON: process.env.RPC_POLYGON,
    RPC_POLYGON_TEST: process.env.RPC_POLYGON_TEST,
    COINMARKET_API: process.env.COINMARKET_API,
    AWS_SECRET: process.env.AWS_SECRET,
    VIRUSTOTAL_API_KEY: process.env.VIRUSTOTAL_API_KEY,
    GET_ALL_STATS_KEY: process.env.GET_ALL_STATS_KEY,

    ADMIN_WALLET: process.env.ADMIN_WALLET,
    ADMIN_NFT_FEE_USD: process.env.ADMIN_NFT_FEE_USD,
    CLIENT_GAME_TOKEN_PASS: process.env.CLIENT_GAME_TOKEN_PASS,
    TEST_MODE: process.env.TEST_MODE,
    ZEALY_KEY: process.env.ZEALY_KEY,
    DISCORD_HIGHLIGHTS_HOOK: process.env.DISCORD_HIGHLIGHTS_HOOK,
    DISCORD_CLIENT: process.env.DISCORD_CLIENT,
    DISCORD_SECRET: process.env.DISCORD_SECRET,
    DISCORD_BOT_TOKEN: process.env.DISCORD_BOT_TOKEN,

    // AWS ARNs
    AWS_ACCESS_KEY: process.env.AWS_ACCESS_KEY,
    COGNITO_POOL_ID: process.env.COGNITO_POOL_ID
}

const CRUCIAL_ENVIRONMENT_VARIABLES = [
    'PORT',
    'JWT_SECRET', 'JWT_EXPIRY',
    'DB_HOST', 'DB_PORT', 'DB_USER', 'DB_PASSWORD', 'DB_AUTH_SOURCE', 'DB_NAME',
    'AWS_S3_BUCKET', 'AWS_REGION', 'ACL_ACCESS',
    'VERIFIER_PRIVATE_KEY', 'NFT_CONTRACT_ADDRESS_ETH', 'MARKETPLACE_CONTRACT_ADDRESS_ETH', 'RPC_ETH', 'NFT_CONTRACT_ADDRESS_POLYGON', 'MARKETPLACE_CONTRACT_ADDRESS_POLYGON', 'RPC_POLYGON',
    'COINMARKET_API',

]

for (let variableItem of CRUCIAL_ENVIRONMENT_VARIABLES) {
    if (!config[variableItem]) {
        console.log(`Crucial env variable missing: ${variableItem}`)
    }
}

module.exports = config
