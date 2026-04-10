"""Domain models for trader activity and copy orders."""
from __future__ import annotations

from dataclasses import dataclass
from datetime import datetime
from typing import Optional


@dataclass(frozen=True)
class TraderActivity:
    trader_id: str
    activity_id: str
    market_id: str
    token_id: str
    outcome: str
    side: str
    price: float
    size: float
    created_at: datetime


@dataclass(frozen=True)
class CopyOrder:
    market_id: str
    token_id: str
    outcome: str
    side: str
    size: float
    price: Optional[float] = None
