#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define DESCRIPTION_SIZE 128

typedef enum {
    DEPOSIT,
    WITHDRAWAL
} TransactionType;

typedef struct {
    time_t timestamp;
    TransactionType type;
    double amount;
    double balance;
    char description[DESCRIPTION_SIZE];
} Transaction;

typedef struct {
    Transaction *transactions;
    size_t count;
    size_t capacity;
    double balance;
} Account;

static void account_init(Account *account) {
    account->transactions = NULL;
    account->count = 0;
    account->capacity = 0;
    account->balance = 0.0;
}

static void account_free(Account *account) {
    free(account->transactions);
    account->transactions = NULL;
    account->count = 0;
    account->capacity = 0;
}

static int account_reserve(Account *account, size_t capacity) {
    if (capacity <= account->capacity) {
        return 1;
    }

    Transaction *transactions = realloc(
        account->transactions,
        capacity * sizeof(*transactions)
    );

    if (transactions == NULL) {
        return 0;
    }

    account->transactions = transactions;
    account->capacity = capacity;
    return 1;
}

static int account_record(
    Account *account,
    TransactionType type,
    double amount,
    const char *description
) {
    if (amount <= 0.0) {
        return 0;
    }

    if (type == WITHDRAWAL && amount > account->balance) {
        return 0;
    }

    if (account->count == account->capacity) {
        size_t new_capacity = account->capacity == 0
            ? 8
            : account->capacity * 2;

        if (!account_reserve(account, new_capacity)) {
            return 0;
        }
    }

    if (type == DEPOSIT) {
        account->balance += amount;
    } else {
        account->balance -= amount;
    }

    Transaction *transaction = &account->transactions[account->count++];

    transaction->timestamp = time(NULL);
    transaction->type = type;
    transaction->amount = amount;
    transaction->balance = account->balance;

    if (description != NULL) {
        snprintf(
            transaction->description,
            sizeof(transaction->description),
            "%s",
            description
        );
    } else {
        transaction->description[0] = '\0';
    }

    return 1;
}

static int account_deposit(
    Account *account,
    double amount,
    const char *description
) {
    return account_record(account, DEPOSIT, amount, description);
}

static int account_withdraw(
    Account *account,
    double amount,
    const char *description
) {
    return account_record(account, WITHDRAWAL, amount, description);
}

static void print_transaction(const Transaction *transaction, size_t index) {
    char timestamp[32];
    struct tm *local_time = localtime(&transaction->timestamp);

    if (local_time != NULL) {
        strftime(
            timestamp,
            sizeof(timestamp),
            "%Y-%m-%d %H:%M:%S",
            local_time
        );
    } else {
        snprintf(timestamp, sizeof(timestamp), "unknown");
    }

    printf(
        "%zu | %s | %-9s | %10.2f | %10.2f | %s\n",
        index + 1,
        timestamp,
        transaction->type == DEPOSIT ? "DEPOSIT" : "WITHDRAW",
        transaction->amount,
        transaction->balance,
        transaction->description
    );
}

static void account_print_history(const Account *account) {
    size_t i;

    if (account->count == 0) {
        printf("No transactions.\n");
        return;
    }

    printf(
        " # | Date and time        | Type      |     Amount |    Balance | Description\n"
        "---+----------------------+-----------+------------+------------+-------------\n"
    );

    for (i = 0; i < account->count; ++i) {
        print_transaction(&account->transactions[i], i);
    }

    printf("\nCurrent balance: %.2f\n", account->balance);
}

static void print_menu(void) {
    printf(
        "\nBank Account Ledger\n"
        "1. Deposit\n"
        "2. Withdraw\n"
        "3. Show balance\n"
        "4. Show history\n"
        "5. Exit\n"
        "Choice: "
    );
}

static void read_line(char *buffer, size_t size) {
    if (fgets(buffer, (int)size, stdin) == NULL) {
        buffer[0] = '\0';
        return;
    }

    buffer[strcspn(buffer, "\n")] = '\0';
}

static int read_amount(double *amount) {
    char input[64];
    char extra;

    read_line(input, sizeof(input));

    if (sscanf(input, "%lf %c", amount, &extra) != 1) {
        return 0;
    }

    return *amount > 0.0;
}

int main(void) {
    Account account;
    char input[32];
    int choice;

    account_init(&account);

    for (;;) {
        print_menu();
        read_line(input, sizeof(input));

        if (sscanf(input, "%d", &choice) != 1) {
            printf("Invalid choice.\n");
            continue;
        }

        if (choice == 1 || choice == 2) {
            double amount;
            char description[DESCRIPTION_SIZE];

            printf("Amount: ");
            if (!read_amount(&amount)) {
                printf("Invalid amount.\n");
                continue;
            }

            printf("Description: ");
            read_line(description, sizeof(description));

            if (choice == 1) {
                if (account_deposit(&account, amount, description)) {
                    printf("Deposit recorded. Balance: %.2f\n", account.balance);
                } else {
                    printf("Unable to record deposit.\n");
                }
            } else {
                if (account_withdraw(&account, amount, description)) {
                    printf("Withdrawal recorded. Balance: %.2f\n", account.balance);
                } else {
                    printf("Insufficient funds or invalid amount.\n");
                }
            }
        } else if (choice == 3) {
            printf("Current balance: %.2f\n", account.balance);
        } else if (choice == 4) {
            account_print_history(&account);
        } else if (choice == 5) {
            break;
        } else {
            printf("Invalid choice.\n");
        }
    }

    account_free(&account);
    return 0;
}
