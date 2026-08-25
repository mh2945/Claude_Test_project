# 상태 코드 → 문자열 매핑을 X-macro + default 없는 switch 로 구현

- 날짜: 2026-08-25
- 관련 파일: include/alcv/status.h, src/status.cpp, src/CMakeLists.txt, tests/status_test.cpp

## 왜 (문제)

`alcv_status_str()` 은 절대 nullptr 을 반환하면 안 되고, 정의된 모든 코드에 서로 다른
메시지를 줘야 한다. 진짜 문제는 함수 자체가 아니라 **유지보수 중에 깨지는 방식**이다.

- enum 정의와 문자열 테이블이 물리적으로 떨어져 있으면, 코드를 추가하고 문자열 추가를
  잊는 실수가 컴파일도 테스트도 통과한다. 새 에러가 조용히 "unknown" 으로 나간다.
- 이건 리뷰로 막는 데 한계가 있다. 사람이 놓치면 그대로 나간다.
- 제약: 에러 코드는 음수이고 sparse 해질 수 있다 (0, -1, -2, ... 중간 결번 가능).
- 제약: SDK 공개 헤더이므로 소비자가 지불할 include 비용과 표준 요구치를 최소화해야 한다.

## 어떻게 (설계)

`ALCV_STATUS_LIST(X)` 하나를 단일 정의 소스로 두고 세 곳에서 전개한다.

| 전개처 | 생성물 |
|---|---|
| `include/alcv/status.h` | enum 열거자 |
| `src/status.cpp` | `case` 레이블 + `return` 문 |
| `tests/status_table.hpp` | 테스트가 순회할 코드/이름 테이블 |

코드와 메시지가 **같은 줄**에 있으므로 "코드만 추가" 자체가 불가능하다.
테스트 테이블도 같은 리스트에서 생성되므로 코드를 추가하면 커버리지가 자동으로 따라온다.

`src/status.cpp` 의 switch 에는 **`default:` 를 두지 않고** switch 뒤에 fallback `return` 을
둔다. `default:` 가 있으면 `-Wswitch` 가 미처리 열거자를 더 이상 보고하지 않기 때문이다.
즉 안전장치를 살리기 위해 방어적으로 보이는 코드를 일부러 뺀다.

여기에 컴파일러 경고 3중 방어를 건다 (`src/CMakeLists.txt`, `PRIVATE`).

| 플래그 | 발동 조건 | 막는 실수 |
|---|---|---|
| `-Wswitch` (`-Wall` 포함) | default 없음 + 열거자 누락 | 코드 추가 후 문자열 누락 |
| `-Wswitch-enum` | **default 가 있어도** 열거자 누락 | 누가 default 를 넣어 위 경고를 죽인 경우 |
| `-Wcovered-switch-default` (clang) | 전부 커버했는데 default 존재 | default 를 넣으려는 시도 자체 |

fallback 은 반드시 문자열 리터럴이다. `snprintf` 로 코드 번호를 static 버퍼에 찍는 방식은
채택하지 않았다 — 스레드 안전성이 깨지고, "같은 코드 → 같은 포인터" 계약이 깨지며,
다음 호출이 이전 반환값을 덮어쓴다.

### 검토한 대안

| 대안 | 기각 사유 |
|---|---|
| 배열 인덱싱 (`msgs[s]`) | 코드가 음수이고 sparse 해질 수 있어 인덱스로 쓸 수 없다. offset 보정을 넣으면 결번 구간에 구멍이 생기고 범위 검사가 또 하나의 실수 지점이 된다 |
| 손으로 쓴 switch | 항목 추가 시 갱신 누락이 정확히 우리가 막으려던 실수다. `-Wswitch` 가 잡아주긴 하지만 `default:` 한 줄로 무력화된다 (아래 검증에서 실측) |
| `std::unordered_map<alcv_status_t, const char*>` | 정적 초기화 순서 문제, 런타임 할당, 조회 비용. 3~수십 개짜리 고정 테이블에 해시맵은 과하다 |
| `constexpr` 함수로 헤더에 inline 정의 | 컴파일 타임 사용은 가능해지지만 문자열이 소비자 바이너리에 박힌다. 메시지 오타 하나 고치는 데 소비자 전체 재컴파일이 필요해진다 |
| X-macro 대신 코드 생성 스크립트 (python → .cpp) | 빌드에 스크립트 의존이 생기고, 생성물과 소스가 어긋날 여지가 새로 생긴다. 이 규모에서는 얻는 게 없다 |

## 트레이드오프 / 리스크

- **X-macro 는 디버깅이 불편하다.** 매크로 전개 결과가 디버거·에러 메시지에 그대로 보이지
  않는다. 전개 확인이 필요하면 `c++ -E` 를 거쳐야 한다.
- **`-Wswitch-enum` 은 시끄러워질 수 있다.** 의도적으로 `default:` 를 쓰는 switch 가 앞으로
  늘면 이 플래그가 그 전부에 대해 경고한다. 그 시점에 `-Wswitch` 로 낮추는 판단이 필요하다.
- **`-Wcovered-switch-default` 는 clang 전용이다.** GCC 에는 대응 경고가 없어 GCC 빌드에서는
  3중 방어가 2중으로 줄어든다. **(GCC 에서 실제로 빌드해 보지 않음 — 미확인)**
- MSVC 의 `/w14061`, `/w14062` 는 문서 기준으로 넣었고 **이 환경(macOS/arm64)에서 검증 불가**.
- 공개 헤더가 `ALCV_STATUS_LIST` 매크로를 노출한다. 소비자 네임스페이스를 매크로 하나만큼
  오염시키는 대가로, 소비자도 같은 리스트를 전개해 자기 테이블을 만들 수 있게 된다.

## 검증

환경: Apple clang 17.0.0 (arm64), CMake 4.3.3, `-std=gnu++17`, `-Werror` ON.

- clean 빌드 무경고, `ctest` 2/2 통과.
- **경고 3중 방어 실측** (손으로 쓴 switch 3종 × 플래그 3종, 직접 컴파일):

  | 케이스 | `-Wall` | `+ -Wswitch-enum` | `+ -Wcovered-switch-default` |
  |---|---|---|---|
  | 열거자 누락, default 없음 | `-Wswitch` 발동 | 발동 | 발동 |
  | 열거자 누락, default 있음 | **침묵** | `-Wswitch-enum` 발동 | 발동 |
  | 전부 커버, default 있음 | 침묵 | 침묵 | `-Wcovered-switch-default` 발동 |

  두 번째 행의 "침묵" 이 이 ADR 의 근거다. `default:` 한 줄로 `-Wall` 이 완전히 눈을 감는다.
- **코드 추가 시나리오**: `X(ALCV_ERR_TIMEOUT, -3, "operation timed out")` 을 리스트에 추가
  → 라이브러리 무경고 빌드, `status_test` 가 새 코드를 자동으로 커버해 통과.
  문자열 누락이 발생할 경로 자체가 없음을 확인.
- **메시지 중복 검출**: 두 코드에 같은 문자열을 넣자 `status_test` 가
  `ALCV_ERR_INVALID_ARG and ALCV_ERR_NO_MEMORY share the message "invalid argument"` 로
  검출, exit code 1.
- **fallback**: `static_cast<alcv_status_t>(-999)`, `12345` 모두 non-null / 비어 있지 않음 /
  정의된 어떤 메시지와도 불일치 확인.
