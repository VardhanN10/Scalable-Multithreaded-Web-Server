# Firmware Update (FWU) Flow — Complete Analysis
> `fwUpdate2.0` via `SlaveFlashing` | 5 Slaves | MLX81116 Bootloader

---

## 🗺️ Bus Topology

```
ESP32 (Master)
    │
    └──[MeLiBu Bus]──┬── Slave 1 (NAD 4)
                     ├── Slave 2 (NAD 5)
                     ├── Slave 3 (NAD 6)
                     ├── Slave 4 (NAD 7)
                     └── Slave 5 (NAD 8)

NAD 0  = Broadcast (all slaves receive simultaneously)
NAD BC = Broadcast (used specifically during flash write)
NAD 4–8 = Individual slave addresses
```

---

## 📋 Command Sequence — Quick Reference

| # | BlCmd | Command | NAD | SID | Delay | Purpose |
|---|-------|---------|-----|-----|-------|---------|
| 0 | BlCmd[0] | CMD_EPM | 0 (BC) | `0x7FF` | 100ms | Enter Programming Mode |
| 1 | BlCmd[1] | CMD_GET_STATUS1 | 0 → 4–8 | `0x700` / `0xF700` | 100µs | Confirm boot mode OK |
| 2 | BlCmd[2] | CMD_UNLOCK_MEM | 0 → 4–8 | `0xe707/0xe708` | none | Seed/Key per slave |
| 3 | BlCmd[3] | CMD_ERASE_FLASH | 0 (BC) | `0x705` | 100µs | Erase flash |
| 4 | BlCmd[4] | CMD_GET_STATUS2 | 0 → 4–8 | `0x700` / `0xF700` | 100µs | Confirm erase OK |
| 5 | BlCmd[5] | CMD_MOVE_PTR | 0 (BC) | `0x704` | 100µs | Set write address 0x5800 |
| 6 | BlCmd[6] | CMD_WRITE_FLASH | NAD BC | `0x111` | 2ms/subpage | Write firmware pages |
| 7 | BlCmd[7] | CMD_CHECK_FLASH | 0 (BC) | `0x701` | 100µs + delay | CRC verify |
| 8 | BlCmd[8] | CMD_GET_STATUS3 | 0 → 4–8 | `0x700` / `0xF700` | 100µs | Confirm CRC OK |
| 9 | BlCmd[9] | CMD_RESET | 0 (BC) | `0xEF55` | none | Reboot to application |
| 10 | BlCmd[10] | CMD_DONE | — | — | — | End marker ✅ |

---

## STAGE 0 — Startup and Scheduler Setup

```mermaid
sequenceDiagram
    participant M as ESP32 Master
    participant SCH as Scheduler

    Note over M: fwUpdate2.0 starts via SlaveFlashing
    Note over M: State = S_INIT, currSchedule = SCHEDULE_DEFAULT
    M->>SCH: S_RQ_SCHEDULE requesting DIRECT_TRANSFER
    SCH-->>M: Cannot switch yet, must halt first
    M->>SCH: HaltScheduler()
    SCH-->>M: SchedulerHaltedCallback fired
    M->>SCH: Request DirectTransferAccess for BlCmd[0]
    SCH-->>M: DT_AccessGrantedCallback
    Note over M: Ready to send CMD_EPM
```

---

## STAGE 1 — BlCmd[0]: CMD_EPM (Enter Programming Mode)

```mermaid
sequenceDiagram
    participant M as ESP32 Master
    participant BC as NAD 0 Broadcast

    M->>BC: BlCmd[0] SID=0x7FF NAD=0 len=10
    Note over BC: Payload bytes
    Note over BC: [00]=0x09 [01]=0xFE [02]=0xDE [03]=0xAD [04]=0xBE
    Note over BC: [05]=0xEF [06]=0xFF [07]=0x7F [08]=0xFF [09]=0xFF
    Note over M: DTxDoneCallback fires
    Note over M: 100ms delay - waiting for slaves to boot into BL mode
    Note over M: Progress to CMD_GET_STATUS1
```

> **What this does:** Sends a magic 10-byte sequence to all 5 slaves simultaneously.
> The payload is the bootloader entry key — slaves recognize it and switch from
> application mode into bootloader (programming) mode.

---

## STAGE 2 — BlCmd[1]: CMD_GET_STATUS1 (Verify Boot Mode)

```mermaid
sequenceDiagram
    participant M as ESP32 Master
    participant BC as NAD 0 Broadcast
    participant S1 as Slave 1 NAD 4
    participant S2 as Slave 2 NAD 5
    participant S3 as Slave 3 NAD 6
    participant S4 as Slave 4 NAD 7
    participant S5 as Slave 5 NAD 8

    M->>BC: GetStatusCmd SID=0x700 NAD=0 len=2
    Note over M: DTxDoneLoopedCallback - now fetch from each slave
    M->>S1: GetStatusRsp[1] SID=0xF700 len=8
    S1-->>M: B5=0x10 OK
    M->>S2: GetStatusRsp[2] SID=0xF700 len=8
    S2-->>M: B5=0x10 OK
    M->>S3: GetStatusRsp[3] SID=0xF700 len=8
    S3-->>M: B5=0x10 OK
    M->>S4: GetStatusRsp[4] SID=0xF700 len=8
    S4-->>M: B5=0x10 OK
    M->>S5: GetStatusRsp[5] SID=0xF700 len=8
    S5-->>M: B5=0x10 OK
    Note over M: All 5 slaves OK - proceed to CMD_UNLOCK_MEM
```

> **Response format:** `B0=0x00 B1=0x00 B2=0x00 B3=0x00 B4=0x00 B5=0x10`
> Only B5 matters — `0x10` = OK. If B5 = PEN (Pending), master retries in a loop until all slaves are ready.

---

## STAGE 3 — BlCmd[2]: CMD_UNLOCK_MEM (Security Seed/Key)

```mermaid
sequenceDiagram
    participant M as ESP32 Master
    participant S1 as Slave 1 NAD 4
    participant S2 as Slave 2 NAD 5
    participant S3 as Slave 3 NAD 6
    participant S4 as Slave 4 NAD 7
    participant S5 as Slave 5 NAD 8

    Note over M: GetSeed broadcast sent SID=0xe707 NAD=0 len=2

    M->>S1: GetSeedRsp[1] SID=0xF707 len=6
    S1-->>M: Seed=0x12345678
    Note over M: Compute key for seed 0x12345678
    M->>S1: SecKey[1] SID=0xe708 key=da f2 9f e6

    M->>S2: GetSeedRsp[2] SID=0xF707 len=6
    S2-->>M: Seed=0x12345678
    M->>S2: SecKey[2] SID=0xe708 key=da f2 9f e6

    M->>S3: GetSeedRsp[3] SID=0xF707 len=6
    S3-->>M: Seed=0x12345678
    M->>S3: SecKey[3] SID=0xe708 key=da f2 9f e6

    M->>S4: GetSeedRsp[4] SID=0xF707 len=6
    S4-->>M: Seed=0x12345678
    M->>S4: SecKey[4] SID=0xe708 key=da f2 9f e6

    M->>S5: GetSeedRsp[5] SID=0xF707 len=6
    S5-->>M: Seed=0x12345678
    M->>S5: SecKey[5] SID=0xe708 key=da f2 9f e6

    Note over M,S5: Memory UNLOCKED on all 5 slaves
```

> **Seed:** `0x12345678` (same from all slaves — fixed in bootloader)
> **Key:** `0xda 0xf2 0x9f 0xe6` (same for all — derived from fixed seed)
> Done slave-by-slave sequentially, not broadcast.

---

## STAGE 4 — BlCmd[3]: CMD_ERASE_FLASH

```mermaid
sequenceDiagram
    participant M as ESP32 Master
    participant BC as NAD 0 Broadcast

    M->>BC: BlCmd[3] SID=0x705 NAD=0 len=6
    Note over BC: Payload bytes
    Note over BC: [00]=0x05 [01]=0x81 [02]=0x24 [03]=0x01 [04]=0x81 [05]=0x04
    Note over M: DTxDoneCallback fires
    Note over M: 100us delay
    Note over M: Slaves erase flash in background
    Note over M: Progress to CMD_GET_STATUS2
```

> **Payload:** Encodes the flash region to erase (start address + size descriptor).
> All 5 slaves erase simultaneously on receipt of this broadcast.

---

## STAGE 5 — BlCmd[4]: CMD_GET_STATUS2 (Confirm Erase OK)

```mermaid
sequenceDiagram
    participant M as ESP32 Master
    participant S1 as Slave 1 NAD 4
    participant S2 as Slave 2 NAD 5
    participant S3 as Slave 3 NAD 6
    participant S4 as Slave 4 NAD 7
    participant S5 as Slave 5 NAD 8

    Note over M: GetStatusCmd sent SID=0x700 NAD=0 len=2
    M->>S1: Fetch SID=0xF700
    S1-->>M: B5=0x10 OK
    M->>S2: Fetch SID=0xF700
    S2-->>M: B5=0x10 OK
    M->>S3: Fetch SID=0xF700
    S3-->>M: B5=0x10 OK
    M->>S4: Fetch SID=0xF700
    S4-->>M: B5=0x10 OK
    M->>S5: Fetch SID=0xF700
    S5-->>M: B5=0x10 OK
    Note over M: All erased OK - proceed to CMD_MOVE_PTR
```

---

## STAGE 6 — BlCmd[5]: CMD_MOVE_PTR (Set Flash Write Address)

```mermaid
sequenceDiagram
    participant M as ESP32 Master
    participant BC as NAD 0 Broadcast

    M->>BC: BlCmd[5] SID=0x704 NAD=0 len=6
    Note over BC: Payload bytes
    Note over BC: [00]=0x03 [01]=0x10 [02]=0x00 [03]=0x58 [04]=0x00 [05]=0x00
    Note over BC: Address = 0x5800 (start of firmware region)
    Note over M: DTxDoneCallback - 100us delay
    Note over M: All slaves set write pointer to 0x5800
    Note over M: Progress to CMD_WRITE_FLASH
```

> **Address decode:** Bytes [01–05] = `10 00 58 00 00` → little-endian → `0x005800`
> This is where the application firmware starts in flash memory.

---

## STAGE 7 — BlCmd[6]: CMD_WRITE_FLASH (Main Flash Loop)

### Page Structure

```
1 Page = 8 Subpages x 16 bytes = 128 bytes of firmware data
Each subpage PDU = 18 bytes (2 header + 16 data)

Subpage 0 → offset   0
Subpage 1 → offset  16
Subpage 2 → offset  32
Subpage 3 → offset  48
Subpage 4 → offset  64
Subpage 5 → offset  80
Subpage 6 → offset  96
Subpage 7 → offset 112
```

### Write + Verify Loop (repeats for every page)

```mermaid
sequenceDiagram
    participant M as ESP32 Master
    participant BC as NAD BC Broadcast
    participant S1 as Slave 1 NAD 4
    participant S2 as Slave 2 NAD 5
    participant S3 as Slave 3 NAD 6
    participant S4 as Slave 4 NAD 7
    participant S5 as Slave 5 NAD 8

    Note over M: start WriteFlash for page 000
    M->>BC: Subpage[0] SID=0x111 len=18 offset=0
    Note over M: 2ms delay
    M->>BC: Subpage[1] SID=0x111 len=18 offset=16
    Note over M: 2ms delay
    M->>BC: Subpage[2] SID=0x111 len=18 offset=32
    Note over M: 2ms delay
    M->>BC: Subpage[3] SID=0x111 len=18 offset=48
    Note over M: 2ms delay
    M->>BC: Subpage[4] SID=0x111 len=18 offset=64
    Note over M: 2ms delay
    M->>BC: Subpage[5] SID=0x111 len=18 offset=80
    Note over M: 2ms delay
    M->>BC: Subpage[6] SID=0x111 len=18 offset=96
    Note over M: 2ms delay
    M->>BC: Subpage[7] SID=0x111 len=18 offset=112
    Note over M: 8 subpages done - now fetch write-status from all slaves
    M->>S1: WriteFlashStatusRsp SID=0xF111 len=6 page=000
    S1-->>M: B5=0x10 OK
    M->>S2: WriteFlashStatusRsp SID=0xF111 len=6 page=000
    S2-->>M: B5=0x10 OK
    M->>S3: WriteFlashStatusRsp SID=0xF111 len=6 page=000
    S3-->>M: B5=0x10 OK
    M->>S4: WriteFlashStatusRsp SID=0xF111 len=6 page=000
    S4-->>M: B5=0x10 OK
    M->>S5: WriteFlashStatusRsp SID=0xF111 len=6 page=000
    S5-->>M: B5=0x10 OK
    Note over M: Page 000 verified - repeat for page 001 002 ...
```

---

## STAGE 8 — BlCmd[7]: CMD_CHECK_FLASH (CRC Integrity Verify)

```mermaid
sequenceDiagram
    participant M as ESP32 Master
    participant BC as NAD 0 Broadcast

    M->>BC: BlCmd[7] SID=0x701 NAD=0 len=4
    Note over BC: Payload bytes
    Note over BC: [00]=0x02 [01]=0x02 [02]=0x01 [03]=0xFF
    Note over M: DTxDoneCallback - 100us delay
    Note over M: ReqBlDelay fires - slaves computing CRC internally
    Note over M: Progress to CMD_GET_STATUS3
```

> **What this does:** Commands all slaves to compute a CRC checksum over the written firmware.
> The `ReqBlDelay` gives slaves enough time to finish the CRC calculation before the master polls.

---

## STAGE 9 — BlCmd[8]: CMD_GET_STATUS3 (Confirm CRC OK)

```mermaid
sequenceDiagram
    participant M as ESP32 Master
    participant S1 as Slave 1 NAD 4
    participant S2 as Slave 2 NAD 5
    participant S3 as Slave 3 NAD 6
    participant S4 as Slave 4 NAD 7
    participant S5 as Slave 5 NAD 8

    Note over M: GetStatusCmd sent SID=0x700 NAD=0 len=2
    M->>S1: Fetch SID=0xF700
    S1-->>M: B5=0x10 CRC OK
    M->>S2: Fetch SID=0xF700
    S2-->>M: B5=0x10 CRC OK
    M->>S3: Fetch SID=0xF700
    S3-->>M: B5=0x10 CRC OK
    M->>S4: Fetch SID=0xF700
    S4-->>M: B5=0x10 CRC OK
    M->>S5: Fetch SID=0xF700
    S5-->>M: B5=0x10 CRC OK
    Note over M: Firmware verified on all 5 slaves - proceed to RESET
```

---

## STAGE 10 — BlCmd[9]: CMD_RESET (Reboot to Application)

```mermaid
sequenceDiagram
    participant M as ESP32 Master
    participant BC as NAD 0 Broadcast

    M->>BC: BlCmd[9] SID=0xEF55 NAD=0 len=6
    Note over BC: Payload bytes
    Note over BC: [00]=0x04 [01]=0x1B [02]=0x7C [03]=0xE4 [04]=0x83 [05]=0xFF
    Note over M: No delay - progress immediately
    Note over M,BC: All slaves reboot into newly flashed application firmware
    Note over M: BlCmd[10] CMD_DONE reached
    Note over M: Flashing procedure finished with 5 slaves flashed
```

> **Payload:** Reset magic sequence — slaves exit bootloader and jump to application start address.

---

## ⏱️ Timing Summary

| Command | Delay After TX | Reason |
|---------|---------------|--------|
| CMD_EPM | **100ms** | Slaves need time to fully boot into BL mode |
| CMD_GET_STATUS | **100µs** | Minimal — response fetched immediately after |
| CMD_ERASE_FLASH | **100µs** | Erase runs in background on slave side |
| CMD_MOVE_PTR | **100µs** | Pointer set instantly |
| CMD_WRITE_FLASH | **2ms per subpage** | Flash write time per 16-byte chunk |
| CMD_CHECK_FLASH | **100µs + ReqBlDelay** | Slaves need time to compute CRC |
| CMD_RESET | **none** | Immediate reboot |

---

## 🧠 Key Terms

| Term | Meaning |
|------|---------|
| **NAD** | Node Address — unique ID of each slave on the bus |
| **SID** | Service Identifier — type of PDU being sent |
| **PDU** | Protocol Data Unit — the actual data packet |
| **BlCmd[N]** | Bootloader Command index N — ordered FWU steps |
| **B5=0x10** | Status byte value meaning slave responded OK |
| **B5=PEN** | Pending — slave not ready, master must retry |
| **Seed/Key** | Security challenge-response to unlock flash |
| **Page** | 128 bytes of firmware (8 subpages x 16 bytes) |
| **Subpage** | 18-byte PDU = 2 header + 16 firmware bytes |
| **ReqBlDelay** | Explicit wait for slave to finish CRC computation |
| **DIRECT_TRANSFER** | One-shot schedule: master sends one frame |
| **DIRECT_TRANSFER_LOOPED** | Looped schedule: used for multi-slave polling and subpage writes |

---

*Generated from AutoAdddrV2_DebugPrints.txt — 08/06/2026*
