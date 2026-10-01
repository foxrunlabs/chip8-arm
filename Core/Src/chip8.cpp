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

#include "chip8.hpp"
#include "main.h"
#include "roms.hpp"

#include "pcd8544.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <random>


////////////////////////////////////////////////////////////////////////////////
// External Variables
////////////////////////////////////////////////////////////////////////////////

extern bool reset_button_pushed;


////////////////////////////////////////////////////////////////////////////////
// Function Prototypes
////////////////////////////////////////////////////////////////////////////////

static constexpr std::size_t compute_index(int x, int y) noexcept;
static constexpr std::size_t NNN(std::uint16_t opcode) noexcept;
static constexpr std::uint8_t NN(std::uint16_t opcode) noexcept;
static constexpr std::uint8_t N(std::uint16_t opcode) noexcept;
static constexpr std::size_t X(std::uint16_t opcode) noexcept;
static constexpr std::size_t Y(std::uint16_t opcode) noexcept;


////////////////////////////////////////////////////////////////////////////////
// Static Data
////////////////////////////////////////////////////////////////////////////////

// clang-format off
////////////////////////////////////////////////////////////////////////////////
static constexpr std::array<std::uint8_t, 80> font
{
    0b1111'0000,   // 0
    0b1001'0000,
    0b1001'0000,
    0b1001'0000,
    0b1111'0000,

    0b0010'0000,   // 1
    0b0110'0000,
    0b0010'0000,
    0b0010'0000,
    0b0111'0000,

    0b1111'0000,   // 2
    0b0001'0000,
    0b1111'0000,
    0b1000'0000,
    0b1111'0000,

    0b1111'0000,   // 3
    0b0001'0000,
    0b1111'0000,
    0b0001'0000,
    0b1111'0000,

    0b1001'0000,   // 4
    0b1001'0000,
    0b1111'0000,
    0b0001'0000,
    0b0001'0000,

    0b1111'0000,   // 5
    0b1000'0000,
    0b1111'0000,
    0b0001'0000,
    0b1111'0000,

    0b1111'0000,   // 6
    0b1000'0000,
    0b1111'0000,
    0b1001'0000,
    0b1111'0000,

    0b1111'0000,   // 7
    0b0001'0000,
    0b0010'0000,
    0b0100'0000,
    0b0100'0000,

    0b1111'0000,   // 8
    0b1001'0000,
    0b1111'0000,
    0b1001'0000,
    0b1111'0000,

    0b1111'0000,   // 9
    0b1001'0000,
    0b1111'0000,
    0b0001'0000,
    0b1111'0000,

    0b1111'0000,   // A
    0b1001'0000,
    0b1111'0000,
    0b1001'0000,
    0b1001'0000,

    0b1111'0000,   // B
    0b1001'0000,
    0b1110'0000,
    0b1001'0000,
    0b1111'0000,

    0b1111'0000,   // C
    0b1000'0000,
    0b1000'0000,
    0b1000'0000,
    0b1111'0000,

    0b1110'0000,   // D
    0b1001'0000,
    0b1001'0000,
    0b1001'0000,
    0b1110'0000,

    0b1111'0000,   // E
    0b1000'0000,
    0b1111'0000,
    0b1000'0000,
    0b1111'0000,

    0b1111'0000,   // F
    0b1000'0000,
    0b1111'0000,
    0b1000'0000,
    0b1000'0000
};
// clang-format on


////////////////////////////////////////////////////////////////////////////////
// Public Member Functions
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
CHIP8::CHIP8()
    : m_engine(m_rd()), m_dist(0U, 255U),
      m_pcd8544(SPI5, LCD_nSCE_GPIO_Port, LCD_nSCE_Pin, LCD_nRST_GPIO_Port,
          LCD_nRST_Pin, LCD_DC_GPIO_Port, LCD_DC_Pin)
{
    std::copy(font.begin(), font.end(), m_memory.begin());

    m_pcd8544.clear();

    LL_TIM_EnableCounter(m_sound_timer);
    LL_TIM_EnableCounter(m_cpu_timer);
    LL_TIM_EnableCounter(m_counters_timer);
}


////////////////////////////////////////////////////////////////////////////////
CHIP8::~CHIP8()
{
    LL_TIM_DisableCounter(m_sound_timer);
    LL_TIM_DisableCounter(m_cpu_timer);
    LL_TIM_DisableCounter(m_counters_timer);

    m_sound_timer    = nullptr;
    m_cpu_timer      = nullptr;
    m_counters_timer = nullptr;
}


////////////////////////////////////////////////////////////////////////////////
void CHIP8::run()
{
    while(true)
    {
        if(reset_button_pushed)
        {
            reset();
        }
        else
        {
            scan_keys();

            switch(m_state)
            {
            case state::splash:
                m_pcd8544.print(
                    "1: Font Test\n2: Key Test\n3: Invaders\n4: Pong");

                m_state = state::menu;
                break;

            case state::menu:
                switch(m_key_status)
                {
                case m_keymap[1]:
                    std::copy(fonttest_rom.begin(), fonttest_rom.end(),
                        m_memory.begin() + org);
                    m_state = state::emulate;
                    break;

                case m_keymap[2]:
                    std::copy(keytest_rom.begin(), keytest_rom.end(),
                        m_memory.begin() + org);
                    m_state = state::emulate;
                    break;

                case m_keymap[3]:
                    std::copy(invaders_rom.begin(), invaders_rom.end(),
                        m_memory.begin() + org);
                    m_state = state::emulate;
                    break;

                case m_keymap[4]:
                    std::copy(pong_rom.begin(), pong_rom.end(),
                        m_memory.begin() + org);
                    m_state = state::emulate;
                    break;

                default:
                    break;
                }

                if(m_state == state::emulate)
                    m_pcd8544.clear();

                break;

            case state::emulate:
                if(LL_TIM_IsActiveFlag_UPDATE(m_cpu_timer) == FlagStatus::SET)
                {
                    LL_TIM_ClearFlag_UPDATE(m_cpu_timer);
                    emulate();
                }
                break;

            default:
                break;
            }
        }
    }
}


////////////////////////////////////////////////////////////////////////////////
// Private Member Functions
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
void CHIP8::reset()
{
    stop_sound();

    m_PC = org;
    m_SP = 0;
    m_DT = 0U;
    m_ST = 0U;

    m_pixels.fill(0U);
    m_pcd8544.clear();

    reset_button_pushed = false;

    m_state = state::splash;
}


////////////////////////////////////////////////////////////////////////////////
void CHIP8::emulate()
{
    // Update 60 Hz delay and sound timers
    if(LL_TIM_IsActiveFlag_UPDATE(m_counters_timer) == FlagStatus::SET)
    {
        LL_TIM_ClearFlag_UPDATE(m_counters_timer);

        if(m_DT > 0U)
            --m_DT;

        if(m_ST > 0U)
            --m_ST;
        else
            stop_sound();
    }

    // extract two bytes from memory and increment program counter
    const auto opcode = static_cast<std::uint16_t>(m_memory[m_PC] << 8) |
                        static_cast<std::uint16_t>(m_memory[m_PC + 1U]);
    m_PC += 2;
    decode(opcode);

    update_display();
}


////////////////////////////////////////////////////////////////////////////////
void CHIP8::decode(const std::uint16_t opcode)
{
    // clang-format off
    switch((opcode & 0xF000U) >> 12)
    {
        case 0x0U:
        {
            switch(opcode)
            {
                case 0x00E0U: CLS(); break;
                case 0x00EEU: RET(); break;
                default:      break;
            }
            break;
        }
        case 0x1U: JUMP(opcode); break; // 0x1nnn
        case 0x2U: CALL(opcode); break; // 0x2nnn
        case 0x3U: SKE(opcode);  break; // 0x3xnn
        case 0x4U: SKNE(opcode); break; // 0x4xnn
        case 0x5U: SKRE(opcode); break; // 0x5xy0
        case 0x6U: LOAD(opcode); break; // 0x6xnn
        case 0x7U: ADD(opcode);  break; // 0x7xnn
        case 0x8U:
        {
            switch(opcode & 0x000FU)
            {
                case 0x0U: MOVE(opcode); break; // 0x8xy0
                case 0x1U: OR(opcode);   break; // 0x8xy1
                case 0x2U: AND(opcode);  break; // 0x8xy2
                case 0x3U: XOR(opcode);  break; // 0x8xy3
                case 0x4U: ADDR(opcode); break; // 0x8xy4
                case 0x5U: SUB(opcode);  break; // 0x8xy5
                case 0x6U: SHR(opcode);  break; // 0x8xy6
                case 0x7U: SUBN(opcode); break; // 0x8xy7
                case 0xEU: SHL(opcode);  break; // 0x8xyE
                default:   break;
            }
            break;
        }
        case 0x9U: SKRNE(opcode); break; // 0x9xy0
        case 0xAU: LOADI(opcode); break; // 0xAnnn
        case 0xBU: JUMPI(opcode); break; // 0xBnnn
        case 0xCU: RAND(opcode);  break; // 0xCxnn
        case 0xDU: DRAW(opcode);  break; // 0xDxyn
        case 0xEU:
        {
            switch(opcode & 0x00FFU)
            {
                case 0x9EU: SKPR(opcode); break; // 0xEx9E
                case 0xA1U: SKUP(opcode); break; // 0xExA1
                default:    break;
            }
            break;
        }
        case 0xFU:
        {
            switch(opcode & 0x00FFU)
            {
                case 0x07U: MOVED(opcode); break; // 0xFx07
                case 0x0AU: KEYD(opcode);  break; // 0xFx0A
                case 0x15U: LOADD(opcode); break; // 0xFx15
                case 0x18U: LOADS(opcode); break; // 0xFx18
                case 0x1EU: ADDI(opcode);  break; // 0xFx1E
                case 0x29U: LDSPR(opcode); break; // 0xFx29
                case 0x33U: BCD(opcode);   break; // 0xFx33
                case 0x55U: STOR(opcode);  break; // 0xFx55
                case 0x65U: READ(opcode);  break; // 0xFx65
                default:    break;
            }
            break;
        }
        default: break;
    }
    // clang-format on
}


////////////////////////////////////////////////////////////////////////////////
void CHIP8::clear_pixel(const int x, const int y)
{
    const auto i = compute_index(x, y);
    m_pixels[i] &= ~(1 << (y % PCD8544::pixels_per_bank));
}


////////////////////////////////////////////////////////////////////////////////
bool CHIP8::is_pixel_set(const int x, const int y) const
{
    const auto i = compute_index(x, y);
    return (m_pixels[i] & (1 << (y % PCD8544::pixels_per_bank))) != 0;
}


////////////////////////////////////////////////////////////////////////////////
void CHIP8::set_pixel(const int x, const int y)
{
    const auto i = compute_index(x, y);
    m_pixels[i] |= (1 << (y % PCD8544::pixels_per_bank));
}


////////////////////////////////////////////////////////////////////////////////
void CHIP8::update_display()
{
    constexpr int total_banks = CHIP8::height / PCD8544::pixels_per_bank;
    constexpr int x_offset    = (PCD8544::screen_width - CHIP8::width) / 2;
    constexpr int bank_offset =
        (PCD8544::screen_height - CHIP8::height) / PCD8544::pixels_per_bank / 2;

    auto it = m_pixels.begin();
    for(int bank{bank_offset}; bank != total_banks + bank_offset; ++bank)
    {
        m_pcd8544.set_ram_addr(x_offset, bank);
        it = std::for_each_n(it, CHIP8::width,
            [this](const auto p) { m_pcd8544.set_pixels(p); });
    }
}


////////////////////////////////////////////////////////////////////////////////
void CHIP8::scan_keys()
{
    std::uint32_t input{};

    LL_GPIO_SetOutputPin(KEY_R1_GPIO_Port, KEY_R1_Pin);
    input = (LL_GPIO_ReadInputPort(KEY_C1_GPIO_Port) >> 6) & 0x0000000FU;
    LL_GPIO_ResetOutputPin(KEY_R1_GPIO_Port, KEY_R1_Pin);

    LL_GPIO_SetOutputPin(KEY_R2_GPIO_Port, KEY_R2_Pin);
    input |= (LL_GPIO_ReadInputPort(KEY_C1_GPIO_Port) >> 2) & 0x000000F0U;
    LL_GPIO_ResetOutputPin(KEY_R2_GPIO_Port, KEY_R2_Pin);

    LL_GPIO_SetOutputPin(KEY_R3_GPIO_Port, KEY_R3_Pin);
    input |= (LL_GPIO_ReadInputPort(KEY_C3_GPIO_Port) << 2) & 0x00000F00U;
    LL_GPIO_ResetOutputPin(KEY_R3_GPIO_Port, KEY_R3_Pin);

    LL_GPIO_SetOutputPin(KEY_R4_GPIO_Port, KEY_R4_Pin);
    input |= (LL_GPIO_ReadInputPort(KEY_C4_GPIO_Port) << 6) & 0x0000F000U;
    LL_GPIO_ResetOutputPin(KEY_R4_GPIO_Port, KEY_R4_Pin);

    m_key_status = input;
}


////////////////////////////////////////////////////////////////////////////////
bool CHIP8::is_key_pressed(const std::uint8_t key) const
{
    if(key > 0x0FU)
        return false;
    else
        return m_key_status == m_keymap[static_cast<std::size_t>(key)];
}


////////////////////////////////////////////////////////////////////////////////
void CHIP8::play_sound() const noexcept
{
    LL_TIM_CC_EnableChannel(TIM2, LL_TIM_CHANNEL_CH1);
}


////////////////////////////////////////////////////////////////////////////////
void CHIP8::stop_sound() const noexcept
{
    LL_TIM_CC_DisableChannel(TIM2, LL_TIM_CHANNEL_CH1);
}


////////////////////////////////////////////////////////////////////////////////
// Private Member Functions - OPCODES
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
void CHIP8::CLS()
{
    // 00E0 - clear the display
    m_pixels.fill(0U);
}


////////////////////////////////////////////////////////////////////////////////
void CHIP8::RET()
{
    // 00EE - return from subroutine
    --m_SP;
    m_PC = m_stack[m_SP];
}


////////////////////////////////////////////////////////////////////////////////
void CHIP8::JUMP(const std::uint16_t opcode) noexcept
{
    // 1nnn - jump to location nnn
    m_PC = NNN(opcode);
}


////////////////////////////////////////////////////////////////////////////////
void CHIP8::CALL(const std::uint16_t opcode)
{
    // 2nnn - call subroutine at nnn
    m_stack[m_SP] = m_PC;
    ++m_SP;
    m_PC = NNN(opcode);
}


////////////////////////////////////////////////////////////////////////////////
void CHIP8::SKE(const std::uint16_t opcode)
{
    // 3xnn - skip next instruction if Vx == nn
    if(m_V[X(opcode)] == NN(opcode))
        m_PC += 2;
}


////////////////////////////////////////////////////////////////////////////////
void CHIP8::SKNE(const std::uint16_t opcode)
{
    // 4xnn - skip next instruction if Vx != nn
    if(m_V[X(opcode)] != NN(opcode))
        m_PC += 2;
}


////////////////////////////////////////////////////////////////////////////////
void CHIP8::SKRE(const std::uint16_t opcode)
{
    // 5xy0 - skip next instruction if Vx == Vy
    if(m_V[X(opcode)] == m_V[Y(opcode)])
        m_PC += 2;
}


////////////////////////////////////////////////////////////////////////////////
void CHIP8::LOAD(const std::uint16_t opcode)
{
    // 6xnn - set Vx = nn
    m_V[X(opcode)] = NN(opcode);
}


////////////////////////////////////////////////////////////////////////////////
void CHIP8::ADD(const std::uint16_t opcode)
{
    // 7xnn - set Vx = Vx + nn (carry flag not set)
    m_V[X(opcode)] += NN(opcode);
}


////////////////////////////////////////////////////////////////////////////////
void CHIP8::MOVE(const std::uint16_t opcode)
{
    // 8xy0 - set Vx = Vy
    m_V[X(opcode)] = m_V[Y(opcode)];
}


////////////////////////////////////////////////////////////////////////////////
void CHIP8::OR(const std::uint16_t opcode)
{
    // 8xy1 - Set Vx = Vx OR Vy
    m_V[X(opcode)] |= m_V[Y(opcode)];
}


////////////////////////////////////////////////////////////////////////////////
void CHIP8::AND(const std::uint16_t opcode)
{
    // 8xy2 - set Vx = Vx AND Vy
    m_V[X(opcode)] &= m_V[Y(opcode)];
}


////////////////////////////////////////////////////////////////////////////////
void CHIP8::XOR(const std::uint16_t opcode)
{
    // 8xy3 - set Vx = Vx XOR Vy
    m_V[X(opcode)] ^= m_V[Y(opcode)];
}


////////////////////////////////////////////////////////////////////////////////
void CHIP8::ADDR(const std::uint16_t opcode)
{
    // 8xy4 - set Vx = Vx + Vy, set VF = carry
    const std::uint16_t temp = static_cast<std::uint16_t>(m_V[X(opcode)]) +
                               static_cast<std::uint16_t>(m_V[Y(opcode)]);
    m_V[15]        = (temp > 255U) ? 1U : 0U;
    m_V[X(opcode)] = static_cast<std::uint8_t>(temp & 0x00FFU);
}


////////////////////////////////////////////////////////////////////////////////
void CHIP8::SUB(const std::uint16_t opcode)
{
    // 8xy5 - set Vx = Vx - Vy, set VF = NOT borrow
    m_V[15] = (m_V[Y(opcode)] > m_V[X(opcode)]) ? 0U : 1U;
    m_V[X(opcode)] -= m_V[Y(opcode)];
}


////////////////////////////////////////////////////////////////////////////////
void CHIP8::SHR(const std::uint16_t opcode)
{
    // 8xy6 - set Vx = Vx >> 1, set VF = LSb
    m_V[15] = (m_V[X(opcode)] & 0x01U) ? 1U : 0U;
    m_V[X(opcode)] >>= 1;
}


////////////////////////////////////////////////////////////////////////////////
void CHIP8::SUBN(const std::uint16_t opcode)
{
    // 8xy7 - set Vx = Vy - Vx, set VF = NOT borrow
    m_V[15]        = (m_V[X(opcode)] > m_V[Y(opcode)]) ? 0U : 1U;
    m_V[X(opcode)] = m_V[Y(opcode)] - m_V[X(opcode)];
}


////////////////////////////////////////////////////////////////////////////////
void CHIP8::SHL(const std::uint16_t opcode)
{
    // 8xyE - set Vx = Vx << 1, set VF = MSb
    m_V[15] = (m_V[X(opcode)] & 0x80U) ? 1U : 0U;
    m_V[X(opcode)] <<= 1;
}


////////////////////////////////////////////////////////////////////////////////
void CHIP8::SKRNE(const std::uint16_t opcode)
{
    // 9xy0 - skip next instruction if Vx != Vy
    if(m_V[X(opcode)] != m_V[Y(opcode)])
        m_PC += 2;
}


////////////////////////////////////////////////////////////////////////////////
void CHIP8::LOADI(const std::uint16_t opcode) noexcept
{
    // Annn - set I = nnn
    m_I = NNN(opcode);
}


////////////////////////////////////////////////////////////////////////////////
void CHIP8::JUMPI(const std::uint16_t opcode)
{
    // Bnnn - jump to location nnn + V0
    m_PC = static_cast<std::size_t>(m_V[0]) + NNN(opcode);
}


////////////////////////////////////////////////////////////////////////////////
void CHIP8::RAND(const std::uint16_t opcode)
{
    // Cxnn - set Vx = random byte and nn
    m_V[X(opcode)] = m_dist(m_engine) & NN(opcode);
}


////////////////////////////////////////////////////////////////////////////////
void CHIP8::DRAW(const std::uint16_t opcode)
{
    // Dxyn - display n-byte sprite starting at memory location I at (Vx, Vy),
    //        set VF = collision
    m_V[15] = 0U;

    for(int y_sprite{}; y_sprite != N(opcode); ++y_sprite)
    {
        const auto addr = m_I + static_cast<std::size_t>(y_sprite);
        const auto y    = static_cast<int>(m_V[Y(opcode)]) + y_sprite;

        for(int x_sprite{}; x_sprite != 8U; ++x_sprite)
        {
            if((m_memory[addr] & (0x80U >> x_sprite)) != 0U)
            {
                const auto x = static_cast<int>(m_V[X(opcode)]) + x_sprite;

                if(is_pixel_set(x, y))
                {
                    m_V[15] = 1U;
                    clear_pixel(x, y);
                }
                else
                {
                    set_pixel(x, y);
                }
            }
        }
    }
}


////////////////////////////////////////////////////////////////////////////////
void CHIP8::SKPR(const std::uint16_t opcode)
{
    // Ex9E - skip next instruction if key with the value of Vx is pressed
    if(is_key_pressed(m_V[X(opcode)]))
        m_PC += 2;
}


////////////////////////////////////////////////////////////////////////////////
void CHIP8::SKUP(const std::uint16_t opcode)
{
    // ExA1 - skip next instruction if key with the value of Vx is not pressed
    if(!is_key_pressed(m_V[X(opcode)]))
        m_PC += 2;
}


////////////////////////////////////////////////////////////////////////////////
void CHIP8::MOVED(const std::uint16_t opcode)
{
    // Fx07 - set Vx = delay time value
    m_V[X(opcode)] = m_DT;
}


////////////////////////////////////////////////////////////////////////////////
void CHIP8::KEYD(const std::uint16_t opcode)
{
    // Fx0A - wait for key press, store the value of the key in Vx
    const auto it = std::find_if(m_keymap.begin(), m_keymap.end(),
        [this](const auto k) { return k == m_key_status; });

    if(it != m_keymap.end())
        m_V[X(opcode)] =
            static_cast<std::uint8_t>(std::distance(m_keymap.begin(), it));
    else
        m_PC -= 2;
}


////////////////////////////////////////////////////////////////////////////////
void CHIP8::LOADD(const std::uint16_t opcode)
{
    // Fx15 - set delay time = Vx
    m_DT = m_V[X(opcode)];
}


////////////////////////////////////////////////////////////////////////////////
void CHIP8::LOADS(const std::uint16_t opcode)
{
    // Fx18 - set sound time = Vx
    m_ST = m_V[X(opcode)];

    if(m_ST > 0U)
        play_sound();
}


////////////////////////////////////////////////////////////////////////////////
void CHIP8::ADDI(const std::uint16_t opcode)
{
    // Fx1E - set I = I + Vx (carry flag not set)
    m_I += static_cast<std::size_t>(m_V[X(opcode)]);
}


////////////////////////////////////////////////////////////////////////////////
void CHIP8::LDSPR(const std::uint16_t opcode)
{
    // Fx29 - set I = location of sprite for digit Vx
    m_I = static_cast<std::size_t>(m_V[X(opcode)] * 5U);
}


////////////////////////////////////////////////////////////////////////////////
void CHIP8::BCD(const std::uint16_t opcode)
{
    // Fx33 - store BCD representation of Vx in memory locations I, I+1, and I+2
    m_memory[m_I]     = m_V[X(opcode)] / 100U;
    m_memory[m_I + 1] = (m_V[X(opcode)] / 10U) % 10U;
    m_memory[m_I + 2] = m_V[X(opcode)] % 10U;
}


////////////////////////////////////////////////////////////////////////////////
void CHIP8::STOR(const std::uint16_t opcode)
{
    // Fx55 - store registers V0 through Vx in memory starting at location I
    for(std::size_t i{}; i <= X(opcode); ++i)
    {
        m_memory[m_I] = m_V[i];
        ++m_I;
    }
}


////////////////////////////////////////////////////////////////////////////////
void CHIP8::READ(const std::uint16_t opcode)
{
    // Fx65 - read registers V0 through Vx from memory starting at location I
    for(std::size_t i{}; i <= X(opcode); ++i)
    {
        m_V[i] = m_memory[m_I];
        ++m_I;
    }
}


////////////////////////////////////////////////////////////////////////////////
// Static Functions
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/// @brief Compute index of pixel from (x,y) coordinates
/// @param x Horizontal coordinate
/// @param y Vertical coordinate
/// @return index
////////////////////////////////////////////////////////////////////////////////
static constexpr std::size_t compute_index(const int x, const int y) noexcept
{
    const int bank = (y % CHIP8::height) / PCD8544::pixels_per_bank;
    return static_cast<std::size_t>((x % CHIP8::width) + CHIP8::width * bank);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief Extract NNN from opcode
/// @param opcode Opcode
/// @return NNN
////////////////////////////////////////////////////////////////////////////////
static constexpr std::size_t NNN(const std::uint16_t opcode) noexcept
{
    return static_cast<std::size_t>(opcode & 0x0FFFU);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief Extract NN from opcode
/// @param opcode Opcode
/// @return NN
////////////////////////////////////////////////////////////////////////////////
static constexpr std::uint8_t NN(const std::uint16_t opcode) noexcept
{
    return static_cast<std::uint8_t>(opcode & 0x00FFU);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief Extract N from opcode
/// @param opcode Opcode
/// @return N
////////////////////////////////////////////////////////////////////////////////
static constexpr std::uint8_t N(const std::uint16_t opcode) noexcept
{
    return static_cast<std::uint8_t>(opcode & 0x000FU);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief Extract Vx from opcode
/// @param opcode Opcode
/// @return Register index
////////////////////////////////////////////////////////////////////////////////
static constexpr std::size_t X(const std::uint16_t opcode) noexcept
{
    return static_cast<std::size_t>((opcode & 0x0F00U) >> 8U);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief Extract Vy from opcode
/// @param opcode Opcode
/// @return Register index
////////////////////////////////////////////////////////////////////////////////
static constexpr std::size_t Y(const std::uint16_t opcode) noexcept
{
    return static_cast<std::size_t>((opcode & 0x00F0U) >> 4U);
}
