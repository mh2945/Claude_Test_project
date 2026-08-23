"""박스 기하 연산.

검출기와 무관한 순수 함수만 둔다. 입력은 좌표 숫자이고 부작용이 없다.
"""

from dataclasses import dataclass


@dataclass(frozen=True)
class Box:
    """검출된 얼굴의 경계 상자. OpenCV 관례대로 좌상단 원점 + 너비/높이."""

    x: int
    y: int
    w: int
    h: int

    def __post_init__(self) -> None:
        if self.w < 0 or self.h < 0:
            raise ValueError(f"박스 크기는 음수일 수 없습니다: w={self.w}, h={self.h}")


def area(box: Box) -> int:
    """박스의 넓이. 위치와 무관하다."""
    return box.w * box.h


def iou(a: Box, b: Box) -> float:
    """두 박스의 IoU (교집합 넓이 / 합집합 넓이)."""
    inter_w = max(0, min(a.x + a.w, b.x + b.w) - max(a.x, b.x))
    inter_h = max(0, min(a.y + a.h, b.y + b.h) - max(a.y, b.y))
    intersection = inter_w * inter_h

    union = area(a) + area(b) - intersection
    if union == 0:
        return 0.0
    return intersection / union
