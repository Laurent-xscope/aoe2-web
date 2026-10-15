import { MAP_SIZE, TerrainType, ELEV_HEIGHT } from '../core/Constants';
import { GameState } from '../core/GameState';
import { Isometric, Point } from '../core/Isometric';

export class TerrainRenderer {
  private terrainImages: Map<string, HTMLImageElement> = new Map();

  constructor() {
    this.loadTextures();
  }

  private loadTextures() {
    const textures = ['g_grs_00_color.png', 'g_gr2_00_color.png', 'g_des_00_color.png'];
    for (const name of textures) {
      const img = new Image();
      img.src = `./assets/terrain/${name}`;
      img.onload = () => {
        this.terrainImages.set(name, img);
      };
    }
  }

  public render(
    ctx: CanvasRenderingContext2D,
    state: GameState,
    cameraX: number,
    cameraY: number,
    zoom: number,
    hoveredTile: Point | null
  ) {
    const width = ctx.canvas.width;
    const height = ctx.canvas.height;

    // Determine visible tile bounds to cull tiles off-screen
    // Approximate bounds for 48x48
    for (let y = 0; y < MAP_SIZE; y++) {
      for (let x = 0; x < MAP_SIZE; x++) {
        // Skip rendering if completely unexplored
        if (!state.explored[y][x]) continue;

        const elev = state.elevations[y][x];
        const screenPos = Isometric.worldToScreen(x, y, elev, cameraX, cameraY, zoom);

        // Broad-phase screen viewport culling
        if (
          screenPos.x < -200 ||
          screenPos.x > width + 200 ||
          screenPos.y < -200 ||
          screenPos.y > height + 200
        ) {
          continue;
        }

        const diamond = Isometric.getTileDiamond(x, y, elev, cameraX, cameraY, zoom);

        // 1. Draw Elevation Cliff / Wall if higher than south/east neighbor
        this.renderElevationCliff(ctx, state, x, y, elev, cameraX, cameraY, zoom);

        // 2. Draw Diamond Tile Base
        ctx.beginPath();
        ctx.moveTo(diamond[0].x, diamond[0].y);
        ctx.lineTo(diamond[1].x, diamond[1].y);
        ctx.lineTo(diamond[2].x, diamond[2].y);
        ctx.lineTo(diamond[3].x, diamond[3].y);
        ctx.closePath();

        const tType = state.map[y][x];
        ctx.fillStyle = this.getTerrainColor(tType, x, y);
        ctx.fill();

        // Subtle tile boundary
        ctx.strokeStyle = 'rgba(0, 0, 0, 0.08)';
        ctx.lineWidth = 1;
        ctx.stroke();

        // 3. Highlight hovered tile
        if (hoveredTile && hoveredTile.x === x && hoveredTile.y === y) {
          ctx.strokeStyle = 'rgba(255, 230, 100, 0.7)';
          ctx.lineWidth = 2;
          ctx.stroke();
        }

        // 4. Fog of war shroud (if explored but not currently visible)
        if (!state.visible[y][x]) {
          ctx.fillStyle = 'rgba(10, 15, 25, 0.55)';
          ctx.fill();
        }
      }
    }
  }

  private renderElevationCliff(
    ctx: CanvasRenderingContext2D,
    state: GameState,
    x: number,
    y: number,
    elev: number,
    cameraX: number,
    cameraY: number,
    zoom: number
  ) {
    if (elev <= 0) return;

    // Check east neighbor (x+1, y) and south neighbor (x, y+1)
    const eastElev = x + 1 < MAP_SIZE ? state.elevations[y][x + 1] : 0;
    const southElev = y + 1 < MAP_SIZE ? state.elevations[y + 1][x] : 0;

    if (elev > eastElev || elev > southElev) {
      const topRight = Isometric.worldToScreen(x + 1, y, elev, cameraX, cameraY, zoom);
      const bottom = Isometric.worldToScreen(x + 1, y + 1, elev, cameraX, cameraY, zoom);
      const bottomLeft = Isometric.worldToScreen(x, y + 1, elev, cameraX, cameraY, zoom);

      const cliffDrop = ELEV_HEIGHT * zoom;

      // Draw South-East cliff skirt
      ctx.beginPath();
      ctx.moveTo(bottomLeft.x, bottomLeft.y);
      ctx.lineTo(bottom.x, bottom.y);
      ctx.lineTo(topRight.x, topRight.y);
      ctx.lineTo(topRight.x, topRight.y + cliffDrop);
      ctx.lineTo(bottom.x, bottom.y + cliffDrop);
      ctx.lineTo(bottomLeft.x, bottomLeft.y + cliffDrop);
      ctx.closePath();

      ctx.fillStyle = '#4a3820'; // Rocky cliff brown
      ctx.fill();
      ctx.strokeStyle = '#2d2212';
      ctx.lineWidth = 1;
      ctx.stroke();
    }
  }

  private getTerrainColor(type: TerrainType, x: number, y: number): string {
    // Slight random variation per tile based on coordinates
    const noise = ((x * 17 + y * 31) % 10) - 5;

    switch (type) {
      case TerrainType.GRASS:
        return `rgb(${62 + noise}, ${123 + noise * 2}, ${48 + noise})`;
      case TerrainType.DIRT:
        return `rgb(${140 + noise}, ${106 + noise}, ${67 + noise})`;
      case TerrainType.DESERT:
        return `rgb(${212 + noise}, ${163 + noise}, ${98 + noise})`;
      case TerrainType.WATER:
        return `rgb(${39 + noise}, ${98 + noise}, ${136 + noise})`;
      case TerrainType.FOREST:
        return `rgb(${34 + noise}, ${78 + noise}, ${26 + noise})`;
      case TerrainType.GOLD:
        return `rgb(${200 + noise}, ${150 + noise}, ${40 + noise})`;
      case TerrainType.STONE:
        return `rgb(${110 + noise}, ${115 + noise}, ${120 + noise})`;
      default:
        return '#3e7b30';
    }
  }
}
