const isValidEmail = function (email) {
    const emailRegex = /^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$/
    return emailRegex.test(email)
}

const isValidPassword = function (password) {
    // Minimum length of 8 characters
    // Maximum length of 32 characters
    // Contains at least one uppercase letter, one lowercase letter, one digit, and one special character

    const passwordRegex = /^(?=.*[a-z])(?=.*[A-Z])(?=.*\d)(?=.*[@$!%*?&])[A-Za-z\d@$!%*?&]{8,32}$/
    return passwordRegex.test(password)
}

const isValidName = function (name) {
    const nameRegex = /^[a-zA-Z\s\-']+$/ // Regular expression to allow alphabetic characters, spaces, hyphens, and apostrophes
    return nameRegex.test(name)
}

function isValidateS3Url(url) {
    const pathStylePattern = /(s3-|s3\.)?(.*)\.amazonaws\.com/g;
    return pathStylePattern.test(url)
  }



const isValidWalletAddress = function (walletAddress) {
    const walletRegex = /^0x[a-fA-F0-9]{40}$/
    return walletRegex.test(walletAddress)
}


module.exports = {
    isValidEmail,
    isValidPassword,
    isValidWalletAddress,
    isValidateS3Url,
    isValidName
}