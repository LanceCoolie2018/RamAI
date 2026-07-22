#!/usr/bin/env python3
"""
Analyze the latest kegbot camera snapshot with OpenCV.

Designed to run on the Raspberry Pi 5 (where the camera writes snapshots under
/media/kegbot_snapshots — the same path Home Assistant is configured for).

Tuned against real camera frames in BeerAutomations/screenshots/: top-down view
of a dark mug opening on a light desk. Primary detection finds the dark circular
interior/rim; fill is estimated from the opening interior (empty cavity is dark).

Outputs human-readable logs by default, or --json for later Home Assistant use.
"""

from __future__ import annotations

import argparse
import glob
import json
import os
import sys
from dataclasses import dataclass, field
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

import cv2
import numpy as np

# Default on the Pi host (HA snapshot path); override with --dir or KEGBOT_SNAPSHOT_DIR.
DEFAULT_SNAPSHOT_DIR = "/media/kegbot_snapshots"
IMAGE_EXTENSIONS = ("*.jpg", "*.jpeg", "*.png", "*.bmp", "*.webp")

# --- Top-down dark opening (real camera / mug) ---
OPENING_MIN_AREA_FRAC = 0.012
OPENING_MAX_AREA_FRAC = 0.28
OPENING_MIN_CIRCULARITY = 0.40
OPENING_MIN_ASPECT = 0.45  # h/w of opening bbox
OPENING_MAX_ASPECT = 1.85
OPENING_THR_RANGE = range(25, 90, 5)

# Interior brightness → fill (empty black mug cavity is ~25–45 mean gray).
# Calibrated on BeerAutomations/screenshots empty-mug sequence.
EMPTY_INTERIOR_MEAN = 38.0
FULL_INTERIOR_MEAN = 110.0

# --- Side-view contour fallback (synthetic / clear glass side shots) ---
SIDE_MIN_AREA_FRAC = 0.02
SIDE_MAX_AREA_FRAC = 0.55
SIDE_MIN_ASPECT = 0.8
SIDE_MAX_ASPECT = 3.5
SIDE_MIN_SOLIDITY = 0.55

# HA-friendly state labels
EMPTY_BELOW = 8.0
FULL_ABOVE = 90.0


@dataclass
class CupResult:
    detected: bool = False
    bbox: list[int] | None = None  # full mug estimate [x, y, w, h]
    opening_bbox: list[int] | None = None  # dark opening only
    fill_percent: float | None = None
    confidence: float = 0.0
    state: str = "no_cup"  # no_cup | empty | partial | full | unknown
    method: str | None = None  # opening | side_contour
    notes: list[str] = field(default_factory=list)


@dataclass
class AnalysisResult:
    ok: bool
    analyzed_at: str
    image_path: str | None = None
    image_mtime: str | None = None
    width: int | None = None
    height: int | None = None
    mean_brightness: float | None = None
    edge_density: float | None = None
    cup: CupResult = field(default_factory=CupResult)
    error: str | None = None
    debug_images: list[str] = field(default_factory=list)

    def to_public_dict(self) -> dict[str, Any]:
        """JSON-serializable payload for Home Assistant / MQTT later."""
        return {
            "ok": self.ok,
            "analyzed_at": self.analyzed_at,
            "image_path": self.image_path,
            "image_mtime": self.image_mtime,
            "width": self.width,
            "height": self.height,
            "mean_brightness": self.mean_brightness,
            "edge_density": self.edge_density,
            "cup_detected": self.cup.detected,
            "fill_percent": self.cup.fill_percent,
            "confidence": round(self.cup.confidence, 3),
            "state": self.cup.state,
            "bbox": self.cup.bbox,
            "opening_bbox": self.cup.opening_bbox,
            "method": self.cup.method,
            "notes": self.cup.notes,
            "error": self.error,
            "debug_images": self.debug_images,
        }


def utc_now_iso() -> str:
    return datetime.now(timezone.utc).isoformat(timespec="seconds")


def mtime_iso(path: str) -> str:
    ts = os.path.getmtime(path)
    return datetime.fromtimestamp(ts, tz=timezone.utc).isoformat(timespec="seconds")


def resolve_snapshot_dir(cli_dir: str | None) -> Path:
    raw = cli_dir or os.environ.get("KEGBOT_SNAPSHOT_DIR") or DEFAULT_SNAPSHOT_DIR
    return Path(raw).expanduser().resolve()


def _is_motion_mask_name(path: str) -> bool:
    """HA often writes companion motion images as 15-20-00m.jpg — skip those."""
    name = Path(path).name
    stem = Path(path).stem
    if stem.endswith("m") and stem[:-1].replace("-", "").isdigit() is False:
        # e.g. 15-20-00m
        if len(stem) > 1 and stem[-1] == "m":
            base = stem[:-1]
            # time-like 15-20-00
            parts = base.split("-")
            if len(parts) == 3 and all(p.isdigit() for p in parts):
                return True
    if name.endswith("m.jpg") or name.endswith("m.jpeg") or name.endswith("m.png"):
        # broader: *m.jpg where * looks like a timestamp
        base = name.rsplit(".", 1)[0]
        if base.endswith("m"):
            return True
    return False


def find_image_files(snapshot_dir: Path) -> list[str]:
    if not snapshot_dir.is_dir():
        return []

    files: list[str] = []
    root = str(snapshot_dir)
    for pattern in IMAGE_EXTENSIONS:
        files.extend(glob.glob(os.path.join(root, "**", pattern), recursive=True))
        upper = pattern.upper()
        if upper != pattern:
            files.extend(glob.glob(os.path.join(root, "**", upper), recursive=True))

    cleaned: list[str] = []
    seen: set[str] = set()
    for path in files:
        norm = path.replace("\\", "/")
        if "/_debug/" in norm:
            continue
        if norm.endswith(("_gray.jpg", "_edges.jpg", "_overlay.jpg", "_opening.jpg")):
            continue
        if _is_motion_mask_name(path):
            continue
        if path not in seen:
            seen.add(path)
            cleaned.append(path)
    return cleaned


def latest_file(paths: list[str]) -> str:
    return max(paths, key=os.path.getmtime)


def edge_density(edges: np.ndarray) -> float:
    if edges.size == 0:
        return 0.0
    return float(np.count_nonzero(edges)) / float(edges.size)


def fill_state(fill_percent: float | None, detected: bool) -> str:
    if not detected:
        return "no_cup"
    if fill_percent is None:
        return "unknown"
    if fill_percent < EMPTY_BELOW:
        return "empty"
    if fill_percent >= FULL_ABOVE:
        return "full"
    return "partial"


def _circularity(cnt: np.ndarray) -> float:
    area = float(cv2.contourArea(cnt))
    peri = float(cv2.arcLength(cnt, True)) or 1.0
    return float(4.0 * np.pi * area / (peri * peri))


def detect_dark_opening(
    gray: np.ndarray,
) -> tuple[tuple[int, int, int, int] | None, np.ndarray | None, float, list[str]]:
    """
    Find the dark circular/elliptical mug opening (top-down camera).

    Returns (opening_bbox, contour, confidence, notes).
    """
    notes: list[str] = []
    h, w = gray.shape[:2]
    frame_area = float(h * w)
    blur = cv2.GaussianBlur(gray, (9, 9), 0)
    kernel = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (9, 9))

    best_score = 0.0
    best_bbox: tuple[int, int, int, int] | None = None
    best_cnt: np.ndarray | None = None
    best_thr: int | None = None

    for thr in OPENING_THR_RANGE:
        dark = (blur < thr).astype(np.uint8) * 255
        dark = cv2.morphologyEx(dark, cv2.MORPH_OPEN, kernel, iterations=1)
        dark = cv2.morphologyEx(dark, cv2.MORPH_CLOSE, kernel, iterations=2)
        contours, _ = cv2.findContours(dark, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

        for cnt in contours:
            area = float(cv2.contourArea(cnt))
            frac = area / frame_area
            if frac < OPENING_MIN_AREA_FRAC or frac > OPENING_MAX_AREA_FRAC:
                continue

            circ = _circularity(cnt)
            if circ < OPENING_MIN_CIRCULARITY:
                continue

            x, y, bw, bh = cv2.boundingRect(cnt)
            if bw < 20 or bh < 20:
                continue
            aspect = bh / float(bw)
            if aspect < OPENING_MIN_ASPECT or aspect > OPENING_MAX_ASPECT:
                continue

            touches_border = x <= 2 or y <= 2 or x + bw >= w - 2 or y + bh >= h - 2
            # Large border-touching blobs are often laptop screens / shadows.
            if touches_border and frac > 0.12:
                continue

            cx = x + bw / 2.0
            cy = y + bh / 2.0
            center_dist = float(np.hypot((cx - w / 2.0) / w, (cy - h / 2.0) / h))
            center_score = max(0.0, 1.0 - center_dist)

            score = (
                0.45 * circ
                + 0.35 * center_score
                + 0.20 * min(frac / 0.08, 1.0)
            )
            if touches_border:
                score *= 0.70

            if score > best_score:
                best_score = score
                best_bbox = (int(x), int(y), int(bw), int(bh))
                best_cnt = cnt
                best_thr = thr

    if best_bbox is None:
        notes.append("no_dark_opening")
        return None, None, 0.0, notes

    notes.append(f"dark_opening_thr={best_thr}")
    notes.append(f"opening_score={best_score:.2f}")
    return best_bbox, best_cnt, float(best_score), notes


def expand_opening_to_body(
    opening: tuple[int, int, int, int],
    frame_shape: tuple[int, ...],
) -> tuple[int, int, int, int]:
    """
    Expand opening bbox downward to approximate full mug body.

    Real camera is angled top-down; the dark opening is the top of the cup and
    the body continues below it.
    """
    h, w = frame_shape[:2]
    x, y, bw, bh = opening
    # Body typically ~1.6–2.2× opening height below the rim for this mug set.
    extra_h = int(bh * 1.15)
    extra_w = int(bw * 0.12)
    x0 = max(0, x - extra_w)
    y0 = max(0, y - int(bh * 0.08))
    x1 = min(w, x + bw + extra_w)
    y1 = min(h, y + bh + extra_h)
    return (x0, y0, x1 - x0, y1 - y0)


def estimate_fill_from_opening(
    gray: np.ndarray,
    bgr: np.ndarray,
    opening_cnt: np.ndarray,
) -> tuple[float, float, list[str], dict[str, float]]:
    """
    Estimate fill from the mug opening interior (top-down view).

    Empty black ceramic cavity is very dark. Liquid raises mean brightness and
    often adds color / reflections. Calibrated on empty-mug screenshots.
    """
    notes: list[str] = []
    kernel = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (7, 7))
    mask = np.zeros(gray.shape, dtype=np.uint8)
    cv2.drawContours(mask, [opening_cnt], -1, 255, thickness=-1)
    # Stay inside the rim — rim highlights skew brightness.
    mask = cv2.erode(mask, kernel, iterations=2)

    vals = gray[mask > 0]
    if vals.size < 80:
        notes.append("opening_interior_too_small")
        return 0.0, 0.2, notes, {}

    mean_v = float(np.mean(vals))
    std_v = float(np.std(vals))
    p10 = float(np.percentile(vals, 10))
    p90 = float(np.percentile(vals, 90))

    sat_mean = 0.0
    if bgr.ndim == 3:
        hsv = cv2.cvtColor(bgr, cv2.COLOR_BGR2HSV)
        sat_mean = float(np.mean(hsv[:, :, 1][mask > 0]))

    # Map interior brightness to fill %. Dark cavity → empty.
    # Mild boost if interior has color (beer) beyond pure gray darkness.
    brightness_fill = (mean_v - EMPTY_INTERIOR_MEAN) / max(
        1.0, FULL_INTERIOR_MEAN - EMPTY_INTERIOR_MEAN
    )
    brightness_fill = float(np.clip(brightness_fill, 0.0, 1.0) * 100.0)

    # Saturation hint (amber beer inside vs black void).
    sat_boost = float(np.clip((sat_mean - 25.0) / 80.0, 0.0, 1.0) * 25.0)
    fill = float(np.clip(brightness_fill + 0.35 * sat_boost, 0.0, 100.0))

    # Confidence: higher when we have a clear dark-or-bright signal.
    if mean_v < EMPTY_INTERIOR_MEAN + 8:
        conf = 0.75  # clearly empty cavity
    elif mean_v > FULL_INTERIOR_MEAN - 15:
        conf = 0.65
    else:
        conf = 0.55

    notes.append("opening_interior_fill")
    stats = {
        "interior_mean": round(mean_v, 2),
        "interior_std": round(std_v, 2),
        "interior_p10": round(p10, 2),
        "interior_p90": round(p90, 2),
        "interior_sat": round(sat_mean, 2),
    }
    return round(fill, 1), conf, notes, stats


def _smooth_1d(values: np.ndarray) -> np.ndarray:
    n = len(values)
    k = max(3, (n // 20) | 1)
    kernel = np.ones(k, dtype=np.float32) / k
    return np.convolve(values.astype(np.float32), kernel, mode="same")


def _longest_true_run(mask: np.ndarray) -> int:
    best = run = 0
    for flag in mask:
        if flag:
            run += 1
            best = max(best, run)
        else:
            run = 0
    return best


def _fill_from_bottom(mask: np.ndarray) -> int:
    n = len(mask)
    gap_budget = max(2, n // 25)
    fill_rows = 0
    gap = 0
    started = False
    for is_liquid in mask:
        if is_liquid:
            started = True
            fill_rows += 1 + gap
            gap = 0
        elif started:
            gap += 1
            if gap > gap_budget:
                break
    return fill_rows


def detect_side_contour(
    gray: np.ndarray,
) -> tuple[tuple[int, int, int, int] | None, float, list[str]]:
    """Fallback: tall solid blob (side-view glass / synthetic tests)."""
    notes: list[str] = []
    h, w = gray.shape[:2]
    frame_area = float(h * w)

    blurred = cv2.GaussianBlur(gray, (5, 5), 0)
    binary = cv2.adaptiveThreshold(
        blurred,
        255,
        cv2.ADAPTIVE_THRESH_GAUSSIAN_C,
        cv2.THRESH_BINARY_INV,
        31,
        5,
    )
    kernel = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (5, 5))
    binary = cv2.morphologyEx(binary, cv2.MORPH_CLOSE, kernel, iterations=2)
    binary = cv2.morphologyEx(binary, cv2.MORPH_OPEN, kernel, iterations=1)

    contours, _ = cv2.findContours(binary, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
    if not contours:
        notes.append("no_side_contours")
        return None, 0.0, notes

    best: tuple[int, int, int, int] | None = None
    best_score = 0.0

    for cnt in contours:
        area = float(cv2.contourArea(cnt))
        area_frac = area / frame_area
        if area_frac < SIDE_MIN_AREA_FRAC or area_frac > SIDE_MAX_AREA_FRAC:
            continue

        x, y, bw, bh = cv2.boundingRect(cnt)
        if bw < 8 or bh < 8:
            continue
        aspect = bh / float(bw)
        if aspect < SIDE_MIN_ASPECT or aspect > SIDE_MAX_ASPECT:
            continue

        hull = cv2.convexHull(cnt)
        hull_area = float(cv2.contourArea(hull)) or 1.0
        solidity = area / hull_area
        if solidity < SIDE_MIN_SOLIDITY:
            continue

        cx = x + bw / 2.0
        cy = y + bh / 2.0
        center_dist = float(np.hypot((cx - w / 2.0) / w, (cy - h / 2.0) / h))
        center_score = max(0.0, 1.0 - center_dist)
        aspect_score = 1.0 - min(abs(aspect - 1.6) / 1.6, 1.0)
        size_score = min(area_frac / 0.15, 1.0)
        score = 0.35 * size_score + 0.25 * aspect_score + 0.25 * solidity + 0.15 * center_score

        if score > best_score:
            best_score = score
            best = (int(x), int(y), int(bw), int(bh))

    if best is None:
        notes.append("no_side_cup_contour")
        return None, 0.0, notes

    notes.append("side_contour_cup")
    return best, float(best_score), notes


def estimate_fill_side_view(
    bgr: np.ndarray,
    gray: np.ndarray,
    bbox: tuple[int, int, int, int],
) -> tuple[float, float, list[str]]:
    """Side-view fill via vertical liquid-score profile (synthetic / clear glass)."""
    notes: list[str] = []
    x, y, bw, bh = bbox
    pad_x = max(2, int(bw * 0.18))
    pad_y = max(2, int(bh * 0.08))
    x0, x1 = x + pad_x, x + bw - pad_x
    y0, y1 = y + pad_y, y + bh - pad_y
    if x1 - x0 < 4 or y1 - y0 < 8:
        notes.append("roi_too_small")
        return 0.0, 0.2, notes

    gray_roi = gray[y0:y1, x0:x1]
    bgr_roi = bgr[y0:y1, x0:x1] if bgr.ndim == 3 else cv2.cvtColor(gray_roi, cv2.COLOR_GRAY2BGR)
    hsv = cv2.cvtColor(bgr_roi, cv2.COLOR_BGR2HSV)
    val = _smooth_1d(hsv[:, :, 2].mean(axis=1)[::-1])
    sat = _smooth_1d(hsv[:, :, 1].mean(axis=1)[::-1])
    n = len(val)
    if n < 8:
        notes.append("roi_too_short")
        return 0.0, 0.2, notes

    score = _smooth_1d(0.55 * (sat / 255.0) + 0.45 * (1.0 - val / 255.0))
    s_lo = float(np.percentile(score, 10))
    s_hi = float(np.percentile(score, 90))
    dynamic = s_hi - s_lo
    score_std = float(np.std(score))
    mean_sat = float(np.mean(sat))
    mean_val = float(np.mean(val))

    if dynamic < 0.05 or score_std < 0.02:
        if mean_sat < 40 and mean_val > 50:
            notes.append("uniform_empty")
            return 0.0, 0.5, notes
        if mean_sat >= 40 or mean_val < 50:
            notes.append("uniform_full")
            return 100.0, 0.5, notes
        notes.append("uniform_unknown")
        return 0.0, 0.3, notes

    thresh = s_lo + 0.45 * dynamic
    liquid_mask = score >= thresh
    fill_rows = max(_fill_from_bottom(liquid_mask), _longest_true_run(liquid_mask))

    margin = max(2, n // 30)
    dscore = np.diff(score.astype(np.float64))
    search = dscore[margin : n - 1 - margin]
    if len(search) > 0:
        rel = int(np.argmin(search))
        surface_idx = margin + rel + 1
        drop = float(-search[rel])
        if drop > 0.02:
            fill_from_surface = 100.0 * surface_idx / float(n)
            fill_from_mask = 100.0 * fill_rows / float(n)
            alpha = float(np.clip(drop / 0.08, 0.35, 0.85))
            fill = alpha * fill_from_surface + (1.0 - alpha) * fill_from_mask
            notes.append("surface_drop_fill")
        else:
            fill = 100.0 * fill_rows / float(n)
            notes.append("score_mask_fill")
    else:
        fill = 100.0 * fill_rows / float(n)
        notes.append("score_mask_fill")

    fill = float(np.clip(fill, 0.0, 100.0))
    conf = float(np.clip(0.4 + dynamic * 2.0, 0.4, 0.95))
    return round(fill, 1), conf, notes


def analyze_image(img: np.ndarray, path: str) -> tuple[AnalysisResult, dict[str, np.ndarray]]:
    """Run cup detection + fill estimation. Returns result + debug image map."""
    h, w = img.shape[:2]
    gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY) if img.ndim == 3 else img
    blurred = cv2.GaussianBlur(gray, (5, 5), 0)
    edges = cv2.Canny(blurred, 50, 150)

    cup = CupResult()
    debug: dict[str, np.ndarray] = {"gray": gray, "edges": edges}
    interior_stats: dict[str, float] = {}

    # 1) Primary: top-down dark opening (real Pi camera / mug screenshots)
    opening, opening_cnt, open_conf, open_notes = detect_dark_opening(gray)
    cup.notes.extend(open_notes)

    if opening is not None and opening_cnt is not None and open_conf >= 0.45:
        cup.detected = True
        cup.method = "opening"
        cup.opening_bbox = [int(v) for v in opening]
        body = expand_opening_to_body(opening, img.shape)
        cup.bbox = [int(v) for v in body]

        fill, fill_conf, fill_notes, interior_stats = estimate_fill_from_opening(
            gray, img, opening_cnt
        )
        cup.fill_percent = fill
        cup.confidence = float(np.clip(0.55 * open_conf + 0.45 * fill_conf, 0.0, 1.0))
        cup.notes.extend(fill_notes)
        for k, v in interior_stats.items():
            cup.notes.append(f"{k}={v}")
        cup.state = fill_state(fill, True)

        overlay = img.copy() if img.ndim == 3 else cv2.cvtColor(img, cv2.COLOR_GRAY2BGR)
        ox, oy, obw, obh = opening
        bx, by, bbw, bbh = body
        cv2.rectangle(overlay, (bx, by), (bx + bbw, by + bbh), (255, 180, 0), 2)
        cv2.rectangle(overlay, (ox, oy), (ox + obw, oy + obh), (0, 255, 0), 3)
        cv2.drawContours(overlay, [opening_cnt], -1, (0, 255, 128), 2)
        label = f"{fill:.0f}% conf={cup.confidence:.2f} open"
        cv2.putText(
            overlay,
            label,
            (ox, max(28, oy - 10)),
            cv2.FONT_HERSHEY_SIMPLEX,
            0.9,
            (0, 255, 0),
            2,
        )
        debug["overlay"] = overlay
        # Opening mask preview
        omask = np.zeros_like(gray)
        cv2.drawContours(omask, [opening_cnt], -1, 255, -1)
        debug["opening"] = omask

    else:
        # 2) Fallback: side-view contour (synthetic / clear glass)
        bbox, det_conf, det_notes = detect_side_contour(gray)
        cup.notes.extend(det_notes)

        if bbox is not None and det_conf >= 0.35:
            cup.detected = True
            cup.method = "side_contour"
            cup.bbox = [int(v) for v in bbox]
            fill, fill_conf, fill_notes = estimate_fill_side_view(img, gray, bbox)
            cup.fill_percent = fill
            cup.confidence = float(np.clip(0.5 * det_conf + 0.5 * fill_conf, 0.0, 1.0))
            cup.notes.extend(fill_notes)
            cup.state = fill_state(fill, True)

            overlay = img.copy() if img.ndim == 3 else cv2.cvtColor(img, cv2.COLOR_GRAY2BGR)
            x, y, bw, bh = bbox
            color = (
                (0, 200, 0)
                if cup.state == "full"
                else (0, 165, 255)
                if cup.state == "partial"
                else (0, 0, 255)
            )
            cv2.rectangle(overlay, (x, y), (x + bw, y + bh), color, 2)
            fill_y = int(y + bh * (1.0 - (fill / 100.0)))
            cv2.line(overlay, (x, fill_y), (x + bw, fill_y), (255, 200, 0), 2)
            cv2.putText(
                overlay,
                f"{fill:.0f}% conf={cup.confidence:.2f} side",
                (x, max(20, y - 8)),
                cv2.FONT_HERSHEY_SIMPLEX,
                0.7,
                color,
                2,
            )
            debug["overlay"] = overlay
        else:
            cup.detected = False
            cup.fill_percent = None
            cup.confidence = 0.0
            cup.state = "no_cup"
            overlay = img.copy() if img.ndim == 3 else cv2.cvtColor(img, cv2.COLOR_GRAY2BGR)
            cv2.putText(overlay, "no cup", (20, 40), cv2.FONT_HERSHEY_SIMPLEX, 1.0, (0, 0, 255), 2)
            debug["overlay"] = overlay

    result = AnalysisResult(
        ok=True,
        analyzed_at=utc_now_iso(),
        image_path=path,
        image_mtime=mtime_iso(path),
        width=w,
        height=h,
        mean_brightness=round(float(np.mean(gray)), 2),
        edge_density=round(edge_density(edges), 4),
        cup=cup,
    )
    return result, debug


def print_report(result: AnalysisResult) -> None:
    if not result.ok:
        print(f"ERROR: {result.error}", file=sys.stderr)
        return

    print(f"[{result.analyzed_at}] Analyzed: {result.image_path}")
    print(f"  Image size : {result.width}x{result.height}")
    print(f"  Brightness : mean={result.mean_brightness}")
    print(f"  Edges      : density={result.edge_density}")
    print(
        f"  Cup        : detected={result.cup.detected} state={result.cup.state} "
        f"method={result.cup.method}"
    )
    if result.cup.detected:
        print(f"  Fill       : {result.cup.fill_percent}%  confidence={result.cup.confidence:.2f}")
        print(f"  BBox       : {result.cup.bbox}")
        if result.cup.opening_bbox:
            print(f"  Opening    : {result.cup.opening_bbox}")
    if result.cup.notes:
        print(f"  Notes      : {', '.join(result.cup.notes)}")
    for path in result.debug_images:
        print(f"  Debug out  : {path}")


def save_debug_outputs(
    path: str,
    debug: dict[str, np.ndarray],
    out_dir: Path,
) -> list[str]:
    out_dir.mkdir(parents=True, exist_ok=True)
    stem = Path(path).stem
    saved: list[str] = []
    for name, image in debug.items():
        out_path = out_dir / f"{stem}_{name}.jpg"
        if cv2.imwrite(str(out_path), image):
            saved.append(str(out_path))
    return saved


def analyze_latest_snapshot(
    snapshot_dir: Path,
    *,
    image_file: Path | None = None,
    save_debug: bool = False,
    debug_dir: Path | None = None,
    as_json: bool = False,
) -> int:
    """
    Analyze one snapshot (explicit file or newest under snapshot_dir).

    Exit codes:
      0 success
      1 soft failure (no images)
      2 hard error (missing dir / unreadable image)
    """
    analyzed_at = utc_now_iso()

    def emit(result: AnalysisResult, code: int) -> int:
        if as_json:
            print(json.dumps(result.to_public_dict(), indent=2 if sys.stdout.isatty() else None))
        else:
            print_report(result)
            if result.ok:
                print("Analysis complete")
        return code

    if image_file is not None:
        path = str(image_file.expanduser().resolve())
        if not os.path.isfile(path):
            return emit(
                AnalysisResult(ok=False, analyzed_at=analyzed_at, error=f"Image not found: {path}"),
                2,
            )
    else:
        if not snapshot_dir.exists():
            return emit(
                AnalysisResult(
                    ok=False,
                    analyzed_at=analyzed_at,
                    error=f"Snapshot directory does not exist: {snapshot_dir}",
                ),
                2,
            )
        if not snapshot_dir.is_dir():
            return emit(
                AnalysisResult(ok=False, analyzed_at=analyzed_at, error=f"Not a directory: {snapshot_dir}"),
                2,
            )

        files = find_image_files(snapshot_dir)
        if not files:
            return emit(
                AnalysisResult(
                    ok=False,
                    analyzed_at=analyzed_at,
                    error=f"No snapshots found under: {snapshot_dir}",
                ),
                1,
            )
        path = latest_file(files)

    img = cv2.imread(path, cv2.IMREAD_COLOR)
    if img is None:
        return emit(
            AnalysisResult(
                ok=False,
                analyzed_at=analyzed_at,
                image_path=path,
                error=f"Failed to load image: {path}",
            ),
            2,
        )

    result, debug = analyze_image(img, path)

    if save_debug:
        target = debug_dir or (Path(path).parent / "_debug")
        result.debug_images = save_debug_outputs(path, debug, target)

    return emit(result, 0)


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Analyze the latest kegbot camera snapshot (Pi 5). "
            "Default dir is /media/kegbot_snapshots (HA path). "
            "JSON mode is for later Home Assistant integration."
        ),
    )
    parser.add_argument(
        "--dir",
        dest="snapshot_dir",
        default=None,
        help=f"Snapshot root on the Pi (default: $KEGBOT_SNAPSHOT_DIR or {DEFAULT_SNAPSHOT_DIR})",
    )
    parser.add_argument(
        "--file",
        dest="image_file",
        default=None,
        help="Analyze a specific image instead of the latest under --dir",
    )
    parser.add_argument(
        "--json",
        action="store_true",
        help="Print a single JSON object (HA / MQTT friendly)",
    )
    parser.add_argument(
        "--save-debug",
        action="store_true",
        help="Write gray / edges / overlay / opening previews",
    )
    parser.add_argument(
        "--debug-dir",
        default=None,
        help="Where to write debug images (default: <image_dir>/_debug)",
    )
    return parser


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    snapshot_dir = resolve_snapshot_dir(args.snapshot_dir)
    image_file = Path(args.image_file) if args.image_file else None
    debug_dir = Path(args.debug_dir).expanduser().resolve() if args.debug_dir else None
    return analyze_latest_snapshot(
        snapshot_dir,
        image_file=image_file,
        save_debug=args.save_debug,
        debug_dir=debug_dir,
        as_json=args.json,
    )


if __name__ == "__main__":
    sys.exit(main())
