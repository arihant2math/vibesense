package fixedpoint

import (
	"errors"
	"math"
	"strconv"
	"strings"
)

const Scale int64 = 100

var (
	ErrOverflow       = errors.New("fixed-point decimal overflow")
	ErrInvalidDecimal = errors.New("invalid decimal")
)

type Decimal struct {
	value int64
}

func NewFromInt(value int64) Decimal {
	return Decimal{value: value * Scale}
}

func NewFromMinor(value int64) Decimal {
	return Decimal{value: value}
}

func Parse(input string) (Decimal, error) {
	input = strings.TrimSpace(input)
	if input == "" {
		return Decimal{}, ErrInvalidDecimal
	}

	negative := false
	if input[0] == '-' || input[0] == '+' {
		negative = input[0] == '-'
		input = input[1:]
	}
	if input == "" {
		return Decimal{}, ErrInvalidDecimal
	}

	parts := strings.Split(input, ".")
	if len(parts) > 2 || parts[0] == "" && len(parts) == 1 {
		return Decimal{}, ErrInvalidDecimal
	}

	whole := parts[0]
	if whole == "" {
		whole = "0"
	}
	if len(parts) == 2 && len(parts[1]) > 2 {
		return Decimal{}, ErrInvalidDecimal
	}

	fraction := "0"
	if len(parts) == 2 {
		fraction = parts[1]
	}
	for len(fraction) < 2 {
		fraction += "0"
	}

	w, err := strconv.ParseInt(whole, 10, 64)
	if err != nil || w > math.MaxInt64/Scale {
		return Decimal{}, ErrInvalidDecimal
	}
	f, err := strconv.ParseInt(fraction, 10, 64)
	if err != nil {
		return Decimal{}, ErrInvalidDecimal
	}

	value := w*Scale + f
	if negative {
		value = -value
	}
	return Decimal{value: value}, nil
}

func (d Decimal) Add(other Decimal) (Decimal, error) {
	if other.value > 0 && d.value > math.MaxInt64-other.value {
		return Decimal{}, ErrOverflow
	}
	if other.value < 0 && d.value < math.MinInt64-other.value {
		return Decimal{}, ErrOverflow
	}
	return Decimal{value: d.value + other.value}, nil
}

func (d Decimal) Multiply(other Decimal) (Decimal, error) {
	if d.value == 0 || other.value == 0 {
		return Decimal{}, nil
	}
	if d.value > math.MaxInt64/other.value ||
		d.value < math.MinInt64/other.value {
		return Decimal{}, ErrOverflow
	}

	product := d.value * other.value
	result := product / Scale
	remainder := product % Scale

	if abs(remainder)*2 >= Scale {
		if product > 0 {
			if result == math.MaxInt64 {
				return Decimal{}, ErrOverflow
			}
			result++
		} else {
			if result == math.MinInt64 {
				return Decimal{}, ErrOverflow
			}
			result--
		}
	}

	return Decimal{value: result}, nil
}

func (d Decimal) MinorUnits() int64 {
	return d.value
}

func (d Decimal) String() string {
	sign := ""
	value := d.value
	if value < 0 {
		sign = "-"
		value = -value
	}

	return sign + strconv.FormatInt(value/Scale, 10) + "." +
		strings.Repeat("0", 2-len(strconv.FormatInt(value%Scale, 10))) +
		strconv.FormatInt(value%Scale, 10)
}

func abs(value int64) int64 {
	if value < 0 {
		return -value
	}
	return value
}
