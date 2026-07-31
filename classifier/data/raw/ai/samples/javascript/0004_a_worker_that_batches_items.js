const { Worker, isMainThread, parentPort } = require("node:worker_threads");

const BATCH_SIZE = 3;
const BATCH_TIMEOUT_MS = 500;

if (!isMainThread) {
  let batch = [];
  let timer;

  const flush = () => {
    if (!batch.length) return;
    parentPort.postMessage({ type: "batch", items: batch });
    batch = [];
    clearTimeout(timer);
    timer = undefined;
  };

  parentPort.on("message", (message) => {
    if (message.type === "item") {
      batch.push(message.item);

      if (batch.length >= BATCH_SIZE) {
        flush();
      } else if (!timer) {
        timer = setTimeout(flush, BATCH_TIMEOUT_MS);
      }
    } else if (message.type === "flush") {
      flush();
    } else if (message.type === "stop") {
      flush();
      parentPort.close();
    }
  });
} else {
  const worker = new Worker(__filename);

  worker.on("message", ({ type, items }) => {
    if (type === "batch") {
      console.log("Processed batch:", items);
    }
  });

  worker.on("error", console.error);

  for (let i = 1; i <= 7; i++) {
    worker.postMessage({ type: "item", item: `item-${i}` });
  }

  setTimeout(() => {
    worker.postMessage({ type: "flush" });
    setTimeout(() => worker.postMessage({ type: "stop" }), 100);
  }, 1000);
}
