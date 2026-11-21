#!/usr/bin/env python3
"""Generate a silent, seamless MP4 menu background from the existing art.
The Wonderwerk cave and UI are NOT redrawn. Wind, dust and sun are cyclic;
the static original remains the playback fallback in Android.
"""
import argparse
import math
import subprocess
from pathlib import Path

import numpy as np
from PIL import Image, ImageOps, ImageDraw, ImageFilter


def generate(source: Path, destination: Path, width: int, height: int,
             fps: int, seconds: int) -> None:
    with Image.open(source) as original:
        base = np.asarray(ImageOps.fit(
            original.convert("RGB"), (width, height),
            method=Image.Resampling.LANCZOS), dtype=np.float32)
    red, green, blue = base[:, :, 0], base[:, :, 1], base[:, :, 2]
    # Localized foliage movement. Geology and the cave entrance stay anchored.
    vegetation = np.clip((green - .76 * red + 5) / 58, 0, 1)
    vegetation *= np.clip((green - .84 * blue) / 45, 0, 1)
    vegetation *= np.clip((190 - (red + green + blue) / 3) / 95, 0, 1)
    vegetation = np.asarray(
        Image.fromarray(np.uint8(vegetation * 255), "L")
        .filter(ImageFilter.GaussianBlur(6)), dtype=np.float32) / 255
    vegetation = np.clip(vegetation * .68, 0, .52)[:, :, None]
    yy, xx = np.indices((height, width))
    yyf = yy.astype(np.float32)
    xxf = xx.astype(np.float32)
    frames = fps * seconds
    particles = np.random.default_rng(20261121).uniform(0, 1, (105, 4))
    destination.parent.mkdir(parents=True, exist_ok=True)
    process = subprocess.Popen([
        "ffmpeg", "-hide_banner", "-loglevel", "error", "-y",
        "-f", "rawvideo", "-pixel_format", "rgb24",
        "-video_size", f"{width}x{height}", "-framerate", str(fps),
        "-i", "pipe:0", "-an", "-c:v", "libx264", "-preset", "veryfast",
        "-crf", "25", "-pix_fmt", "yuv420p", "-movflags", "+faststart",
        str(destination)
    ], stdin=subprocess.PIPE)
    try:
        for frame in range(frames):
            phase = 2 * math.pi * frame / frames
            shift = np.rint(3 * np.sin(
                phase + yyf * .027 + xxf * .008)).astype(np.int32)
            shifted = base[yy, np.clip(xx + shift, 0, width - 1)]
            scene = base * (1 - vegetation) + shifted * vegetation
            sunlight = .035 * np.sin(phase + xxf / width * 3.4)
            cloud = .023 * np.cos(phase + yyf / height * 3)
            scene = np.clip(scene * (1 + sunlight + cloud)[:, :, None],
                            0, 255).astype("uint8")
            image = Image.fromarray(scene, "RGB").convert("RGBA")
            dust = Image.new("RGBA", (width, height))
            painter = ImageDraw.Draw(dust)
            for i, (px, py, radius, bright) in enumerate(particles):
                x = px * width + math.sin(phase + i * .8) * (7 + radius * 24)
                y = py * height + math.cos(phase + i * .23) * (5 + bright * 18)
                r = max(.6, .5 + radius * 1.8)
                painter.ellipse((x-r, y-r, x+r, y+r),
                                fill=(235, 204, 153, int(17 + 34 * bright)))
            dust = dust.filter(ImageFilter.GaussianBlur(1))
            image = Image.alpha_composite(image, dust)
            process.stdin.write(image.convert("RGB").tobytes())
    except BaseException:
        process.kill()
        raise
    finally:
        if process.stdin and not process.stdin.closed:
            process.stdin.close()
    if process.wait() or not destination.is_file() or destination.stat().st_size < 12000:
        raise RuntimeError("Wonderwerk MP4 rendering failed")
    print(f"MP4 ready: {destination} ({seconds}s, {fps}fps, "
          f"{destination.stat().st_size} bytes)")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", type=Path,
                        default=Path("app/src/main/assets/menu/wonderwerk_main_menu.webp"))
    parser.add_argument("--output", type=Path,
                        default=Path("app/src/main/assets/menu/wonderwerk_loop.mp4"))
    parser.add_argument("--width", type=int, default=960)
    parser.add_argument("--height", type=int, default=540)
    parser.add_argument("--fps", type=int, default=18)
    parser.add_argument("--seconds", type=int, default=6)
    args = parser.parse_args()
    if args.width < 64 or args.height < 64 or not (1 <= args.fps <= 60) or not (1 <= args.seconds <= 30):
        parser.error("Invalid video dimensions or duration")
    generate(args.input, args.output, args.width, args.height,
             args.fps, args.seconds)


if __name__ == "__main__":
    main()
