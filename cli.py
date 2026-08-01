"""Command-line interface for inspecting a local or GitHub repository."""

import argparse
from collections.abc import Sequence

from access import DirectoryAccessor, GitHubAccessor
from pipeline import run_pipeline


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Detect AI-generated code in a repository."
    )
    source = parser.add_mutually_exclusive_group(required=True)
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
    parser.add_argument(
        "--ref",
        help="Git branch, tag, or commit to inspect (only valid with --github)",
    )
    return parser


def main(argv: Sequence[str] | None = None) -> None:
    parser = build_parser()
    args = parser.parse_args(argv)

    if args.github is not None:
        accessor = GitHubAccessor(args.github, ref=args.ref)
    else:
        if args.ref is not None:
            parser.error("--ref can only be used with --github")
        accessor = DirectoryAccessor(args.directory)

    run_pipeline(accessor)


if __name__ == "__main__":
    main()
