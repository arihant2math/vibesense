import java.io.BufferedReader
import java.io.InputStreamReader

private class Evaluator(private val source: String) {
    private var index = 0
    private var values = DoubleArray(16)
    private var valueTop = 0
    private var operators = CharArray(16)
    private var operatorTop = 0

    fun evaluate(): Double {
        var expectOperand = true

        while (true) {
            skipWhitespace()
            if (index >= source.length) break

            val c = source[index]

            if (expectOperand) {
                when {
                    c == '+' || c == '-' -> {
                        pushOperator(if (c == '+') 'p' else 'u')
                        index++
                    }
                    c == '(' -> {
                        pushOperator(c)
                        index++
                    }
                    c.isDigit() || c == '.' -> {
                        pushValue(readNumber())
                        expectOperand = false
                    }
                    else -> error("Expected operand")
                }
            } else {
                when {
                    c == '+' || c == '-' || c == '*' || c == '/' -> {
                        val precedence = precedence(c)
                        while (
                            operatorTop > 0 &&
                            operators[operatorTop - 1] != '(' &&
                            precedence(operators[operatorTop - 1]) >= precedence
                        ) {
                            applyTopOperator()
                        }
                        pushOperator(c)
                        index++
                        expectOperand = true
                    }
                    c == ')' -> {
                        while (operatorTop > 0 && operators[operatorTop - 1] != '(') {
                            applyTopOperator()
                        }
                        if (operatorTop == 0) error("Unmatched ')'")
                        operatorTop--
                        index++
                    }
                    else -> error("Unexpected character")
                }
            }
        }

        if (expectOperand && valueTop == 0) error("Empty expression")
        if (expectOperand) error("Expected operand")

        while (operatorTop > 0) {
            if (operators[operatorTop - 1] == '(') error("Unmatched '('")
            applyTopOperator()
        }

        if (valueTop != 1) error("Invalid expression")
        return values[0]
    }

    private fun readNumber(): Double {
        val start = index
        var hasDigits = false

        while (index < source.length && source[index].isDigit()) {
            hasDigits = true
            index++
        }

        if (index < source.length && source[index] == '.') {
            index++
            while (index < source.length && source[index].isDigit()) {
                hasDigits = true
                index++
            }
        }

        if (!hasDigits) error("Invalid number")

        if (index < source.length && (source[index] == 'e' || source[index] == 'E')) {
            index++
            if (index < source.length && (source[index] == '+' || source[index] == '-')) {
                index++
            }
            val exponentStart = index
            while (index < source.length && source[index].isDigit()) index++
            if (exponentStart == index) error("Invalid exponent")
        }

        return source.substring(start, index).toDouble()
    }

    private fun applyTopOperator() {
        val op = operators[--operatorTop]
        when (op) {
            'u' -> values[valueTop - 1] = -values[valueTop - 1]
            'p' -> Unit
            '+', '-', '*', '/' -> {
                if (valueTop < 2) error("Missing operand")
                val right = values[--valueTop]
                val left = values[--valueTop]
                values[valueTop++] = when (op) {
                    '+' -> left + right
                    '-' -> left - right
                    '*' -> left * right
                    else -> left / right
                }
            }
        }
    }

    private fun precedence(op: Char): Int = when (op) {
        'u', 'p' -> 3
        '*', '/' -> 2
        '+', '-' -> 1
        else -> 0
    }

    private fun pushValue(value: Double) {
        if (valueTop == values.size) values = values.copyOf(values.size * 2)
        values[valueTop++] = value
    }

    private fun pushOperator(operator: Char) {
        if (operatorTop == operators.size) operators = operators.copyOf(operators.size * 2)
        operators[operatorTop++] = operator
    }

    private fun skipWhitespace() {
        while (index < source.length && source[index].isWhitespace()) index++
    }
}

fun main() {
    val input = BufferedReader(InputStreamReader(System.`in`)).readText()
    println(Evaluator(input).evaluate())
}
