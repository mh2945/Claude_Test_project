"""facegate.detector 의 '순수한 절반' 테스트.

cv2 호출 자체는 여기서 검증하지 않는다 -- OpenCV 가 자기 일을 하는지는
OpenCV 의 CI 가 볼 문제다. 우리가 지는 책임은 그 출력을 우리 타입으로
정확히 옮기는 부분이고, 그 부분만 순수 함수로 떼어 두었다.
"""

import numpy as np

from facegate.geometry import Box
from facegate.detector import to_boxes


class TestToBoxes:
    def test_opencv_rects_become_boxes(self):
        # detectMultiScale 은 int32 numpy 배열로 [x, y, w, h] 를 돌려준다
        detections = np.array([[10, 20, 30, 40]], dtype=np.int32)

        assert to_boxes(detections) == [Box(x=10, y=20, w=30, h=40)]
