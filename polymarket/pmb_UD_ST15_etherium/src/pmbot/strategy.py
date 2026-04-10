"""Signal generation for 15-minute ETH trends."""

from __future__ import annotations

from dataclasses import dataclass
from datetime import datetime, timezone
import math

from .storage import PricePoint


@dataclass(frozen=True)
class Signal:
    direction: str  # "UP", "DOWN", or "HOLD"
    slope: float
    price_change: float
    volatility: float
    confidence: float
    horizon_probs: dict[str, dict[str, float]]
    points: int
    window_minutes: int
    as_of: datetime


def compute_signal(
    prices: list[PricePoint],
    window_minutes: int,
    min_points: int,
    slope_threshold: float,
    baseline_price: float | None,
    start_delta_threshold: float,
) -> Signal:
    if len(prices) < min_points:
        return Signal(
            direction="HOLD",
            slope=0.0,
            price_change=0.0,
            volatility=0.0,
            confidence=0.0,
            horizon_probs={},
            points=len(prices),
            window_minutes=window_minutes,
            as_of=prices[-1].timestamp if prices else datetime.now(timezone.utc),
        )

    slope = _linear_regression_slope(prices)
    price_change = prices[-1].price - prices[0].price
    price_change_from_start = 0.0
    if baseline_price is not None:
        price_change_from_start = prices[-1].price - baseline_price
    volatility = _stddev([p.price for p in prices])
    confidence = _confidence(price_change, volatility)
    horizon_probs = compute_horizon_probabilities(prices, baseline_price, slope)
    if baseline_price is None:
        direction = "HOLD"
    elif price_change_from_start > start_delta_threshold and slope > slope_threshold:
        direction = "UP"
    elif price_change_from_start < -start_delta_threshold and slope < -slope_threshold:
        direction = "DOWN"
    else:
        direction = "HOLD"

    return Signal(
        direction=direction,
        slope=slope,
        price_change=price_change,
        volatility=volatility,
        confidence=confidence,
        horizon_probs=horizon_probs,
        points=len(prices),
        window_minutes=window_minutes,
        as_of=prices[-1].timestamp,
    )


def _linear_regression_slope(prices: list[PricePoint]) -> float:
    times = [p.timestamp.timestamp() for p in prices]
    values = [p.price for p in prices]
    n = len(times)
    mean_t = sum(times) / n
    mean_v = sum(values) / n

    numerator = sum((t - mean_t) * (v - mean_v) for t, v in zip(times, values))
    denominator = sum((t - mean_t) ** 2 for t in times)
    if denominator == 0:
        return 0.0
    return numerator / denominator


def _stddev(values: list[float]) -> float:
    if not values:
        return 0.0
    mean = sum(values) / len(values)
    variance = sum((v - mean) ** 2 for v in values) / len(values)
    return variance**0.5


def _confidence(price_change: float, volatility: float) -> float:
    if volatility <= 0:
        return 0.0
    return abs(price_change) / volatility


def compute_horizon_probabilities(
    prices: list[PricePoint],
    target_price: float | None,
    slope: float,
) -> dict[str, dict[str, float]]:
    if target_price is None or len(prices) < 2:
        return {}
    per_sec_sigma = _per_second_volatility(prices)
    if per_sec_sigma <= 0:
        current = prices[-1].price
        deterministic = 1.0 if current >= target_price else 0.0
        return {
            "10s": {"up": deterministic, "down": 1.0 - deterministic},
            "1m": {"up": deterministic, "down": 1.0 - deterministic},
            "3m": {"up": deterministic, "down": 1.0 - deterministic},
            "5m": {"up": deterministic, "down": 1.0 - deterministic},
            "10m": {"up": deterministic, "down": 1.0 - deterministic},
        }

    current = prices[-1].price
    horizons = {
        "10s": 10,
        "1m": 60,
        "3m": 180,
        "5m": 300,
        "10m": 600,
    }
    results: dict[str, dict[str, float]] = {}
    for label, seconds in horizons.items():
        mean_price = current + slope * seconds
        stddev = per_sec_sigma * math.sqrt(seconds)
        if stddev <= 0:
            up_prob = 1.0 if mean_price >= target_price else 0.0
        else:
            z = (target_price - mean_price) / stddev
            up_prob = 1.0 - _normal_cdf(z)
        results[label] = {"up": up_prob, "down": 1.0 - up_prob}
    return results


def _per_second_volatility(prices: list[PricePoint]) -> float:
    changes: list[float] = []
    for prev, curr in zip(prices, prices[1:]):
        dt = (curr.timestamp - prev.timestamp).total_seconds()
        if dt <= 0:
            continue
        changes.append((curr.price - prev.price) / dt)
    return _stddev(changes)


def _normal_cdf(value: float) -> float:
    return 0.5 * (1.0 + math.erf(value / math.sqrt(2.0)))
