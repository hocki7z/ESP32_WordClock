"""Read the shared ProductInfo identifier and record format from the C++ header.

The generation and packaging scripts use this helper so the identifier and
format version are defined only once, in src/ProductInfo.h. The identifier
marks ProductInfo records; it does not encrypt or authenticate firmware.
"""

from pathlib import Path
import re


def load_format(project_dir):
    """Load and validate the ProductInfo definitions without compiling C++.

    Args:
        project_dir: Project root containing src/ProductInfo.h.

    Returns:
        A tuple of the 16 raw identifier bytes and integer format version.

    Raises:
        OSError: If the header cannot be read.
        ValueError: If either definition is missing or has an invalid value.

    Only single-line macros are supported: comma-separated hexadecimal byte
    literals for the identifier and a decimal integer (0-255) for the version.
    This is a targeted parser, not a general C++ preprocessor.
    """
    header = (Path(project_dir) / "src" / "ProductInfo.h").read_text(encoding="utf-8")

    # Match each definition independently. Horizontal whitespace allows aligned
    # macros; MULTILINE makes ^ and $ refer to individual header lines.
    identifier = re.search(r'^[ \t]*#define[ \t]+PRODUCTINFO_MAGIC_BYTES[ \t]+([^\r\n]+)$', header, re.MULTILINE)
    version = re.search(r'^[ \t]*#define[ \t]+PRODUCTINFO_FORMAT_VERSION[ \t]+(\d+)[ \t]*$', header, re.MULTILINE)
    if identifier is None or version is None or not 0 <= int(version[1]) <= 255:
        raise ValueError("Invalid ProductInfo.h: expected PRODUCTINFO_MAGIC_BYTES and a byte-sized format version")

    # Accept exactly 16 byte literals (0x00-0xFF), with an optional C++ unsigned
    # suffix U/u. Reject malformed or out-of-range values before serialization.
    tokens = [token.strip() for token in identifier[1].split(",")]
    if len(tokens) != 16 or any(re.fullmatch(r"0[xX][0-9a-fA-F]{1,2}[uU]?", token) is None for token in tokens):
        raise ValueError("PRODUCTINFO_MAGIC_BYTES must contain exactly 16 comma-separated hexadecimal bytes")

    # Remove C++ suffixes and decode hexadecimal values into the actual bytes
    # stored in the binary record, rather than storing their textual spelling.
    return bytes(int(token.rstrip("uU"), 16) for token in tokens), int(version[1])