# e-lab innovations splash

Boot splash: the square logo for 3 seconds, then the wide wordmark with
`www.elabins.com` under it.

| Frame | Image | Flash |
| ----- | ----- | ----- |
| 1 | `icon-96.png` at 96x96 | 622 runs / 2488 B |
| 2 | `logo-wide.png` scaled to 124x38 | 1232 runs / 4928 B |
| 2 | `url.png`, 76x7 | 224 runs / 896 B |

~8.3 KB total, versus ~29 KB for the same three images uncompressed. Everything
is streamed from flash by `display_draw_image_rle()`; there is no framebuffer.

## Regenerating the headers

```bash
cd assets
python3 make_url.py                      # rebuild the URL strip
cd ..
python3 ../../tools/img2c.py assets/icon-96.png   --name elabins_icon --rle -o elabins_icon.h
python3 ../../tools/img2c.py assets/logo-wide.png --name elabins_wide --rle --width 124 -o elabins_wide.h
python3 ../../tools/img2c.py assets/url.png       --name elabins_url  --rle -o elabins_url.h
```

`--width 124` scales height to match the aspect ratio (620x192 → 124x38) and
area-averages while shrinking, which is what keeps the thin "innovations"
strokes readable at 1/5 scale — nearest-neighbor drops half of them.

## Why the URL is an image

`www.elabins.com` measures 165 px in `FreeMono9pt7b` and 181 px in
`FreeSans12pt7b`; the panel is 128 px wide, and neither font scales down. So
[assets/make_url.py](assets/make_url.py) draws the string with a 7 px hand-made
pixel face into `url.png`, and it ships as one more RLE blit. Swap in a smaller
GFX font later and the strip can go away.

## Colors

Images are stored BGR565 because red and blue are swapped on this panel — see
[../image/README.md](../image/README.md) for the details.

## Build and flash

```bash
docker run --rm -v $(pwd)/../..:/src -w /src/examples/logo_splash -it tlsr8232-sdk make

cd ../../tools/tlsr82-debugger-client
python tlsr82-debugger-client.py --serial-port /dev/ttyACM0 \
  write_flash ../../examples/logo_splash/_build/logo_splash.bin
```

Source artwork is copied from the elabins-v3 site (`public/icons/icon-96.png`,
`public/logo-wide.png`).
