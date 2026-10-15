import { GameState } from './core/GameState';
import { TerrainRenderer } from './render/TerrainRenderer';
import { SpriteRenderer } from './render/SpriteRenderer';
import { Minimap } from './ui/Minimap';
import { AudioManager } from './audio/AudioManager';
import { UIManager } from './ui/UIManager';
import { InputHandler } from './input/InputHandler';
import { Isometric } from './core/Isometric';

class GameEngine {
  private canvas: HTMLCanvasElement;
  private ctx: CanvasRenderingContext2D;

  private state: GameState;
  private terrainRenderer: TerrainRenderer;
  private spriteRenderer: SpriteRenderer;
  private minimap: Minimap;
  private audio: AudioManager;
  private ui: UIManager;
  private input: InputHandler;

  private lastTime = performance.now();
  private accumulator = 0;
  private readonly tickInterval = 1000 / 30; // 30 ticks / second

  private frameCount = 0;
  private lastFpsTime = performance.now();
  private currentFps = 60;

  constructor() {
    this.canvas = document.getElementById('canvas-viewport') as HTMLCanvasElement;
    this.ctx = this.canvas.getContext('2d', { alpha: false })!;

    this.resizeCanvas();
    window.addEventListener('resize', () => this.resizeCanvas());

    this.audio = new AudioManager();
    this.audio.preloadSounds();

    this.state = new GameState();
    this.ui = new UIManager(this.audio);
    this.input = new InputHandler(this.canvas, this.state, this.audio, this.ui);

    this.terrainRenderer = new TerrainRenderer();
    this.spriteRenderer = new SpriteRenderer();

    const minimapCanvas = document.getElementById('minimap-canvas') as HTMLCanvasElement;
    this.minimap = new Minimap(minimapCanvas);

    this.setupMinimapInteraction(minimapCanvas);

    // Initial greeting banner
    this.ui.showBanner('Age of Empires II (Web Edition) • Ready for Command');

    requestAnimationFrame((t) => this.gameLoop(t));
  }

  private resizeCanvas() {
    this.canvas.width = window.innerWidth;
    this.canvas.height = window.innerHeight;
  }

  private setupMinimapInteraction(canvas: HTMLCanvasElement) {
    let isMinimapDown = false;

    const navigate = (e: MouseEvent) => {
      const rect = canvas.getBoundingClientRect();
      const mx = (e.clientX - rect.left) * (canvas.width / rect.width);
      const my = (e.clientY - rect.top) * (canvas.height / rect.height);
      const worldTarget = this.minimap.minimapToWorld(mx, my);

      // Center camera on world target
      const targetIso = Isometric.worldToScreen(worldTarget.x, worldTarget.y, 0, 0, 0, this.input.zoom);
      this.input.cameraX = this.canvas.width / 2 - targetIso.x;
      this.input.cameraY = this.canvas.height / 2 - targetIso.y;
    };

    canvas.addEventListener('mousedown', (e) => {
      isMinimapDown = true;
      navigate(e);
    });

    window.addEventListener('mousemove', (e) => {
      if (isMinimapDown) navigate(e);
    });

    window.addEventListener('mouseup', () => {
      isMinimapDown = false;
    });
  }

  private gameLoop(time: number) {
    const delta = time - this.lastTime;
    this.lastTime = time;
    this.accumulator += delta;

    // Fixed-timestep simulation ticks (30 Hz)
    while (this.accumulator >= this.tickInterval) {
      this.state.tick();
      this.accumulator -= this.tickInterval;
    }

    // Input & Camera Pan Update
    this.input.update();

    // Render Viewport
    this.render();

    // FPS Counter
    this.frameCount++;
    if (time - this.lastFpsTime >= 1000) {
      this.currentFps = (this.frameCount * 1000) / (time - this.lastFpsTime);
      this.frameCount = 0;
      this.lastFpsTime = time;
    }

    // Update DOM UI HUD
    this.ui.update(this.state, this.currentFps);

    requestAnimationFrame((t) => this.gameLoop(t));
  }

  private render() {
    const ctx = this.ctx;
    ctx.fillStyle = '#0a0d0a';
    ctx.fillRect(0, 0, this.canvas.width, this.canvas.height);

    // 1. Render Terrain Tiles & Fog
    this.terrainRenderer.render(
      ctx,
      this.state,
      this.input.cameraX,
      this.input.cameraY,
      this.input.zoom,
      this.input.hoveredTile
    );

    // 2. Render Sprites (Trees, Buildings, Units)
    this.spriteRenderer.render(
      ctx,
      this.state,
      this.input.cameraX,
      this.input.cameraY,
      this.input.zoom
    );

    // 3. Render Selection Box, Placement Ghost, Pings
    this.input.renderOverlay(ctx);

    // 4. Render Minimap
    this.minimap.render(
      this.state,
      this.input.cameraX,
      this.input.cameraY,
      this.input.zoom,
      this.canvas.width,
      this.canvas.height
    );
  }
}

window.addEventListener('DOMContentLoaded', () => {
  new GameEngine();
});
