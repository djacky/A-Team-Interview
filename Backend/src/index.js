const express = require('express')
const bodyParser = require('body-parser')
const cors = require('cors')
const multer = require('multer')
const rateLimit = require('express-rate-limit');

//connect with db
require('./db')

// cron jobs
require('./cronjobs')

const app = express()
// Trust the proxy headers coming from the load balancer
app.set('trust proxy', 1);

const { PORT } = require('./config')
const routes = require('./routes')
const defaultLimiter = rateLimit({
    windowMs: 15 * 60 * 1000, // 15 minutes
    max: 100, // limit each IP to 100 requests per windowMs
  });

//Sever config

app.use(defaultLimiter);
const http = require('http').Server(app)

const storage = multer.memoryStorage() // Use memory storage
const upload = multer({ storage: storage })

// Use multer as middleware for all routes that need to handle FormData
app.use(upload.single('file')) // Process FormData for all routes

app.use(cors())

// for parsing application/json
app.use(bodyParser.json())

// For backend APIs
app.use(routes)

http.listen(PORT, () => {
    console.log(`Server is listening on ${PORT}`)
})
