const { ScheduledTasks } = require('../models')

const startTasks = () => {
	return ScheduledTasks.startTasks()
}

module.exports = {
	startTasks
}
