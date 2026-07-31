package main

import (
	"encoding/json"
	"errors"
	"flag"
	"fmt"
	"io"
	"os"
	"path/filepath"
	"sort"
	"strings"
)

type Report struct {
	Root       string         `json:"root"`
	Files      uint64         `json:"files"`
	Bytes      uint64         `json:"bytes"`
	ByExtension []ExtensionSize `json:"by_extension"`
}

type ExtensionSize struct {
	Extension string `json:"extension"`
	Files     uint64 `json:"files"`
	Bytes     uint64 `json:"bytes"`
}

type extensionStats struct {
	files uint64
	bytes uint64
}

func main() {
	rootFlag := flag.String("root", ".", "directory to walk")
	flag.Parse()

	if flag.NArg() != 0 {
		exitError("unexpected positional arguments")
	}

	root, err := validateRoot(*rootFlag)
	if err != nil {
		exitError(err.Error())
	}

	report, err := walkDirectory(root)
	if err != nil {
		exitError(err.Error())
	}

	encoder := json.NewEncoder(os.Stdout)
	encoder.SetIndent("", "  ")

	if err := encoder.Encode(report); err != nil {
		exitError(fmt.Sprintf("write report: %v", err))
	}
}

func validateRoot(path string) (string, error) {
	path = strings.TrimSpace(path)
	if path == "" {
		return "", errors.New("root directory must not be empty")
	}

	absolute, err := filepath.Abs(path)
	if err != nil {
		return "", fmt.Errorf("resolve root directory: %w", err)
	}

	info, err := os.Stat(absolute)
	if err != nil {
		return "", fmt.Errorf("stat root directory %q: %w", absolute, err)
	}

	if !info.IsDir() {
		return "", fmt.Errorf("root path %q is not a directory", absolute)
	}

	return filepath.Clean(absolute), nil
}

func walkDirectory(root string) (Report, error) {
	stats := make(map[string]extensionStats)
	var totalFiles, totalBytes uint64

	err := filepath.WalkDir(root, func(path string, entry os.DirEntry, walkErr error) error {
		if walkErr != nil {
			return fmt.Errorf("access %q: %w", path, walkErr)
		}
		if entry == nil {
			return fmt.Errorf("missing directory entry for %q", path)
		}
		if entry.IsDir() {
			return nil
		}

		info, err := entry.Info()
		if err != nil {
			return fmt.Errorf("inspect %q: %w", path, err)
		}

		if !info.Mode().IsRegular() {
			return nil
		}
		if info.Size() < 0 {
			return fmt.Errorf("negative file size reported for %q", path)
		}

		size := uint64(info.Size())
		extension := extensionFor(path)
		current := stats[extension]
		current.files++
		current.bytes += size
		stats[extension] = current

		totalFiles++
		totalBytes += size
		return nil
	})
	if err != nil {
		return Report{}, err
	}

	extensions := make([]string, 0, len(stats))
	for extension := range stats {
		extensions = append(extensions, extension)
	}
	sort.Strings(extensions)

	result := Report{
		Root:        root,
		Files:       totalFiles,
		Bytes:       totalBytes,
		ByExtension: make([]ExtensionSize, 0, len(extensions)),
	}

	for _, extension := range extensions {
		value := stats[extension]
		result.ByExtension = append(result.ByExtension, ExtensionSize{
			Extension: extension,
			Files:     value.files,
			Bytes:     value.bytes,
		})
	}

	return result, nil
}

func extensionFor(path string) string {
	name := filepath.Base(path)
	extension := filepath.Ext(name)

	if extension == "" {
		return "[no extension]"
	}

	extension = strings.ToLower(strings.TrimSpace(extension))
	if extension == "." || extension == "" {
		return "[no extension]"
	}

	return extension
}

func exitError(message string) {
	_, _ = fmt.Fprintln(os.Stderr, message)
	os.Exit(1)
}

var _ io.Writer = os.Stdout
