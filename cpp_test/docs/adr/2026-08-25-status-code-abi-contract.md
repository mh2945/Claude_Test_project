# 상태 코드의 ABI 계약 — 번호 영구 예약과 `enum : int` 표현

- 날짜: 2026-08-25
- 관련 파일: include/alcv/status.h, tests/status_abi_test.cpp

## 왜 (문제)

alcv 는 SDK 다. 앱 코드와 달리 **이미 배포된 헤더로 컴파일된 소비자 바이너리를 우리가
고칠 수 없다.** 상태 코드는 모든 공개 함수의 반환 타입이므로 여기서 내린 결정이 앞으로
추가될 모든 API 에 상속된다.

아래는 전부 **컴파일 에러 없이 런타임에 조용히 깨지는** 변경이다.

| 변경 | 증상 |
|---|---|
| `ALCV_ERR_NO_MEMORY` 를 -2 → -3 | 소비자 바이너리엔 -2 가 박혀 있어 분기가 어긋난다 |
| `ALCV_OK` 가 0이 아니게 됨 | 모든 `if (rc)` / `if (!rc)` 관용구가 반대로 동작한다 |
| enum underlying type 변경 | 인자 전달 폭이 바뀐다. 링크는 되고 런타임에 깨진다 |
| `noexcept` 제거 (C++17) | 함수 **타입**이 바뀌어 함수 포인터를 저장하던 코드가 깨진다 |

제약: 시나리오 요구사항으로 `if (rc)` 관용구가 반드시 살아 있어야 한다.
제약: 정의되지 않은 값(`-999`, `12345`)을 넘겨도 정의된 동작이어야 한다.

## 어떻게 (설계)

**정책**: 한 번 배포된 번호는 영구 예약. 값 변경·항목 삭제·번호 재사용 전부 금지.
새 코드는 리스트 끝에 다음 미사용 음수 값으로만 추가한다. 헤더 주석에 명시.

**표현**: fixed underlying type 을 가진 unscoped enum.

```cpp
enum alcv_status : int { ... };
using alcv_status_t = alcv_status;
```

`: int` 가 두 가지를 표준 보장으로 만든다.
- `static_cast<alcv_status_t>(-999)` 가 well-defined. fixed underlying type 이 있으면
  열거형의 값 범위가 열거자 범위가 아니라 **underlying type 의 범위**다.
- `sizeof(alcv_status_t) == sizeof(int)` 가 플랫폼 ABI 관행이 아니라 표준 보장이 된다.

**강제 수단**은 두 층으로 나눴다.

*컴파일 타임* — 소비자 쪽에서도 터지므로 우리 테스트를 기다릴 필요가 없는 것들:

| 위치 | assert | 잡는 것 |
|---|---|---|
| 공개 헤더 | `ALCV_OK == 0` | `if (rc)` 관용구 파괴 |
| `status_abi_test.cpp` | `is_same_v<underlying_type_t<...>, int>` | underlying type 변경 |
| `status_abi_test.cpp` | `is_convertible_v<alcv_status_t, int>` | `enum class` 로의 전환 |

`std::is_scoped_enum` 은 C++23 이라 못 쓴다. "int 로 암시적 변환되는가" 가 C++17 에서의
대체 지표다 — scoped enum 이 되는 순간 false 가 된다.
무거운 `<type_traits>` 는 공개 헤더가 아니라 테스트에만 include 한다. SDK 헤더의 include
비용은 모든 소비자가 지불하기 때문이다.

*런타임 테스트* — 의도적 변경 시 "어디를 고쳐야 하는지" 알려주는 것들:

```cpp
CHECK(ALCV_OK              ==  0);
CHECK(ALCV_ERR_INVALID_ARG == -1);
CHECK(ALCV_ERR_NO_MEMORY   == -2);
CHECK_MSG(k_count == 3u, "...pin the new code above and bump this number");
```

이 pin 목록은 **손으로 관리한다. X-macro 에서 생성하지 않는다.** 생성하면 값이 바뀔 때
검증하는 쪽도 같이 바뀌어서 영원히 통과한다 — 검증하는 척만 하는 테스트가 된다.
마지막 `k_count == 3u` 가 forcing function 이다. 코드를 추가하면 이 줄이 깨지고, 개발자는
pin 을 추가하면서 "이 번호는 이제 영구 예약된다"를 의식하게 된다.

번호 **중복**은 별도 장치가 필요 없다. 같은 값의 열거자 둘은 생성된 switch 에서
duplicate case value 컴파일 에러가 된다.

### 검토한 대안

| 대안 | 기각 사유 |
|---|---|
| `enum class alcv_status : int` | 암시적 변환이 없어 `if (rc)` 가 깨진다. 요구사항 위반. 타입 안전성은 얻지만 SDK 소비자에게 `static_cast` 를 강요하고 C ABI 래퍼를 얹을 때도 걸린다 |
| `typedef int alcv_status_t` + 익명 enum | 범위 밖 값이 어떤 int 든 유효해져 가장 안전해 보이지만, switch 대상이 int 가 되어 **`-Wswitch` 누락 검출이 통째로 죽는다**. 이 프로젝트의 핵심 안전장치를 잃는다 |
| underlying type 미지정 (`enum alcv_status {`) | C++ 에서 범위 밖 값 캐스팅이 UB 가 되고, `sizeof` 가 표준 보장에서 플랫폼 ABI 관행으로 내려간다 |
| pin 목록을 X-macro 로 생성 | 값과 함께 바뀌어 항상 통과한다. 검증 가치가 0 |
| 번호 재사용 허용 (deprecated 코드 회수) | 구버전 소비자 바이너리와 신버전 SDK 가 같은 숫자를 다르게 해석한다. 진단 불가능한 버그가 된다 |
| `extern "C"` C ABI 경계 유지 | `: int` 를 `#ifdef __cplusplus` 로 감싸야 하고, C 빌드 경로에서는 범위 밖 값 unspecified 리스크가 그대로 남는다. C 소비자 요구가 없어 보류 |

## 트레이드오프 / 리스크

- **C 소비자 지원을 포기했다.** 전면 C++ 헤더라 C 코드에서 include 할 수 없다. 나중에
  필요해지면 `extern "C"` 래퍼 레이어를 별도로 얹어야 한다.
- **unscoped enum 이라 타입 안전성이 낮다.** `alcv_status_t` 가 int 와 자유롭게 섞인다.
  `if (rc)` 를 살리기 위해 의도적으로 지불한 대가다.
- **`noexcept` 는 이제 뗄 수 없다.** C++17 에서 함수 타입의 일부(P0012R1)이므로 제거는
  ABI 변경이다. 헤더 주석에 명시했지만 컴파일러가 잡아주지는 않는다 — 규율에 의존한다.
- **pin 목록은 수동이다.** 사람이 값을 pin 과 리스트 양쪽에서 똑같이 바꾸면 통과한다.
  이 장치가 막는 것은 "무심코 바꾸는 것"이지 "작정하고 바꾸는 것"이 아니다.
- **실제 ABI 검증 도구를 쓰지 않는다.** abi-compliance-checker, symbol versioning(`.map`),
  심볼 diff 는 도입하지 않았다. 여기서 잠근 것은 소스 레벨 계약까지다.
- 에러 **컨텍스트**(어디서 왜 실패했는지)는 상태 코드만으로 전달할 수 없다. thread-local
  상세 메시지 같은 후속 설계가 별도로 필요하다.

## 검증

환경: Apple clang 17.0.0 (arm64), CMake 4.3.3, `-std=gnu++17`.

- clean 빌드 무경고, `ctest` 2/2 통과.
- **번호 pin**: 리스트에 `X(ALCV_ERR_TIMEOUT, -3, ...)` 추가 → 라이브러리는 무경고 빌드,
  `status_test` 통과, `status_abi_test` 만 실패:
  `FAIL tests/status_abi_test.cpp:72: k_count == 3u`. forcing function 이 의도대로 발동.
- **underlying type**: `: int` → `: short` 로 변경 → **컴파일 단계에서** 실패.
  `static assertion failed ... 'std::is_same_v<short, int>'` 및 `sizeof` assert 2건.
- **`ALCV_OK == 0`**: 값을 1로 변경 → 공개 헤더의 static_assert 가
  `include/alcv/status.h:39` 에서 컴파일 에러. 테스트 실행 전에 차단됨을 확인.
- **포인터 동일성 (static storage 계약)**: 정의된 전 코드 + fallback 에 대해
  `alcv_status_str(c) == alcv_status_str(c)` 성립 확인.
- **범위 밖 값**: `-999`, `12345` 모두 정의된 동작으로 fallback 문자열 반환.
