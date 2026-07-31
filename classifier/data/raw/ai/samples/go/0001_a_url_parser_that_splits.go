package urlparser

import (
	"errors"
	"fmt"
	"net"
	"net/url"
	"strconv"
	"strings"
	"unicode"
)

type URL struct {
	Scheme string
	Host   string
	Path   string
	Query  string
}

var (
	ErrEmptyURL       = errors.New("URL is empty")
	ErrMissingScheme  = errors.New("URL scheme is missing")
	ErrInvalidScheme  = errors.New("URL scheme is invalid")
	ErrMissingHost    = errors.New("URL host is missing")
	ErrInvalidHost    = errors.New("URL host is invalid")
	ErrInvalidQuery   = errors.New("URL query is invalid")
	ErrUnsupportedURL = errors.New("URL contains unsupported components")
)

func Parse(raw string) (*URL, error) {
	if strings.TrimSpace(raw) == "" {
		return nil, ErrEmptyURL
	}

	for _, r := range raw {
		if unicode.IsControl(r) || unicode.IsSpace(r) {
			return nil, fmt.Errorf("%w: contains whitespace or control characters", ErrUnsupportedURL)
		}
	}

	parsed, err := url.ParseRequestURI(raw)
	if err != nil {
		return nil, fmt.Errorf("parse URL: %w", err)
	}

	if parsed.Scheme == "" {
		return nil, ErrMissingScheme
	}
	if !isValidScheme(parsed.Scheme) {
		return nil, ErrInvalidScheme
	}
	if parsed.Opaque != "" {
		return nil, fmt.Errorf("%w: opaque URLs are not supported", ErrUnsupportedURL)
	}
	if parsed.User != nil {
		return nil, fmt.Errorf("%w: user information is not supported", ErrUnsupportedURL)
	}
	if parsed.Fragment != "" {
		return nil, fmt.Errorf("%w: fragments are not supported", ErrUnsupportedURL)
	}
	if parsed.Host == "" {
		return nil, ErrMissingHost
	}
	if err := validateHost(parsed.Host); err != nil {
		return nil, err
	}

	if _, err := url.ParseQuery(parsed.RawQuery); err != nil {
		return nil, fmt.Errorf("%w: %v", ErrInvalidQuery, err)
	}

	return &URL{
		Scheme: parsed.Scheme,
		Host:   parsed.Host,
		Path:   parsed.EscapedPath(),
		Query:  parsed.RawQuery,
	}, nil
}

func isValidScheme(scheme string) bool {
	if scheme == "" {
		return false
	}
	if !((scheme[0] >= 'a' && scheme[0] <= 'z') ||
		(scheme[0] >= 'A' && scheme[0] <= 'Z')) {
		return false
	}
	for _, c := range scheme[1:] {
		if !((c >= 'a' && c <= 'z') ||
			(c >= 'A' && c <= 'Z') ||
			(c >= '0' && c <= '9') ||
			c == '+' || c == '-' || c == '.') {
			return false
		}
	}
	return true
}

func validateHost(host string) error {
	if host == "" {
		return ErrMissingHost
	}

	hostname := host
	if strings.HasPrefix(host, "[") || strings.Contains(host, ":") {
		var err error
		hostname, _, err = net.SplitHostPort(host)
		if err != nil {
			if strings.HasPrefix(host, "[") && strings.HasSuffix(host, "]") {
				hostname = strings.Trim(host, "[]")
			} else if strings.Count(host, ":") > 1 {
				hostname = host
			} else {
				return fmt.Errorf("%w: %v", ErrInvalidHost, err)
			}
		}
	}

	hostname = strings.Trim(hostname, "[]")
	if hostname == "" {
		return ErrInvalidHost
	}

	if ip := net.ParseIP(hostname); ip != nil {
		return nil
	}

	if len(hostname) > 253 || strings.HasPrefix(hostname, ".") ||
		strings.HasSuffix(hostname, ".") {
		return ErrInvalidHost
	}

	for _, label := range strings.Split(hostname, ".") {
		if label == "" || len(label) > 63 ||
			label[0] == '-' || label[len(label)-1] == '-' {
			return ErrInvalidHost
		}
		for _, c := range label {
			if !((c >= 'a' && c <= 'z') ||
				(c >= 'A' && c <= 'Z') ||
				(c >= '0' && c <= '9') || c == '-') {
				return ErrInvalidHost
			}
		}
	}

	if port := portFromHost(host); port != "" {
		n, err := strconv.Atoi(port)
		if err != nil || n < 1 || n > 65535 {
			return ErrInvalidHost
		}
	}

	return nil
}

func portFromHost(host string) string {
	if _, port, err := net.SplitHostPort(host); err == nil {
		return port
	}
	return ""
}
