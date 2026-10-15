import { TILE_WIDTH, TILE_HEIGHT, ELEV_HEIGHT } from './Constants';

export interface Point {
  x: number;
  y: number;
}

export class Isometric {
  static readonly HALF_W = TILE_WIDTH / 2;
  static readonly HALF_H = TILE_HEIGHT / 2;

  /**
   * Converts grid/world coordinates (float x, y) and elevation to 2D screen pixels.
   */
  static worldToScreen(
    worldX: number,
    worldY: number,
    elevation: number,
    cameraX: number,
    cameraY: number,
    zoom: number
  ): Point {
    const isoX = (worldX - worldY) * this.HALF_W;
    const isoY = (worldX + worldY) * this.HALF_H - elevation * ELEV_HEIGHT;
    return {
      x: isoX * zoom + cameraX,
      y: isoY * zoom + cameraY,
    };
  }

  /**
   * Converts screen pixel (mouse pointer) back to world map grid coordinate (float x, y).
   */
  static screenToWorld(
    screenX: number,
    screenY: number,
    cameraX: number,
    cameraY: number,
    zoom: number
  ): Point {
    const isoX = (screenX - cameraX) / zoom;
    const isoY = (screenY - cameraY) / zoom;

    const x = (isoX / this.HALF_W + isoY / this.HALF_H) / 2;
    const y = (isoY / this.HALF_H - isoX / this.HALF_W) / 2;

    return { x, y };
  }

  /**
   * Returns the 4 vertices of an isometric diamond tile on screen.
   */
  static getTileDiamond(
    tileX: number,
    tileY: number,
    elev: number,
    cameraX: number,
    cameraY: number,
    zoom: number
  ): Point[] {
    // 4 corners of tile (x, y) to (x+1, y+1)
    const top = this.worldToScreen(tileX, tileY, elev, cameraX, cameraY, zoom);
    const right = this.worldToScreen(tileX + 1, tileY, elev, cameraX, cameraY, zoom);
    const bottom = this.worldToScreen(tileX + 1, tileY + 1, elev, cameraX, cameraY, zoom);
    const left = this.worldToScreen(tileX, tileY + 1, elev, cameraX, cameraY, zoom);

    return [top, right, bottom, left];
  }

  /**
   * Calculates angle direction index (0..7: N, NE, E, SE, S, SW, W, NW) from delta vector.
   */
  static getDirection(dx: number, dy: number): number {
    // Convert world dx, dy to isometric angle
    // In AoE2: 0 is North (dx < 0, dy < 0 in screen or world terms)
    const angle = Math.atan2(dy, dx); // radians -PI to PI
    // Normalize to 0..7
    // 0 = East, 1 = South-East, 2 = South, etc.
    let octant = Math.round((angle * 8) / (2 * Math.PI));
    if (octant < 0) octant += 8;
    return (octant + 2) % 8; // Adjust to standard AoE2 cardinal orientation
  }
}
