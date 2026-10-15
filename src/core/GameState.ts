import {
  MAP_SIZE,
  TerrainType,
  UnitType,
  BuildingType,
  UnitState,
  ResourceType,
  UNIT_CONFIGS,
  BUILDING_CONFIGS,
  Cost,
  TICKS_PER_SECOND
} from './Constants';
import { Point, Isometric } from './Isometric';
import { Pathfinding } from './Pathfinding';

export interface Unit {
  id: number;
  type: UnitType;
  player: number;
  x: number;
  y: number;
  direction: number;
  hp: number;
  maxHp: number;
  speed: number;
  attack: number;
  range: number;
  state: UnitState;
  path: Point[];
  targetUnitId: number | null;
  targetBuildingId: number | null;
  targetResource: { x: number; y: number; type: ResourceType } | null;
  carrying: { type: ResourceType; amount: number } | null;
  attackCooldown: number;
  animFrame: number;
  animTimer: number;
}

export interface Building {
  id: number;
  type: BuildingType;
  player: number;
  x: number;
  y: number;
  sizeX: number;
  sizeY: number;
  hp: number;
  maxHp: number;
  isConstructed: boolean;
  buildProgress: number; // 0..100
  productionQueue: UnitType[];
  productionTimer: number;
  rallyPoint: Point | null;
}

export interface ResourceNode {
  type: ResourceType;
  x: number;
  y: number;
  amount: number;
}

export interface PlayerState {
  id: number;
  name: string;
  resources: Cost;
  pop: number;
  maxPop: number;
}

export class GameState {
  map: TerrainType[][];
  elevations: number[][];
  resources: Map<number, ResourceNode>; // key: y * MAP_SIZE + x
  units: Map<number, Unit>;
  buildings: Map<number, Building>;
  players: Map<number, PlayerState>;

  explored: boolean[][];
  visible: boolean[][];

  nextEntityId = 1;
  tickCount = 0;

  // Selected entities by player 1
  selectedUnitIds: Set<number> = new Set();
  selectedBuildingId: number | null = null;

  // AI strategy timers
  private aiActionTimer = 0;

  constructor() {
    this.map = [];
    this.elevations = [];
    this.resources = new Map();
    this.units = new Map();
    this.buildings = new Map();
    this.players = new Map();
    this.explored = [];
    this.visible = [];

    this.initMap();
    this.initPlayers();
    this.initEntities();
    this.updateVision();
  }

  private initMap() {
    for (let y = 0; y < MAP_SIZE; y++) {
      this.map[y] = [];
      this.elevations[y] = [];
      this.explored[y] = [];
      this.visible[y] = [];
      for (let x = 0; x < MAP_SIZE; x++) {
        // Default grass
        this.map[y][x] = TerrainType.GRASS;
        this.elevations[y][x] = 0;
        this.explored[y][x] = false;
        this.visible[y][x] = false;

        // Subtle elevation rolling hills in center
        const distFromCenter = Math.hypot(x - MAP_SIZE / 2, y - MAP_SIZE / 2);
        if (distFromCenter < 12 && (x + y) % 5 === 0) {
          this.elevations[y][x] = 1;
        }

        // River / Water on bottom edge
        if (x + y > MAP_SIZE * 1.8) {
          this.map[y][x] = TerrainType.WATER;
        }
      }
    }

    // Place Forests (Tree clumps)
    const forestSeeds = [
      { x: 8, y: 12 }, { x: 18, y: 6 }, { x: 38, y: 32 }, { x: 28, y: 40 },
      { x: 6, y: 38 }, { x: 40, y: 8 }, { x: 24, y: 24 }
    ];
    for (const seed of forestSeeds) {
      for (let ox = -3; ox <= 3; ox++) {
        for (let oy = -3; oy <= 3; oy++) {
          if (Math.hypot(ox, oy) <= 3.2 && Math.random() > 0.15) {
            const fx = seed.x + ox;
            const fy = seed.y + oy;
            if (this.isValidTile(fx, fy) && this.map[fy][fx] === TerrainType.GRASS) {
              this.map[fy][fx] = TerrainType.FOREST;
              this.addResourceNode(fx, fy, ResourceType.WOOD, 100);
            }
          }
        }
      }
    }

    // Place Gold & Stone Nodes
    const goldPiles = [
      { x: 14, y: 16 }, { x: 15, y: 16 }, { x: 14, y: 17 },
      { x: 34, y: 34 }, { x: 35, y: 34 }, { x: 34, y: 35 },
      { x: 22, y: 10 }, { x: 23, y: 10 }
    ];
    for (const g of goldPiles) {
      if (this.isValidTile(g.x, g.y)) {
        this.map[g.y][g.x] = TerrainType.GOLD;
        this.addResourceNode(g.x, g.y, ResourceType.GOLD, 800);
      }
    }

    const stonePiles = [
      { x: 10, y: 22 }, { x: 11, y: 22 },
      { x: 38, y: 26 }, { x: 39, y: 26 }
    ];
    for (const s of stonePiles) {
      if (this.isValidTile(s.x, s.y)) {
        this.map[s.y][s.x] = TerrainType.STONE;
        this.addResourceNode(s.x, s.y, ResourceType.STONE, 350);
      }
    }
  }

  private addResourceNode(x: number, y: number, type: ResourceType, amount: number) {
    this.resources.set(y * MAP_SIZE + x, { type, x, y, amount });
  }

  private initPlayers() {
    this.players.set(1, {
      id: 1,
      name: 'Player 1',
      resources: { food: 200, wood: 200, gold: 100, stone: 150 },
      pop: 4,
      maxPop: 10,
    });

    this.players.set(2, {
      id: 2,
      name: 'Player 2 (AI)',
      resources: { food: 200, wood: 200, gold: 100, stone: 150 },
      pop: 4,
      maxPop: 10,
    });
  }

  private initEntities() {
    // Player 1 Base (Top Left quadrant: (12, 12))
    this.spawnBuilding(BuildingType.TOWN_CENTER, 1, 10, 10, true);
    this.spawnBuilding(BuildingType.HOUSE, 1, 8, 16, true);
    this.spawnUnit(UnitType.VILLAGER, 1, 15, 12);
    this.spawnUnit(UnitType.VILLAGER, 1, 15, 14);
    this.spawnUnit(UnitType.VILLAGER, 1, 13, 15);
    this.spawnUnit(UnitType.SCOUT, 1, 16, 16);

    // Player 2 Base (Bottom Right quadrant: (36, 34))
    this.spawnBuilding(BuildingType.TOWN_CENTER, 2, 34, 32, true);
    this.spawnBuilding(BuildingType.HOUSE, 2, 32, 38, true);
    this.spawnBuilding(BuildingType.BARRACKS, 2, 38, 30, true);
    this.spawnUnit(UnitType.VILLAGER, 2, 33, 30);
    this.spawnUnit(UnitType.VILLAGER, 2, 32, 32);
    this.spawnUnit(UnitType.VILLAGER, 2, 30, 34);
    this.spawnUnit(UnitType.SCOUT, 2, 30, 30);
  }

  public spawnUnit(type: UnitType, player: number, x: number, y: number): Unit {
    const cfg = UNIT_CONFIGS[type];
    const unit: Unit = {
      id: this.nextEntityId++,
      type,
      player,
      x,
      y,
      direction: 4, // South
      hp: cfg.hp,
      maxHp: cfg.hp,
      speed: cfg.speed,
      attack: cfg.attack,
      range: cfg.range,
      state: UnitState.IDLE,
      path: [],
      targetUnitId: null,
      targetBuildingId: null,
      targetResource: null,
      carrying: null,
      attackCooldown: 0,
      animFrame: 0,
      animTimer: 0,
    };
    this.units.set(unit.id, unit);
    this.updatePlayerPop(player);
    return unit;
  }

  public spawnBuilding(
    type: BuildingType,
    player: number,
    x: number,
    y: number,
    isCompleted = false
  ): Building {
    const cfg = BUILDING_CONFIGS[type];
    const building: Building = {
      id: this.nextEntityId++,
      type,
      player,
      x,
      y,
      sizeX: cfg.sizeX,
      sizeY: cfg.sizeY,
      hp: isCompleted ? cfg.hp : Math.floor(cfg.hp * 0.1),
      maxHp: cfg.hp,
      isConstructed: isCompleted,
      buildProgress: isCompleted ? 100 : 10,
      productionQueue: [],
      productionTimer: 0,
      rallyPoint: { x: x + cfg.sizeX / 2, y: y + cfg.sizeY + 1 },
    };
    this.buildings.set(building.id, building);
    this.updatePlayerPop(player);
    return building;
  }

  public updatePlayerPop(playerId: number) {
    const player = this.players.get(playerId);
    if (!player) return;

    let pop = 0;
    for (const u of this.units.values()) {
      if (u.player === playerId && u.state !== UnitState.DEAD) pop++;
    }

    let maxPop = 5; // Base pop
    for (const b of this.buildings.values()) {
      if (b.player === playerId && b.isConstructed) {
        maxPop += BUILDING_CONFIGS[b.type].providesPop;
      }
    }

    player.pop = pop;
    player.maxPop = Math.min(200, maxPop);
  }

  public isValidTile(x: number, y: number): boolean {
    return x >= 0 && x < MAP_SIZE && y >= 0 && y < MAP_SIZE;
  }

  public isTileWalkable(x: number, y: number): boolean {
    if (!this.isValidTile(x, y)) return false;
    const t = this.map[y][x];
    if (t === TerrainType.WATER || t === TerrainType.FOREST || t === TerrainType.GOLD || t === TerrainType.STONE) {
      return false;
    }

    // Check buildings footprint
    for (const b of this.buildings.values()) {
      if (x >= b.x && x < b.x + b.sizeX && y >= b.y && y < b.y + b.sizeY) {
        return false;
      }
    }
    return true;
  }

  /**
   * Main deterministic simulation tick (30 times/sec).
   */
  public tick() {
    this.tickCount++;

    // 1. Update Unit States & Movements
    for (const unit of this.units.values()) {
      this.updateUnit(unit);
    }

    // 2. Update Building Queues & Production
    for (const building of this.buildings.values()) {
      this.updateBuilding(building);
    }

    // 3. AI Opponent Logic (Player 2)
    this.aiActionTimer++;
    if (this.aiActionTimer >= 45) { // Every 1.5 seconds
      this.aiActionTimer = 0;
      this.updateAI();
    }

    // 4. Update Fog of War visibility every few ticks
    if (this.tickCount % 4 === 0) {
      this.updateVision();
    }
  }

  private updateUnit(unit: Unit) {
    if (unit.state === UnitState.DEAD) return;

    if (unit.attackCooldown > 0) unit.attackCooldown--;

    // Movement along path
    if (unit.path.length > 0) {
      unit.state = UnitState.MOVING;
      const target = unit.path[0];
      const dx = target.x - unit.x;
      const dy = target.y - unit.y;
      const dist = Math.hypot(dx, dy);

      unit.direction = Isometric.getDirection(dx, dy);

      const step = unit.speed / 15; // speed per tick
      if (dist <= step) {
        unit.x = target.x;
        unit.y = target.y;
        unit.path.shift();
        if (unit.path.length === 0 && unit.state === UnitState.MOVING) {
          unit.state = UnitState.IDLE;
        }
      } else {
        unit.x += (dx / dist) * step;
        unit.y += (dy / dist) * step;
      }

      // Bobbing animation timer
      unit.animTimer++;
      if (unit.animTimer >= 5) {
        unit.animTimer = 0;
        unit.animFrame = (unit.animFrame + 1) % 10;
      }
      return;
    }

    // Attacking target unit
    if (unit.targetUnitId !== null) {
      const enemy = this.units.get(unit.targetUnitId);
      if (!enemy || enemy.state === UnitState.DEAD) {
        unit.targetUnitId = null;
        unit.state = UnitState.IDLE;
        return;
      }

      const dist = Math.hypot(enemy.x - unit.x, enemy.y - unit.y);
      if (dist > unit.range + 0.5) {
        // Re-path towards target
        unit.path = Pathfinding.findPath(unit.x, unit.y, enemy.x, enemy.y, (x, y) => this.isTileWalkable(x, y));
      } else {
        // In range: Attack!
        unit.state = UnitState.ATTACKING;
        unit.direction = Isometric.getDirection(enemy.x - unit.x, enemy.y - unit.y);
        if (unit.attackCooldown <= 0) {
          unit.attackCooldown = 25; // 25 ticks cooldown
          enemy.hp -= unit.attack;
          if (enemy.hp <= 0) {
            enemy.hp = 0;
            enemy.state = UnitState.DEAD;
            this.updatePlayerPop(enemy.player);
            unit.targetUnitId = null;
            unit.state = UnitState.IDLE;
          }
        }
      }
      return;
    }

    // Attacking target building
    if (unit.targetBuildingId !== null) {
      const bldg = this.buildings.get(unit.targetBuildingId);
      if (!bldg || bldg.hp <= 0) {
        unit.targetBuildingId = null;
        unit.state = UnitState.IDLE;
        return;
      }

      const bCenterX = bldg.x + bldg.sizeX / 2;
      const bCenterY = bldg.y + bldg.sizeY / 2;
      const dist = Math.hypot(bCenterX - unit.x, bCenterY - unit.y);
      if (dist > unit.range + bldg.sizeX / 2 + 0.5) {
        unit.path = Pathfinding.findPath(unit.x, unit.y, bCenterX, bCenterY, (x, y) => this.isTileWalkable(x, y));
      } else {
        unit.state = UnitState.ATTACKING;
        unit.direction = Isometric.getDirection(bCenterX - unit.x, bCenterY - unit.y);
        if (unit.attackCooldown <= 0) {
          unit.attackCooldown = 30;
          bldg.hp -= unit.attack;
          if (bldg.hp <= 0) {
            this.buildings.delete(bldg.id);
            this.updatePlayerPop(bldg.player);
            unit.targetBuildingId = null;
            unit.state = UnitState.IDLE;
          }
        }
      }
      return;
    }

    // Gathering Resources
    if (unit.targetResource !== null) {
      const resKey = unit.targetResource.y * MAP_SIZE + unit.targetResource.x;
      const resNode = this.resources.get(resKey);
      if (!resNode || resNode.amount <= 0) {
        unit.targetResource = null;
        unit.state = UnitState.IDLE;
        return;
      }

      const dist = Math.hypot(resNode.x + 0.5 - unit.x, resNode.y + 0.5 - unit.y);
      if (dist > 1.3) {
        unit.path = Pathfinding.findPath(unit.x, unit.y, resNode.x, resNode.y, (x, y) => this.isTileWalkable(x, y));
      } else {
        // At resource: Gather!
        unit.state = UnitState.GATHERING;
        unit.direction = Isometric.getDirection(resNode.x - unit.x, resNode.y - unit.y);
        unit.animTimer++;
        if (unit.animTimer >= 20) {
          unit.animTimer = 0;
          if (!unit.carrying) {
            unit.carrying = { type: resNode.type, amount: 0 };
          }
          unit.carrying.amount += 1;
          resNode.amount -= 1;

          // Max carry capacity is 10
          if (unit.carrying.amount >= 10) {
            this.sendVillagerToDeposit(unit);
          }
        }
      }
      return;
    }
  }

  private sendVillagerToDeposit(unit: Unit) {
    // Find closest Town Center
    let bestDist = Infinity;
    let closestTc: Building | null = null;
    for (const b of this.buildings.values()) {
      if (b.player === unit.player && b.type === BuildingType.TOWN_CENTER && b.isConstructed) {
        const d = Math.hypot(b.x + b.sizeX / 2 - unit.x, b.y + b.sizeY / 2 - unit.y);
        if (d < bestDist) {
          bestDist = d;
          closestTc = b;
        }
      }
    }

    if (closestTc) {
      const tcX = closestTc.x + closestTc.sizeX / 2;
      const tcY = closestTc.y + closestTc.sizeY / 2;
      unit.path = Pathfinding.findPath(unit.x, unit.y, tcX, tcY, (x, y) => this.isTileWalkable(x, y));
      // Save returning target
      const returnRes = unit.targetResource;
      const checkDeposit = () => {
        if (unit.carrying && Math.hypot(tcX - unit.x, tcY - unit.y) < 2.5) {
          const player = this.players.get(unit.player);
          if (player) {
            player.resources[unit.carrying.type] += unit.carrying.amount;
          }
          unit.carrying = null;
          // Go back to gathering
          if (returnRes) {
            unit.targetResource = returnRes;
          }
        }
      };
      // Check in future ticks
      setTimeout(checkDeposit, 1500);
    }
  }

  private updateBuilding(building: Building) {
    if (!building.isConstructed) {
      building.buildProgress += 0.5;
      building.hp = Math.min(building.maxHp, Math.floor((building.buildProgress / 100) * building.maxHp));
      if (building.buildProgress >= 100) {
        building.isConstructed = true;
        this.updatePlayerPop(building.player);
      }
      return;
    }

    // Production Queue
    if (building.productionQueue.length > 0) {
      building.productionTimer++;
      const currentUnitType = building.productionQueue[0];
      const trainDuration = UNIT_CONFIGS[currentUnitType].trainTime * TICKS_PER_SECOND;

      if (building.productionTimer >= trainDuration) {
        building.productionTimer = 0;
        building.productionQueue.shift();

        // Spawn unit at rally point or adjacent walkable tile
        const spawnX = building.x + building.sizeX / 2;
        const spawnY = building.y + building.sizeY + 0.5;
        const unit = this.spawnUnit(currentUnitType, building.player, spawnX, spawnY);
        if (building.rallyPoint) {
          unit.path = Pathfinding.findPath(spawnX, spawnY, building.rallyPoint.x, building.rallyPoint.y, (x, y) => this.isTileWalkable(x, y));
        }
      }
    }
  }

  private updateAI() {
    const ai = this.players.get(2);
    if (!ai) return;

    // 1. Train villagers if food available and pop limit not reached
    const tc = Array.from(this.buildings.values()).find(b => b.player === 2 && b.type === BuildingType.TOWN_CENTER);
    if (tc && tc.productionQueue.length < 2 && ai.resources.food >= 50 && ai.pop < ai.maxPop) {
      ai.resources.food -= 50;
      tc.productionQueue.push(UnitType.VILLAGER);
    }

    // 2. Put idle villagers to work
    for (const unit of this.units.values()) {
      if (unit.player === 2 && unit.type === UnitType.VILLAGER && unit.state === UnitState.IDLE) {
        // Find nearest tree or gold
        let nearestRes: ResourceNode | null = null;
        let minDist = Infinity;
        for (const res of this.resources.values()) {
          const d = Math.hypot(res.x - unit.x, res.y - unit.y);
          if (d < minDist && res.amount > 0) {
            minDist = d;
            nearestRes = res;
          }
        }
        if (nearestRes) {
          unit.targetResource = { x: nearestRes.x, y: nearestRes.y, type: nearestRes.type };
          unit.path = Pathfinding.findPath(unit.x, unit.y, nearestRes.x, nearestRes.y, (x, y) => this.isTileWalkable(x, y));
        }
      }
    }

    // 3. Train Militia at Barracks
    const barracks = Array.from(this.buildings.values()).find(b => b.player === 2 && b.type === BuildingType.BARRACKS);
    if (barracks && barracks.productionQueue.length < 2 && ai.resources.food >= 60 && ai.resources.gold >= 20 && ai.pop < ai.maxPop) {
      ai.resources.food -= 60;
      ai.resources.gold -= 20;
      barracks.productionQueue.push(UnitType.MILITIA);
    }

    // 4. Attack when army >= 3
    const military = Array.from(this.units.values()).filter(u => u.player === 2 && (u.type === UnitType.MILITIA || u.type === UnitType.SCOUT));
    if (military.length >= 3) {
      const p1Tc = Array.from(this.buildings.values()).find(b => b.player === 1 && b.type === BuildingType.TOWN_CENTER);
      const targetPoint = p1Tc ? { x: p1Tc.x + 2, y: p1Tc.y + 2 } : { x: 12, y: 12 };
      for (const m of military) {
        if (m.state === UnitState.IDLE) {
          m.path = Pathfinding.findPath(m.x, m.y, targetPoint.x, targetPoint.y, (x, y) => this.isTileWalkable(x, y));
        }
      }
    }
  }

  private updateVision() {
    // Reset visible array
    for (let y = 0; y < MAP_SIZE; y++) {
      for (let x = 0; x < MAP_SIZE; x++) {
        this.visible[y][x] = false;
      }
    }

    // Player 1 line of sight
    const sightSources: { x: number; y: number; radius: number }[] = [];
    for (const u of this.units.values()) {
      if (u.player === 1 && u.state !== UnitState.DEAD) {
        sightSources.push({ x: u.x, y: u.y, radius: 5 });
      }
    }
    for (const b of this.buildings.values()) {
      if (b.player === 1) {
        sightSources.push({ x: b.x + b.sizeX / 2, y: b.y + b.sizeY / 2, radius: 7 });
      }
    }

    for (const src of sightSources) {
      const r = src.radius;
      const minX = Math.max(0, Math.floor(src.x - r));
      const maxX = Math.min(MAP_SIZE - 1, Math.ceil(src.x + r));
      const minY = Math.max(0, Math.floor(src.y - r));
      const maxY = Math.min(MAP_SIZE - 1, Math.ceil(src.y + r));

      for (let y = minY; y <= maxY; y++) {
        for (let x = minX; x <= maxX; x++) {
          if (Math.hypot(x - src.x, y - src.y) <= r) {
            this.explored[y][x] = true;
            this.visible[y][x] = true;
          }
        }
      }
    }
  }
}
