import { MAP_SIZE } from './Constants';
import { Point } from './Isometric';

export interface GridNode {
  x: number;
  y: number;
  g: number;
  h: number;
  f: number;
  parent: GridNode | null;
}

export class Pathfinding {
  private static D1 = 1.0;
  private static D2 = 1.41421356;

  /**
   * Finds shortest path using 8-directional A* algorithm.
   */
  static findPath(
    startX: number,
    startY: number,
    targetX: number,
    targetY: number,
    isWalkable: (x: number, y: number) => boolean,
    maxSearchNodes = 1200
  ): Point[] {
    const sX = Math.floor(startX);
    const sY = Math.floor(startY);
    let tX = Math.floor(targetX);
    let tY = Math.floor(targetY);

    if (sX === tX && sY === tY) return [];

    // If target itself is not walkable (e.g. clicking directly on a tree or building),
    // find the closest adjacent walkable tile!
    if (!isWalkable(tX, tY)) {
      const neighbors = this.getAdjacentWalkable(tX, tY, isWalkable);
      if (neighbors.length === 0) return [];
      // Pick closest neighbor to start
      neighbors.sort((a, b) => this.heuristic(sX, sY, a.x, a.y) - this.heuristic(sX, sY, b.x, b.y));
      tX = neighbors[0].x;
      tY = neighbors[0].y;
    }

    const openList: GridNode[] = [];
    const openMap = new Map<number, GridNode>();
    const closedSet = new Set<number>();

    const startNode: GridNode = {
      x: sX,
      y: sY,
      g: 0,
      h: this.heuristic(sX, sY, tX, tY),
      f: this.heuristic(sX, sY, tX, tY),
      parent: null,
    };

    openList.push(startNode);
    openMap.set(sX * MAP_SIZE + sY, startNode);

    let nodesEvaluated = 0;

    while (openList.length > 0 && nodesEvaluated < maxSearchNodes) {
      nodesEvaluated++;

      // Pop lowest f node
      let lowestIdx = 0;
      for (let i = 1; i < openList.length; i++) {
        if (openList[i].f < openList[lowestIdx].f) {
          lowestIdx = i;
        }
      }
      const current = openList.splice(lowestIdx, 1)[0];
      const curKey = current.x * MAP_SIZE + current.y;
      openMap.delete(curKey);
      closedSet.add(curKey);

      // Reached target?
      if (current.x === tX && current.y === tY) {
        return this.reconstructPath(current);
      }

      // Check 8 neighbors
      for (let dx = -1; dx <= 1; dx++) {
        for (let dy = -1; dy <= 1; dy++) {
          if (dx === 0 && dy === 0) continue;

          const nx = current.x + dx;
          const ny = current.y + dy;

          if (nx < 0 || nx >= MAP_SIZE || ny < 0 || ny >= MAP_SIZE) continue;

          const nKey = nx * MAP_SIZE + ny;
          if (closedSet.has(nKey)) continue;

          // Check if walkable
          if (!isWalkable(nx, ny)) continue;

          // Prevent cutting corners through diagonal walls
          if (dx !== 0 && dy !== 0) {
            if (!isWalkable(current.x + dx, current.y) || !isWalkable(current.x, current.y + dy)) {
              continue;
            }
          }

          const moveCost = (dx !== 0 && dy !== 0) ? this.D2 : this.D1;
          const gScore = current.g + moveCost;

          let neighbor = openMap.get(nKey);
          if (!neighbor) {
            const h = this.heuristic(nx, ny, tX, tY);
            neighbor = {
              x: nx,
              y: ny,
              g: gScore,
              h: h,
              f: gScore + h,
              parent: current,
            };
            openList.push(neighbor);
            openMap.set(nKey, neighbor);
          } else if (gScore < neighbor.g) {
            neighbor.g = gScore;
            neighbor.f = gScore + neighbor.h;
            neighbor.parent = current;
          }
        }
      }
    }

    return [];
  }

  private static heuristic(x1: number, y1: number, x2: number, y2: number): number {
    const dx = Math.abs(x1 - x2);
    const dy = Math.abs(y1 - y2);
    return this.D1 * (dx + dy) + (this.D2 - 2 * this.D1) * Math.min(dx, dy);
  }

  private static reconstructPath(node: GridNode): Point[] {
    const path: Point[] = [];
    let curr: GridNode | null = node;
    while (curr) {
      // Return tile center (x + 0.5, y + 0.5)
      path.push({ x: curr.x + 0.5, y: curr.y + 0.5 });
      curr = curr.parent;
    }
    path.reverse();
    // Drop the starting tile center
    if (path.length > 1) {
      path.shift();
    }
    return path;
  }

  private static getAdjacentWalkable(
    x: number,
    y: number,
    isWalkable: (x: number, y: number) => boolean
  ): Point[] {
    const res: Point[] = [];
    const dirs = [
      { x: 1, y: 0 }, { x: -1, y: 0 }, { x: 0, y: 1 }, { x: 0, y: -1 },
      { x: 1, y: 1 }, { x: -1, y: -1 }, { x: 1, y: -1 }, { x: -1, y: 1 }
    ];
    for (const d of dirs) {
      const nx = x + d.x;
      const ny = y + d.y;
      if (nx >= 0 && nx < MAP_SIZE && ny >= 0 && ny < MAP_SIZE && isWalkable(nx, ny)) {
        res.push({ x: nx, y: ny });
      }
    }
    return res;
  }
}
