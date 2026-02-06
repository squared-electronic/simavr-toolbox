#include "sim_gu7000.hpp"

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace Font {
#include "font.h"
}

inline constexpr unsigned char operator""_u8(unsigned long long arg) noexcept {
  return static_cast<unsigned char>(arg);
}

inline constexpr uint8_t u8(auto x) {
  return static_cast<uint8_t>(x);
}

// constexpr std::string us_command(uint8_t group, uint8_t cmd) {
//   return std::format("\x1F\x28{:X}{:X}", group, cmd);
// }

// constexpr std::string grouped_command(uint8_t prefix, uint8_t group,
//                                       uint8_t cmd) {
//   return std::format("{0}{1}{2}", prefix, group, cmd);
// }

SimGu7000::SimGu7000() {
  std::vector<uint8_t> v;
  Stream s(v);
  ProcessInitializeDisplay(s);
}

void SimGu7000::ProcessCommand(uint8_t byte) {
  if (state_ == State::Idle) {
    if (byte >= CMD_CHARACTER_DISPLAY_START && byte <= CMD_CHARACTER_DISPLAY_END) {
      ProcessCharacterDisplay(byte);
      ResetCommandState();
      return;
    } else {
      state_ = State::GettingCommand;
    }
  }

  if (state_ == State::GettingCommand) {
    command_buffer_.push_back(byte);

    auto command_it = CommandTable.find(command_buffer_);

    if (command_it != CommandTable.end()) {
      CurrentCommand_ = &command_it->second;
      if (command_it->second.FixedArgumentBytes == 0) {
        ExecuteCurrentCommandAndReset();
      } else {
        state_ = State::GettingCommandArguments;
      }
    }
  } else if (state_ == State::GettingCommandArguments) {
    command_arguments_.push_back(byte);
    if (command_arguments_.size() == CurrentCommand_->FixedArgumentBytes) {
      if (CurrentCommand_->SizeGetFn) {
        CurrentCommandVariableBytes_ = (this->*CurrentCommand_->SizeGetFn)();
        if (CurrentCommandVariableBytes_ > 0) {
          state_ = State::GettingVariableArgs;
        } else {
          ExecuteCurrentCommandAndReset();
        }
      } else {
        ExecuteCurrentCommandAndReset();
      }
    }
  } else if (state_ == State::GettingVariableArgs) {
    command_arguments_.push_back(byte);
    if (command_arguments_.size() ==
        (CurrentCommand_->FixedArgumentBytes + CurrentCommandVariableBytes_)) {
      ExecuteCurrentCommandAndReset();
    }
  }
}

SimGu7000::CommandMap SimGu7000::CommandTable = {
    // Single byte commands
    // ------------------------------------------------------- //
    {"\x08",
     {
         .Name = "Backspace",
         .Execute = &SimGu7000::ProcessBackspace,
         .SizeGetFn = nullptr,
         .FixedArgumentBytes = 0,
     }},
    {"\x09",
     {
         .Name = "HorizontalTab",
         .Execute = &SimGu7000::ProcessHorizontalTab,
         .SizeGetFn = nullptr,
         .FixedArgumentBytes = 0,
     }},
    {"\x0A",
     {
         .Name = "LineFeed",
         .Execute = &SimGu7000::ProcessLineFeed,
         .SizeGetFn = nullptr,
         .FixedArgumentBytes = 0,
     }},
    {"\x0B",
     {
         .Name = "HomePosition",
         .Execute = &SimGu7000::ProcessHomePosition,
         .SizeGetFn = nullptr,
         .FixedArgumentBytes = 0,
     }},
    {"\x0C",
     {
         .Name = "DisplayClear",
         .Execute = &SimGu7000::ProcessDisplayClearCommand,
         .SizeGetFn = nullptr,
         .FixedArgumentBytes = 0,
     }},
    {"\x0D",
     {
         .Name = "CarriageReturn",
         .Execute = &SimGu7000::ProcessCarriageReturn,
         .SizeGetFn = nullptr,
         .FixedArgumentBytes = 0,
     }},
    // ESC commands (\x1B prefix)
    // ------------------------------------------------------- //
    {"\x1B\x40",
     {
         .Name = "InitializeDisplay",
         .Execute = &SimGu7000::ProcessInitializeDisplay,
         .SizeGetFn = nullptr,
         .FixedArgumentBytes = 0,
     }},
    {"\x1B\x25",
     {
         .Name = "SpecifyDownloadRegister",
         .Execute = &SimGu7000::ProcessSpecifyDownloadRegister,
         .SizeGetFn = nullptr,
         .FixedArgumentBytes = 0,
     }},
    {"\x1B\x26",
     {
         .Name = "DownloadCharacter",
         .Execute = &SimGu7000::ProcessDownloadCharacter,
         .SizeGetFn = nullptr,
         .FixedArgumentBytes = 0,
     }},
    {"\x1B\x3F",
     {
         .Name = "DeleteDownloadedCharacter",
         .Execute = &SimGu7000::ProcessDeleteDownloadedCharacter,
         .SizeGetFn = nullptr,
         .FixedArgumentBytes = 0,
     }},
    {"\x1B\x52",
     {
         .Name = "InternationalFontSet",
         .Execute = &SimGu7000::ProcessInternationalFontSet,
         .SizeGetFn = nullptr,
         .FixedArgumentBytes = 0,
     }},
    {"\x1B\x74",
     {
         .Name = "CharacterCodeType",
         .Execute = &SimGu7000::ProcessCharacterCodeType,
         .SizeGetFn = nullptr,
         .FixedArgumentBytes = 0,
     }},
    // US commands (\x1F\x28 prefix)
    // ------------------------------------------------------- //
    {"\x1F\x28\x01",
     {
         .Name = "OverwriteMode",
         .Execute = &SimGu7000::ProcessOverwriteMode,
         .SizeGetFn = nullptr,
         .FixedArgumentBytes = 1,
     }},
    {"\x1F\x28\x02",
     {
         .Name = "VerticalScrollMode",
         .Execute = &SimGu7000::ProcessVerticalScrollMode,
         .SizeGetFn = nullptr,
         .FixedArgumentBytes = 0,
     }},
    {"\x1F\x28\x64\x30",
     {
         .Name = "PrintAtPosition",
         .Execute = &SimGu7000::ProcessCharacterDisplayAtPosition,
         .SizeGetFn = &SimGu7000::ProcessCharacterDisplayAtPositionSize,
         .FixedArgumentBytes = 6,
     }},
    {"\x1F\x28\x03",
     {
         .Name = "HorizontalScrollMode",
         .Execute = &SimGu7000::ProcessHorizontalScrollMode,
         .SizeGetFn = nullptr,
         .FixedArgumentBytes = 0,
     }},
    {"\x1F\x24",
     {
         .Name = "CursorSet",
         .Execute = &SimGu7000::ProcessCursorSet,
         .SizeGetFn = nullptr,
         .FixedArgumentBytes = 4,
     }},
    {"\x1F\x28\x58",
     {
         .Name = "BrightnessControl",
         .Execute = &SimGu7000::ProcessBrightnessControl,
         .SizeGetFn = nullptr,
         .FixedArgumentBytes = 1,
     }},
    {"\x1F\x28\x72",
     {
         .Name = "ReverseDisplay",
         .Execute = &SimGu7000::ProcessReverseDisplay,
         .SizeGetFn = nullptr,
         .FixedArgumentBytes = 1,
     }},
    {"\x1F\x28\x73",
     {
         .Name = "HorizontalScrollSpeed",
         .Execute = &SimGu7000::ProcessHorizontalScrollSpeed,
         .SizeGetFn = nullptr,
         .FixedArgumentBytes = 1,
     }},
    {"\x1F\x28\x77",
     {
         .Name = "CompositionMode",
         .Execute = &SimGu7000::ProcessCompositionMode,
         .SizeGetFn = nullptr,
         .FixedArgumentBytes = 1,
     }},
    // US Extended commands (\x1F\x28 prefix with sub-commands)
    // ------------------------------------------------------- //
    {"\x1F\x28\x61\x01",
     {
         .Name = "Wait",
         .Execute = &SimGu7000::ProcessWait,
         .SizeGetFn = nullptr,
         .FixedArgumentBytes = 0,
     }},
    {"\x1F\x28\x61\x10",
     {
         .Name = "ScrollDisplayAction",
         .Execute = &SimGu7000::ProcessScrollDisplayAction,
         .SizeGetFn = nullptr,
         .FixedArgumentBytes = 0,
     }},
    {"\x1F\x28\x61\x11",
     {
         .Name = "DisplayBlink",
         .Execute = &SimGu7000::ProcessDisplayBlink,
         .SizeGetFn = nullptr,
         .FixedArgumentBytes = 0,
     }},
    {"\x1F\x28\x61\x40",
     {
         .Name = "ScreenSaver",
         .Execute = &SimGu7000::ProcessScreenSaver,
         .SizeGetFn = nullptr,
         .FixedArgumentBytes = 1,
     }},
    {"\x1F\x28\x64\x21",
     {
         .Name = "RealTimeBitImageDisplay",
         .Execute = &SimGu7000::ProcessRealTimeBitImageDisplayXy,
         .SizeGetFn = &SimGu7000::ProcessRealTimeBitImageDisplaySize,
         .FixedArgumentBytes = 9,
     }},
    {"\x1F\x28\x67\x03",
     {
         .Name = "CharacterFontWidthAndSpace",
         .Execute = &SimGu7000::ProcessCharacterFontWidthAndSpace,
         .SizeGetFn = nullptr,
         .FixedArgumentBytes = 1,
     }},
    {"\x1F\x28\x67\x40",
     {
         .Name = "FontMagnificationSet",
         .Execute = &SimGu7000::ProcessFontMagnificationSet,
         .SizeGetFn = nullptr,
         .FixedArgumentBytes = 2,
     }},
    {"\x1F\x28\x77\x01",
     {
         .Name = "CurrentWindowSelect",
         .Execute = &SimGu7000::ProcessCurrentWindowSelect,
         .SizeGetFn = nullptr,
         .FixedArgumentBytes = 1,
     }},
    {"\x1F\x28\x77\x02",
     {
         .Name = "UserWindowDefinitionCancel",
         .Execute = &SimGu7000::ProcessUserWindowDefinitionCancel,
         .SizeGetFn = nullptr,
         .FixedArgumentBytes = 0,
     }},
    {"\x1F\x28\x77\x10",
     {
         .Name = "WriteScreenModeSelect",
         .Execute = &SimGu7000::ProcessWriteScreenModeSelect,
         .SizeGetFn = nullptr,
         .FixedArgumentBytes = 1,
     }},
};

const SimGu7000::DisplayMemory& SimGu7000::GetDisplayMemory() const {
  return display_memory_;
}

uint8_t SimGu7000::Width() const {
  return DISPLAY_WIDTH;
}

uint8_t SimGu7000::Height() const {
  return DISPLAY_HEIGHT;
}

void SimGu7000::ClearDisplayMemory() {
  for (auto& column : display_memory_) {
    column.fill(false);
  }
}

void SimGu7000::SetCursor(uint16_t x, uint16_t y) {
  cursor_x_ = std::min(x, static_cast<uint16_t>(DISPLAY_WIDTH - 1));
  cursor_y_ = std::min(static_cast<uint16_t>(y * ROW_HEIGHT_DOTS),
                       static_cast<uint16_t>(DISPLAY_HEIGHT - 1));
}

uint16_t SimGu7000::GetCursorX() const {
  return cursor_x_;
}

uint16_t SimGu7000::GetCursorY() const {
  return cursor_y_;
}

void SimGu7000::DrawCharacterAtCursor(uint8_t character) {
  DrawFontCharacter(cursor_x_, cursor_y_, character);
  AdvanceCursor(cursor_x_, cursor_y_);
}

void SimGu7000::AdvanceCursor(uint16_t& x, uint16_t& y) const {
  x += FONT_WIDTH * font_magnification_x_;
  if (x >= DISPLAY_WIDTH) {
    x = 0;
    y += FONT_HEIGHT * font_magnification_y_;
    if (y >= DISPLAY_HEIGHT) {
      y = 0;
    }
  }
}

void SimGu7000::DrawCharacterAt(uint16_t x, uint16_t y, uint8_t character) {
  DrawFontCharacter(x, y, character);
}

// Helper methods
void SimGu7000::SetPixel(uint16_t x, uint16_t y, bool on) {
  if (x >= DISPLAY_WIDTH || y >= DISPLAY_HEIGHT) {
    return;
  }

  display_memory_.at(x).at(y) = on;
}

bool SimGu7000::GetPixel(uint16_t x, uint16_t y) const {
  if (x >= DISPLAY_WIDTH || y >= DISPLAY_HEIGHT) {
    return false;
  }

  return display_memory_[x][y];
}

void SimGu7000::DrawFontCharacter(uint16_t x, uint16_t y, uint8_t character) {
  auto font_data = GetFontData(character);
  DrawImage(x, y, font_data, FONT_WIDTH, FONT_HEIGHT);
}

void SimGu7000::DrawImage(uint16_t x, uint16_t y, const FontCharSpan& image, uint8_t width,
                          uint8_t height) {
  if (image.empty()) {
    return;
  }

  // Draw each column of the character
  for (uint8_t row = 0; row < height; ++row) {
    int row_data = image[row];

    // Draw each pixel in the column with magnification
    for (uint8_t col = 0; col < width; ++col) {
      bool pixel_on = (row_data & (1 << (7 - col))) != 0;

      // Apply magnification
      for (uint8_t mx = 0; mx < font_magnification_x_; ++mx) {
        for (uint8_t my = 0; my < font_magnification_y_; ++my) {
          uint16_t pixel_x = x + col * font_magnification_x_ + mx;
          uint16_t pixel_y = y + row * font_magnification_y_ + my;

          if (pixel_x < DISPLAY_WIDTH && pixel_y < DISPLAY_HEIGHT) {
            SetPixel(pixel_x, pixel_y, pixel_on);
          }
        }
      }
    }
  }
}

SimGu7000::FontCharSpan SimGu7000::GetFontData(uint8_t character) const {
  auto GetTheData = [](char c) {
    auto begin = &Font::font.Bitmap[(c - 31) * Font::font.Height];
    return FontCharSpan(begin, Font::font.Height);
  };

  if (character < 32 || character > 126) {
    return GetTheData(' ');  // Return space for invalid characters
  }

  return GetTheData(character);
}

void SimGu7000::ExecuteCurrentCommandAndReset() {
  Stream s(command_arguments_);
  (this->*CurrentCommand_->Execute)(s);
  ResetCommandState();
}

void SimGu7000::ResetCommandState() {
  command_buffer_.clear();
  command_arguments_.clear();
  state_ = State::Idle;
  CurrentCommand_ = nullptr;
  CurrentCommandVariableBytes_ = 0;
}

// Command implementations
void SimGu7000::ProcessCharacterDisplay(uint8_t character) {
  DrawCharacterAtCursor(character);
}

void SimGu7000::ProcessCharacterDisplayAtPosition(Stream& params) {
  uint16_t x, y;
  ExtractXY(params, x, y);
  uint8_t _ = params.get_uint8();  // Unused "m"
  uint8_t len = params.get_uint8();
  for (unsigned i = 0; i < len; ++i) {
    DrawFontCharacter(x, y, params.get_uint8());
    AdvanceCursor(x, y);
  }
}

uint16_t SimGu7000::ProcessCharacterDisplayAtPositionSize() const {
  Stream s(command_arguments_);
  uint16_t x, y;
  ExtractXY(s, x, y);
  uint8_t _ = s.get_uint8();  // Unused "m"
  uint8_t len = s.get_uint8();
  return len;
}

void SimGu7000::ProcessBackspace(Stream&) {
  if (cursor_x_ >= FONT_WIDTH * font_magnification_x_) {
    cursor_x_ -= FONT_WIDTH * font_magnification_x_;
  } else {
    cursor_x_ = DISPLAY_WIDTH - FONT_WIDTH * font_magnification_x_;
    if (cursor_y_ >= FONT_HEIGHT * font_magnification_y_) {
      cursor_y_ -= FONT_HEIGHT * font_magnification_y_;
    } else {
      cursor_y_ = DISPLAY_HEIGHT - FONT_HEIGHT * font_magnification_y_;
    }
  }
}

void SimGu7000::ProcessHorizontalTab(Stream&) {
  uint16_t tab_width = FONT_WIDTH * font_magnification_x_ * 4;  // Tab = 4 characters
  cursor_x_ = ((cursor_x_ / tab_width) + 1) * tab_width;
  if (cursor_x_ >= DISPLAY_WIDTH) {
    cursor_x_ = 0;
    cursor_y_ += FONT_HEIGHT * font_magnification_y_;
    if (cursor_y_ >= DISPLAY_HEIGHT) {
      cursor_y_ = 0;
    }
  }
}

void SimGu7000::ProcessLineFeed(Stream&) {
  cursor_y_ += FONT_HEIGHT * font_magnification_y_;
  if (cursor_y_ >= DISPLAY_HEIGHT) {
    cursor_y_ = 0;
  }
}

void SimGu7000::ProcessHomePosition(Stream&) {
  cursor_x_ = 0;
  cursor_y_ = 0;
}

void SimGu7000::ProcessCarriageReturn(Stream&) {
  cursor_x_ = 0;
}

void SimGu7000::ProcessDisplayClear(Stream& params) {
  ClearDisplayMemory();
  ProcessHomePosition(params);
}

void SimGu7000::ProcessInitializeDisplay(Stream&) {
  ClearDisplayMemory();
  state_ = State::Idle;
  initialized_ = true;
  cursor_x_ = 0;
  cursor_y_ = 0;
  international_font_set_ = 0;
  character_code_type_ = 0;
  overwrite_mode_ = false;
  scroll_mode_ = 0;
  horizontal_scroll_speed_ = 0;
  brightness_level_ = 8;
  reverse_display_ = false;
  composition_mode_ = 0;
  current_window_ = 0;
  font_magnification_x_ = 1;
  font_magnification_y_ = 1;
}

void SimGu7000::ProcessCursorSet(Stream& params) {
  uint16_t x, y;
  ExtractXY(params, x, y);
  SetCursor(x, y);
}

void SimGu7000::ProcessDisplayClearCommand(Stream& params) {
  ProcessDisplayClear(params);
}

void SimGu7000::ProcessBrightnessControl(Stream& params) {
  brightness_level_ = std::clamp(1_u8, 8_u8, params.get_uint8());
}

void SimGu7000::ProcessReverseDisplay(Stream& params) {
  reverse_display_ = (params.get_uint8() != 0);
}

void SimGu7000::ProcessHorizontalScrollSpeed(Stream& params) {
  horizontal_scroll_speed_ = std::clamp(params.get_uint8(), 0_u8, 31_u8);
}

void SimGu7000::ProcessCompositionMode(Stream& params) {
  composition_mode_ = std::clamp(params.get_uint8(), 0_u8, 3_u8);
}
void SimGu7000::ProcessInternationalFontSet(Stream& params) {
  international_font_set_ = params.get_uint8();
}

void SimGu7000::ProcessCharacterCodeType(Stream& params) {
  character_code_type_ = params.get_uint8();
}

void SimGu7000::ProcessOverwriteMode(Stream& params) {
  overwrite_mode_ = (params.get_uint8() == 1);
}

void SimGu7000::ProcessVerticalScrollMode(Stream& params) {
  scroll_mode_ = 2;  // Vertical scroll
}

void SimGu7000::ProcessHorizontalScrollMode(Stream& params) {
  scroll_mode_ = 3;  // Horizontal scroll
}

void SimGu7000::ProcessWait(Stream& params) {
  // Wait command - in simulation, we just ignore timing
  // In real hardware this would wait for t*0.5 seconds
}

void SimGu7000::ProcessScrollDisplayAction(Stream& params) {
  // Scroll display action - complex scrolling implementation
  // For now, just stub this out
}

void SimGu7000::ProcessDisplayBlink(Stream& params) {
  // Display blink - complex blinking implementation
  // For now, just stub this out
}

void SimGu7000::ProcessScreenSaver(Stream& params) {
  switch (params.get_uint8()) {
    case 0:  // Power save
    case 2:  // All dots off
      ClearDisplayMemory();
      break;
    case 3:  // All dots on
      for (auto& column : display_memory_) {
        column.fill(true);
      }
      break;
    case 1:  // Power on
    case 4:  // Repeat normal & reverse display
    default:
      // No change for other modes
      break;
  }
}

void SimGu7000::ProcessRealTimeBitImageDisplayXy(Stream& params) {
  uint16_t x, y;
  ExtractXY(params, x, y);
  ProcessRealTimeBitImageDisplay(params, x, y);
}

void SimGu7000::ProcessRealTimeBitImageDisplay(Stream& params, uint8_t x, uint8_t y) {
  uint16_t w, h;
  ExtractXY(params, w, h);
  uint8_t _ = params.get_uint8();  // Discard "g" (always 1)

  for (uint8_t i = 0; i < (h / 8) * w; i++) {
    uint8_t byte = params.get_uint8();
    for (uint8_t j = 0; j < h; ++j) {
      SetPixel(x + i, y + j, byte & (1 << (7 - j)));
    }
  }
}

void SimGu7000::ProcessCharacterFontWidthAndSpace(Stream& params) {
  // Character font width and space - font spacing implementation
  // For now, just stub this out
}

void SimGu7000::ProcessFontMagnificationSet(Stream& params) {
  font_magnification_x_ = std::clamp(params.get_uint8(), 1_u8, 4_u8);
  font_magnification_y_ = std::clamp(params.get_uint8(), 1_u8, 2_u8);
}

void SimGu7000::ProcessCurrentWindowSelect(Stream& params) {
  current_window_ = std::clamp(params.get_uint8(), 0_u8, 4_u8);
}

void SimGu7000::ProcessUserWindowDefinitionCancel(Stream& params) {
  // User window definition/cancel - complex window management
  // For now, just stub this out
}

void SimGu7000::ProcessWriteScreenModeSelect(Stream& params) {
  // Write screen mode select - screen mode management
  // For now, just stub this out
}

void SimGu7000::ProcessSpecifyDownloadRegister(Stream& params) {
  // Specify download register - download enable/disable
  // For now, just stub this out
}

void SimGu7000::ProcessDownloadCharacter(Stream& params) {
  // Download character - custom character definition
  // For now, just stub this out
}

void SimGu7000::ProcessDeleteDownloadedCharacter(Stream& params) {
  // Delete downloaded character - custom character removal
  // For now, just stub this out
}

void SimGu7000::ExtractXY(Stream& s, uint16_t& x, uint16_t& y) const {
  x = s.get_uint16le();
  y = s.get_uint16le();
}

uint16_t SimGu7000::ProcessRealTimeBitImageDisplaySize() const {
  Stream s(command_arguments_);

  uint16_t x, y, w, h;
  ExtractXY(s, x, y);
  ExtractXY(s, w, h);

  return (h / 8) * w;
}
