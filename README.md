# Pro Editor Pocket for PicoRuby

![Demo](image/tdeck-main.jpg)

## A VS Code–like Editor in Your Pocket

**ProEditorPocket** is a VS Code–inspired editor that allows you to
edit and execute PicoRuby code directly on your device.

This project is intended as an experimental and playful piece of software.
Features such as file saving or multi-file editing are not supported.

---

## Setup

### Step 1: Initialize submodules

```bash
git submodule update --init --recursive
```

---

### Step 2: Update CMakeLists

Edit `components/picoruby-esp32/CMakeLists.txt`

Add the following entries to `SRCS`:

```cmake
${COMPONENT_DIR}/../picoruby-tft/ports/esp32/tft_native.c
${COMPONENT_DIR}/../picoruby-tft/ports/esp32/st7789_spi.c
```

Add the following entry to `INCLUDE_DIRS`:

```cmake
${COMPONENT_DIR}/../picoruby-tft/include
```

---

### Step 3: Update build configuration

Edit `components/picoruby-esp32/picoruby/build_config/xtensa-esp.rb`

```ruby
conf.gem File.expand_path('../../picoruby-tft', __dir__)
```

---

### Step 4: Build and flash

```bash
. $(YOUR_ESP_IDF_PATH)/export.sh
idf.py set-target esp32s3
idf.py build
idf.py flash
```

---

![Demo](image/tdeck-main.jpg)

## Features

- Line numbers with automatic alignment
- Ruby syntax highlighting (keywords, strings, numbers, variables, etc.)
- Multi-line input with automatic indentation
- Basic code completion

---

## Special Key Mapping

The keyboard layout is mostly identical to the default T-Keyboard configuration.
However, some keys are specially assigned to make Ruby coding more convenient.

The following symbols can be entered by pressing the `sym` key first,
then holding `Shift` while pressing the corresponding key.

- `=` : `Shift + Q`
- `[` : `Shift + G`
- `]` : `Shift + H`
- `{` : `Shift + V`
- `}` : `Shift + B`
- `<` : `Shift + Z`
- `>` : `Shift + X`

---

## Special Controls

These are additional controls designed to make the editor easier and more enjoyable to use.

- Trackball Up / Down  
  Select completion candidates

- Trackball Left / Right  
  Scroll the result area horizontally

- Alt + C  
  Clear all currently entered code

---

## Known Issues

This project is still under development.

- Repeated execution may exhaust system resources and cause the device to restart
- If any buttons stop responding, try resetting or reflashing the device

---