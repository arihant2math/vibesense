def fizzbuzz(limit: int = 100) -> list[str]:
    """Return the FizzBuzz sequence from 1 through limit."""
    if limit < 1:
        return []

    values: list[str] = []
    for number in range(1, limit + 1):
        value = ""
        if number % 3 == 0:
            value += "Fizz"
        if number % 5 == 0:
            value += "Buzz"
        values.append(value or str(number))

    return values


if __name__ == "__main__":
    print("\n".join(fizzbuzz()))
