# README
# Bare-Metal Automotive bxCAN Bus Network Logger

A low-level, high-reliability device driver implemented from scratch on the **ARM Cortex-M4 (STM32F407VG)** architecture. This project configures the microcontroller's internal **bxCAN (Basic Extended CAN)** peripheral via direct register manipulation to establish a self-testing, hardware-filtered automotive communication node operating at **125 Kbps**—completely bypassing all vendor Hardware Abstraction Layers (HAL).

## 🛠️ Tech Stack & Core Competencies
*   **Language:** Embedded C
*   **Hardware Architecture:** ARM Cortex-M4 (STM32F407 Discovery)
*   **Peripherals Configuration:** bxCAN, RCC, GPIO Alternative Function Routing, NVIC
*   **Development Utilities:** STM32CubeIDE, GDB Debugger via ST-LINK/V2, Live Expressions Memory Mapping

---

## System & Protocol Architecture

To eliminate the need for external hardware nodes and transceiver chips during validation, the bxCAN controller is initialized into **Loopback Mode**. In this internal diagnostic configuration, the transmitter's output path is internally routed back to the receiver's input register stages, enabling complete protocol stack validation using a single physical board.


