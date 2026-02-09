function createQueueProcessor(handlerFunction) {
  const requestQueue = [];
  let isProcessing = false;

  return async function queueHandler(req, res) {
    requestQueue.push({ req, res });

    if (!isProcessing) {
      isProcessing = true;

      while (requestQueue.length > 0) {
        const { req, res } = requestQueue.shift();

        try {
          await handlerFunction(req, res);
        } catch (err) {
          console.error("Error handling request:", err);
          res.status(500).send("Internal Server Error");
        }

        await new Promise(resolve => setTimeout(resolve, 100));
      }

      isProcessing = false;
    }
  };
}

module.exports = createQueueProcessor;