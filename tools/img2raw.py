#!/usr/bin/env python3
"""
Convertitore di immagini in formato RAW RGB565 per PicoCalc

Converte immagini PNG, JPG, BMP, ecc. in formato RAW RGB565
compatibile con il comando 'showimg' del PicoCalc.

Formato file RAW:
  - Byte 0-1: Larghezza (16-bit little-endian)
  - Byte 2-3: Altezza (16-bit little-endian)
  - Byte 4+:  Dati pixel in formato RGB565 (2 byte per pixel)

Uso:
  python3 img2raw.py input.png output.raw [--width WIDTH] [--height HEIGHT]

Esempio:
  python3 img2raw.py foto.jpg foto.raw --width 320 --height 240
"""

import sys
import struct
import argparse
from PIL import Image

def rgb888_to_rgb565(r, g, b):
    """Converte RGB888 (8-bit per canale) in RGB565 (16-bit totale)"""
    r5 = (r >> 3) & 0x1F  # 5 bit per rosso
    g6 = (g >> 2) & 0x3F  # 6 bit per verde
    b5 = (b >> 3) & 0x1F  # 5 bit per blu
    return (r5 << 11) | (g6 << 5) | b5

def convert_image(input_path, output_path, max_width=None, max_height=None):
    """Converte un'immagine in formato RAW RGB565"""
    try:
        # Apri l'immagine
        img = Image.open(input_path)
        print(f"Immagine originale: {img.size[0]}x{img.size[1]} pixel, modo: {img.mode}")

        # Converti in RGB se necessario
        if img.mode != 'RGB':
            print(f"Conversione da {img.mode} a RGB...")
            img = img.convert('RGB')

        # Ridimensiona se necessario
        if max_width or max_height:
            original_size = img.size
            if max_width and img.size[0] > max_width:
                aspect_ratio = img.size[1] / img.size[0]
                new_width = max_width
                new_height = int(max_width * aspect_ratio)
                img = img.resize((new_width, new_height), Image.Resampling.LANCZOS)
                print(f"Ridimensionata da {original_size[0]}x{original_size[1]} a {new_width}x{new_height}")

            if max_height and img.size[1] > max_height:
                aspect_ratio = img.size[0] / img.size[1]
                new_height = max_height
                new_width = int(max_height * aspect_ratio)
                img = img.resize((new_width, new_height), Image.Resampling.LANCZOS)
                print(f"Ridimensionata a {new_width}x{new_height}")

        width, height = img.size

        # Verifica limiti PicoCalc (320x320)
        if width > 320 or height > 320:
            print(f"ATTENZIONE: L'immagine è più grande del display (320x320)")
            print(f"L'immagine sarà centrata e tagliata se necessario")

        # Apri file di output
        with open(output_path, 'wb') as f:
            # Scrivi header (larghezza e altezza in little-endian)
            f.write(struct.pack('<HH', width, height))

            # Converti e scrivi pixel
            pixels = img.load()
            pixel_count = 0

            for y in range(height):
                for x in range(width):
                    r, g, b = pixels[x, y]
                    rgb565 = rgb888_to_rgb565(r, g, b)
                    f.write(struct.pack('<H', rgb565))
                    pixel_count += 1

                # Mostra progresso
                if (y + 1) % 50 == 0:
                    print(f"Processate {y + 1}/{height} righe...")

        file_size = 4 + (pixel_count * 2)
        print(f"\n✓ Conversione completata!")
        print(f"  Output: {output_path}")
        print(f"  Dimensioni: {width}x{height} pixel")
        print(f"  File size: {file_size} bytes ({file_size/1024:.2f} KB)")
        print(f"\nPer visualizzare sul PicoCalc:")
        print(f"  1. Copia {output_path} sulla SD card")
        print(f"  2. Esegui: showimg {output_path.split('/')[-1]}")

        return True

    except FileNotFoundError:
        print(f"Errore: File '{input_path}' non trovato.")
        return False
    except Exception as e:
        print(f"Errore durante la conversione: {e}")
        return False

def main():
    parser = argparse.ArgumentParser(
        description='Converti immagini in formato RAW RGB565 per PicoCalc',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__
    )
    parser.add_argument('input', help='File immagine di input (PNG, JPG, BMP, ecc.)')
    parser.add_argument('output', help='File RAW di output')
    parser.add_argument('--width', type=int, help='Larghezza massima (ridimensiona se necessario)')
    parser.add_argument('--height', type=int, help='Altezza massima (ridimensiona se necessario)')

    args = parser.parse_args()

    # Verifica che PIL/Pillow sia installato
    try:
        import PIL
    except ImportError:
        print("Errore: Pillow non è installato.")
        print("Installa con: pip install Pillow")
        sys.exit(1)

    # Converti immagine
    success = convert_image(args.input, args.output, args.width, args.height)
    sys.exit(0 if success else 1)

if __name__ == '__main__':
    main()
