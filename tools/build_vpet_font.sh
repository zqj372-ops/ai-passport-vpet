#!/usr/bin/env bash
# 生成 main/vpet_font_16.c:从 Noto Sans CJK SC(OFL)抽取项目实际用到的字形。
#
# 用法:
#   tools/build_vpet_font.sh              # 用系统自带 Noto Sans CJK
#   VPET_FONT_SRC=/path/NotoSansSC.otf tools/build_vpet_font.sh
#
# 依赖:node(跑 lv_font_conv)、python3 + fontTools(从 .ttc 里拆出单字体)、
#      tools/node_modules/lv_font_conv(npm install lv_font_conv@1.5.3)
set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
repo="${root}/firmware-baseline"
tools_dir="${root}/tools"
font_src="${VPET_FONT_SRC:-/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc}"
face_index="${VPET_FONT_FACE_INDEX:-2}"     # Noto Sans CJK SC
size="${VPET_FONT_SIZE:-16}"
bpp="${VPET_FONT_BPP:-4}"
output="${repo}/main/vpet_font_16.c"
lv_font_conv="${tools_dir}/node_modules/.bin/lv_font_conv"

[[ -x "${lv_font_conv}" ]] || {
    echo "缺少 lv_font_conv,请先执行:(cd tools && npm install lv_font_conv@1.5.3)" >&2
    exit 1
}
[[ -f "${font_src}" ]] || {
    echo "找不到字体文件 ${font_src};可用 VPET_FONT_SRC 指定一个 CJK 字体(otf/ttf)" >&2
    exit 1
}

# 1) .ttc 是多字体集合,先拆出简体中文那一支
work_font="${font_src}"
if [[ "${font_src}" == *.ttc ]]; then
    work_font="/tmp/vpet-font-sc.otf"
    if [[ ! -f "${work_font}" || "${font_src}" -nt "${work_font}" ]]; then
        echo "==> 从 ${font_src} 拆出 face ${face_index} -> ${work_font}"
        venv="/tmp/vpet-fontenv"
        if [[ ! -x "${venv}/bin/python" ]]; then
            python3 -m venv "${venv}"
            "${venv}/bin/pip" install --quiet fonttools
        fi
        "${venv}/bin/python" - "$font_src" "$face_index" "$work_font" <<'PY'
import sys
from fontTools.ttLib import TTCollection

source, index, target = sys.argv[1], int(sys.argv[2]), sys.argv[3]
collection = TTCollection(source)
collection.fonts[index].save(target)
print(f"saved face {index} -> {target}")
PY
    fi
fi

# 2) 收集源码字符串里出现的非 ASCII 字形
symbols="$(python3 "${tools_dir}/extract_font_symbols.py" \
    "${repo}/main/main.c" \
    "${repo}/main/vpet_view.c" \
    "${repo}/main/vpet_app.c" \
    "${repo}/main/vpet_species.c" \
    "${repo}/main/vpet_model.c" \
    "${repo}/main/vpet_battle.c" \
    "${repo}/main/vpet_link.c")"
echo "==> 需要的中文字形: ${#symbols} 个"

# 3) 生成 LVGL 字体
"${lv_font_conv}" \
    --font "${work_font}" \
    --range 0x20-0x7E \
    --symbols "${symbols}" \
    --size "${size}" \
    --bpp "${bpp}" \
    --format lvgl \
    --lv-include lvgl.h \
    --lv-font-name vpet_font_16 \
    --no-compress \
    --force-fast-kern-format \
    -o "${output}"

echo "==> 已生成 ${output} ($(wc -c < "${output}") 字节)"

# 4) 自检:源码里用到的字必须都能在生成的字体里找到,否则上机就是空白方块
python3 - "$output" "$symbols" <<'PY'
import re
import sys

font_path, symbols = sys.argv[1], sys.argv[2]
with open(font_path, encoding="utf-8", errors="ignore") as handle:
    text = handle.read()
present = {int(code, 16) for code in re.findall(r"U\+([0-9A-F]{4,6})", text)}
missing = sorted({char for char in symbols if ord(char) not in present})
if missing:
    print("缺少字形(界面会显示空白): " + " ".join(missing), file=sys.stderr)
    sys.exit(1)
print(f"==> 字形自检通过: {len(symbols)} 个非 ASCII 字形全部命中")
PY
