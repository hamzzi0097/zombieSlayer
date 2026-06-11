# 🧟 SpitMaster

> DirectX 11 기반 탑다운 슈터 — 밀려오는 좀비 떼를 막아라

---

## 📋 세부 기획

👉 [Notion 기획 문서](https://gold-composer-055.notion.site/26-1-359f04b1e0af806a9f9dc078c1334e7b?pvs=74)

---

## 🎮 게임 소개

**SpitMaster**는 Win32 API와 DirectX 11로 제작된 탑다운 슈터 게임입니다.  
플레이어는 사방에서 밀려오는 좀비를 총알로 처치하며 생존을 이어갑니다.

---

## 🛠️ 기술 스택

| 항목 | 내용 |
|------|------|
| 언어 | C++ |
| 그래픽 API | DirectX 11 |
| 플랫폼 API | Win32 API |
| 셰이더 | HLSL |
| 빌드 환경 | Visual Studio 2022 |

---

## ✨ 주요 기능

- 🔫 **슈팅 시스템** — 탄환 발사 및 충돌 처리
- 🧟 **몬스터 AI** — FSM 기반 좀비 행동 패턴
- 🎯 **컴포넌트 기반 구조** — GameLoop, ColliderComponent, ConstantBuffer 등
- 📐 **화면 비율 보정** — Aspect Ratio Correction 적용

### ➕ 추가 구현 사항

- 🧱 **장애물 시스템** — 맵 내 벽 배치 및 탄환 차단/반사

---

## 👥 팀원

| 이름 | 학번 | 역할 |
|------|------|------|
| 김태현 | 12211593 | 게임 루프 구현 |
| 김동우 | 12211569 | 플레이어 구현 |
| 노우현 | 12211599 | 몬스터 구현 |

---

## 📁 프로젝트 구조

```
SpitMaster/
├── src/
│   ├── Core/           # GameLoop, GameManager
│   ├── Components/     # ColliderComponent, RenderComponent 등
│   ├── Objects/        # Player, Monster, Bullet, Wall
│   └── Shaders/        # HLSL 셰이더 파일
├── assets/             # 텍스처, 사운드 리소스
└── README.md
```

---

## 🚀 빌드 및 실행

1. Visual Studio 2022 에서 솔루션 파일 열기
2. DirectX 11 SDK 설치 확인
3. `x64 / Debug` 또는 `x64 / Release` 로 빌드
4. `SpitMaster.exe` 실행

---

## 🎓 개발 배경

인하대학교 **게임프로그래밍** 수업 (2026년 1학기) 팀 프로젝트
