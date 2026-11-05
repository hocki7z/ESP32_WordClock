"""Read the shared ProductInfo identifier and record format from the C++ header.

The generation and packaging scripts use this helper so the identifier and
format version are defined only once, in src/ProductInfo.h. The identifier
marks ProductInfo records; it does not encrypt or authenticate firmware.
"""

from pathlib import Path
import ast
import re
from typing import NamedTuple


class ProductInfoFormat(NamedTuple):
    """Decoded ProductInfo constants and derived field offsets."""

    magic: bytes
    format_version: int
    format_version_size: int
    software_version_size: int
    build_datetime_size: int
    branch_size: int
    short_branch_name_size: int
    commit_hash_size: int
    crc32_size: int
    crc32_offset: int
    record_size: int

    @property
    def format_version_offset(self):
        return len(self.magic)

    @property
    def software_version_offset(self):
        return self.format_version_offset + self.format_version_size

    @property
    def build_datetime_offset(self):
        return self.software_version_offset + self.software_version_size

    @property
    def branch_offset(self):
        return self.build_datetime_offset + self.build_datetime_size

    @property
    def commit_hash_offset(self):
        return self.branch_offset + self.branch_size


def _read_integer_macros(header):
    """Collect ProductInfo #define expressions, joining C continuation lines."""
    macros = {}
    lines = header.splitlines()
    index = 0
    while index < len(lines):
        line = lines[index]
        match = re.match(r"^[ \t]*#define[ \t]+(PRODUCTINFO_[A-Z0-9_]+)[ \t]+(.*)$", line)
        index += 1
        if match is None:
            continue

        name, expression = match.groups()
        while expression.rstrip().endswith("\\"):
            expression = expression.rstrip()[:-1] + " "
            if index >= len(lines):
                raise ValueError("Unterminated continued definition for {}".format(name))
            expression += lines[index].strip()
            index += 1
        macros[name] = expression.strip()
    return macros


def _evaluate_integer_macro(name, macros, cache, active=()):
    """Evaluate integer literals, macro references, parentheses, unary +, addition, and multiplication."""
    if name in cache:
        return cache[name]
    if name in active:
        raise ValueError("Recursive ProductInfo macro definition: {}".format(name))
    expression = macros.get(name)
    if expression is None:
        raise ValueError("Invalid ProductInfo.h: missing numeric definition {}".format(name))

    # Reject every token not needed by these simple integer definitions before
    # parsing. This intentionally does not implement the C preprocessor.
    tokens = re.findall(r"[A-Za-z_][A-Za-z0-9_]*|\d+|[()+*]", expression)
    if "".join(tokens) != re.sub(r"\s+", "", expression):
        raise ValueError("Unsupported expression for {}: {}".format(name, expression))
    try:
        parsed = ast.parse(expression, mode="eval")
    except SyntaxError as error:
        raise ValueError("Invalid integer expression for {}: {}".format(name, expression)) from error

    def evaluate(node):
        if isinstance(node, ast.Expression):
            return evaluate(node.body)
        if isinstance(node, ast.Constant) and isinstance(node.value, int) and not isinstance(node.value, bool):
            return node.value
        if isinstance(node, ast.Name) and node.id.startswith("PRODUCTINFO_"):
            return _evaluate_integer_macro(node.id, macros, cache, active + (name,))
        if isinstance(node, ast.BinOp) and isinstance(node.op, ast.Add):
            return evaluate(node.left) + evaluate(node.right)
        if isinstance(node, ast.BinOp) and isinstance(node.op, ast.Mult):
            return evaluate(node.left) * evaluate(node.right)
        if isinstance(node, ast.UnaryOp) and isinstance(node.op, ast.UAdd):
            return evaluate(node.operand)
        raise ValueError("Unsupported integer expression for {}: {}".format(name, expression))

    value = evaluate(parsed)
    if value < 0:
        raise ValueError("ProductInfo definition {} must not be negative".format(name))
    cache[name] = value
    return value


def load_format(project_dir):
    """Load and validate the ProductInfo definitions without compiling C++.

    Args:
        project_dir: Project root containing src/ProductInfo.h.

    Returns:
        ProductInfoFormat containing the decoded identifier, format version,
        field lengths, CRC location, record length, and derived field offsets.

    Raises:
        OSError: If the header cannot be read.
        ValueError: If either definition is missing or has an invalid value.

    Numeric definitions may use decimal literals, parentheses, unary plus,
    addition, multiplication, and references to other PRODUCTINFO_* definitions. This is a
    bounded parser for the header's simple layout expressions, not a general
    C++ preprocessor.
    """
    header = (Path(project_dir) / "src" / "ProductInfo.h").read_text(encoding="utf-8")

    # Match each definition independently. Horizontal whitespace allows aligned
    # macros; MULTILINE makes ^ and $ refer to individual header lines.
    identifier = re.search(r'^[ \t]*#define[ \t]+PRODUCTINFO_MAGIC_BYTES[ \t]+([^\r\n]+)$', header, re.MULTILINE)
    numeric_names = (
        "PRODUCTINFO_MAGIC_SIZE",
        "PRODUCTINFO_FORMAT_VERSION",
        "PRODUCTINFO_FORMAT_VERSION_SIZE",
        "PRODUCTINFO_SOFTWARE_VERSION_SIZE",
        "PRODUCTINFO_BUILD_DATETIME_SIZE",
        "PRODUCTINFO_BRANCH_SIZE",
        "PRODUCTINFO_SHORT_BRANCH_NAME_SIZE",
        "PRODUCTINFO_COMMIT_HASH_SIZE",
        "PRODUCTINFO_CRC32_SIZE",
        "PRODUCTINFO_CRC32_OFFSET",
        "PRODUCTINFO_RECORD_SIZE",
    )
    macros = _read_integer_macros(header)
    constants = {}
    cache = {}
    for name in numeric_names:
        constants[name] = _evaluate_integer_macro(name, macros, cache)

    version = constants["PRODUCTINFO_FORMAT_VERSION"]
    if identifier is None or not 0 <= version <= 255:
        raise ValueError("Invalid ProductInfo.h: expected a byte-sized format version and magic byte list")

    # Accept exactly 16 byte literals (0x00-0xFF), with an optional C++ unsigned
    # suffix U/u. Reject malformed or out-of-range values before serialization.
    tokens = [token.strip() for token in identifier[1].split(",")]
    magic_size = constants["PRODUCTINFO_MAGIC_SIZE"]
    if len(tokens) != magic_size or any(re.fullmatch(r"0[xX][0-9a-fA-F]{1,2}[uU]?", token) is None for token in tokens):
        raise ValueError("PRODUCTINFO_MAGIC_BYTES must contain exactly PRODUCTINFO_MAGIC_SIZE hexadecimal bytes")

    # Remove C++ suffixes and decode hexadecimal values into the actual bytes
    # stored in the binary record, rather than storing their textual spelling.
    magic = bytes(int(token.rstrip("uU"), 16) for token in tokens)
    format_info = ProductInfoFormat(
        magic=magic,
        format_version=version,
        format_version_size=constants["PRODUCTINFO_FORMAT_VERSION_SIZE"],
        software_version_size=constants["PRODUCTINFO_SOFTWARE_VERSION_SIZE"],
        build_datetime_size=constants["PRODUCTINFO_BUILD_DATETIME_SIZE"],
        branch_size=constants["PRODUCTINFO_BRANCH_SIZE"],
        short_branch_name_size=constants["PRODUCTINFO_SHORT_BRANCH_NAME_SIZE"],
        commit_hash_size=constants["PRODUCTINFO_COMMIT_HASH_SIZE"],
        crc32_size=constants["PRODUCTINFO_CRC32_SIZE"],
        crc32_offset=constants["PRODUCTINFO_CRC32_OFFSET"],
        record_size=constants["PRODUCTINFO_RECORD_SIZE"],
    )

    if format_info.format_version_size != 1:
        raise ValueError("The ProductInfo format version must occupy exactly one byte")
    if format_info.software_version_size != 3:
        raise ValueError("The ProductInfo software version must contain exactly three bytes")
    if format_info.crc32_size != 4:
        raise ValueError("The ProductInfo CRC-32 field must occupy exactly four bytes")
    if format_info.short_branch_name_size * 4 >= format_info.branch_size:
        raise ValueError("PRODUCTINFO_BRANCH_SIZE must fit every supported UTF-8 branch name plus NUL")

    expected_crc_offset = (
        len(magic) + format_info.format_version_size + format_info.software_version_size
        + format_info.build_datetime_size + format_info.branch_size + format_info.commit_hash_size
    )
    if format_info.crc32_offset != expected_crc_offset:
        raise ValueError("PRODUCTINFO_CRC32_OFFSET does not match the declared ProductInfo field lengths")
    if format_info.record_size != format_info.crc32_offset + format_info.crc32_size:
        raise ValueError("PRODUCTINFO_RECORD_SIZE does not match the CRC offset and size")

    return format_info