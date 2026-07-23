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
    chars = "阈值设置高低开关运行模式当前自动控制窗户翻页选"
    start = 119
    for i, ch in enumerate(chars):
        data = char_to_bytes(ch, font_path)
        hexs = ",".join("0x%02X" % b for b in data)
        print('{%s},/*"%s",%d*/' % (hexs, ch, start + i))


if __name__ == "__main__":
    main()
