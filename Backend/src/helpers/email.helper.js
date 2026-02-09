const nodemailer = require('nodemailer')
// const sendgridTransport = require('nodemailer-sendgrid-transport')
const handlebars = require('handlebars')
const fs = require('fs')

const { SERVICE_NAME, SERVICE_EMAIL, SERVICE_APP_PASS } = require('../config')
const { generateNonce } = require('../utils/helperFunctions')

// Register the split helper
handlebars.registerHelper('split', function (str, delimiter) {
    return str.split(delimiter)
})

const transporter = nodemailer.createTransport({
    service: SERVICE_NAME,
    auth: {
        user: SERVICE_EMAIL,
        pass: SERVICE_APP_PASS

    }
}
)

const _sendEmail = async (options, emailTemplateName) => {
    const templatePath = `src/emailTemplates/${emailTemplateName}`
    const emailTemplate = fs.readFileSync(templatePath, 'utf8')
    const compiledTemplate = handlebars.compile(emailTemplate)

    const mailOptions = {
        from: options?.from,
        to: options?.to,
        subject: options?.subject,
        html: compiledTemplate({
            title: options?.title,
            greetings: options?.greetings,
            message: options?.message
        })
    }

    try {
        await transporter.sendMail(mailOptions)
        console.log(`Email sent successfully from ${options?.from} to ${options?.to} for subject: ${options?.subject}`)
    } catch (error) {
        console.error(`Error sending email with Nodemailer and Handlebars: ${error.message}`)
    }
}

const user_newProfileWaitVerfication = async (user) => {
    try {
        const verificationCode = generateNonce()

        let options = {
            from: MAIL_SENDGRID_FROM,
            to: user.email,
            subject: 'WELCOME',
            title: 'Kode Zero!',
            greetings: `Hello ${user.nickName}`,
            message: `Thanks for signing up for Kode Zero! To verify your email address, please enter the following verification code into the Sugar Head app: ${verificationCode}`
        }
        await _sendEmail(options, 'emailTemplate.hbs')
    } catch (error) {
        console.error(`Error sending email with user_newProfileWaitVerfication: ${error.message}`)
        throw error
    }
}

const admin_newProfileWaitVerification = async (admin, verificationCode) => {
    try {
        let options = {
            from: SERVICE_EMAIL,
            to: admin.email,
            subject: 'WELCOME',
            title: 'Kode Zero!',
            greetings: 'Dear Admin',
            message: `Thanks for signing up for Kode Zero! To verify your email address, please enter the following verification code into the Kode Zero admin portal: ${verificationCode}`
        }
        await _sendEmail(options, 'emailTemplate.hbs')
    } catch (error) {
        console.error(`Error sending email with admin_newProfileWaitVerification: ${error.message}`)
        throw error
    }
}

const user_sendVerificationCode = async (user, verificationCode) => {
    try {
        // const resetPasswordUrl = `BASE_URL/api/user/reset-password?token=${verificationCode}`

        let options = {
            from: MAIL_SENDGRID_FROM,
            to: user.email,
            subject: 'Reset Password',
            title: 'Kode Zero!',
            greetings: `Hello ${user.nickName}`,
            message: `Someone, hopefully you, has requested to reset the password for your Kode Zero account.
            If you did not perform this request, you can safely ignore this email.
            Otherwise, use this verification code to reset your password.
            ${verificationCode}`
        }
        await _sendEmail(options, 'emailTemplate.hbs')
    } catch (error) {
        console.error(`Error sending email with user_forgotPassword: ${error.message}`)
        throw error
    }
}

const admin_forgotPassword = async (admin, verificationCode) => {
    try {
        const resetPasswordUrl = ''

        let options = {
            from: SERVICE_EMAIL,
            to: admin.email,
            subject: 'Reset Password',
            title: 'Kode Zero!',
            greetings: 'Dear Admin',
            message: `Someone, hopefully you, has requested to reset the password for your Kode Zero account.\n   
            If you did not perform this request, you can safely ignore this email.\n
            Otherwise, use this verification code to reset your password.
            ${verificationCode}`         
            
            // Otherwise, click the link below to complete the process.
            // ${resetPasswordUrl}
        }
        await _sendEmail(options, 'emailTemplate.hbs')
    } catch (error) {
        console.error(`Error sending email with admin_forgotPassword: ${error.message}`)
        throw error
    }
}

module.exports = {
    admin_newProfileWaitVerification,
    user_sendVerificationCode,
    admin_forgotPassword
}