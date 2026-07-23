# -*- coding: utf-8 -*-
from PIL import Image, ImageDraw, ImageFont


def char_to_bytes(ch, font_path, size=16):
    img = Image.new("1", (16, 16), 0)
    draw = ImageDraw.Draw(img)
    font = ImageFont.truetype(font_path, size)
    bbox = draw.textbbox((0, 0), ch, font=font)
    w = bbox[2] - bbox[0]
    h = bbox[3] - bbox[1]
    x = (16 - w) // 2 - bbox[0]
    y = (16 - h) // 2 - bbox[1]
    draw.text((x, y), ch, font=font, fill=1)
    data = []
    for row in range(16):
        b1 = b2 = 0
        for col in range(8):
            if img.getpixel((col, row)):
                b1 |= 0x80 >> col
        for col in range(8, 16):
            if img.getpixel((col, row)):
                b2 |= 0x80 >> (col - 8)
        data.extend([b1, b2])
    return data


def main():
    font_path = r"C:\Windows\Fonts\simsun.ttc"
    chars = "\u9608\u503c\u8bbe\u7f6e\u9ad8\u4f4e\u5f00\u5173\u8fd0\u884c\u6a21\u5f0f\u5f53\u524d\u81ea\u52a8\u624b\u63a7\u5236\u7a97\u6237\u7ffb\u9875\u9009"
    start = 119
    lines = []
    for i, ch in enumerate(chars):
        data = char_to_bytes(ch, font_path)
        hexs = ",".join("0x%02X" % b for b in data)
        lines.append('{%s},/*"%s",%d*/' % (hexs, ch, start + i))
    out = r"d:\Project\STM32F103C8T6\标准库\智慧果园\Tools\page345_font.txt"
    with open(out, "w", encoding="utf-8") as f:
        f.write("\n".join(lines))
    print("wrote", len(lines), "chars")


if __name__ == "__main__":
    main()
