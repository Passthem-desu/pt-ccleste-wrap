build:
    nu ./scripts/build.nu

generate-bitmap:
    uv run ./scripts/bitmap_convert.py
