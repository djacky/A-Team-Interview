const CONSTANTS = require('../constants')

const RESPONSE_ERRORS = {
    SERVER_SIDE_ERROR: {
        CODE: 'SERVER_SIDE_ERROR',
        MESSAGE: 'Something went wrong',
        STATUS: 500
    },
    BAD_REQUEST: {
        CODE: 'BAD_REQUEST',
        MESSAGE: 'Invalid request',
        STATUS: 400
    },
    CLIENT_UNAUTHORIZED: {
        CODE: 'CLIENT_UNAUTHORIZED',
        MESSAGE: 'You are not authorized to request/modify the resource',
        STATUS: 401
    },
    // Common Errors
    INVALID_OBJECT_ID_FORMAT: {
        CODE: 'INVALID_OBJECT_ID_FORMAT',
        MESSAGE: 'Object id given is invalid'
    },
    OBJECT_NOT_FOUND: {
        CODE: 'OBJECT_NOT_FOUND',
        MESSAGE: 'Object not found'
    },
    INVALID_TOKEN_TYPE: {
        CODE: 'INVALID_TOKEN_TYPE',
        MESSAGE: 'Invalid token type'
    },
    APP_IS_PAUSED: {
        CODE: 'APP_IS_PAUSED',
        MESSAGE: 'App is paused by admin'
    },
    APP_CONFIG_MISSING: {
        CODE: 'APP_CONFIG_MISSING',
        MESSAGE: 'App config is missing'
    },
    INVALID_NETWORK_TYPE: {
        CODE: 'INVALID_NETWORK_TYPE',
        MESSAGE: 'Invalid network type'
    },
    // User Details Errors
    ACCOUNT_ALREADY_EXISTS: {
        CODE: 'ACCOUNT_ALREADY_EXISTS',
        MESSAGE: 'Account already exists'
    },
    NO_USER_FOUND: {
        CODE: 'NO_USER_FOUND',
        MESSAGE: 'No user found'
    },
    INVALID_WALLET_ADDRESS: {
        CODE: 'INVALID_WALLET_ADDRESS',
        MESSAGE: 'Invalid wallet address'
    },
    WALLET_ADDRESS_NOT_REGISTERED: {
        CODE: 'WALLET_ADDRESS_NOT_REGISTERED',
        MESSAGE: 'Wallet address not registered'
    },
    BANNED_USER: {
        CODE: 'BANNED_USER',
        MESSAGE: 'You are banned, please contact admin'
    },
    WALLET_LINKED_TO_SOME_OTHER_ACCOUNT: {
        CODE: 'WALLET_LINKED_TO_SOME_OTHER_ACCOUNT',
        MESSAGE: 'Wallet linked to some other account'
    },
    // NFT Details Errors
    INVALID_S3_IMAGE_URI_TYPE: {
        CODE: 'INVALID_S3_IMAGE_URI_TYPE',
        MESSAGE: 'Invalid S3 image uri type'
    },
    INVALID_S3_IMAGE_URI: {
        CODE: 'INVALID_S3_IMAGE_URI',
        MESSAGE: 'Invalid S3 image uri'
    },
    S3_IMAGE_URI_NOT_UNIQUE: {
        CODE: 'S3_IMAGE_URI_NOT_UNIQUE',
        MESSAGE: 'S3 image uri is not unique'
    },
    INVALID_S3_MODEL_URI_TYPE: {
        CODE: 'INVALID_S3_MODEL_URI_TYPE',
        MESSAGE: 'Invalid S3 model uri type'
    },
    INVALID_S3_MODEL_URI: {
        CODE: 'INVALID_S3_MODEL_URI',
        MESSAGE: 'Invalid S3 model uri'
    },
    S3_MODEL_URI_NOT_UNIQUE: {
        CODE: 'S3_MODEL_URI_NOT_UNIQUE',
        MESSAGE: 'S3 model uri is not unique'
    },
    INVALID_NFT_NAME_TYPE: {
        CODE: 'INVALID_NFT_NAME_TYPE',
        MESSAGE: 'Invalid nft name type'
    },
    INVALID_NFT_NAME: {
        CODE: 'INVALID_NFT_NAME',
        MESSAGE: 'Invalid nft name'
    },
    NFT_NAME_NOT_UNIQUE: {
        CODE: 'NFT_NAME_NOT_UNIQUE',
        MESSAGE: 'NFT name is not unique'
    },
    INVALID_NFT_DESCRIPTION_TYPE: {
        CODE: 'INVALID_NFT_DESCRIPTION_TYPE',
        MESSAGE: 'Invalid nft description type'
    },
    INVALID_NFT_PRICE_TYPE: {
        CODE: 'INVALID_NFT_PRICE_TYPE',
        MESSAGE: 'Invalid nft price type'
    },
    INVALID_AUCTION_PRICE_RANGE: {
        CODE: 'INVALID_AUCTION_PRICE_RANGE',
        MESSAGE: `Valid auction price range is ${CONSTANTS.NFT_MIN_PRICE + 1}-${CONSTANTS.NFT_MAX_PRICE + 1}`
    },
    INVALID_NFT_AMOUNT_TYPE: {
        CODE: 'INVALID_NFT_AMOUNT_TYPE',
        MESSAGE: 'Invalid nft amount type'
    },
    INVALID_STATUS_TYPE: {
        CODE: 'INVALID_STATUS_TYPE',
        MESSAGE: 'Invalid status type'
    },
    INVALID_TOOL_TYPE: {
        CODE: 'INVALID_TOOL_TYPE',
        MESSAGE: 'Invalid tool type'
    },
    INVALID_FEEDBACK_TYPE: {
        CODE: 'INVALID_FEEDBACK_TYPE',
        MESSAGE: 'Invalid feedback type'
    },
    INVALID_NFT_AMOUNT_RANGE: {
        CODE: 'INVALID_NFT_AMOUNT_RANGE',
        MESSAGE: `Valid nft anount range is ${CONSTANTS.NFT_MIN_AMOUNT}-${CONSTANTS.NFT_MAX_AMOUNT}`
    },
    NO_MINT_REQUEST_FOUND: {
        CODE: 'NO_MINT_REQUEST_FOUND',
        MESSAGE: 'No mint request found'
    },
    UNABLE_TO_REGENERATE_REQUEST: {
        CODE: 'UNABLE_TO_REGENERATE_REQUEST',
        MESSAGE: 'Unable to regenerate request'
    },
    UNABLE_TO_MINT: {
        CODE: 'UNABLE_TO_MINT',
        MESSAGE: 'Unable to mint'
    },
    ALREADY_MINTED: {
        CODE: 'ALREADY_MINTED',
        MESSAGE: 'Already minted'
    },
    NO_NFT_FOUND: {
        CODE: 'NO_NFT_FOUND',
        MESSAGE: 'No nft found'
    },
    UNABLE_TO_UPDATE: {
        CODE: 'UNABLE_TO_UPDATE',
        MESSAGE: 'Unable to update as nft already minted'
    },
    INSUFFICIENT_NFT_BALANCE: {
        CODE: 'INSUFFICIENT_NFT_BALANCE',
        MESSAGE: 'Insufficient nft balance'
    },
    // UPLOAD SERVICE - PINATA
    FILE_UPLOAD_FAILED: {
        CODE: 'FILE_UPLOAD_FAILED',
        MESSAGE: 'Failed to upload file to IPFS'
    },
    S3_FILE_UPLOAD_FAILED: {
        CODE: 'S3_FILE_UPLOAD_FAILED',
        MESSAGE: 'Failed to upload file to S3'
    },
    DELETE_S3_FILE_FAILED: {
        CODE: 'DELETE_S3_FILE_FAILED',
        MESSAGE: 'Failed to delete file from S3'
    },
    JSON_UPLOAD_FAILED: {
        CODE: 'JSON_UPLOAD_FAILED',
        MESSAGE: 'Failed to upload JSON to IPFS'
    },
    INVALID_NFT_METADATA_TYPE: {
        CODE: 'INVALID_NFT_METADATA_TYPE',
        MESSAGE: 'Invalid nft metadata type'
    },
    INVALID_NFT_METADATA_FILENAME_TYPE: {
        CODE: 'INVALID_NFT_METADATA_FILENAME_TYPE',
        MESSAGE: 'Invalid nft metadata filename type'
    },
    // USER DETAILS ERRORS
    MALFORMED_EMAIL: {
        CODE: 'MALFORMED_EMAIL',
        MESSAGE: 'Email address is invalid'
    },
    EMAIL_ALREADY_IN_USE: {
        CODE: 'EMAIL_ALREADY_IN_USE',
        MESSAGE: 'Email address is already in use'
    },
    INVALID_PASSWORD_TYPE: {
        CODE: 'INVALID_PASSWORD_TYPE',
        MESSAGE: 'Invalid password type'
    },
    INVALID_PASSWORD: {
        CODE: 'INVALID_PASSWORD',
        MESSAGE: 'Invalid password given'
    },
    INVALID_NONCE_TYPE: {
        CODE: 'INVALID_NONCE_TYPE',
        MESSAGE: 'Invalid nonce type'
    },
    INVALID_VERIFICATION_CODE_TYPE: {
        CODE: 'INVALID_VERIFICATION_CODE_TYPE',
        MESSAGE: 'Invalid verification type'
    },
    INVALID_EMAIL_VERIFICATION_CODE: {
        CODE: 'INVALID_EMAIL_VERIFICATION_CODE',
        MESSAGE: 'Invalid email verification code'
    },
    INVALID_BAN_TYPE: {
        CODE: 'INVALID_BAN_TYPE',
        MESSAGE: 'Invalid ban type'
    },
    INVALID_PAUSE_TYPE: {
        CODE: 'INVALID_PAUSE_TYPE',
        MESSAGE: 'Invalid pause type'
    },
    INVALID_PAUSE_REASON_TYPE: {
        CODE: 'INVALID_PAUSE_REASON_TYPE',
        MESSAGE: 'Invalid pause reason type'
    },
    NONCE_EXPIRED: {
        CODE: 'NONCE_EXPIRED',
        MESSAGE: 'Nonce is expired'
    },
    // AUCTION ERRORS
    INVALID_DURATION: {
        CODE: 'INVALID_DURATION',
        MESSAGE: 'Invalid duration'
    },
    UNABLE_TO_CREATE_AUCTION: {
        CODE: 'UNABLE_TO_CREATE_AUCTION',
        MESSAGE: 'Unable to create auction'
    },
    AUCTION_ALREADY_ACTIVE: {
        CODE: 'AUCTION_ALREADY_ACTIVE',
        MESSAGE: 'Auction already active for this NFT'
    },
    INVALID_AUCTION_PRICE: {
        CODE: 'INVALID_AUCTION_PRICE',
        MESSAGE: 'Invalid auction price'
    },
    NO_AUCTION_FOUND: {
        CODE: 'NO_AUCTION_FOUND',
        MESSAGE: 'No auction found'
    },
    NO_BID_FOUND: {
        CODE: 'NO_BID_FOUND',
        MESSAGE: 'No bid found'
    },
    NO_REFUND_FOUND: {
        CODE: 'NO_REFUND_FOUND',
        MESSAGE: 'No refund found'
    },
    INVALID_BID_PRICE: {
        CODE: 'INVALID_BID_PRICE',
        MESSAGE: 'Invalid bid price'
    },
    LOW_BID_PRICE: {
        CODE: 'LOW_BID_PRICE',
        MESSAGE: 'Bid price should high'
    },
    AUCTION_ALREADY_ENDED: {
        CODE: 'AUCTION_ALREADY_ENDED',
        MESSAGE: 'Auction already ended'
    },
    AUCTION_NOT_ENDED_YET: {
        CODE: 'AUCTION_NOT_ENDED_YET',
        MESSAGE: 'Auction not ended yet'
    },
    NOT_ALLOWED_TO_END_AUCTION: {
        CODE: 'NOT_ALLOWED_TO_END_AUCTION',
        MESSAGE: 'Only the auction creator can end the auction'
    },
    NOT_ALLOWED_TO_CANCEL_AUCTION: {
        CODE: 'NOT_ALLOWED_TO_CANCEL_AUCTION',
        MESSAGE: 'Only the auction creator can cancel the auction'
    },
    UNABLE_TO_CANCEL_AUCTION: {
        CODE: 'UNABLE_TO_CANCEL_AUCTION',
        MESSAGE: 'Cannot cancel an auction that has started'
    },
    // BLOCKCHAIN ERRORS
    SIGNATURE_FAILED: {
        CODE: 'SIGNATURE_FAILED',
        MESSAGE: 'Failed to sign the message'
    },
    TRANSACTION_SIGN_FAILED: {
        CODE: 'TRANSACTION_SIGN_FAILED',
        MESSAGE: 'Failed to sign the transaction'
    },
    UNPREDICTABLE_GAS_LIMIT: {
        CODE: 'UNPREDICTABLE_GAS_LIMIT',
        MESSAGE: 'Gas Estimation Failed: due to insufficient funds'
    },
    INSUFFICIENT_FUNDS: {
        CODE: 'INSUFFICIENT_FUNDS_FOR_TRANSACTION',
        MESSAGE: 'The sending account has insufficient funds to cover the entire transaction cost.'
    },
    CALL_EXCEPTION: {
        CODE: 'CALL_EXCEPTION',
        MESSAGE: 'Transaction reverted'
    },
    REPLACEMENT_UNDERPRICED: {
        CODE: 'REPLACEMENT_UNDERPRICED',
        MESSAGE: 'An attempt was made to replace a transaction, but with an insufficient additional fee to afford evicting the old transaction from the memory pool.'
    },
    TRANSACTION_REPLACED: {
        CODE: 'TRANSACTION_REPLACED',
        MESSAGE: 'A pending transaction was replaced by another.'
    },
    BAD_WEAPON_NAME: {
        CODE: 'BAD_WEAPON_NAME',
        MESSAGE: 'Weapon name is not valid.'
    },
    FAILED_FILE_SCAN: {
        CODE: 'FAILED_FILE_SCAN',
        MESSAGE: 'File could not be processed on the server. Please modify the file and try again.'
    },
    KYC_NOT_PASSED: {
        CODE: 'KYC_NOT_PASSED',
        MESSAGE: 'User did not pass the KYC check. KYC needs to be completed.'
    }

}

module.exports = RESPONSE_ERRORS
