import { GameState } from './core/GameState';
import { TerrainRenderer } from './render/TerrainRenderer';
import { SpriteRenderer } from './render/SpriteRenderer';
import { Minimap } from './ui/Minimap';
import { AudioManager } from './audio/AudioManager';
import { UIManager } from './ui/UIManager';
import { InputHandler } from './input/InputHandler';
import { Isometric } from './core/Isometric';
import { DataLoader } from './ui/DataLoader';
import { MainMenu, GameLaunchConfig } from './ui/MainMenu';

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

  private mainMenu: MainMenu;
  private isGameRunning = false;

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
    this.setupHUDControls();

    // 1. Data Loader downloads and decompresses assets first
    new DataLoader(this.audio, () => {
      // 2. Transition into authentic Main Menu
      this.mainMenu.show();
    });

    // 3. Main Menu allows Single Player, Campaign, Skirmish setup
    this.mainMenu = new MainMenu(this.audio, (cfg) => {
      this.startGame(cfg);
    });

    requestAnimationFrame((t) => this.gameLoop(t));
  }

  private setupHUDControls() {
    // Hide game HUD initially until launched from Main Menu
    const resBar = document.getElementById('resource-bar');
    const hudPanel = document.getElementById('hud-panel');
    if (resBar) resBar.style.display = 'none';
    if (hudPanel) hudPanel.style.display = 'none';

    document.getElementById('btn-in-game-menu')?.addEventListener('click', () => {
      this.pauseAndShowMenu();
    });
  }

  private startGame(cfg: GameLaunchConfig) {
    this.isGameRunning = true;

    // Show HUD
    const resBar = document.getElementById('resource-bar');
    const hudPanel = document.getElementById('hud-panel');
    if (resBar) resBar.style.display = 'flex';
    if (hudPanel) hudPanel.style.display = 'flex';

    // Center camera on Player 1 base
    const startIso = Isometric.worldToScreen(12, 12, 0, 0, 0, 1.0);
    this.input.cameraX = this.canvas.width / 2 - startIso.x;
    this.input.cameraY = this.canvas.height / 2 - startIso.y;

    const civName = cfg.civ === 'franks' ? 'Franks' : 'Britons';
    this.ui.showBanner(`Age of Empires II • ${civName} command ready (${cfg.mapType.toUpperCase()})`);
  }

  private pauseAndShowMenu() {
    this.isGameRunning = false;
    const resBar = document.getElementById('resource-bar');
    const hudPanel = document.getElementById('hud-panel');
    if (resBar) resBar.style.display = 'none';
    if (hudPanel) hudPanel.style.display = 'none';

    this.mainMenu.show();
  }

  private resizeCanvas() {
    this.canvas.width = window.innerWidth;
    this.canvas.height = window.innerHeight;
  }

  private setupMinimapInteraction(canvas: HTMLCanvasElement) {
    let isMinimapDown = false;

    const navigate = (e: MouseEvent) => {
      if (!this.isGameRunning) return;
      const rect = canvas.getBoundingClientRect();
      const mx = (e.clientX - rect.left) * (canvas.width / rect.width);
      const my = (e.clientY - rect.top) * (canvas.height / rect.height);
      const worldTarget = this.minimap.minimapToWorld(mx, my);

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

    if (this.isGameRunning) {
      // Fixed-timestep simulation ticks (30 Hz)
      while (this.accumulator >= this.tickInterval) {
        this.state.tick();
        this.accumulator -= this.tickInterval;
      }

      // Input & Camera Pan Update
      this.input.update();

      // Render Viewport
      this.render();

      // Update DOM UI HUD
      this.ui.update(this.state, this.currentFps);
    }

    // FPS Counter
    this.frameCount++;
    if (time - this.lastFpsTime >= 1000) {
      this.currentFps = (this.frameCount * 1000) / (time - this.lastFpsTime);
      this.frameCount = 0;
      this.lastFpsTime = time;
    }

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
