let wasmModule = null;

function resizeCanvases() {
    const canvases = [
        document.getElementById('canvas-red'),
        document.getElementById('canvas-blue'),
        document.getElementById('canvas-purple'),
        document.getElementById('canvas-brown')
    ];

    // Get viewport dimensions (each viewport is 50% of screen width and height)
    const viewportWidth = Math.floor(window.innerWidth / 2);
    const viewportHeight = Math.floor(window.innerHeight / 2);

    // Use full viewport dimensions for wider aspect ratio (fits 4-way split screen naturally)
    // This gives us a wider aspect ratio that matches the split screen layout
    const canvasWidth = viewportWidth;
    const canvasHeight = viewportHeight;

    // Update all canvases - set internal resolution to match viewport
    // CSS will handle display sizing (100% width/height)
    for (const canvas of canvases) {
        canvas.width = canvasWidth;
        canvas.height = canvasHeight;
    }

    // Update renderer if initialized
    if (wasmModule && wasmModule._resize_renderer) {
        wasmModule._resize_renderer(canvasWidth, canvasHeight);
    }

    return { width: canvasWidth, height: canvasHeight };
}

async function init() {
    // Resize canvases to proper aspect ratio before initialization
    resizeCanvases();

    const canvases = [
        document.getElementById('canvas-red'),
        document.getElementById('canvas-blue'),
        document.getElementById('canvas-purple'),
        document.getElementById('canvas-brown')
    ];

    // Check WebGL2 support on first canvas
    if (!canvases[0].getContext('webgl2') && !canvases[0].getContext('experimental-webgl2')) {
        alert('WebGL 2.0 not supported!');
        return;
    }

    // Handle window resize
    window.addEventListener('resize', resizeCanvases);
    
    // Load WASM module - let Emscripten create the WebGL context
    try {
        wasmModule = await Module({
            onRuntimeInitialized: function() {
                // 'this' refers to the Module instance
                // Set wasmModule so input handlers can access it
                wasmModule = this;
                
                // Initialize game - WebGL context should be ready now
                try {
                    wasmModule._init();
                    
                    // Start game loop - suppress Emscripten's harmless "unwind" exception
                    try {
                        wasmModule._start_game_loop();
                    } catch (loopError) {
                        // Suppress Emscripten's internal "unwind" exception - it's harmless
                        if (String(loopError) !== 'unwind') {
                            console.warn('Game loop error:', loopError);
                        }
                        // Game loop is still running despite the exception
                    }
                } catch (initError) {
                    throw initError;
                }
            }
        });
        
    } catch (error) {
        console.error('Error loading WASM:', error);
    }
}

// Start initialization when page loads
if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', init);
} else {
    init();
}
