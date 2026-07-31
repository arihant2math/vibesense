#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

class BankAccountLedger {
public:
    struct Entry {
        std::string type;
        std::int64_t amount;
        std::int64_t balance;
    };

    explicit BankAccountLedger(std::int64_t initialBalance = 0) {
        if (initialBalance < 0) {
            throw std::invalid_argument("Initial balance cannot be negative");
        }

        balance_ = initialBalance;
        history_.push_back({"Initial balance", 0, balance_});
    }

    void deposit(std::int64_t amount) {
        validateAmount(amount);

        if (balance_ > std::numeric_limits<std::int64_t>::max() - amount) {
            throw std::overflow_error("Balance overflow");
        }

        balance_ += amount;
        history_.push_back({"Deposit", amount, balance_});
    }

    void withdraw(std::int64_t amount) {
        validateAmount(amount);

        if (amount > balance_) {
            throw std::underflow_error("Insufficient funds");
        }

        balance_ -= amount;
        history_.push_back({"Withdrawal", amount, balance_});
    }

    std::int64_t balance() const noexcept {
        return balance_;
    }

    const std::vector<Entry>& history() const noexcept {
        return history_;
    }

private:
    static void validateAmount(std::int64_t amount) {
        if (amount <= 0) {
            throw std::invalid_argument("Transaction amount must be positive");
        }
    }

    std::int64_t balance_{0};
    std::vector<Entry> history_;
};
