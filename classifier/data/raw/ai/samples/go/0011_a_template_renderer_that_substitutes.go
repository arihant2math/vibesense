package renderer

import (
	"fmt"
	"strings"
	"unicode"
)

var (
	ErrEmptyTemplate       = fmt.Errorf("template must not be empty")
	ErrNilVariables        = fmt.Errorf("variables map must not be nil")
	ErrMalformedPlaceholder = fmt.Errorf("malformed placeholder")
	ErrMissingVariable     = fmt.Errorf("missing variable")
	ErrUnmatchedClosing    = fmt.Errorf("unmatched closing delimiter")
)

func Render(template string, variables map[string]string) (string, error) {
	if template == "" {
		return "", ErrEmptyTemplate
	}
	if variables == nil {
		return "", ErrNilVariables
	}

	var output strings.Builder
	output.Grow(len(template))

	for i := 0; i < len(template); {
		switch {
		case strings.HasPrefix(template[i:], "{{"):
			start := i
			endOffset := strings.Index(template[i+2:], "}}")
			if endOffset < 0 {
				return "", fmt.Errorf("%w at byte %d: missing closing delimiter", ErrMalformedPlaceholder, start)
			}

			end := i + 2 + endOffset
			name := template[i+2 : end]

			if strings.Contains(name, "{{") || strings.Contains(name, "}}") {
				return "", fmt.Errorf("%w at byte %d: nested delimiter", ErrMalformedPlaceholder, start)
			}
			if !validIdentifier(name) {
				return "", fmt.Errorf("%w at byte %d: invalid name %q", ErrMalformedPlaceholder, start, name)
			}

			value, ok := variables[name]
			if !ok {
				return "", fmt.Errorf("%w %q at byte %d", ErrMissingVariable, name, start)
			}

			output.WriteString(value)
			i = end + 2

		case strings.HasPrefix(template[i:], "}}"):
			return "", fmt.Errorf("%w at byte %d", ErrUnmatchedClosing, i)

		default:
			output.WriteByte(template[i])
			i++
		}
	}

	return output.String(), nil
}

func validIdentifier(name string) bool {
	if name == "" {
		return false
	}

	for i, r := range name {
		if i == 0 {
			if r != '_' && !unicode.IsLetter(r) {
				return false
			}
			continue
		}
		if r != '_' && !unicode.IsLetter(r) && !unicode.IsDigit(r) {
			return false
		}
	}

	return true
}
