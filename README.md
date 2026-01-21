# WASM Grid Game

A full-screen splitscreen AI simulation viewer built with C++, WebGL, and WebAssembly. Watch 4 AI entities move randomly in a shared world with centered camera perspectives.

## 🚀 Live Demo

[View on GitHub Pages](https://Aptyll.github.io/Low-Level-Prototype/)

## 🛠️ Quick Start

### Local Development

1. **Build:** Run `build.bat` in the project root (Windows) or use Emscripten directly
2. **Serve:** Start a local HTTP server:
   ```bash
   python -m http.server 8000
   # or
   npx serve
   ```
3. **View:** Open `http://localhost:8000` to see 4-way splitscreen AI simulation

### GitHub Pages Deployment

1. Push your code to GitHub
2. Go to repository Settings → Pages
3. Select source branch (usually `main` or `master`)
4. The site will be available at `https://yourusername.github.io/repository-name/`

## ✨ Features

- **4-Way Splitscreen:** Full-screen view with 4 simultaneous AI perspectives
- **AI Entities:** 4 autonomous entities with random movement (red, blue, purple, brown teams)
- **Directional Arrows:** Edge indicators show off-screen AI locations
- **Dynamic Resizing:** Maintains proper aspect ratio on window resize
- **Subtle Dark Grid:** Soft dark grid world with subtle grid lines

## 📁 Project Structure

```
├── src/
│   ├── cpp/          # C++ source code
│   │   ├── engine/   # Game engine (renderer, game logic)
│   │   └── main.cpp  # Entry point
│   └── js/           # JavaScript WASM loader
├── build/            # Compiled WASM files (committed for GitHub Pages)
├── index.html        # Main HTML file
├── build.bat         # Build script (Windows)
└── .gitignore       # Git ignore rules
```

## 📋 Requirements

- **Development:**
  - Emscripten SDK (install in `emsdk/` directory)
  - HTTP server (WASM requires HTTP serving due to CORS)
  
- **Runtime:**
  - Browser with WebGL 2.0 support (Chrome, Firefox, Edge, Safari)

## 🔧 Building

The project uses Emscripten to compile C++ to WebAssembly. The build script (`build.bat`) compiles:
- `src/cpp/main.cpp` - Entry point
- `src/cpp/engine/renderer.cpp` - WebGL rendering
- `src/cpp/engine/game.cpp` - Game logic

Output files are generated in `build/` directory:
- `game.js` - JavaScript wrapper
- `game.wasm` - WebAssembly binary

## 📝 License

MIT License - see LICENSE file for details
