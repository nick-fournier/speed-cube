import argparse
from pathlib import Path
from PIL import Image, ImageFont, ImageDraw
import numpy as np


def compute_font_bounds(
    font: ImageFont,
    characters: str
    ):
    """
    Determine the max width and height for the given characters to avoid cut-offs.
    
    This function calculates the maximum width and height of the characters
    using the provided font. It iterates through each character, retrieves its
    bounding box, and updates the maximum width and height accordingly.
    This is useful for ensuring that the rendered characters fit within the
    specified dimensions.
    
    Args:
        font (ImageFont): The font to use for rendering.
        characters (str): The characters to calculate bounds for.
        
    Returns:
        tuple: A tuple containing the maximum width and height of the characters.
    """
    max_width = 0
    # max_height = 0
    ascent, descent = font.getmetrics()  # Get baseline metrics
    max_height = ascent + descent      # Use a consistent height based on baseline

    
    for c in characters:
        bbox = font.getbbox(c)
        if bbox:
            width = bbox[2] - bbox[0]
            # height = bbox[3] - bbox[1]
            max_width = max(max_width, width)
            # max_height = max(max_height, height)
    return max_width, max_height


def render_char_data(
    char: str,
    font: ImageFont,
    width: int
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
    
    ascent, descent = font.getmetrics()
    height = ascent + descent
    
    image = Image.new("L", (width, height), 0)
    draw = ImageDraw.Draw(image)

    bbox = font.getbbox(char)
    x_offset = -bbox[0]  # Adjust for left overhang
    # y_offset = -bbox[1]  # Adjust for top overhang
    y_offset = 0

    draw.text((x_offset, y_offset), char, font=font, fill=255)
    

    # Convert to monochrome binary (1-bit)
    image = image.point(lambda p: 255 if p > 128 else 0).convert("1")
    pixels = np.array(image)

    bytes_per_row = (width + 7) // 8  # Number of bytes per row
    bitmap = []

    for y in range(height):
        row = pixels[y]
        for b in range(bytes_per_row):
            byte = 0
            for i in range(8):
                x = b * 8 + i
                if x < width and row[x]:  
                    byte |= 1 << (7 - i)  # Pack 8 pixels into a byte
            bitmap.append(byte)

    return bitmap, width, height

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

    # Load the font
    font = ImageFont.truetype(font_path, font_size)
    
    # Compute max character width & height
    width, height = compute_font_bounds(font, characters)

    # Sort out the output path
    output_prefix = f"font{font_size}"
    c_filename = Path(output_path) / f"{output_prefix}.c"

    # Generate and write the C file
    with open(c_filename, "w") as c_file:
        c_file.write('#include "fonts.h"\n\n')
        c_file.write(f"const uint8_t Font{font_size}_Table[] = {{\n")
        
        for c in characters:
            char_data, width, height = render_char_data(c, font, width)
            # Write a comment for the character
            c_file.write(f"  /* '{c}' */\n")
            # Write the bytes for this character
            for i in range(0, len(char_data), 12):
                line = ", ".join(f"0x{b:02X}" for b in char_data[i:i+12])
                c_file.write(f"  {line},\n")

        c_file.write("};\n\n")
        
        c_file.write(f"sFONT Font{font_size} = {{\n")
        c_file.write(f"  Font{font_size}_Table,\n")
        c_file.write(f"  {width}, /* Width */\n")
        c_file.write(f"  {height}, /* Height */\n")
        c_file.write("};\n")

    print(f"Generated {c_filename}")

DEFAULT_CHARS = [
    " ", # 0
    "!", # 1
    '"', # 2
    "#", # 3
    "$", # 4
    "%", # 5
    "&", # 6
    "'", # 7
    "(", # 8
    ")", # 9
    "*", # 10
    "+", # 11
    ",", # 12
    "-", # 13
    ".", # 14
    "/", # 15
    "0", # 16
    "1", # 17
    "2", # 18
    "3", # 19
    "4", # 20
    "5", # 21
    "6", # 22
    "7", # 23
    "8", # 24
    "9", # 25
    ":", # 26
    ";", # 27
    "<", # 28
    "=", # 29
    ">", # 30
    "?", # 31
    "@", # 32
    "A", # 33
    "B", # 34
    "C", # 35
    "D", # 36
    "E", # 37
    "F", # 38
    "G", # 39
    "H", # 40
    "I", # 41
    "J", # 42
    "K", # 43
    "L", # 44
    "M", # 45
    "N", # 46
    "O", # 47
    "P", # 48
    "Q", # 49
    "R", # 50
    "S", # 51
    "T", # 52
    "U", # 53
    "V", # 54
    "W", # 55
    "X", # 56
    "Y", # 57
    "Z", # 58
    "[", # 59
    "\\", # 60
    "]", # 61
    "^", # 62
    "_", # 63
    "`", # 64
    "a", # 65
    "b", # 66
    "c", # 67
    "d", # 68
    "e", # 69
    "f", # 70
    "g", # 71
    "h", # 72
    "i", # 73
    "j", # 74
    "k", # 75
    "l", # 76
    "m", # 77
    "n", # 78
    "o", # 79
    "p", # 80
    "q", # 81
    "r", # 82
    "s", # 83
    "t", # 84
    "u", # 85
    "v", # 86
    "w", # 87
    "x", # 88
    "y", # 89
    "z", # 90
    "{", # 91
    "|", # 92
    "}", # 93
    "~", # 94
]

if __name__ == "__main__":
    
    default_chars = "".join(DEFAULT_CHARS)
    
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
        default=default_chars,
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
    sizes = [36, 48, 72, 96, 120, 144]
    for size in sizes:
        generate_font_c(size, args.characters, args.font_path, args.output_path)
