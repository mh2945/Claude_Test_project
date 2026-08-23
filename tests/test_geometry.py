"""facegate.geometry 의 순수 박스 연산 테스트.

검출기(OpenCV)는 여기에 등장하지 않는다. 입력은 좌표 숫자뿐이므로
실제 인물 사진 없이 전부 검증된다.
"""

import pytest

from facegate.geometry import Box, area, iou


class TestArea:
    def test_area_of_10x20_box_is_200(self):
        box = Box(x=0, y=0, w=10, h=20)

        assert area(box) == 200

    def test_area_ignores_position(self):
        at_origin = Box(x=0, y=0, w=4, h=5)
        moved = Box(x=137, y=91, w=4, h=5)

        assert area(at_origin) == area(moved)

    def test_zero_width_box_has_zero_area(self):
        box = Box(x=5, y=5, w=0, h=20)

        assert area(box) == 0

    def test_negative_dimension_is_rejected(self):
        with pytest.raises(ValueError):
            Box(x=0, y=0, w=-1, h=10)


class TestIou:
    def test_half_overlap_gives_one_third(self):
        # 교집합 50, 합집합 100 + 100 - 50 = 150 -> 1/3
        a = Box(x=0, y=0, w=10, h=10)
        b = Box(x=5, y=0, w=10, h=10)

        assert iou(a, b) == pytest.approx(1 / 3)

    def test_two_empty_boxes_give_zero_instead_of_dividing_by_zero(self):
        # 합집합이 0이 되는 유일한 경우. 한쪽만 비면 다른 쪽 넓이가 분모를 채운다.
        empty = Box(x=0, y=0, w=0, h=0)
        also_empty = Box(x=3, y=7, w=0, h=0)

        assert iou(empty, also_empty) == pytest.approx(0.0)
