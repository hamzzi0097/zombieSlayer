# 🦟 Spit Master

> **SPIT TO SURVIVE!** — DirectX 11로 직접 만든 탑다운 생존 슈터

사방에서 몰려오는 벌레 떼를 침(스핏)으로 쏘아 맞히며 최대한 오래 살아남는 게임입니다.
엔진 프레임워크부터 게임플레이, 사운드, 온라인 리더보드까지 **외부 게임 엔진 없이 C++ / DirectX 11로 직접 구현**했습니다.

---

## 📋 세부 기획

👉 [Notion 기획 문서](https://gold-composer-055.notion.site/26-1-359f04b1e0af806a9f9dc078c1334e7b?pvs=74)

---

## 🎮 게임 소개

| 항목 | 내용 |
|------|------|
| 장르 | 탑다운 생존 슈터 |
| 목표 | 최대한 오래 생존하며 많은 킬 기록하기 |
| 플랫폼 | Windows |
| 개발 환경 | DirectX 11, Win32 |

### 등장 몬스터

| 몬스터 | 타입 | 행동 |
|--------|------|------|
| 🦟 모기 (Mosquito) | 근접 | 플레이어를 추적해 몸통 박치기 |
| 🪲 노린재 (StinkBug) | 원거리 | 일정 거리를 유지하며 탄 발사 |

시간이 지날수록 몬스터 스폰 간격이 짧아져 난이도가 점점 올라갑니다.

---

## ⌨️ 조작법

| 입력 | 동작 |
|------|------|
| `W` `A` `S` `D` | 이동 |
| 마우스 이동 | 조준 |
| 마우스 좌클릭 | 일반 공격 (침 발사) |
| `R` | 재장전 |
| 마우스 우클릭 | 모기향 폭탄 설치 |
| `F` | 전체화면 ↔ 창모드 전환 (공통) |
| `SPACE` | 게임 시작 / 재시작 |
| `1` | 로비로 이동 (GameOver) |
| `ESC` | 게임 종료 (Lobby) |

---

## 🏆 점수 & 리더보드

```
SCORE = 생존 시간(초) × 10 + 킬 수 × 100
```

게임 오버 시 점수가 서버(Firebase Realtime DB)에 업로드되고, **TOP 10 리더보드**가 결과 화면에 표시됩니다.

---

## 🛠️ 기술 스택

| 항목 | 내용 |
|------|------|
| 언어 | C++ |
| 그래픽 | DirectX 11, HLSL (런타임 셰이더 컴파일) |
| 사운드 | XAudio2 + Media Foundation (mp3 디코딩) |
| 네트워크 | WinHTTP 기반 자체 HttpClient + 경량 JSON 파서 (MiniJson) |
| 플랫폼 API | Win32 |
| 빌드 환경 | Visual Studio 2022 (x64) |

**외부 라이브러리 의존성 없음** — 렌더링·입력·텍스처 로딩(WIC)·오디오·HTTP 통신 모두 Windows 기본 API로 구현했습니다.

---

## 🧩 아키텍처

Unity의 설계 철학을 C++로 옮긴 **컴포넌트 기반 구조**입니다.

- **GameObject + Component** — 모든 게임 요소는 빈 컨테이너(`GameObject`)에 기능 단위(`Component`)를 조합해 구성
- **GameLoop (FSM)** — `Lobby → Playing → GameOver` 상태 기계가 게임 흐름과 상태별 UI 캔버스를 제어
- **pendingObjects 큐** — 순회 중 생성된 오브젝트를 다음 프레임에 합류시켜 이터레이터 무효화 방지
- **콜백 기반 이펙트** — 피격(`onDamaged`)·폭발(`onExplode`)을 콜백으로 배선해 결합도 최소화
- **이중 포인터 UI 참조** — 라운드마다 재생성되는 플레이어를 UI가 `GameObject**`로 안전하게 추적
- **비동기 네트워크** — 리더보드 업로드/조회와 원격 로그 전송은 워커 스레드에서 처리, 게임 루프 무정지

### 주요 모듈

```
SpitMaster/
├── ZombieSlayer.sln
└── ZombieSlayer/
    ├── main.cpp                  # 엔트리 포인트
    ├── GameLoop.hpp              # 상태머신 · 메인 루프
    ├── ObjectBase.hpp            # GameObject / Component 기반 구조
    │
    ├── (코어) GraphicsContext · WindowContext · Timer · Logger · Framework
    ├── (렌더) Mesh · Material · MeshRenderer · Background · TextUI
    ├── (플레이어) PlayerController · PlayerHealth · PlayerBulletSpawner · BombSpawner
    ├── (몬스터) MonsterSpawner · Melee/Ranged/DeadMonsterController · MonsterBulletSpawner
    ├── (충돌) Collider (CircleCollider, 레이어 기반)
    ├── (UI) HeartUI · AmmoUI · BombCooldownUI · Stats/Result/Blink/LeaderboardUIUpdater
    ├── (연출) HitEffect · ScreenShakeEffect
    ├── (사운드) AudioSystem · AudioSource
    ├── (온라인) Leaderboard · HttpClient · MiniJson · RemoteLogger
    └── (리소스) *.png · *.mp3 · *.hlsl
```

---

## 🚀 빌드 및 실행

1. `ZombieSlayer.sln`을 **Visual Studio 2022**로 열기
2. 구성: `x64 / Debug` 또는 `x64 / Release`
3. 빌드 후 실행 — 셰이더(`.hlsl`)·이미지(`.png`)·사운드(`.mp3`)는 빌드 시 출력 폴더로 자동 복사됩니다

> ⚠️ 실행 파일을 단독 배포할 경우, 같은 폴더에 `.hlsl` / `.png` / `.mp3` 리소스가 함께 있어야 합니다.

---

## 👥 팀원

| 이름 | 학번 | 역할 |
|------|------|------|
| 김태현 | 12211593 | 게임 루프 구현 |
| 김동우 | 12211569 | 플레이어 구현 |
| 노우현 | 12211599 | 몬스터 구현 |

---

## 🎓 개발 배경

인하대학교 **게임프로그래밍** 수업 (2026년 1학기) 팀 프로젝트

---

## 📝 개발 후기

> 처음엔 막막함뿐이었지만, 부딪히며 만들다 보니 하나둘 기능이 쌓였습니다.
> 완성품을 보며 느낀 뿌듯함, 그리고 아쉬움.
> 그 아쉬움을 발판 삼아 앞으로도 보수와 업그레이드를 이어가려 합니다.
