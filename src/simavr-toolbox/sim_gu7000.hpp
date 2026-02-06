#pragma once

#include <sys/types.h>

#include <array>
#include <cstdint>
#include <cstdlib>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

class Stream {
 public:
  Stream(const std::vector<uint8_t>& data) : data_(data.begin(), data.end()) {}

  uint16_t get_uint16le() {
    auto lo = get_uint8();
    auto hi = get_uint8();
    return (hi << 8) | lo;
  }

  uint8_t get_uint8() {
    return next();
  }

 private:
  uint8_t next() {
    if (position_ >= data_.size()) {
      std::abort();
    }
    return data_[position_++];
  }

  const std::span<const uint8_t> data_;
  unsigned position_{0};
};

class SimGu7000 {
 public:
  static constexpr uint8_t DISPLAY_WIDTH = 112;
  static constexpr uint8_t DISPLAY_HEIGHT = 16;

  typedef std::array<std::array<bool, DISPLAY_HEIGHT>, DISPLAY_WIDTH> DisplayMemory;

  SimGu7000();
  void ProcessCommand(uint8_t command);
  uint8_t Width() const;
  uint8_t Height() const;
  const DisplayMemory& GetDisplayMemory() const;

 private:
  typedef std::span<const uint8_t, 7> FontCharSpan;

  // Font dimensions (5x7)
  static constexpr uint8_t FONT_WIDTH = 5;
  static constexpr uint8_t FONT_HEIGHT = 7;
  static constexpr uint8_t ROW_HEIGHT_DOTS = (DISPLAY_HEIGHT / 2);

  // Some command constants
  static constexpr uint8_t CMD_CHARACTER_DISPLAY_START = 0x20;
  static constexpr uint8_t CMD_CHARACTER_DISPLAY_END = 0xFF;

  // Display memory: 112x16 pixels as bool array
  // Each element represents one pixel
  DisplayMemory display_memory_;

  // Cursor position (in pixels)
  uint16_t cursor_x_;
  uint16_t cursor_y_;

  // Font and display settings
  std::vector<uint8_t> commandBytes_;
  bool initialized_;
  uint8_t international_font_set_;
  uint8_t character_code_type_;
  bool overwrite_mode_;
  uint8_t scroll_mode_;
  uint8_t horizontal_scroll_speed_;
  uint8_t brightness_level_;
  bool reverse_display_;
  uint8_t composition_mode_;
  uint8_t current_window_;
  uint8_t font_magnification_x_;
  uint8_t font_magnification_y_;

  // Command state tracking
  std::string command_buffer_;
  std::vector<uint8_t> command_arguments_;

  // Display memory access
  void ClearDisplayMemory();

  // Cursor management
  void SetCursor(uint16_t x, uint16_t y);
  uint16_t GetCursorX() const;
  uint16_t GetCursorY() const;

  // Character rendering
  void AdvanceCursor(uint16_t& x, uint16_t& y) const;
  void DrawCharacterAtCursor(uint8_t character);
  void DrawCharacterAt(uint16_t x, uint16_t y, uint8_t character);

  // Helper methods
  void SetPixel(uint16_t x, uint16_t y, bool on);
  bool GetPixel(uint16_t x, uint16_t y) const;
  void DrawFontCharacter(uint16_t x, uint16_t y, uint8_t character);
  void DrawImage(uint16_t x, uint16_t y, const FontCharSpan& image, uint8_t width, uint8_t height);
  FontCharSpan GetFontData(uint8_t character) const;

  // Command state tracking

  enum class State {
    Idle,
    GettingCommand,
    GettingCommandArguments,
    GettingVariableArgs,
  };

  typedef void (SimGu7000::*CommandFunction)(Stream&);
  typedef uint16_t (SimGu7000::*SizeGetFnFunction)() const;

  struct CommandItem {
    const char* Name;
    const CommandFunction Execute;
    const SizeGetFnFunction SizeGetFn;
    const uint8_t FixedArgumentBytes;
  };

  typedef std::unordered_map<std::string, CommandItem> CommandMap;

  State state_{State::Idle};
  static CommandMap CommandTable;
  CommandItem* CurrentCommand_{nullptr};
  uint16_t CurrentCommandVariableBytes_{0};

  void ExecuteCurrentCommandAndReset();
  void ResetCommandState();

  // Command implementations
  void ProcessCharacterDisplay(uint8_t character);
  void ProcessCharacterDisplayAtPosition(Stream& params);
  uint16_t ProcessCharacterDisplayAtPositionSize() const;
  void ProcessBackspace(Stream& params);
  void ProcessHorizontalTab(Stream& params);
  void ProcessLineFeed(Stream& params);
  void ProcessHomePosition(Stream& params);
  void ProcessCarriageReturn(Stream& params);
  void ProcessDisplayClear(Stream& params);
  void ProcessInitializeDisplay(Stream& params);
  void ProcessCursorSet(Stream& params);
  void ProcessDisplayClearCommand(Stream& params);
  void ProcessBrightnessControl(Stream& params);
  void ProcessReverseDisplay(Stream& params);
  void ProcessHorizontalScrollSpeed(Stream& params);
  void ProcessCompositionMode(Stream& params);
  void ProcessInternationalFontSet(Stream& params);
  void ProcessCharacterCodeType(Stream& params);
  void ProcessOverwriteMode(Stream& params);
  void ProcessVerticalScrollMode(Stream& params);
  void ProcessHorizontalScrollMode(Stream& params);
  void ProcessWait(Stream& params);
  void ProcessScrollDisplayAction(Stream& params);
  void ProcessDisplayBlink(Stream& params);
  void ProcessScreenSaver(Stream& params);
  void ProcessRealTimeBitImageDisplayXy(Stream& params);
  void ProcessRealTimeBitImageDisplay(Stream& params, uint8_t x, uint8_t y);
  uint16_t ProcessRealTimeBitImageDisplaySize() const;
  void ProcessCharacterFontWidthAndSpace(Stream& params);
  void ProcessFontMagnificationSet(Stream& params);
  void ProcessCurrentWindowSelect(Stream& params);
  void ProcessUserWindowDefinitionCancel(Stream& params);
  void ProcessWriteScreenModeSelect(Stream& params);
  void ProcessSpecifyDownloadRegister(Stream& params);
  void ProcessDownloadCharacter(Stream& params);
  void ProcessDeleteDownloadedCharacter(Stream& params);

  // Utilities
  void ExtractXY(Stream& s, uint16_t& x, uint16_t& y) const;
};
