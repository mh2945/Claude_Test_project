# CMake 타깃 구성 — 표준 요구치 분리와 테스트 등록 위치

- 날짜: 2026-08-25
- 관련 파일: CMakeLists.txt, src/CMakeLists.txt, tests/CMakeLists.txt

## 왜 (문제)

- SDK 라이브러리가 소비자 빌드에 무엇을 강요하는지가 명확해야 한다. 라이브러리가 자기
  편의로 건 `-Werror` 나 표준 요구치가 소비자에게 전염되면, 소비자는 컴파일러를 올리는
  것만으로 빌드가 깨진다.
- 공개 헤더가 실제로 요구하는 C++ 표준과, 우리 `.cpp` 를 컴파일하고 싶은 표준이 다르다.
  헤더는 `enum : int` / `static_assert` / `using` 별칭 = C++11 이면 충분한데,
  구현·테스트는 C++17 로 쓰고 싶다.
- 테스트 실행파일 2개를 `ctest` 로 한 번에 돌려야 한다.

## 어떻게 (설계)

**표준 요구치 분리**

```cmake
target_compile_features(alcv
    PUBLIC  cxx_std_11      # 헤더가 실제로 요구하는 최소치
    PRIVATE cxx_std_17)     # 우리 .cpp 가 컴파일되는 표준
```

CMake 는 타깃 자신의 표준을 두 요구의 최댓값(17)으로 잡고, `INTERFACE_COMPILE_FEATURES`
에는 11만 실어 보낸다. 소비자는 C++11 만 있으면 헤더를 쓸 수 있다.
공개 인터페이스에 표준 라이브러리 타입이 없어서(`const char*` 와 enum 뿐) TU 간 표준
혼용이 안전하다는 전제 위에서 성립한다. 공개 헤더에 `std::string`/`std::optional` 이
들어오면 PUBLIC 요구치를 그에 맞게 올려야 한다.

**전파 범위**

| 항목 | 범위 | 이유 |
|---|---|---|
| `target_include_directories(include)` | PUBLIC | 공개 헤더는 인터페이스의 일부. 링크하는 쪽이 찾을 수 있어야 함 |
| 경고 옵션 / `-Werror` | PRIVATE | 경고 정책은 우리 소스를 컴파일할 때의 문제. 소비자에게 전염되면 안 됨 |
| `alcv::alcv` ALIAS | — | `add_subdirectory` 든 `find_package` 든 소비자 코드가 동일. `::` 가 있으면 오타가 링크 에러가 아니라 configure 단계 에러가 됨 |

**테스트 등록**: `enable_testing()` 을 **최상위**에서, `add_subdirectory(tests)` 앞에 호출.
이 명령은 현재 디렉토리와 그 하위에 대해 테스트를 활성화하고 해당 빌드 디렉토리에
`CTestTestfile.cmake` 를 만든다. `ctest` 는 빌드 루트의 그 파일부터 읽고 `subdirs()` 를
따라 내려간다.

테스트 실행파일은 관심사로 나눴다 — `status_test`(동작, 실패 시 "코드를 고쳐라") 와
`status_abi_test`(계약, 실패 시 "이 변경을 하지 마라"). 조치가 정반대라 한 파일에 두면
실패 리포트를 읽고 뭘 해야 할지 판단하는 비용이 생긴다.

### 검토한 대안

| 대안 | 기각 사유 |
|---|---|
| `set(CMAKE_CXX_STANDARD 17)` 전역 | 디렉토리 스코프 변수라 타깃별 요구를 표현할 수 없고, `add_subdirectory` 로 편입될 때 부모 설정에 묻힌다 |
| `target_compile_features(alcv PUBLIC cxx_std_17)` | 헤더가 요구하지도 않는 C++17 을 소비자에게 강요한다 |
| `include(CTest)` | `enable_testing()` 을 포함하지만 CDash 대시보드용 타깃(`Experimental`, `Nightly` 등)을 함께 만든다. 쓰지 않을 타깃이다 |
| 테스트 실행파일 1개로 통합 | 동작 실패와 계약 위반이 같은 리포트에 섞인다. 조치가 정반대라 분리가 낫다 |
| `gtest_discover_tests` 식 자동 등록 | 외부 프레임워크 의존이 필요하다. 실행파일이 2개뿐이라 얻는 게 없다 |

## 트레이드오프 / 리스크

- **`PUBLIC cxx_std_11` 은 컴파일러 기본값에 의존하는 약한 안전장치다.** 검증에서 확인했듯
  요구치가 11이면 CMake 는 `-std` 플래그를 **아예 붙이지 않는다**(기본값이 이미 11 이상이므로).
  이 환경은 AppleClang 기본값이 C++14 라 테스트가 C++17 을 직접 요구하지 않으면 깨졌지만,
  기본값이 C++17 이상인 컴파일러에서는 요구치를 빠뜨려도 **우연히 빌드된다.** 즉 이 구성은
  "소비자에게 강요하지 않는다"는 보장이지, "우리가 요구를 빠뜨리면 잡아준다"는 보장이 아니다.
- 라이브러리와 테스트가 서로 다른 표준으로 컴파일된다. 지금은 인터페이스에 표준 라이브러리
  타입이 없어 안전하지만, 이 전제가 깨지는 순간 ODR/ABI 문제로 번질 수 있다.
- `install()` / `find_package` 지원은 넣지 않았다. `$<BUILD_INTERFACE:>` 만 있고
  `$<INSTALL_INTERFACE:>` 가 없어 현재는 `add_subdirectory` 소비만 가능하다.
- MSVC / GCC 에서 실제 빌드하지 않았다. 경고 플래그와 표준 분리 동작 **미확인**.

## 검증

환경: Apple clang 17.0.0 (arm64, 기본 표준 C++14), CMake 4.3.3, Unix Makefiles.

- `compile_commands.json` 대조: `status.cpp` → `-std=gnu++17` (PRIVATE 요구가 타깃 표준을
  17로 올림), 테스트 2개도 `-std=gnu++17` (자기 요구로).
- **PUBLIC/PRIVATE 분리 실증**: `tests/CMakeLists.txt` 에서 `target_compile_features(... cxx_std_17)`
  를 제거 → 테스트 TU 에 `-std` 플래그가 **하나도 붙지 않고**(요구치 11을 기본값 C++14 가
  이미 만족) 컴파일러 기본 C++14 로 컴파일 → `std::size`, `std::is_enum_v` 미존재로 빌드 실패.
  라이브러리가 11만 export 한다는 것이 실측으로 확인됨.
- **`enable_testing()` 위치**: 호출을 `tests/CMakeLists.txt` 로 옮기고 재설정 →
  빌드 루트에 `CTestTestfile.cmake` 없음(`tests/` 에만 생성) → `ctest` 가
  `No tests were found!!!` 출력. **이때 ctest 의 exit code 는 0(성공)** 이었다.
  CI 가 초록불인데 실제로는 0개를 돌린 상태가 그대로 재현된다 — CI 스크립트에서 실행된
  테스트 수가 0이면 실패로 처리하는 방어가 별도로 필요하다.
- 정상 구성에서 `ctest --test-dir build` → `100% tests passed, 0 tests failed out of 2`.
