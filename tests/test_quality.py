"""facegate.quality 의 등록 품질 판정 테스트.

입력은 좌표와 이미지 크기뿐이다. 실제 인물 사진이 픽스처에 들어가지 않는
것은 이 설계의 부수 효과다 -- 셸(OpenCV)과 코어(판정)를 갈라 놓았기 때문.
"""

from facegate.geometry import Box
from facegate.quality import Policy, evaluate


class TestEvaluate:
    def test_large_centred_face_is_accepted(self):
        face = Box(x=400, y=400, w=200, h=200)

        result = evaluate(face, image_size=(1000, 1000), policy=Policy())

        assert result.accepted is True

    def test_face_smaller_than_min_ratio_is_rejected_with_reason(self):
        # 짧은 변 1000px 의 5% -> 기본 기준 10% 미달
        face = Box(x=400, y=400, w=50, h=50)

        result = evaluate(face, image_size=(1000, 1000), policy=Policy())

        assert result.accepted is False
        assert "FACE_TOO_SMALL" in result.reasons
