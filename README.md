
# IOCP 기반 비동기 서버 코어
[IOCP Server](https://www.notion.so/IOCP-25292dff9081805aae49c7d2d1165d73?source=copy_link)

멀티스레드 워커 풀과 세션 풀(Session Pool)을 갖춘 IOCP(I/O Completion Port) 기반 네트워크 서버 코어입니다. 안정적인 연결 처리, 패킷 프레이밍, 전송 상태 머신을 제공하며 Gate / Game / GameDB 등 확장형 구조의 공통 네트워크 코어로 사용됩니다.

---

## 주요 특징

- **세션 풀 & 팩토리 주입**
  - 타입별 세션을 사전 생성/재사용(pop/push)하여 지연과 메모리 파편화 최소화
  - Accept 시 할당, 종료 시 반납

- **IOCP 워커 루프**
  - `GetQueuedCompletionStatus`로 완료 이벤트 수신
  - `ACCEPT / RECV / SEND`로 분기하여 해당 세션 메서드로 디스패치

- **Accept 경로 최적화**
  - `AcceptEx` 완료 → `SO_UPDATE_ACCEPT_CONTEXT` → 소켓 IOCP 등록 → 첫 `WSARecv` 게시 → 다음 Accept 재게시

- **수신(Recv) 링버퍼 & 패킷 조립**
  - TCP 스트림을 `[size][id][payload]` 단위로 복원
  - 헤더 미만은 보류, 길이 충족 시 1패킷 추출, 잔여 바이트는 다음 조립에 사용

- **전송(Send) 상태 머신**
  - 송신 큐 기반 조각 전송
  - 완료 시 pop 후 잔여 있으면 즉시 재전송, 없으면 idle 전환

- **패킷 디스패처**
  - 정적(화이트리스트) 등록 + 사이즈/ID 검증 후 핸들러 호출

- **종료/예외 처리**
  - 새 Accept 취소, 워커 종료 키 Post, 스레드 join/Close
  - 프로토콜 위반(QPS 과다, 비정상 size/ID)은 즉시 Close + 로그

- **환경설정**
  - 포트 등 네트워크 설정을 `Server.ini`로 관리

---

## 구조 개요

- **WorkerLoop → IocpContext.eOperation 분기**
  - `ACCEPT → OnAcceptCompleted`
  - `RECV → OnRecvCompleted`
  - `SEND → OnSendCompleted`

- **패킷 프레이밍 헤더**
  ```cpp
  struct PacketHeader {
      uint16_t size; // 전체 패킷 길이
      uint16_t id;   // 패킷 ID
      // payload follows...
  };
