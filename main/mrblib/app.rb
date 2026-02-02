require 'shell'
require 'i2c'
require 'gpio'
require 'adc'
require 'tft'
require 'sdcard'

GPIO.pull_up_at 10
boot = GPIO.new(10, GPIO::OUT)
boot.write 1

sleep_ms 500

# track ball
left = GPIO.new(1, GPIO::IN)
right = GPIO.new(2, GPIO::IN) 
up = GPIO.new(3, GPIO::IN) 
down = GPIO.new(15, GPIO::IN) 
click = GPIO.new(0, GPIO::IN) 

# Internal constants to ignore in completion
INTERNAL_CONSTANTS = [
  'HIGHLIGHT_KEYWORDS',
  'INDENT_INCREASE',
  'INDENT_DECREASE',
  'INTERNAL_CONSTANTS',
  'CODE_AREA_Y_START',
  'CODE_AREA_Y_END',
  'KEYBOARD_I2C_ADDR',
  'I2C_SDA_PIN',
  'I2C_SCL_PIN',
  'KEYBOARD_I2C'
]

# Initialize TFT Display
TFT.init
TFT.fill_screen(0x070707)
TFT.set_text_size(1)
TFT.set_text_wrap(false)

# Keyboard Setup
KEYBOARD_I2C_ADDR = 0x55
I2C_SDA_PIN = 18
I2C_SCL_PIN = 8
KEYBOARD_I2C = I2C.new(unit: "ESP32_I2C1", sda_pin: I2C_SDA_PIN, scl_pin: I2C_SCL_PIN, frequency: 200000)

# Screen layout
CODE_AREA_Y_START = 33
CODE_AREA_Y_END = 201

# Battery ADC
$bat_adc = ADC.new(4)

# Keywords for syntax highlighting
HIGHLIGHT_KEYWORDS = [
  'def', 
  'class',
  'module',
  'end',
  'if',
  'elsif',
  'else',
  'unless',
  'case',
  'when',
  'then',
  'while',
  'until',
  'for',
  'do',
  'begin',
  'rescue',
  'ensure',
  'return',
  'yield',
  'break',
  'next',
  'redo',
  'and',
  'or',
  'not',
  'super',
  'attr_accessor',
  'attr_reader',
  'alias',
  'require',
  'true',
  'false',
  'nil',
  'self'
]

INDENT_INCREASE = [
  'class',
  'module',
  'def',
  'if',
  'unless',
  'elsif',
  'else',
  'do',
  'case',
  'when',
  'while',
  'until',
  'for'
]

INDENT_DECREASE = ['end', 'else', 'elsif', 'when']

# ti-doc: Check if string is a number
def is_number?(str)
  return false if str == '' || str.nil?
  str.each_char do |c|
    return false if c < '0' || c > '9'
  end
  true
end

# ti-doc: Tokenize code
def tokenize(code)
  tokens = []
  current_token = ''
  in_string = false
  string_end_char = ''
  token_ends = ['(', ')', '{', '}', '[', ']', ',', ';', '.', '+', '-', '*', '/', '=', '<', '>', '!', '&', '|']

  code.each_char do |c|
    if in_string
      current_token << c
      if c == string_end_char
        tokens << current_token
        current_token = ''
        in_string = false
        string_end_char = ''
      end
    elsif c == "'" || c == '"'
      tokens << current_token if current_token != ''
      current_token = c
      in_string = true

      if c == "'"
        string_end_char = "'"
      else
        string_end_char = '"'
      end
    elsif c == ' '
      tokens << current_token if current_token != ''
      tokens << ' '
      current_token = ''
    elsif token_ends.include?(c)
      tokens << current_token if current_token != ''
      tokens << c
      current_token = ''
    else
      current_token << c
    end
  end
  tokens << current_token if current_token != ''
  tokens
end

# ti-doc: Draw text with color
def draw_text(text, x, y, color)
  TFT.set_text_color(color)
  TFT.set_cursor(x, y)
  TFT.print(text)
end

# ti-doc: Draw code with syntax highlighting
def draw_code_highlighted(code_str, x, y)
  tokens = tokenize(code_str)
  is_def = false

  tokens.each do |token|
    color = 0xD4D4D4  # default white

    if token.length > 0 && (token[0] == "'" || token[0] == '"')
      color = 0xCE9178  # string brown
    elsif token.length > 0 && token[0] == ':'
      color = 0x569CD6  # symbol blue
    elsif token.length > 1 && token[-1] == ':'
      color = 0x569CD6  # symbol blue
    elsif token.length > 1 && token[0] == '@' && token[1] == '@'
      color = 0x9CDCFE  # variable light blue
    elsif token.length > 0 && token[0] == '@'
      color = 0x9CDCFE  # variable light blue
    elsif token.length > 0 && token[0] == '$'
      color = 0x9CDCFE  # global light blue
    elsif is_number?(token)
      color = 0xB5CEA8  # number green
    elsif ['nil', 'true', 'false', 'self'].include?(token)
      # pseudo variables color (blue)
      color = 0x569CD6
    elsif token == 'def'
      is_def = true
      color = 0xC586C0
    elsif HIGHLIGHT_KEYWORDS.include?(token)
      color = 0xC586C0  # keyword pink
    elsif token.length > 0 && token[0] >= 'A' && token[0] <= 'Z'
      color = 0x4EC9B0  # constant teal
    elsif token == ' ' || token == '.'
      # normal text color (white) 
      color = 0xD4D4D4
    elsif is_def
      color = 0xEEEECC  # method yellow
      is_def = false
    end

    draw_text(token, x, y, color)
    x += token.length * 6
  end
end

# ti-doc: Draw Ruby icon
def draw_ruby_icon(x, y, c = 0xCC342D)
  # Row 1
  TFT.draw_pixel(x + 2, y + 3, c)
  TFT.draw_pixel(x + 3, y + 3, c)
  TFT.draw_pixel(x + 4, y + 3, c)
  TFT.draw_pixel(x + 5, y + 3, c)
  TFT.draw_pixel(x + 6, y + 3, c)

  # Row 2
  TFT.draw_pixel(x + 1, y + 4, c)
  TFT.draw_pixel(x + 2, y + 4, c)
  TFT.draw_pixel(x + 3, y + 4, c)
  TFT.draw_pixel(x + 4, y + 4, c)
  TFT.draw_pixel(x + 5, y + 4, c)
  TFT.draw_pixel(x + 6, y + 4, c)
  TFT.draw_pixel(x + 7, y + 4, c)

  # Row 3
  TFT.draw_pixel(x + 2, y + 5, c)
  TFT.draw_pixel(x + 3, y + 5, c)
  TFT.draw_pixel(x + 4, y + 5, c)
  TFT.draw_pixel(x + 5, y + 5, c)
  TFT.draw_pixel(x + 6, y + 5, c)

  # Row 4
  TFT.draw_pixel(x + 3, y + 6, c)
  TFT.draw_pixel(x + 4, y + 6, c)
  TFT.draw_pixel(x + 5, y + 6, c)

  # Row 5
  TFT.draw_pixel(x + 4, y + 7, c)
end

# ti-doc: Draw Static UI frame
def draw_ui
  # Tab bar background (full width)
  TFT.fill_rect(0, 0, 320, 22, 0x2D2D2D)

  icon_width = 12

  # Start Active tab (calc.rb)
  tab_text = 'app.rb'
  tab_width = (tab_text.length * 6) + 42

  # Active tab background (same as editor bg)
  TFT.fill_rect(0, 0, tab_width, 22, 0x070707)

  # Top accent line (blue highlight for active tab)
  TFT.fill_rect(0, 0, tab_width, 2, 0x007ACC)

  # Ruby icon
  draw_ruby_icon(4, 7)

  # Filename
  draw_text(tab_text, 18, 8, 0xD4D4D4)

  # Close button (x)
  close_x = (tab_text.length * 6) + 30
  draw_text('x', close_x, 8, 0x6E6E6E)
  # End Active tab (calc.rb)

  # Start Inactive tab (dummy.rb)
  dummy_text = 'dummy.rb'
  dummy_start = tab_width + 1
  dummy_width = (dummy_text.length * 6) + 42

  # Inactive tab background (same as tab bar, no accent line)
  TFT.fill_rect(dummy_start, 1, dummy_width, 21, 0x0F0F0F)

  # Ruby icon
  draw_ruby_icon(dummy_start + 4, 7, 0x992828)

  # Filename (dimmed)
  draw_text(dummy_text, dummy_start + 18, 8, 0x6E6E6E)

  # Close button (x)
  dummy_close_x = dummy_start + (dummy_text.length * 6) + 30
  draw_text('x', dummy_close_x, 8, 0x6E6E6E)
  # End Inactive tab (dummy.rb)


  # Result area
  TFT.draw_fast_h_line(0, 202, 320, 0x303030)
  draw_text('>>', 2, 210, 0x6E6E6E)

  # Status bar separator
  TFT.draw_fast_h_line(0, 226, 320, 0x303030)
end

# ti-doc: Calculate current line Y position
def current_line_y(code_lines_count)
  max_visible = 16
  start_line = [0, code_lines_count - max_visible + 1].max
  visible_lines = code_lines_count - start_line
  CODE_AREA_Y_START + visible_lines * 10
end

# ti-doc: Draw current line only
def draw_current_line(current_code, indent_ct, current_row, code_lines_count)
  y = current_line_y(code_lines_count)
  return if y > CODE_AREA_Y_END - 10

  TFT.fill_rect(0, y, 320, 10, 0x070707)
  TFT.draw_fast_v_line(28, y - 2, 10, 0x303030)

  line_number = current_row > 9 ? 1 : 2
  draw_text("#{' ' * line_number}#{current_row}", 0, y, 0x858585)

  code_display = "#{'  ' * indent_ct}#{current_code}"
  draw_code_highlighted(code_display, 38, y)
  draw_text('_', 38 + code_display.length * 6, y, 0xD4D4D4)

  draw_completion(current_code, code_lines_count)
end

# ti-doc: Draw code area (full redraw)
def draw_code_area(code_lines, current_code, indent_ct, current_row)
  # Reset completion for class context
  $dict.delete('attr_reader')
  $dict.delete('attr_accessor')
  $dict.delete('initialize')

  TFT.fill_rect(0, CODE_AREA_Y_START, 320, CODE_AREA_Y_END - CODE_AREA_Y_START, 0x070707)

  max_visible = 16
  total = code_lines.length
  start_line = [0, total - max_visible + 1].max
  y = CODE_AREA_Y_START

  # Draw history lines
  (start_line...total).each do |i|
    line = code_lines[i]
    ln = i + 1
    line_number = ln > 9 ? 1 : 2

    TFT.fill_rect(0, y, 34, 10, 0x070707)
    TFT.draw_fast_v_line(28, y - 2, 10, 0x303030)
    draw_text("#{' ' * line_number}#{ln}", 0, y, 0x6E6E6E)
    draw_code_highlighted("#{'  ' * line[:indent]}#{line[:text]}", 38, y)
    y += 10
  end

  # Draw current line
  if y <= CODE_AREA_Y_END - 10
    line_number = current_row > 9 ? 1 : 2
    
    TFT.draw_fast_v_line(28, y - 2, 10, 0x303030)
    draw_text("#{' ' * line_number}#{current_row}", 0, y, 0xD4D4D4)
    code_display = "#{'  ' * indent_ct}#{current_code}"
    draw_code_highlighted(code_display, 38, y)
    draw_text('_', 38 + code_display.length * 6, y, 0xD4D4D4)
  end

  code_lines.each do |line|
    tokens = tokenize(line[:text])
    tokens.each_with_index do |token, idx|
      if token == 'class' && idx == 0
        $dict['attr_reader'] = true
        $dict['attr_accessor'] = true
        $dict['initialize'] = true
        break
      end
    end
  end

  draw_completion(current_code, current_row)
end

# ti-doc: Draw result with horizontal scroll offset
def draw_result(res, offset = 0)
  TFT.fill_rect(22, 208, 298, 12, 0x070707)
  color = 0xD4D4D4
  if res.class == Integer || res.class == Float
    color = 0xB5CEA8
  elsif res.class == String
    color = 0xCE9178
  elsif res.class == NilClass
    color = 0x569CD6
    res = 'nil'
  elsif res.class == TrueClass || res.class == FalseClass
    color = 0x569CD6
  end
  display_str = res.to_s
  if offset > 0 && display_str.length > offset
    display_str = display_str[offset..]
  end
  draw_text(display_str, 22, 210, color)
end

# ti-doc: Draw status
def draw_status(msg, line_num = nil)
  battery_voltage = $bat_adc.read_voltage

  # Skip if nothing changed
  if line_num == $last_status_line
    return
  end

  $last_status_line = line_num

  TFT.fill_rect(0, 227, 320, 13, 0x007ACC)
  draw_text(msg, 4, 230, 0xFFFFFF)

  right_items = []

  if line_num
    right_items << "Ln #{line_num}"
  end

  right_items << "Ruby"
  right_items << "#{battery_voltage}V"

  right_text = right_items.join('  ')
  right_x = 320 - (right_text.length * 6) - 4
  draw_text(right_text, right_x, 230, 0xFFFFFF)
end

# Initial draw
draw_ui
draw_status('--NORMAL--', 1)

# Completion
$dict = {}
$completion_chars = nil
$completion_candidates = []
$completion_index = 0
$draw_completion_box_y = CODE_AREA_Y_START
$completion_box_visible = false

# Slot selection modal
$slot_modal_mode = nil  # nil, :save, :load
$slot_selected = 0

# ti-doc: Draw slot selection modal
def draw_slot_modal(mode)
  # Modal background
  box_x = 80
  box_y = 50
  box_w = 160
  box_h = 120

  # Shadow
  TFT.fill_rect(box_x + 2, box_y + 2, box_w, box_h, 0x000000)

  # Main background
  TFT.fill_rect(box_x, box_y, box_w, box_h, 0x252526)

  # Border
  TFT.draw_rect(box_x, box_y, box_w, box_h, 0x007ACC)

  # Title
  title = mode == :save ? 'Save to Slot' : 'Load from Slot'
  title_x = box_x + (box_w - title.length * 6) / 2
  draw_text(title, title_x, box_y + 6, 0xD4D4D4)

  # Separator
  TFT.draw_fast_h_line(box_x + 1, box_y + 18, box_w - 2, 0x303030)

  # Slots (0-7) in 2 columns
  8.times do |i|
    col = i % 2
    row = i / 2
    slot_x = box_x + 10 + col * 75
    slot_y = box_y + 24 + row * 22

    # Highlight selected
    if i == $slot_selected
      TFT.fill_rect(slot_x - 2, slot_y - 2, 70, 18, 0x094771)
    end

    # Slot label
    label = "Slot #{i}"
    draw_text(label, slot_x, slot_y, 0xD4D4D4)
  end

  # Instructions
  inst = 'Ball:Select Click:OK'
  inst_x = box_x + (box_w - inst.length * 6) / 2
  draw_text(inst, inst_x, box_y + box_h - 12, 0x6E6E6E)
end

# ti-doc: Close slot modal and restore screen
def close_slot_modal
  $slot_modal_mode = nil
  $slot_selected = 0
end

# ti-doc: Load constants for completion
def load_constants
  Object.constants.each do |constant|
    constant_str = constant.to_s

    if INTERNAL_CONSTANTS.include?(constant_str) || constant_str.index('Error') != nil
      next
    end

    $dict[constant_str] = true
  end
end

# ti-doc: Clear completion box area
def clear_completion_box
  return unless $completion_box_visible

  box_x = 220
  box_y = $draw_completion_box_y
  box_w = 100
  box_h = 6 * 10 + 20
  max_h = CODE_AREA_Y_END - box_y
  if box_h > max_h
    box_h = max_h
  end
  TFT.fill_rect(box_x, box_y, box_w, box_h, 0x070707)
  $completion_box_visible = false
end

# ti-doc: Draw completion and set $completion_chars
def draw_completion(current_code, code_lines_count)
  $completion_chars = nil
  $completion_candidates = []
  clear_completion_box
  return if current_code == ''

  tokens = tokenize(current_code)
  target = tokens.last
  return if target.nil? || target == '' || target == ' '

  candidates = []
  $dict.keys.each do |key|
    if key.length > target.length && key[0, target.length] == target
      candidates << key
    end
  end

  return if candidates.length == 0

  candidates = candidates[0, 6]
  candidates << '(skip)'
  $completion_candidates = candidates

  # Reset index if out of bounds
  if $completion_index >= candidates.length
    $completion_index = 0
  end

  # Set completion chars (nil for skip option)
  if $completion_index == candidates.length - 1
    $completion_chars = nil
  else
    selected = candidates[$completion_index]
    $completion_chars = selected[target.length, selected.length]
  end

  # Draw completion box
  box_x = 222
  box_y = CODE_AREA_Y_START + 2 + (code_lines_count * 10 + 10)
  if (box_y + 70) >= 207
    box_y = 116
  end
  box_w = 96
  box_h = candidates.length * 10 + 4

  $draw_completion_box_y = box_y
  $completion_box_visible = true

  # Shadow effect
  TFT.fill_rect(box_x + 1, box_y + 1, box_w, box_h, 0x000000)

  # Main background
  TFT.fill_rect(box_x, box_y, box_w, box_h, 0x252526)

  # Border
  TFT.draw_rect(box_x, box_y, box_w, box_h, 0x303030)

  # Selected candidate highlight
  highlight_y = box_y + 1 + $completion_index * 10
  TFT.fill_rect(box_x + 1, highlight_y, box_w - 2, 10, 0x094771)

  # Draw candidates
  candidates.each_with_index do |candidate, idx|
    y = box_y + 2 + idx * 10

    if candidate == '(skip)'
      draw_text(candidate, box_x + 2, y, 0x6E6E6E)
    else
      disp_name = candidate.length > 14 ? candidate[0, 13] + '..' : candidate
      color = (candidate[0] >= 'A' && candidate[0] <= 'Z') ? 0x4EC9B0 : 0xFFFCDA
      draw_text(disp_name, box_x + 2, y, color)
    end
  end
end

# ti-doc: Draw newline without scroll (prev line + new line)
def draw_newline_no_scroll(prev_line, current_code, indent_ct, current_row, code_lines_count)
  prev_y = current_line_y(code_lines_count - 1)
  prev_row = current_row - 1

  TFT.fill_rect(0, prev_y, 320, 10, 0x070707)
  line_number = prev_row > 9 ? 1 : 2

  TFT.fill_rect(0, prev_y, 34, 10, 0x070707)
  TFT.draw_fast_v_line(28, prev_y - 2, 10, 0x303030)

  draw_text("#{' ' * line_number}#{prev_row}", 0, prev_y, 0x6E6E6E)
  draw_code_highlighted("#{'  ' * prev_line[:indent]}#{prev_line[:text]}", 38, prev_y)

  y = current_line_y(code_lines_count)
  return if y > CODE_AREA_Y_END - 10

  line_number = current_row > 9 ? 1 : 2

  # Line number with highlight (current line)
  TFT.draw_fast_v_line(28, y - 2, 10, 0x303030)
  draw_text("#{' ' * line_number}#{current_row}", 0, y, 0xD4D4D4)
  code_display = "#{'  ' * indent_ct}#{current_code}"
  draw_code_highlighted(code_display, 38, y)
  draw_text('_', 38 + code_display.length * 6, y, 0xD4D4D4)

  draw_completion(current_code, code_lines_count)
end

# State for main loop
code = ''
code_lines = []
indent_ct = 0
current_row = 1
execute_code = ''
result = nil
result_offset = 0
need_full_redraw = true
need_line_redraw = false
need_newline_redraw = false
need_result_redraw = false
prev_line_for_newline = nil
right_pressed = right.high?
left_pressed = left.high?
up_pressed = up.high?
down_pressed = down.high?
click_pressed = click.high?
loop_counter = 0

sandbox = Sandbox.new('')

load_constants

loop do
  loop_counter += 1

  # Get keyboard input
  key_event = 0
  begin
    data = KEYBOARD_I2C.read(KEYBOARD_I2C_ADDR, 1)
    key_event = data.ord if data && data.length > 0
  rescue
  end

  if key_event > 0
    # Debug: show key code at top right
    # if key_event != 7
    #   TFT.fill_rect(280, 4, 40, 14, 0x2D2D2D)
    #   draw_text("K:#{key_event}", 282, 8, 0x6E6E6E)
    # end

    draw_status('--NORMAL--', current_row)

    # Cancel slot modal with backspace
    if $slot_modal_mode && key_event == 8
      close_slot_modal
      need_full_redraw = true
      next
    end

    # alt + c
    if key_event == 12
      code = ''
      code_lines = []
      indent_ct = 0
      execute_code = ''
      current_row = 1
      need_full_redraw = true
      next

    # backspace
    elsif key_event == 8
      if code == '' && code_lines.length > 0
        # Move cursor to prev line
        prev_line = code_lines.pop
        code = prev_line[:text]
        indent_ct = prev_line[:indent]
        current_row -= 1

        # Delete last line
        if execute_code.length > 0
          lines = execute_code.split("\n")
          lines.pop
          execute_code = lines.length > 0 ? lines.join("\n") + "\n" : ''
        end

        $completion_index = 0
        need_full_redraw = true
      else
        code = code[0..-2]
        $completion_index = 0
        need_line_redraw = true
      end

    # return
    elsif key_event == 13
      # Select completion candidate
      if $completion_chars.is_a?(String)
        code << $completion_chars
        $completion_index = 0
        $completion_chars = nil
        need_line_redraw = true
        next
      elsif $completion_candidates.length > 0
        # Skip selected - just close completion modal
        $completion_index = 0
        $completion_candidates = []
        clear_completion_box
        next
      end

      if code != ''
        execute_code << code
        execute_code << "\n"

        tokens = tokenize(code)

        tokens.each do |token|
          if INDENT_DECREASE.include?(token)
            indent_ct -= 1 if indent_ct > 0
            break
          end
        end

        prev_line_for_newline = {text: code, indent: indent_ct}
        code_lines << prev_line_for_newline

        if tokens[0] == 'class'
          $dict['attr_reader'] = true
          $dict['attr_accessor'] = true
          $dict['initialize'] = true
        end

        tokens.each_with_index do |token, idx|
          if (INDENT_INCREASE.include?(token) && idx == 0) || token == 'do'
            indent_ct += 1
            break
          end
        end

        code = ''
        current_row += 1

        # Judgement redraw pattern for scroll
        if code_lines.length >= 17
          need_full_redraw = true
        else
          need_newline_redraw = true
        end

      # Execute code
      elsif indent_ct == 0 && execute_code != ''
        if sandbox.compile("_ = (#{execute_code})", remove_lv: true)
          sandbox.execute
          sandbox.wait(timeout: nil)

          err = sandbox.error
          if err.nil?
            result = sandbox.result
          else
            result = "#{err} #{err.message}"
          end

          sandbox.suspend
        else
          result = "syntax error"
        end

        result_offset = 0
        draw_result(result, result_offset)

        # Add to completion dict
        code_lines.each do |line|
          tokens = tokenize(line[:text])
          is_def = false
          is_attr = false
          ignore = ['self', '.', ' ', ',', '(', ')', 'initialize']
          define_tokens = ['def', 'class', 'module']
          attr_tokens = ['attr_accessor', 'attr_reader']

          tokens.each_with_index do |token, idx|
            if define_tokens.include?(token) && idx == 0
              $dict['new'] = true if token == 'class'
              is_def = true
              next
            end

            if attr_tokens.include?(token) && idx == 0
              is_attr = true
              next
            end

            if is_def && !ignore.include?(token)
              $dict[token] = true
              is_def = false
            end

            if is_attr && !ignore.include?(token)
              if token[0] == ':'
                $dict[token[1, token.length]] = true
              end
            end

            if token == 'require'
              load_constants
            end
          end
          is_attr = false
        end

        # Reset
        $dict.delete('attr_reader')
        $dict.delete('attr_accessor')
        $dict.delete('initialize')

        code_lines = []
        execute_code = ''
        indent_ct = 0
        current_row = 1
        need_full_redraw = true
      end

    elsif key_event == 15
      char = '['
      code << char
      $completion_index = 0
      need_line_redraw = true

    elsif key_event == 26
      char = ']'
      code << char
      $completion_index = 0
      need_line_redraw = true

    elsif key_event == 31
      char = '{'
      code << char
      $completion_index = 0
      need_line_redraw = true

    elsif key_event == 1
      char = '}'
      code << char
      $completion_index = 0
      need_line_redraw = true

    elsif key_event == 23
      char = '<'
      code << char
      $completion_index = 0
      need_line_redraw = true

    elsif key_event == 24
      char = '>'
      code << char
      $completion_index = 0
      need_line_redraw = true

    elsif key_event == 3
      char = '='
      code << char
      $completion_index = 0
      need_line_redraw = true

    elsif key_event == 224
      char = '|'
      code << char
      $completion_index = 0
      need_line_redraw = true

    # SDCard save - open slot modal
    elsif key_event == 20
      $slot_modal_mode = :save
      $slot_selected = 0
      draw_slot_modal(:save)
      next

    # SDCard load - open slot modal
    elsif key_event == 2
      $slot_modal_mode = :load
      $slot_selected = 0
      draw_slot_modal(:load)
      next

    elsif key_event >= 32 && key_event < 127
      char = key_event.chr
      code << char
      $completion_index = 0
      need_line_redraw = true
    end
  end

  # Track ball (check every 5 loops)
  if loop_counter >= 5
    if $completion_candidates.length == 0
      down_pressed = true
    end

    loop_counter = 0

    r_high = right.high?
    l_high = left.high?
    u_high = up.high?
    d_high = down.high?
    c_high = click.high?

    # Debug: show trackball state
    # TFT.fill_rect(200, 4, 120, 14, 0x2D2D2D)
    # draw_text("U:#{u_high ? 1 : 0} D:#{d_high ? 1 : 0} R:#{r_high ? 1 : 0} L:#{l_high ? 1 : 0}", 202, 8, 0x6E6E6E)

    # Reset flags when released
    right_pressed = false if !r_high
    left_pressed = false if !l_high
    up_pressed = false if !u_high
    down_pressed = false if !d_high
    click_pressed = false if !c_high

    # Slot modal navigation
    if $slot_modal_mode
      if u_high && !up_pressed
        up_pressed = true
        down_pressed = true
        left_pressed = true
        right_pressed = true

        if $slot_selected >= 2
          $slot_selected -= 2
          draw_slot_modal($slot_modal_mode)
        end
        next
      elsif d_high && !down_pressed
        up_pressed = true
        down_pressed = true
        left_pressed = true
        right_pressed = true

        if $slot_selected <= 5
          $slot_selected += 2
          draw_slot_modal($slot_modal_mode)
        end
        next
      elsif l_high && !left_pressed
        up_pressed = true
        down_pressed = true
        left_pressed = true
        right_pressed = true

        if $slot_selected % 2 == 1
          $slot_selected -= 1
          draw_slot_modal($slot_modal_mode)
        end
        next
      elsif r_high && !right_pressed
        up_pressed = true
        down_pressed = true
        left_pressed = true
        right_pressed = true

        if $slot_selected % 2 == 0
          $slot_selected += 1
          draw_slot_modal($slot_modal_mode)
        end
        next
      elsif c_high && !click_pressed
        up_pressed = true
        down_pressed = true
        left_pressed = true
        right_pressed = true
        click_pressed = true

        slot = $slot_selected
        mode = $slot_modal_mode
        close_slot_modal

        if mode == :save
          full_code = ''
          code_lines.each do |line|
            full_code << "#{'  ' * line[:indent]}#{line[:text]}\n"
          end
          full_code << "#{'  ' * indent_ct}#{code}" if code != ''

          save_result = SDCard.save(slot, full_code)
          TFT.init
          TFT.fill_screen(0x070707)
          draw_ui
          $last_status_line = nil
          draw_status(save_result ? "Saved to #{slot}!" : 'Save failed', current_row)
        else
          loaded = SDCard.load(slot)
          TFT.init
          TFT.fill_screen(0x070707)
          draw_ui
          if loaded
            lines = loaded.split("\n")
            code_lines = []
            lines.each do |line|
              stripped = line.lstrip
              indent = (line.length - stripped.length) / 2
              code_lines << {text: stripped, indent: indent}
            end
            code = ''
            indent_ct = 0
            current_row = code_lines.length + 1
            execute_code = loaded + "\n"
          end
          $last_status_line = nil
          draw_status(loaded ? "Loaded from #{slot}!" : 'Load failed', current_row)
        end
        need_full_redraw = true
        next
      end
      next
    end

    # Completion navigation
    if $completion_candidates.length > 0
      if u_high && !up_pressed
        up_pressed = true
        down_pressed = true
        left_pressed = true
        right_pressed = true
        click_pressed = true

        if $completion_index > 0
          $completion_index -= 1
          need_line_redraw = true
        end
        next
      elsif d_high && !down_pressed
        up_pressed = true
        down_pressed = true
        left_pressed = true
        right_pressed = true
        click_pressed = true

        if $completion_index < $completion_candidates.length - 1
          $completion_index += 1
          need_line_redraw = true
        end
        next
      elsif $completion_chars.is_a?(String) && c_high && !click_pressed
        up_pressed = true
        down_pressed = true
        left_pressed = true
        right_pressed = true
        click_pressed = true

        code << $completion_chars
        $completion_index = 0
        $completion_chars = nil
        need_line_redraw = true
        next

      elsif c_high && !click_pressed
        up_pressed = true
        down_pressed = true
        left_pressed = true
        right_pressed = true
        click_pressed = true

        # Skip selected - just close completion modal
        $completion_index = 0
        $completion_candidates = []
        clear_completion_box
        next
      end
    end

    # Result scroll with left/right
    if result && r_high && !right_pressed
      up_pressed = true
      down_pressed = true
      left_pressed = true
      right_pressed = true
      click_pressed = true

      res_str = result.to_s
      check_offset = result_offset + 8
      if res_str.length >= check_offset
        result_offset += 8
        need_result_redraw = true
      end
      next
    elsif result && l_high && !left_pressed
      up_pressed = true
      down_pressed = true
      left_pressed = true
      right_pressed = true
      click_pressed = true

      check_offset = result_offset - 8
      if check_offset >= 0
        result_offset = check_offset
      else
        result_offset = 0
      end

      need_result_redraw = true
      next
    elsif result && c_high && !click_pressed
      up_pressed = true
      down_pressed = true
      left_pressed = true
      right_pressed = true
      click_pressed = true

      result_offset = 0
      need_result_redraw = true
      next
    end
  end

  # Redraw
  if need_full_redraw
    draw_code_area(code_lines, code, indent_ct, current_row)
    need_full_redraw = false
    need_line_redraw = false
    need_newline_redraw = false
  elsif need_newline_redraw
    draw_newline_no_scroll(prev_line_for_newline, code, indent_ct, current_row, code_lines.length)
    need_newline_redraw = false
  elsif need_line_redraw
    draw_current_line(code, indent_ct, current_row, code_lines.length)
    need_line_redraw = false
  end

  if need_result_redraw
    draw_result(result, result_offset)
    need_result_redraw = false
  end
end
