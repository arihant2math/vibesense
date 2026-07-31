"use strict";

const readline = require("readline");

const inventory = new Map();

function addItem(name, quantity) {
  quantity = Number(quantity);
  if (!name || !Number.isInteger(quantity) || quantity <= 0) {
    return "Usage: add <item> <positive integer>";
  }

  inventory.set(name, (inventory.get(name) || 0) + quantity);
  return `Added ${quantity} ${name}.`;
}

function removeItem(name, quantity) {
  quantity = Number(quantity);
  if (!name || !Number.isInteger(quantity) || quantity <= 0) {
    return "Usage: remove <item> <positive integer>";
  }

  const current = inventory.get(name) || 0;
  if (current < quantity) {
    return `Insufficient stock for ${name}. Available: ${current}.`;
  }

  const remaining = current - quantity;
  if (remaining === 0) inventory.delete(name);
  else inventory.set(name, remaining);

  return `Removed ${quantity} ${name}.`;
}

function queryItem(name) {
  if (name) {
    return `${name}: ${inventory.get(name) || 0}`;
  }

  if (inventory.size === 0) return "Inventory is empty.";

  return [...inventory.entries()]
    .sort(([a], [b]) => a.localeCompare(b))
    .map(([item, quantity]) => `${item}: ${quantity}`)
    .join("\n");
}

const rl = readline.createInterface({
  input: process.stdin,
  output: process.stdout,
  terminal: false
});

rl.on("line", (line) => {
  const input = line.trim();
  if (!input) return;

  const [command, ...args] = input.split(/\s+/);
  const name = args[0];
  const quantity = args[1];

  switch (command.toLowerCase()) {
    case "add":
      console.log(addItem(name, quantity));
      break;
    case "remove":
      console.log(removeItem(name, quantity));
      break;
    case "query":
    case "list":
      console.log(queryItem(name));
      break;
    case "exit":
    case "quit":
      rl.close();
      break;
    default:
      console.log("Commands: add, remove, query, exit");
  }
});
