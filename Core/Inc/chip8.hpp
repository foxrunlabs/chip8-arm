////////////////////////////////////////////////////////////////////////////////
// CHIP-8 Emulator
// Copyright 2022 Ryan Clarke
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
////////////////////////////////////////////////////////////////////////////////

#ifndef CHIP8_HPP
#define CHIP8_HPP

#include "main.h"

#include "pcd8544.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <random>


////////////////////////////////////////////////////////////////////////////////
/// @brief CHIP8 emulator
////////////////////////////////////////////////////////////////////////////////
class CHIP8
{
  public:
    static constexpr int width{64};    // Screen width
    static constexpr int height{32};   // Screen height

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Constructor
    ////////////////////////////////////////////////////////////////////////////
    CHIP8();

    CHIP8(const CHIP8&)            = delete;
    CHIP8& operator=(const CHIP8&) = delete;
    CHIP8(CHIP8&&)                 = delete;
    CHIP8&& operator=(CHIP8&&)     = delete;

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Destructor
    ////////////////////////////////////////////////////////////////////////////
    ~CHIP8();

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Main loop
    ////////////////////////////////////////////////////////////////////////////
    void run();

  private:
    enum class state
    {
        splash,
        menu,
        emulate
    };

    state m_state{state::splash};

    static constexpr std::size_t org{512};   // Program start address
    std::size_t m_PC{org};                   // Program counter
    std::size_t m_SP{};                      // Stack pointer
    std::size_t m_I{};                       // Index register
    std::array<std::uint8_t, 16> m_V{};      // General purpose registers

    std::array<std::uint8_t, 4096> m_memory{};   // Program and data memory
    std::array<std::size_t, 16> m_stack{};       // Call stack

    TIM_TypeDef* m_sound_timer{TIM2};      // sound clock
    TIM_TypeDef* m_cpu_timer{TIM3};        // emulator clock
    TIM_TypeDef* m_counters_timer{TIM4};   // DT and ST clock
    std::uint8_t m_DT{};                   // Delay timer
    std::uint8_t m_ST{};                   // Sound timer

    std::random_device m_rd{};
    std::default_random_engine m_engine;
    std::uniform_int_distribution<std::uint8_t> m_dist;

    std::uint32_t m_key_status{};
    // clang-format off
    static constexpr std::array<std::uint32_t, 16> m_keymap
    {
        0x2000U,  // 0
        0x0001U,  // 1
        0x0002U,  // 2
        0x0004U,  // 3
        0x0010U,  // 4
        0x0020U,  // 5
        0x0040U,  // 6
        0x0100U,  // 7
        0x0200U,  // 8
        0x0400U,  // 9
        0x0008U,  // A
        0x0080U,  // B
        0x0800U,  // C
        0x8000U,  // D
        0x1000U,  // E
        0x4000U   // F
    };
    // clang-format on

    std::array<std::uint8_t, height * width / PCD8544::pixels_per_bank>
        m_pixels{};
    PCD8544 m_pcd8544;

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Reset the emulator
    ////////////////////////////////////////////////////////////////////////////
    void reset();

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Update the timers, fetch a new opcode, and decode
    ////////////////////////////////////////////////////////////////////////////
    void emulate();

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Decode an opcode
    /// @param opcode Opcode
    ////////////////////////////////////////////////////////////////////////////
    void decode(std::uint16_t opcode);

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Clear pixel at (x,y) screen coordinate
    /// @param x Horizontal coordinate [0-63]
    /// @param y Vertical coordinate [0-31]
    ////////////////////////////////////////////////////////////////////////////
    void clear_pixel(int x, int y);

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Check if pixel set at (x,y) screen coordinate
    /// @param x Horizontal coordinate [0-63]
    /// @param y Vertical coordinate [0-31]
    /// @return true if set, false if clear
    ////////////////////////////////////////////////////////////////////////////
    bool is_pixel_set(int x, int y) const;

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Set pixel at (x,y) screen coordinate
    /// @param x Horizontal coordinate [0-63]
    /// @param y Vertical coordinate [0-31]
    ////////////////////////////////////////////////////////////////////////////
    void set_pixel(int x, int y);

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Update display with current pixels
    ////////////////////////////////////////////////////////////////////////////
    void update_display();

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Scan key presses
    ////////////////////////////////////////////////////////////////////////////
    void scan_keys();

    bool is_key_pressed(std::uint8_t key) const;

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Play sound
    ////////////////////////////////////////////////////////////////////////////
    void play_sound() const noexcept;

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Stop sound
    ////////////////////////////////////////////////////////////////////////////
    void stop_sound() const noexcept;

    ////////////////////////////////////////////////////////////////////////////
    // Opcode Functions
    ////////////////////////////////////////////////////////////////////////////
    void CLS();                                  // 00E0 - CLS
    void RET();                                  // 00EE - RET
    void JUMP(std::uint16_t opcode) noexcept;    // 1nnn - JP addr
    void CALL(std::uint16_t opcode);             // 2nnn - CALL addr
    void SKE(std::uint16_t opcode);              // 3xnn - SE Vx, byte
    void SKNE(std::uint16_t opcode);             // 4xnn - SNE Vx, byte
    void SKRE(std::uint16_t opcode);             // 5xy0 - SE Vx, Vy
    void LOAD(std::uint16_t opcode);             // 6xnn - LD Vx, byte
    void ADD(std::uint16_t opcode);              // 7xnn - ADD Vx, byte
    void MOVE(std::uint16_t opcode);             // 8xy0 - LD Vx, Vy
    void OR(std::uint16_t opcode);               // 8xy1 - OR Vx, Vy
    void AND(std::uint16_t opcode);              // 8xy2 - AND Vx, Vy
    void XOR(std::uint16_t opcode);              // 8xy3 - XOR Vx, Vy
    void ADDR(std::uint16_t opcode);             // 8xy4 - ADD Vx, Vy
    void SUB(std::uint16_t opcode);              // 8xy5 - SUB Vx, Vy
    void SHR(std::uint16_t opcode);              // 8xy6 - SHR Vx
    void SUBN(std::uint16_t opcode);             // 8xy7 - SUBN Vx, Vy
    void SHL(std::uint16_t opcode);              // 8xyE - SHL Vx
    void SKRNE(std::uint16_t opcode);            // 9xy0 - SNE Vx, Vy
    void LOADI(std::uint16_t opcode) noexcept;   // Annn - LD I, addr
    void JUMPI(std::uint16_t opcode);            // Bnnn - JP V0, addr
    void RAND(std::uint16_t opcode);             // Cxnn - RND Vx, byte
    void DRAW(std::uint16_t opcode);             // Dxyn - DRW Vx, Vy, nibble
    void SKPR(std::uint16_t opcode);             // Ex9E - SKP Vx
    void SKUP(std::uint16_t opcode);             // ExA1 - SKNP Vx
    void MOVED(std::uint16_t opcode);            // Fx07 - LD Vx, DT
    void KEYD(std::uint16_t opcode);             // Fx0A - LD Vx, K
    void LOADD(std::uint16_t opcode);            // Fx15 - LD DT, Vx
    void LOADS(std::uint16_t opcode);            // Fx18 - LD ST, Vx
    void ADDI(std::uint16_t opcode);             // Fx1E - ADD I, Vx
    void LDSPR(std::uint16_t opcode);            // Fx29 - LD F, Vx
    void BCD(std::uint16_t opcode);              // Fx33 - LD B, Vx
    void STOR(std::uint16_t opcode);             // Fx55 - LD [I], Vx
    void READ(std::uint16_t opcode);             // Fx65 - LD Vx, [I]
};

#endif   // CHIP8_HPP
