#!/usr/bin/env python3
"""
ota_config.py - Thêm MD5 header vào file OTA firmware
Viết lại từ Ai-Thinker OTA Config Tools (C#)

Cấu trúc file OTA output:
  [version 5B][chip_type 4B][MD5 32B][URL 128B][firmware data]
"""

import hashlib
import sys
import os


# Chip type mapping
CHIP_TYPES = {
    "ESP": "0001",
    "RTL": "0002",
    "XW":  "0003",
    "TG":  "0004",
    "HI":  "0005",
}

VERSION = "1.0.0"  # 5 bytes


def get_file_md5(filepath):
    """Tính MD5 hash của file, trả về uppercase hex string (32 chars)"""
    md5 = hashlib.md5()
    with open(filepath, "rb") as f:
        for chunk in iter(lambda: f.read(8192), b""):
            md5.update(chunk)
    return md5.hexdigest().upper()


def create_ota_with_md5(input_path, output_path=None, chip_type="ESP"):
    """
    Tạo file OTA mới với header chứa MD5.

    Args:
        input_path: Đường dẫn file OTA gốc (FW_OTA.bin.xz)
        output_path: Đường dẫn file output. Mặc định = input_path (ghi đè)
        chip_type: Loại chip (ESP, RTL, XW, TG, HI)
    """
    if chip_type not in CHIP_TYPES:
        print(f"Error: Unknown chip type '{chip_type}'. Valid: {list(CHIP_TYPES.keys())}")
        sys.exit(1)

    if not os.path.exists(input_path):
        print(f"Error: File not found: {input_path}")
        sys.exit(1)

    if output_path is None:
        output_path = input_path

    # Tính MD5 của file gốc
    md5_hex = get_file_md5(input_path)
    print(f"  Input:     {input_path}")
    print(f"  MD5:       {md5_hex}")
    print(f"  Chip type: {chip_type} ({CHIP_TYPES[chip_type]})")

    # Đọc firmware data
    with open(input_path, "rb") as f:
        firmware_data = f.read()

    # Tạo header
    version_bytes = VERSION.encode("utf-8")[:5].ljust(5, b'\x00')     # 5 bytes
    chip_type_bytes = CHIP_TYPES[chip_type].encode("utf-8")           # 4 bytes
    md5_bytes = md5_hex.encode("utf-8")                               # 32 bytes
    url_bytes = b'\xFF' * 128                                          # 128 bytes

    # Ghép header + firmware
    new_data = version_bytes + chip_type_bytes + md5_bytes + url_bytes + firmware_data

    # Ghi file output
    with open(output_path, "wb") as f:
        f.write(new_data)

    header_size = len(version_bytes) + len(chip_type_bytes) + len(md5_bytes) + len(url_bytes)
    print(f"  Header:    {header_size} bytes")
    print(f"  Firmware:  {len(firmware_data)} bytes")
    print(f"  Total:     {len(new_data)} bytes")
    print(f"  Output:    {output_path}")

    return md5_hex


def main():
    import argparse
    parser = argparse.ArgumentParser(description="Ai-Thinker OTA Config - Add MD5 header to OTA firmware")
    parser.add_argument("input", help="Input OTA firmware file (e.g. FW_OTA.bin.xz)")
    parser.add_argument("-o", "--output", help="Output file path (default: overwrite input)", default=None)
    parser.add_argument("-c", "--chip", help="Chip type (ESP/RTL/XW/TG/HI)", default="ESP")
    args = parser.parse_args()

    create_ota_with_md5(args.input, args.output, args.chip)


if __name__ == "__main__":
    main()
