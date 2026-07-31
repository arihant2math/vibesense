package main

import (
	"bufio"
	"fmt"
	"os"
	"strconv"
	"strings"
)

type Inventory struct {
	items map[string]int
}

func NewInventory() *Inventory {
	return &Inventory{items: make(map[string]int)}
}

func (inv *Inventory) Add(item string, quantity int) {
	inv.items[item] += quantity
}

func (inv *Inventory) Remove(item string, quantity int) error {
	current := inv.items[item]
	if current < quantity {
		return fmt.Errorf("insufficient quantity of %q", item)
	}

	current -= quantity
	if current == 0 {
		delete(inv.items, item)
	} else {
		inv.items[item] = current
	}

	return nil
}

func (inv *Inventory) Query(item string) int {
	return inv.items[item]
}

func (inv *Inventory) List() {
	if len(inv.items) == 0 {
		fmt.Println("inventory is empty")
		return
	}

	for item, quantity := range inv.items {
		fmt.Printf("%s: %d\n", item, quantity)
	}
}

func main() {
	inventory := NewInventory()
	scanner := bufio.NewScanner(os.Stdin)

	for scanner.Scan() {
		line := strings.TrimSpace(scanner.Text())
		if line == "" {
			continue
		}

		parts := strings.Fields(line)
		command := strings.ToLower(parts[0])

		switch command {
		case "add":
			if len(parts) != 3 {
				fmt.Println("usage: add <item> <quantity>")
				continue
			}

			quantity, err := strconv.Atoi(parts[2])
			if err != nil || quantity <= 0 {
				fmt.Println("quantity must be a positive integer")
				continue
			}

			inventory.Add(parts[1], quantity)
			fmt.Printf("%s: %d\n", parts[1], inventory.Query(parts[1]))

		case "remove":
			if len(parts) != 3 {
				fmt.Println("usage: remove <item> <quantity>")
				continue
			}

			quantity, err := strconv.Atoi(parts[2])
			if err != nil || quantity <= 0 {
				fmt.Println("quantity must be a positive integer")
				continue
			}

			if err := inventory.Remove(parts[1], quantity); err != nil {
				fmt.Println(err)
				continue
			}

			fmt.Printf("%s: %d\n", parts[1], inventory.Query(parts[1]))

		case "query":
			if len(parts) != 2 {
				fmt.Println("usage: query <item>")
				continue
			}

			fmt.Printf("%s: %d\n", parts[1], inventory.Query(parts[1]))

		case "list":
			inventory.List()

		case "help":
			fmt.Println("commands: add <item> <quantity>, remove <item> <quantity>, query <item>, list, exit")

		case "exit", "quit":
			return

		default:
			fmt.Println("unknown command")
		}
	}

	if err := scanner.Err(); err != nil {
		fmt.Fprintln(os.Stderr, err)
	}
}
