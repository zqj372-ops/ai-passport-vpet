#!/usr/bin/env python3
"""从源码的字符串字面量里抽取需要的字形,减少嵌入字库的体积。

只扫描双引号字符串(注释里出现的中文不会被算进来),输出一行字符,供 lv_font_conv 用。
"""
import sys


def extract(paths):
    chars = set()
    for path in paths:
        try:
            with open(path, encoding="utf-8") as handle:
                text = handle.read()
        except OSError as error:
            print(f"skip {path}: {error}", file=sys.stderr)
            continue
        in_string = False
        escaped = False
        for char in text:
            if in_string:
                if escaped:
                    escaped = False
                    chars.add(char)
                    continue
                if char == "\\":
                    escaped = True
                    continue
                if char == '"':
                    in_string = False
                    continue
                if ord(char) > 0x7E:
                    chars.add(char)
            elif char == '"':
                in_string = True
    return "".join(sorted(chars))


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("usage: extract_font_symbols.py FILE...", file=sys.stderr)
        sys.exit(2)
    print(extract(sys.argv[1:]))
