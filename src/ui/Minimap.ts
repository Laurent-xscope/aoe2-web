import { MAP_SIZE, TerrainType } from '../core/Constants';
import { GameState } from '../core/GameState';
import { Point } from '../core/Isometric';

export class Minimap {
  private ctx: CanvasRenderingContext2D;
  private width: number;
  private height: number;

  constructor(canvas: HTMLCanvasElement) {
    this.ctx = canvas.getContext('2d')!;
    this.width = canvas.width;
    this.height = canvas.height;
  }

  public render(
    state: GameState,
    cameraX: number,
    cameraY: number,
    zoom: number,
    viewWidth: number,
    viewHeight: number
  ) {
    const ctx = this.ctx;
    ctx.clearRect(0, 0, this.width, this.height);

    // Background
    ctx.fillStyle = '#060a08';
    ctx.fillRect(0, 0, this.width, this.height);

    const centerX = this.width / 2;
    const tileW = (this.width * 0.9) / MAP_SIZE;
    const tileH = (this.height * 0.8) / MAP_SIZE;

    // 1. Draw Explored Terrain
    for (let y = 0; y < MAP_SIZE; y++) {
      for (let x = 0; x < MAP_SIZE; x++) {
        if (!state.explored[y][x]) continue;

        const mx = centerX + (x - y) * (tileW / 2);
        const my = 15 + (x + y) * (tileH / 2);

        const t = state.map[y][x];
        ctx.fillStyle = this.getMinimapColor(t, state.visible[y][x]);
        ctx.fillRect(mx - 1, my - 1, 2.5, 2.5);
      }
    }

    // 2. Draw Buildings
    for (const bldg of state.buildings.values()) {
      if (!state.explored[bldg.y][bldg.x]) continue;
      const bx = centerX + (bldg.x - bldg.y) * (tileW / 2);
      const by = 15 + (bldg.x + bldg.y) * (tileH / 2);

      ctx.fillStyle = bldg.player === 1 ? '#00a8ff' : '#e84118';
      ctx.fillRect(bx - 2, by - 2, 4, 4);
    }

    // 3. Draw Units
    for (const unit of state.units.values()) {
      const uX = Math.floor(unit.x);
      const uY = Math.floor(unit.y);
      if (unit.player !== 1 && (!state.isValidTile(uX, uY) || !state.visible[uY][uX])) continue;

      const ux = centerX + (unit.x - unit.y) * (tileW / 2);
      const uy = 15 + (unit.x + unit.y) * (tileH / 2);

      ctx.fillStyle = unit.player === 1 ? '#2ecc71' : '#ff4757';
      ctx.fillRect(ux - 1.5, uy - 1.5, 3, 3);
    }

    // 4. Draw Camera Viewport Box
    this.renderCameraBox(ctx, centerX, tileW, tileH, cameraX, cameraY, zoom, viewWidth, viewHeight);
  }

  private renderCameraBox(
    ctx: CanvasRenderingContext2D,
    centerX: number,
    tileW: number,
    tileH: number,
    camX: number,
    camY: number,
    zoom: number,
    w: number,
    h: number
  ) {
    // Top-left, top-right, bottom-right, bottom-left screen points into world grid
    const corners = [
      { sx: 0, sy: 0 },
      { sx: w, sy: 0 },
      { sx: w, sy: h },
      { sx: 0, sy: h },
    ];

    ctx.strokeStyle = '#f1c40f';
    ctx.lineWidth = 1.2;
    ctx.beginPath();

    corners.forEach((pt, i) => {
      // Inverse screen to world
      const isoX = (pt.sx - camX) / zoom;
      const isoY = (pt.sy - camY) / zoom;
      const wx = (isoX / 48 + isoY / 24) / 2;
      const wy = (isoY / 24 - isoX / 48) / 2;

      const mx = centerX + (wx - wy) * (tileW / 2);
      const my = 15 + (wx + wy) * (tileH / 2);

      if (i === 0) ctx.moveTo(mx, my);
      else ctx.lineTo(mx, my);
    });

    ctx.closePath();
    ctx.stroke();
  }

  private getMinimapColor(t: TerrainType, isVisible: boolean): string {
    switch (t) {
      case TerrainType.GRASS:
        return isVisible ? '#27ae60' : '#1b7943';
      case TerrainType.WATER:
        return isVisible ? '#2980b9' : '#1a5276';
      case TerrainType.DIRT:
      case TerrainType.DESERT:
        return isVisible ? '#d35400' : '#933c00';
      case TerrainType.FOREST:
        return isVisible ? '#196f3d' : '#114a29';
      case TerrainType.GOLD:
        return '#f1c40f';
      case TerrainType.STONE:
        return '#bdc3c7';
      default:
        return '#27ae60';
    }
  }

  /**
   * Converts minimap click (pixel X, Y) back to world map tile (x, y).
   */
  public minimapToWorld(mx: number, my: number): Point {
    const centerX = this.width / 2;
    const tileW = (this.width * 0.9) / MAP_SIZE;
    const tileH = (this.height * 0.8) / MAP_SIZE;

    const relX = mx - centerX;
    const relY = my - 15;

    const diff = relX / (tileW / 2);
    const sum = relY / (tileH / 2);

    const x = (sum + diff) / 2;
    const y = (sum - diff) / 2;

    return {
      x: Math.max(0, Math.min(MAP_SIZE - 1, x)),
      y: Math.max(0, Math.min(MAP_SIZE - 1, y)),
    };
  }
}
