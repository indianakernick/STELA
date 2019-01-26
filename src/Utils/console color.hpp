//
//  console color.hpp
//  Simpleton Engine
//
//  Created by Indi Kernick on 24/6/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef engine_utils_console_color_hpp
#define engine_utils_console_color_hpp

#include <string_view>

#define ESC "\x1B"
#define CSI "\x1B["
#define CONSTR constexpr std::string_view

namespace con {
  /// Reset all formatting states to their default values. Disable italic and
  /// underline, set text color and background colors to their defaults, etc.
  CONSTR reset         = CSI "0m";
  
  /// Either change to a bolder font, use a brighter text color or both
  CONSTR bold          = CSI "1m";
  /// Either change to a lighter font, use a dimmer text color or both
  CONSTR faint         = CSI "2m";
  /// Change font to an italic one [iTerm2 but not Terminal.app]
  CONSTR italic        = CSI "3m";
  /// Put a line underneath text
  CONSTR underline     = CSI "4m";
  /// Blink text slowly [Terminal.app but not iTerm2]
  CONSTR blink_slow    = CSI "5m";
  /// Set text color to background color and background color to text color
  CONSTR video_neg     = CSI "7m";
  /// Hide or "conceal" text [Terminal.app but not iTerm2]
  CONSTR conceal       = CSI "8m";
  
  /// Disable bold and faint modes
  CONSTR no_bold_faint = CSI "22m";
  /// Disable italic mode [iTerm2 but not Terminal.app]
  CONSTR no_italic     = CSI "23m";
  /// Disable underline mode
  CONSTR no_underline  = CSI "24m";
  /// Undo the effects of `video_neg`
  CONSTR video_pos     = CSI "27m";
  /// Undo the effects of `conceal` and show the text [Terminal.app but not iTerm2]
  CONSTR reveal        = CSI "28m";
  
  /// Set text color to black
  CONSTR text_black    = CSI "30m";
  /// Set text color to red
  CONSTR text_red      = CSI "31m";
  /// Set text color to green
  CONSTR text_green    = CSI "32m";
  /// Set text color to yellow
  CONSTR text_yellow   = CSI "33m";
  /// Set text color to blue
  CONSTR text_blue     = CSI "34m";
  /// Set text color to magenta
  CONSTR text_magenta  = CSI "35m";
  /// Set text color to cyan
  CONSTR text_cyan     = CSI "36m";
  /// Set text color to white
  CONSTR text_white    = CSI "37m";
  /// Set text color to its default
  CONSTR text_default  = CSI "39m";
  
  /// Set background color to black
  CONSTR back_black    = CSI "40m";
  /// Set background color to red
  CONSTR back_red      = CSI "41m";
  /// Set background color to green
  CONSTR back_green    = CSI "42m";
  /// Set background color to yellow
  CONSTR back_yellow   = CSI "43m";
  /// Set background color to blue
  CONSTR back_blue     = CSI "44m";
  /// Set background color to magenta
  CONSTR back_magenta  = CSI "45m";
  /// Set background color to cyan
  CONSTR back_cyan     = CSI "46m";
  /// Set background color to white
  CONSTR back_white    = CSI "47m";
  /// Set background color to its default
  CONSTR back_default  = CSI "49m";
}

#undef CONSTR
#undef CSI
#undef ESC

#endif
