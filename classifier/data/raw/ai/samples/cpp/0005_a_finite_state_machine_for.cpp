#include <iostream>
#include <map>
#include <string>

class VendingMachine {
public:
    enum class State {
        Idle,
        ProductSelected,
        WaitingForMoney,
        Dispensing,
        OutOfStock
    };

    enum class EventType {
        SelectProduct,
        InsertMoney,
        Cancel,
        DispenseComplete,
        Restock
    };

    struct Event {
        EventType type;
        std::string product;
        int amount = 0;
    };

    VendingMachine()
        : state(State::Idle), selectedProduct(""), insertedMoney(0) {
        inventory["Water"] = {125, 5};
        inventory["Soda"] = {150, 3};
        inventory["Juice"] = {175, 2};
    }

    void handleEvent(const Event& event) {
        switch (state) {
            case State::Idle:
                handleIdleState(event);
                break;

            case State::ProductSelected:
                handleProductSelectedState(event);
                break;

            case State::WaitingForMoney:
                handleWaitingForMoneyState(event);
                break;

            case State::Dispensing:
                handleDispensingState(event);
                break;

            case State::OutOfStock:
                handleOutOfStockState(event);
                break;
        }
    }

    State getState() const {
        return state;
    }

private:
    struct Product {
        int price;
        int quantity;
    };

    State state;
    std::string selectedProduct;
    int insertedMoney;
    std::map<std::string, Product> inventory;

    void handleIdleState(const Event& event) {
        if (event.type != EventType::SelectProduct) {
            return;
        }

        auto product = inventory.find(event.product);

        if (product == inventory.end()) {
            std::cout << "Product not found.\n";
            return;
        }

        selectedProduct = event.product;

        if (product->second.quantity == 0) {
            state = State::OutOfStock;
            std::cout << "Product is out of stock.\n";
            return;
        }

        state = State::ProductSelected;
        std::cout << selectedProduct << " selected. Price: "
                  << product->second.price << " cents.\n";
    }

    void handleProductSelectedState(const Event& event) {
        if (event.type == EventType::InsertMoney) {
            insertedMoney += event.amount;
            state = State::WaitingForMoney;

            std::cout << "Inserted " << event.amount
                      << " cents. Total: " << insertedMoney << " cents.\n";

            tryToDispense();
        } else if (event.type == EventType::Cancel) {
            refund();
            reset();
        }
    }

    void handleWaitingForMoneyState(const Event& event) {
        if (event.type == EventType::InsertMoney) {
            insertedMoney += event.amount;

            std::cout << "Inserted " << event.amount
                      << " cents. Total: " << insertedMoney << " cents.\n";

            tryToDispense();
        } else if (event.type == EventType::Cancel) {
            refund();
            reset();
        }
    }

    void handleDispensingState(const Event& event) {
        if (event.type != EventType::DispenseComplete) {
            return;
        }

        Product& product = inventory[selectedProduct];
        product.quantity--;

        int change = insertedMoney - product.price;

        std::cout << "Dispensed " << selectedProduct << ".\n";

        if (change > 0) {
            std::cout << "Returned change: " << change << " cents.\n";
        }

        reset();
    }

    void handleOutOfStockState(const Event& event) {
        if (event.type == EventType::Restock) {
            auto product = inventory.find(event.product);

            if (product != inventory.end()) {
                product->second.quantity += event.amount;
                std::cout << "Restocked " << event.product << ".\n";
                reset();
            }
        } else if (event.type == EventType::Cancel) {
            reset();
        }
    }

    void tryToDispense() {
        const Product& product = inventory[selectedProduct];

        if (insertedMoney >= product.price) {
            state = State::Dispensing;
            std::cout << "Dispensing product...\n";
        } else {
            std::cout << "Additional money required: "
                      << product.price - insertedMoney << " cents.\n";
        }
    }

    void refund() {
        if (insertedMoney > 0) {
            std::cout << "Refunded " << insertedMoney << " cents.\n";
        }
    }

    void reset() {
        state = State::Idle;
        selectedProduct.clear();
        insertedMoney = 0;
    }
};

int main() {
    VendingMachine machine;

    machine.handleEvent({VendingMachine::EventType::SelectProduct, "Water"});
    machine.handleEvent({VendingMachine::EventType::InsertMoney, "", 100});
    machine.handleEvent({VendingMachine::EventType::InsertMoney, "", 50});
    machine.handleEvent({VendingMachine::EventType::DispenseComplete});

    return 0;
}
