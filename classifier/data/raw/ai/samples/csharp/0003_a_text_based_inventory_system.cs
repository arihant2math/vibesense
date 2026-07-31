using System;
using System.Collections.Generic;

public class InventoryItem
{
    public string Name { get; }
    public int Quantity { get; private set; }

    public InventoryItem(string name, int quantity)
    {
        Name = name;
        Quantity = quantity;
    }

    public void Add(int quantity)
    {
        Quantity += quantity;
    }

    public bool Remove(int quantity)
    {
        if (quantity > Quantity)
        {
            return false;
        }

        Quantity -= quantity;
        return true;
    }
}

public class Inventory
{
    private readonly Dictionary<string, InventoryItem> items =
        new Dictionary<string, InventoryItem>(StringComparer.OrdinalIgnoreCase);

    public void Add(string name, int quantity)
    {
        if (string.IsNullOrWhiteSpace(name) || quantity <= 0)
        {
            return;
        }

        if (items.TryGetValue(name, out InventoryItem item))
        {
            item.Add(quantity);
        }
        else
        {
            items[name] = new InventoryItem(name, quantity);
        }
    }

    public bool Remove(string name, int quantity)
    {
        if (string.IsNullOrWhiteSpace(name) || quantity <= 0)
        {
            return false;
        }

        if (!items.TryGetValue(name, out InventoryItem item))
        {
            return false;
        }

        if (!item.Remove(quantity))
        {
            return false;
        }

        if (item.Quantity == 0)
        {
            items.Remove(name);
        }

        return true;
    }

    public int Query(string name)
    {
        if (items.TryGetValue(name, out InventoryItem item))
        {
            return item.Quantity;
        }

        return 0;
    }

    public void Print()
    {
        if (items.Count == 0)
        {
            Console.WriteLine("Inventory is empty.");
            return;
        }

        foreach (InventoryItem item in items.Values)
        {
            Console.WriteLine($"{item.Name}: {item.Quantity}");
        }
    }
}

public static class Program
{
    public static void Main()
    {
        Inventory inventory = new Inventory();

        while (true)
        {
            Console.Write("Enter command (add, remove, query, list, exit): ");
            string input = Console.ReadLine();

            if (string.IsNullOrWhiteSpace(input))
            {
                continue;
            }

            string[] parts = input.Split(
                new[] { ' ' },
                StringSplitOptions.RemoveEmptyEntries);

            string command = parts[0].ToLowerInvariant();

            if (command == "exit")
            {
                break;
            }

            if (command == "list")
            {
                inventory.Print();
                continue;
            }

            if (parts.Length != 3 ||
                !int.TryParse(parts[2], out int quantity))
            {
                Console.WriteLine("Usage: add/remove/query <item> <quantity>");
                continue;
            }

            string itemName = parts[1];

            if (command == "add")
            {
                inventory.Add(itemName, quantity);
                Console.WriteLine("Item added.");
            }
            else if (command == "remove")
            {
                Console.WriteLine(
                    inventory.Remove(itemName, quantity)
                        ? "Item removed."
                        : "Unable to remove item.");
            }
            else if (command == "query")
            {
                Console.WriteLine($"{itemName}: {inventory.Query(itemName)}");
            }
            else
            {
                Console.WriteLine("Unknown command.");
            }
        }
    }
}
