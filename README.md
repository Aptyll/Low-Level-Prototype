# 🎮 WASM Grid Game

4-way splitscreen AI simulation in **C++/WebGL/WebAssembly**.

## ⚡ Quick Start

**Build:**
```bash
# Install Emscripten SDK in emsdk/
# Run build script
build.bat
```

**Run:**
```bash
python -m http.server 8000
# Open http://localhost:8000
```

## 📁 Structure

- `src/cpp/` - C++ source code
- `src/js/` - WASM loader
- `build/` - Compiled output
- `index.html` - Entry point

## 🔧 Requirements

- Emscripten SDK
- WebGL 2.0 browser

---

**License:** MIT