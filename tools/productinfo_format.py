"""Read the shared ProductInfo identifier and record format from the C++ header.

The generation and packaging scripts use this helper so the identifier and
format version are defined only once, in src/ProductInfo.h. The identifier
marks ProductInfo records; it does not encrypt or authenticate firmware.
"""

from pathlib import Path
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

    Only single-line macros are supported: comma-separated hexadecimal byte
    literals for the identifier and a decimal integer (0-255) for the version.
    This is a targeted parser, not a general C++ preprocessor.
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
        "PRODUCTINFO_COMMIT_HASH_SIZE",
        "PRODUCTINFO_CRC32_SIZE",
        "PRODUCTINFO_CRC32_OFFSET",
        "PRODUCTINFO_RECORD_SIZE",
    )
    constants = {}
    for name in numeric_names:
        match = re.search(
            r"^[ \t]*#define[ \t]+{}[ \t]+(\d+)[ \t]*$".format(name),
            header,
            re.MULTILINE,
        )
        if match is None:
            raise ValueError("Invalid ProductInfo.h: missing numeric definition {}".format(name))
        constants[name] = int(match[1])

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

    expected_crc_offset = (
        len(magic) + format_info.format_version_size + format_info.software_version_size
        + format_info.build_datetime_size + format_info.branch_size + format_info.commit_hash_size
    )
    if format_info.crc32_offset != expected_crc_offset:
        raise ValueError("PRODUCTINFO_CRC32_OFFSET does not match the declared ProductInfo field lengths")
    if format_info.record_size != format_info.crc32_offset + format_info.crc32_size:
        raise ValueError("PRODUCTINFO_RECORD_SIZE does not match the CRC offset and size")

    return format_info