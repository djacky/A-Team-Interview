const router = require('express').Router()
const { jwtValidators } = require('../middlewares')

const authorized = require('./authorized')
const unauthorized = require('./unauthorized')
const adminRouter = require('./admin.routes')
const game = require('./game')

router.use(unauthorized)//, jwtValidators.authenticate, authorized)
router.use('/admin', adminRouter)// jwtValidators.adminAuthenticate, adminRouter)
router.use(game)

router.get('/health', async (req, res) => {
    res.status(200).send('Health Ok');
});

module.exports = router
