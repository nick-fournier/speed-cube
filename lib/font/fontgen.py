import argparse
from pathlib import Path
from PIL import Image, ImageFont, ImageDraw
import numpy as np

def render_char_data(
    char: str,
    font: ImageFont,
    width: int,
    height: int
    ):
    """
    Render a character to a bitmap array.
    This function creates a bitmap representation of a character using the specified font.
    It converts the character to a binary image and extracts the pixel data.

    Args:
        char (str): The character to render.
        font (ImageFont): The font to use for rendering.
        width (int): The width of the character in pixels.
        height (int): The height of the character in pixels.

    Returns:
        list: A list of bytes representing the bitmap of the character.
    """
    image = Image.new("L", (width, height), 0)
    draw = ImageDraw.Draw(image)
    draw.text((0, 0), char, font=font, fill=255)

    image = image.point(lambda p: 255 if p > 128 else 0).convert("1")
    pixels = np.array(image)

    bytes_per_row = (width + 7) // 8
    bitmap = []

    for y in range(height):
        row = pixels[y]
        for b in range(bytes_per_row):
            byte = 0
            for i in range(8):
                x = b * 8 + i
                if x < width and row[x]:
                    byte |= 1 << (7 - i)
            bitmap.append(byte)

    return bitmap

def generate_font_c(
    font_size: int,
    characters: str,
    font_path: str,
    output_path: str
    ):
    if not Path(font_path).exists():
        raise FileNotFoundError(f"Font file not found: {font_path}")

    if not Path(output_path).exists():
        raise FileNotFoundError(f"Output path not found: {output_path}")

    font = ImageFont.truetype(font_path, font_size)
    
    try:
        width, height = font.getsize("M")

    except AttributeError:
        # Use getbbox() to get the bounding box
        bbox = font.getbbox("M")
        width = bbox[2] - bbox[0]  # Right minus left
        height = bbox[3] - bbox[1]  # Bottom minus top

    output_prefix = f"font{font_size}"
    c_filename = Path(output_path) / f"{output_prefix}.c"
    
    

    with open(c_filename, "w") as c_file:
        c_file.write('#include "fonts.h"\n\n')
        c_file.write(f"static const uint8_t Font{font_size}_Table = {{\n")
        
        for c in characters:
            char_data = render_char_data(c, font, width, height)
            # Write a comment for the character
            c_file.write(f"  /* '{c}' */\n")
            # Write the bytes for this character
            for i in range(0, len(char_data), 12):
                line = ", ".join(f"0x{b:02X}" for b in char_data[i:i+12])
                c_file.write(f"  {line},\n")

        c_file.write("};\n\n")
        
        c_file.write(f"sFONT Font{font_size} [] = {{\n")
        c_file.write(f"  Font{font_size}_Table,\n")
        c_file.write(f"  {width}, /* Width */\n")
        c_file.write(f"  {height}, /* Height */\n")
        c_file.write("};\n")

    print(f"Generated {c_filename}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Generate font data in C format.")

    parser.add_argument(
        "-p", "--font_path",
        metavar="font_path",
        type=str,
        default="/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
        help="Path to the TTF font file"
    )
    parser.add_argument(
        "-s", "--font_size",
        metavar="font_size",
        type=int,
        help="Font size"
    )
    parser.add_argument(
        "-c", "--characters",
        metavar="characters",
        default=r"""!"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~""",
        type=str,
        help="Characters to include in the font"
    )
    parser.add_argument(
        "-o", "--output_path",
        metavar="output_path",
        default=Path(__file__).parent.__str__(),
        help="Output prefix for the generated files"
    )

    args = parser.parse_args()

    # generate_font_c(args.font_size, args.characters, args.font_path, args.output_path)
    
    # Generate font data for all sizes
    sizes = [36, 48, 72]
    for size in sizes:
        generate_font_c(size, args.characters, args.font_path, args.output_path)
