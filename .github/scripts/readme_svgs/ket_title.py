# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
"""Generate the Ket README logo: a cat face built from Dirac ket symbols.

The cat is assembled from three ket symbols (a vertical bar followed by an
angle bracket, |⟩):

  * two kets for the ears, rotated so each angle bracket points up and out,
  * one ket for the nose, rotated so the angle bracket points down,

plus two filled dots for the eyes and six lines for the whiskers.

Everything is drawn in the current GitHub text color. Because a static SVG
cannot read GitHub's theme directly, the fill/stroke colors are defined with a
CSS class that is overridden inside an `@media (prefers-color-scheme: dark)`
block, so the logo follows light/dark mode automatically.
"""

import argparse
from pathlib import Path

W, H = 360, 300

TEXT_FILL_LIGHT = "#1f2328"  # GitHub fgColor-default (light mode)
TEXT_FILL_DARK = "#d1d7e0"   # GitHub fgColor-default (dark mode)

# Ket strokes use a non-scaling stroke (see ket_group), so this width is the
# true rendered thickness regardless of each ket's scale — keeping the ears
# and the nose the same line weight.
STROKE_WIDTH = 9
WHISKER_WIDTH = 9

# --- Local geometry of a single ket symbol "|>" ---------------------------
# Drawn around the local origin, pointing right (the angle bracket tip is at
# +x). Transforms below rotate/mirror copies into ears and nose.
BAR_X = -26        # x position of the vertical bar
BAR_HALF = 52      # half-height of the bar / bracket
BRK_NEAR = 6       # x of the bracket's open side
BRK_TIP = 44       # x of the bracket's tip

# --- Ears ------------------------------------------------------------------
EAR_SCALE = 0.7
# Angle bracket points up; the extra tilt past -90 leans each ear outward
# (left ear to the left, right ear — mirrored — to the right).
EAR_ROT = -116
EAR_LEFT = (118, 82)
EAR_RIGHT = (242, 82)

# --- Nose ------------------------------------------------------------------
NOSE_SCALE = 0.45
NOSE = (180, 184)

# --- Eyes ------------------------------------------------------------------
EYE_R = 12
EYE_LEFT = (140, 132)
EYE_RIGHT = (220, 132)
BLINK_PERIOD = 4.0  # seconds between blinks

# --- Whiskers --------------------------------------------------------------
# Three lines per side, fanning out from beside the nose toward the edges.
WHISKER_INNER_X = 132
WHISKER_OUTER_X = 66
WHISKER_INNER_Y = (174, 188, 202)
WHISKER_OUTER_Y = (160, 190, 220)


def ket_group(cx: float, cy: float, scale: float, rot: float,
              mirror: bool) -> str:
    """A single ket symbol transformed into place."""
    parts = [f'<g transform="translate({cx},{cy})']
    if mirror:
        parts.append(" scale(-1,1)")
    parts.append(f' rotate({rot}) scale({scale})">')
    # Vertical bar of the ket.
    parts.append(
        f'<line class="s" vector-effect="non-scaling-stroke" '
        f'x1="{BAR_X}" y1="{-BAR_HALF}" x2="{BAR_X}" y2="{BAR_HALF}"/>'
    )
    # Angle bracket of the ket.
    parts.append(
        f'<polyline class="s" vector-effect="non-scaling-stroke" '
        f'points="{BRK_NEAR},{-BAR_HALF} {BRK_TIP},0 {BRK_NEAR},{BAR_HALF}"/>'
    )
    parts.append("</g>")
    return "".join(parts)


def eye_group(cx: float, cy: float) -> str:
    """A dot eye that blinks on a loop (a quick vertical squash to a slit)."""
    # The circle sits at the local origin so the vertical scale collapses it
    # toward its own center; the group translates it into position. The eye
    # stays open for most of the cycle, then blinks shut and reopens quickly.
    return (
        f'<g transform="translate({cx},{cy})">'
        f'<circle class="f" cx="0" cy="0" r="{EYE_R}">'
        f'<animateTransform attributeName="transform" type="scale" '
        f'values="1 1;1 1;1 0.08;1 1;1 1" keyTimes="0;0.9;0.94;0.98;1" '
        f'dur="{BLINK_PERIOD:.1f}s" repeatCount="indefinite"/>'
        f"</circle>"
        f"</g>"
    )


def whiskers() -> list[str]:
    out: list[str] = []
    for iy, oy in zip(WHISKER_INNER_Y, WHISKER_OUTER_Y):
        # Left side.
        out.append(
            f'<line class="w" x1="{WHISKER_INNER_X}" y1="{iy}" '
            f'x2="{WHISKER_OUTER_X}" y2="{oy}"/>'
        )
        # Right side (mirrored across the vertical center line).
        out.append(
            f'<line class="w" x1="{W - WHISKER_INNER_X}" y1="{iy}" '
            f'x2="{W - WHISKER_OUTER_X}" y2="{oy}"/>'
        )
    return out


def build_svg() -> str:
    out: list[str] = []
    out.append(
        f'<svg xmlns="http://www.w3.org/2000/svg" '
        f'viewBox="0 0 {W} {H}" width="{W}" height="{H}" '
        f'role="img" aria-label="Ket">'
    )
    out.append(
        "<style>"
        f".s {{ fill: none; stroke: {TEXT_FILL_LIGHT}; "
        f"stroke-width: {STROKE_WIDTH}; stroke-linecap: round; "
        "stroke-linejoin: round; }"
        f".w {{ fill: none; stroke: {TEXT_FILL_LIGHT}; "
        f"stroke-width: {WHISKER_WIDTH}; stroke-linecap: round; }}"
        f".f {{ fill: {TEXT_FILL_LIGHT}; stroke: none; }}"
        "@media (prefers-color-scheme: dark) {"
        f"  .s {{ stroke: {TEXT_FILL_DARK}; }}"
        f"  .w {{ stroke: {TEXT_FILL_DARK}; }}"
        f"  .f {{ fill: {TEXT_FILL_DARK}; }}"
        "}"
        "</style>"
    )

    # Ears (two kets).
    out.append(ket_group(*EAR_LEFT, EAR_SCALE, EAR_ROT, mirror=False))
    out.append(ket_group(*EAR_RIGHT, EAR_SCALE, EAR_ROT, mirror=True))

    # Nose (one ket, angle bracket pointing down).
    out.append(ket_group(*NOSE, NOSE_SCALE, 90, mirror=False))

    # Eyes (two blinking dots).
    out.append(eye_group(*EYE_LEFT))
    out.append(eye_group(*EYE_RIGHT))

    # Whiskers (six lines).
    out.extend(whiskers())

    out.append("</svg>")
    return "\n".join(out)


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__)
    default_out = Path(__file__).parent / "ket_title.svg"
    ap.add_argument("-o", "--output", type=Path, default=default_out)
    args = ap.parse_args()
    args.output.write_text(build_svg())
    print(f"wrote {args.output}")


if __name__ == "__main__":
    main()
