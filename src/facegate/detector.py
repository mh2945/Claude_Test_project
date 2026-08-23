"""OpenCV 검출기 어댑터.

이 모듈은 둘로 나뉜다.

- to_boxes(): 순수 함수. OpenCV 의 출력 형식을 우리 타입으로 옮긴다. 테스트 대상.
- detect_faces(): cv2 를 실제로 부르는 얇은 셸. 단위 테스트 대상이 아니다.

경계를 이렇게 그은 이유는 단위 테스트에 실제 얼굴 이미지를 넣지 않기 위해서다.
"""

from facegate.geometry import Box


def to_boxes(detections) -> list[Box]:
    """detectMultiScale 의 [x, y, w, h] 배열을 Box 목록으로 옮긴다."""
    return [Box(x=int(x), y=int(y), w=int(w), h=int(h)) for x, y, w, h in detections]


def detect_faces(image) -> list[Box]:
    """이미지에서 얼굴을 찾는다. cv2 를 부르는 얇은 셸 -- 단위 테스트 대상이 아니다."""
    import cv2

    cascade_path = cv2.data.haarcascades + "haarcascade_frontalface_default.xml"
    cascade = cv2.CascadeClassifier(cascade_path)
    grey = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
    return to_boxes(cascade.detectMultiScale(grey, scaleFactor=1.1, minNeighbors=5))
