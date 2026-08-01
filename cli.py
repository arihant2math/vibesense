"""Command-line interface for inspecting repositories or source code."""

import argparse
import json
import sys
from collections.abc import Sequence

import requests

from access import DirectoryAccessor, GitHubAccessor
from inference_service import InferenceService
from pipeline import run_pipeline


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Detect AI-generated code in a repository or source file.",
        epilog="Source code is read from stdin when no source option is supplied.",
    )
    source = parser.add_mutually_exclusive_group()
    source.add_argument(
        "--github",
        metavar="OWNER/REPOSITORY",
        help="GitHub repository as owner/repository or a GitHub repository URL",
    )
    source.add_argument(
        "--directory",
        metavar="PATH",
        help="path to a local repository directory",
    )
    source.add_argument(
        "--url",
        metavar="URL",
        help="URL of a UTF-8 source file to classify",
    )
    parser.add_argument(
        "--ref",
        help="Git branch, tag, or commit to inspect (only valid with --github)",
    )
    return parser


def main(argv: Sequence[str] | None = None) -> None:
    parser = build_parser()
    args = parser.parse_args(argv)

    if args.ref is not None and args.github is None:
        parser.error("--ref can only be used with --github")

    if args.github is not None:
        run_pipeline(GitHubAccessor(args.github, ref=args.ref), InferenceService())
    elif args.directory is not None:
        run_pipeline(DirectoryAccessor(args.directory), InferenceService())
    else:
        if args.url is not None:
            try:
                response = requests.get(args.url, timeout=30)
                response.raise_for_status()
            except requests.RequestException as error:
                parser.error(f"could not download {args.url!r}: {error}")
            code = response.text
            name = args.url
        else:
            code = sys.stdin.read()
            name = "<stdin>"

        result = InferenceService().classify(code, name)
        print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
