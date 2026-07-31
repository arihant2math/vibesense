import re
import html


def inline_to_html(text: str) -> str:
    text = html.escape(text, quote=False)
    placeholders = []

    def protect(value: str) -> str:
        placeholders.append(value)
        return f"\x00{len(placeholders) - 1}\x00"

    text = re.sub(r"\*\*(.+?)\*\*|__(.+?)__", lambda m: protect(f"<strong>{m.group(1) or m.group(2)}</strong>"), text)
    text = re.sub(r"(?<!\*)\*(?!\s)(.+?)(?<!\s)\*(?!\*)|(?<!_)_(?!\s)(.+?)(?<!\s)_(?!_)",
                  lambda m: protect(f"<em>{m.group(1) or m.group(2)}</em>"), text)

    for index, value in enumerate(placeholders):
        text = text.replace(f"\x00{index}\x00", value)

    return text


def markdown_to_html(markdown: str) -> str:
    lines = markdown.splitlines()
    output = []
    in_list = False
    list_type = None

    def close_list():
        nonlocal in_list, list_type
        if in_list:
            output.append(f"</{list_type}>")
            in_list = False
            list_type = None

    for line in lines:
        stripped = line.strip()

        if not stripped:
            close_list()
            continue

        heading = re.match(r"^(#{1,6})\s+(.+?)\s*#*$", stripped)
        unordered = re.match(r"^[-*+]\s+(.+)$", stripped)
        ordered = re.match(r"^\d+[.)]\s+(.+)$", stripped)

        if heading:
            close_list()
            level = len(heading.group(1))
            output.append(f"<h{level}>{inline_to_html(heading.group(2))}</h{level}>")
        elif unordered or ordered:
            current_type = "ul" if unordered else "ol"
            content = (unordered or ordered).group(1)

            if not in_list or list_type != current_type:
                close_list()
                output.append(f"<{current_type}>")
                in_list = True
                list_type = current_type

            output.append(f"<li>{inline_to_html(content)}</li>")
        else:
            close_list()
            output.append(f"<p>{inline_to_html(stripped)}</p>")

    close_list()
    return "\n".join(output)


def main() -> None:
    markdown = """# Welcome

This is **bold** and *emphasized* text.

## Features

- Headings
- Lists
- **Strong** and *emphasized* text

1. First item
2. Second item
"""

    print(markdown_to_html(markdown))


if __name__ == "__main__":
    main()

