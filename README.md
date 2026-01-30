# 💎 Pro Editor Pocket For Picoruby ✨

![Calculator Demo](image/main.jpg)

🚀 **VS Code Like Editor in your pocket!** Write and execute Ruby code 

---

## 🛠️ Setup

### 1️⃣ Update submodules
```bash
git submodule update --init --recursive
```

### 2️⃣ Update CMakeLists

📝 Edit `components/picoruby-esp32/CMakeLists.txt`:

**➕ Add to SRCS:**
```cmake
${COMPONENT_DIR}/../tft/ports/esp32/tft_native.c
${COMPONENT_DIR}/../tft/ports/esp32/st7789_spi.c
```

**➕ Add to INCLUDE_DIRS:**
```cmake
${COMPONENT_DIR}/../tft/include
```

### 3️⃣ Update build configuration

📝 Edit `components/picoruby-esp32/picoruby/build_config/xtensa-esp.rb`:

```ruby
conf.gem File.expand_path('../../../tft', __dir__)
```

### 4 Build and flash 🔥

```bash
. $(YOUR_ESP_IDF_PATH)/export.sh
idf.py set-target esp32s3
idf.py build
idf.py flash
```

---

**Features:**
- Line numbers with auto-alignment
- Full Ruby syntax highlighting (keywords, strings, numbers, variables, etc.)
- Multi-line input support with auto-indentation

---

## ⚠️ Known Issues

🚧 **Work in Progress** - The following issues are currently being fixed:

- 🔄 **Resource Exhaustion**: After multiple executions, resources may become exhausted, causing the device to restart

Stay tuned for updates! 🛠️

