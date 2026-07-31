import re
import sys
import math
from dataclasses import dataclass
from typing import List, Union

Number = Union[int, float]


class ExpressionError(ValueError):
    """Raised when an arithmetic expression is invalid."""


@dataclass(frozen=True)
class Token:
    kind: str
    value: str
    position: int


_TOKEN_RE = re.compile(
    r"""
    (?P<WHITESPACE>\s+)
  | (?P<NUMBER>
        (?:
            (?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?
        )
    )
  | (?P<OP>\*\*|[+\-*/%()])
  | (?P<INVALID>.)
    """,
    re.VERBOSE,
)


def tokenize(expression: str) -> List[Token]:
    if not isinstance(expression, str):
        raise TypeError("expression must be a string")
    if not expression.strip():
        raise ExpressionError("expression cannot be empty")

    tokens: List[Token] = []

    for match in _TOKEN_RE.finditer(expression):
        kind = match.lastgroup
        value = match.group()
        position = match.start()

        if kind == "WHITESPACE":
            continue
        if kind == "INVALID":
            raise ExpressionError(
                f"invalid character {value!r} at position {position}"
            )

        if kind == "NUMBER":
            try:
                number = float(value)
            except ValueError as exc:
                raise ExpressionError(
                    f"invalid number {value!r} at position {position}"
                ) from exc

            if not math.isfinite(number):
                raise ExpressionError(
                    f"number {value!r} is out of range at position {position}"
                )

            tokens.append(Token("NUMBER", value, position))
        elif kind == "OP":
            tokens.append(
                Token("LPAREN" if value == "(" else
                      "RPAREN" if value == ")" else "OP", value, position)
            )

    tokens.append(Token("EOF", "", len(expression)))
    return tokens


class Parser:
    def __init__(self, tokens: List[Token]):
        self.tokens = tokens
        self.index = 0

    @property
    def current(self) -> Token:
        return self.tokens[self.index]

    def advance(self) -> Token:
        token = self.current
        self.index += 1
        return token

    def parse(self) -> Number:
        result = self.parse_expression(0)
        if self.current.kind != "EOF":
            token = self.current
            raise ExpressionError(
                f"unexpected token {token.value!r} at position {token.position}"
            )
        return result

    def parse_expression(self, minimum_precedence: int) -> Number:
        left = self.parse_prefix()

        precedence = {
            "+": 10,
            "-": 10,
            "*": 20,
            "/": 20,
            "%": 20,
            "**": 30,
        }

        while self.current.kind == "OP":
            operator = self.current.value
            if operator not in precedence:
                break

            current_precedence = precedence[operator]
            if current_precedence < minimum_precedence:
                break

            self.advance()

            if operator == "**":
                right = self.parse_expression(current_precedence)
            else:
                right = self.parse_expression(current_precedence + 1)

            left = self.apply_operator(operator, left, right)

        return left

    def parse_prefix(self) -> Number:
        token = self.current

        if token.kind == "NUMBER":
            self.advance()
            value = float(token.value)
            return int(value) if value.is_integer() and "." not in token.value and "e" not in token.value.lower() else value

        if token.kind == "OP" and token.value in ("+", "-"):
            operator = self.advance().value
            operand = self.parse_expression(25)
            return operand if operator == "+" else -operand

        if token.kind == "LPAREN":
            self.advance()
            if self.current.kind == "RPAREN":
                raise ExpressionError(
                    f"empty parentheses at position {self.current.position}"
                )
            result = self.parse_expression(0)
            if self.current.kind != "RPAREN":
                raise ExpressionError(
                    f"expected ')' at position {self.current.position}"
                )
            self.advance()
            return result

        if token.kind == "EOF":
            raise ExpressionError("unexpected end of expression")

        raise ExpressionError(
            f"expected a number, unary operator, or '(' at position {token.position}"
        )

    @staticmethod
    def apply_operator(operator: str, left: Number, right: Number) -> Number:
        try:
            if operator == "+":
                result = left + right
            elif operator == "-":
                result = left - right
            elif operator == "*":
                result = left * right
            elif operator == "/":
                if right == 0:
                    raise ExpressionError("division by zero")
                result = left / right
            elif operator == "%":
                if right == 0:
                    raise ExpressionError("modulo by zero")
                result = left % right
            elif operator == "**":
                if left == 0 and right < 0:
                    raise ExpressionError("zero cannot be raised to a negative power")
                result = left ** right
            else:
                raise ExpressionError(f"unsupported operator {operator!r}")
        except OverflowError as exc:
            raise ExpressionError("arithmetic result is too large") from exc
        except ZeroDivisionError as exc:
            raise ExpressionError("division by zero") from exc
        except ValueError as exc:
            raise ExpressionError(f"invalid arithmetic operation: {exc}") from exc

        if isinstance(result, float) and not math.isfinite(result):
            raise ExpressionError("arithmetic result is not finite")

        return result


def evaluate(expression: str) -> Number:
    return Parser(tokenize(expression)).parse()


def main() -> int:
    if len(sys.argv) > 1:
        expression = " ".join(sys.argv[1:])
    else:
        expression = sys.stdin.read()

    try:
        result = evaluate(expression)
        print(result)
        return 0
    except (ExpressionError, TypeError) as error:
        print(f"Error: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
