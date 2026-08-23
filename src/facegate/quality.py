"""등록(enrollment) 품질 판정.

검출기가 얼굴을 찾았다는 것과, 그 얼굴을 등록해도 되는가는 다른 문제다.
여기서는 좌표와 이미지 크기만 보고 판정한다 -- 픽셀도 모델도 보지 않는다.
"""

from dataclasses import dataclass

from facegate.geometry import Box


@dataclass(frozen=True)
class Policy:
    """판정 기준. 현장마다 달라지므로 값이 아니라 정책으로 주입받는다."""

    min_face_ratio: float = 0.10


@dataclass(frozen=True)
class QualityResult:
    """판정 결과. 거절이면 사유를 남긴다 -- 사용자에게 무엇을 고치라 할지가 나온다."""

    accepted: bool
    reasons: tuple[str, ...] = ()


def evaluate(face: Box, image_size: tuple[int, int], policy: Policy) -> QualityResult:
    """얼굴 하나가 등록 가능한지 판정한다."""
    width, height = image_size
    reasons: list[str] = []

    shorter_side = min(width, height)
    if shorter_side > 0 and min(face.w, face.h) / shorter_side < policy.min_face_ratio:
        reasons.append("FACE_TOO_SMALL")

    return QualityResult(accepted=not reasons, reasons=tuple(reasons))
