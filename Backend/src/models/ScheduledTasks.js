const mongoose = require('mongoose')
const { taskSchema } = require('./schema')

taskSchema.static('startTask', async function ()
{

})

const ScheduledTasks = mongoose.model('tasks', taskSchema)
module.exports = ScheduledTasks
