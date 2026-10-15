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
import { AudioManager } from '../audio/AudioManager';

export interface Projectile {
  id: number;
  player: number;
  startX: number;
  startY: number;
  startElev: number;
  currentX: number;
  currentY: number;
  currentHeight: number; // Parabolic arc height
  targetX: number;
  targetY: number;
  targetElev: number;
  targetUnitId: number | null;
  targetBuildingId: number | null;
  speed: number;
  damage: number;
  isPierce: boolean;
  progress: number; // 0..1
}

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
  armorMelee: number;
  armorPierce: number;
  range: number;
  state: UnitState;
  path: Point[];
  targetUnitId: number | null;
  targetBuildingId: number | null;
  targetResource: { x: number; y: number; type: ResourceType } | null;
  targetConstructBuildingId: number | null;
  carrying: { type: ResourceType; amount: number } | null;
  attackCooldown: number;
  animFrame: number;
  animTimer: number;

  // Stored state for Town Bell return
  previousTask?: {
    resource?: { x: number; y: number; type: ResourceType };
    buildingId?: number;
  };
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
  armorMelee: number;
  armorPierce: number;
  isConstructed: boolean;
  buildProgress: number; // 0..100
  productionQueue: UnitType[];
  productionTimer: number;
  rallyPoint: Point | null;
  garrisonedUnits: number[]; // Unit IDs inside
  attackCooldown: number;
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
  age: 'Dark Age' | 'Feudal Age';
}

export class GameState {
  map: TerrainType[][];
  elevations: number[][];
  resources: Map<number, ResourceNode>;
  units: Map<number, Unit>;
  buildings: Map<number, Building>;
  players: Map<number, PlayerState>;
  projectiles: Projectile[] = [];

  explored: boolean[][];
  visible: boolean[][];

  nextEntityId = 1;
  tickCount = 0;

  selectedUnitIds: Set<number> = new Set();
  selectedBuildingId: number | null = null;

  // Commodity prices (AoE2 market: 100 goods)
  marketPrices: Record<ResourceType, number> = {
    [ResourceType.WOOD]: 70,
    [ResourceType.FOOD]: 70,
    [ResourceType.STONE]: 130,
    [ResourceType.GOLD]: 100,
  };

  private aiActionTimer = 0;

  constructor(private audio?: AudioManager) {
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
        this.map[y][x] = TerrainType.GRASS;
        this.elevations[y][x] = 0;
        this.explored[y][x] = false;
        this.visible[y][x] = false;

        // Elevation rolling plateau
        if (x >= 20 && x <= 36 && y >= 18 && y <= 34) {
          this.elevations[y][x] = 1;
          if (x >= 25 && x <= 31 && y >= 23 && y <= 29) {
            this.elevations[y][x] = 2; // Hilltop peak
          }
        }

        // River at the far corner
        if (x + y > MAP_SIZE * 1.82) {
          this.map[y][x] = TerrainType.WATER;
        }
      }
    }

    // Forest clumps
    const forestSeeds = [
      { x: 8, y: 12 }, { x: 18, y: 6 }, { x: 44, y: 38 }, { x: 32, y: 46 },
      { x: 6, y: 44 }, { x: 46, y: 10 }, { x: 26, y: 26 }, { x: 12, y: 32 }
    ];
    for (const seed of forestSeeds) {
      for (let ox = -3; ox <= 3; ox++) {
        for (let oy = -3; oy <= 3; oy++) {
          if (Math.hypot(ox, oy) <= 3.3 && Math.random() > 0.12) {
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

    // Gold & Stone Piles
    const goldPiles = [
      { x: 14, y: 16 }, { x: 15, y: 16 }, { x: 14, y: 17 },
      { x: 40, y: 40 }, { x: 41, y: 40 }, { x: 40, y: 41 },
      { x: 28, y: 12 }, { x: 29, y: 12 }, { x: 22, y: 40 }
    ];
    for (const g of goldPiles) {
      if (this.isValidTile(g.x, g.y)) {
        this.map[g.y][g.x] = TerrainType.GOLD;
        this.addResourceNode(g.x, g.y, ResourceType.GOLD, 800);
      }
    }

    const stonePiles = [
      { x: 10, y: 22 }, { x: 11, y: 22 },
      { x: 44, y: 32 }, { x: 45, y: 32 },
      { x: 34, y: 18 }
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
      name: 'Player 1 (Britons)',
      resources: { food: 200, wood: 200, gold: 100, stone: 150 },
      pop: 4,
      maxPop: 10,
      age: 'Dark Age'
    });

    this.players.set(2, {
      id: 2,
      name: 'Player 2 (Franks AI)',
      resources: { food: 200, wood: 200, gold: 100, stone: 150 },
      pop: 4,
      maxPop: 10,
      age: 'Dark Age'
    });
  }

  private initEntities() {
    // Player 1 Base (Top-Left)
    this.spawnBuilding(BuildingType.TOWN_CENTER, 1, 10, 10, true);
    this.spawnBuilding(BuildingType.HOUSE, 1, 8, 16, true);
    this.spawnBuilding(BuildingType.LUMBER_CAMP, 1, 9, 8, true);
    this.spawnUnit(UnitType.VILLAGER, 1, 15, 12);
    this.spawnUnit(UnitType.VILLAGER, 1, 15, 14);
    this.spawnUnit(UnitType.VILLAGER, 1, 13, 15);
    this.spawnUnit(UnitType.SCOUT, 1, 16, 16);

    // Player 2 Base (Bottom-Right)
    this.spawnBuilding(BuildingType.TOWN_CENTER, 2, 42, 38, true);
    this.spawnBuilding(BuildingType.HOUSE, 2, 40, 44, true);
    this.spawnBuilding(BuildingType.BARRACKS, 2, 46, 36, true);
    this.spawnBuilding(BuildingType.LUMBER_CAMP, 2, 44, 34, true);
    this.spawnUnit(UnitType.VILLAGER, 2, 41, 36);
    this.spawnUnit(UnitType.VILLAGER, 2, 40, 38);
    this.spawnUnit(UnitType.VILLAGER, 2, 38, 40);
    this.spawnUnit(UnitType.SCOUT, 2, 38, 36);
  }

  public spawnUnit(type: UnitType, player: number, x: number, y: number): Unit {
    const cfg = UNIT_CONFIGS[type];
    const unit: Unit = {
      id: this.nextEntityId++,
      type,
      player,
      x,
      y,
      direction: 4,
      hp: cfg.hp,
      maxHp: cfg.hp,
      speed: cfg.speed,
      attack: cfg.attack,
      armorMelee: cfg.armorMelee,
      armorPierce: cfg.armorPierce,
      range: cfg.range,
      state: UnitState.IDLE,
      path: [],
      targetUnitId: null,
      targetBuildingId: null,
      targetResource: null,
      targetConstructBuildingId: null,
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
      armorMelee: cfg.armorMelee,
      armorPierce: cfg.armorPierce,
      isConstructed: isCompleted,
      buildProgress: isCompleted ? 100 : 5,
      productionQueue: [],
      productionTimer: 0,
      rallyPoint: { x: x + cfg.sizeX / 2, y: y + cfg.sizeY + 1 },
      garrisonedUnits: [],
      attackCooldown: 0,
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

    let maxPop = 5;
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

    // 1. Update Projectiles
    this.updateProjectiles();

    // 2. Update Units
    for (const unit of this.units.values()) {
      this.updateUnit(unit);
    }

    // 3. Update Buildings
    for (const building of this.buildings.values()) {
      this.updateBuilding(building);
    }

    // 4. Update AI Opponent
    this.aiActionTimer++;
    if (this.aiActionTimer >= 40) {
      this.aiActionTimer = 0;
      this.updateAI();
    }

    // 5. Update Fog of War
    if (this.tickCount % 4 === 0) {
      this.updateVision();
    }
  }

  private updateProjectiles() {
    for (let i = this.projectiles.length - 1; i >= 0; i--) {
      const p = this.projectiles[i];
      p.progress += p.speed / 100;

      // Current position interpolation
      p.currentX = p.startX + (p.targetX - p.startX) * p.progress;
      p.currentY = p.startY + (p.targetY - p.startY) * p.progress;

      // Parabolic Arc Height (Peak at 0.5)
      p.currentHeight = Math.sin(p.progress * Math.PI) * 28;

      if (p.progress >= 1.0) {
        // Projectile impacted target!
        this.applyProjectileDamage(p);
        this.projectiles.splice(i, 1);
      }
    }
  }

  private applyProjectileDamage(p: Projectile) {
    if (p.targetUnitId !== null) {
      const targetUnit = this.units.get(p.targetUnitId);
      if (targetUnit && targetUnit.state !== UnitState.DEAD) {
        const damage = this.calculateDamage(p.damage, targetUnit.armorPierce, p.startElev, p.targetElev);
        targetUnit.hp -= damage;
        if (targetUnit.hp <= 0) {
          targetUnit.hp = 0;
          targetUnit.state = UnitState.DEAD;
          this.updatePlayerPop(targetUnit.player);
        }
      }
    } else if (p.targetBuildingId !== null) {
      const targetBldg = this.buildings.get(p.targetBuildingId);
      if (targetBldg && targetBldg.hp > 0) {
        const damage = this.calculateDamage(p.damage, targetBldg.armorPierce, p.startElev, p.targetElev);
        targetBldg.hp -= damage;
        if (targetBldg.hp <= 0) {
          this.buildings.delete(targetBldg.id);
          this.updatePlayerPop(targetBldg.player);
        }
      }
    }
  }

  /**
   * Authentic AoE2 Damage Calculation Formula with Elevation Bonus
   */
  public calculateDamage(attack: number, armor: number, attackerElev = 0, defenderElev = 0): number {
    let base = Math.max(1, attack - armor);

    // Elevation combat modifier: +25% if higher, -25% if lower
    if (attackerElev > defenderElev) {
      base = Math.floor(base * 1.25);
    } else if (attackerElev < defenderElev) {
      base = Math.max(1, Math.floor(base * 0.75));
    }

    return Math.max(1, base);
  }

  private updateUnit(unit: Unit) {
    if (unit.state === UnitState.DEAD || unit.state === UnitState.GARRISONED) return;

    if (unit.attackCooldown > 0) unit.attackCooldown--;

    // Moving along path
    if (unit.path.length > 0) {
      unit.state = UnitState.MOVING;
      const target = unit.path[0];
      const dx = target.x - unit.x;
      const dy = target.y - unit.y;
      const dist = Math.hypot(dx, dy);

      unit.direction = Isometric.getDirection(dx, dy);

      const step = unit.speed / 15;
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

      unit.animTimer++;
      if (unit.animTimer >= 5) {
        unit.animTimer = 0;
        unit.animFrame = (unit.animFrame + 1) % 10;
      }
      return;
    }

    // Constructing / Repairing Building
    if (unit.targetConstructBuildingId !== null) {
      const bldg = this.buildings.get(unit.targetConstructBuildingId);
      if (!bldg || bldg.isConstructed) {
        unit.targetConstructBuildingId = null;
        unit.state = UnitState.IDLE;
        return;
      }

      const bCenterX = bldg.x + bldg.sizeX / 2;
      const bCenterY = bldg.y + bldg.sizeY / 2;
      const dist = Math.hypot(bCenterX - unit.x, bCenterY - unit.y);

      if (dist > bldg.sizeX / 2 + 1.2) {
        unit.path = Pathfinding.findPath(unit.x, unit.y, bCenterX, bCenterY, (x, y) => this.isTileWalkable(x, y));
      } else {
        unit.state = UnitState.BUILDING;
        unit.direction = Isometric.getDirection(bCenterX - unit.x, bCenterY - unit.y);
        unit.animTimer++;
        if (unit.animTimer >= 15) {
          unit.animTimer = 0;
          bldg.buildProgress += 2.0;
          bldg.hp = Math.min(bldg.maxHp, Math.floor((bldg.buildProgress / 100) * bldg.maxHp));
          this.audio?.playHammer();

          if (bldg.buildProgress >= 100) {
            bldg.isConstructed = true;
            this.updatePlayerPop(bldg.player);
            this.audio?.playBuildingComplete();
            unit.targetConstructBuildingId = null;
            unit.state = UnitState.IDLE;
          }
        }
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
        unit.path = Pathfinding.findPath(unit.x, unit.y, enemy.x, enemy.y, (x, y) => this.isTileWalkable(x, y));
      } else {
        unit.state = UnitState.ATTACKING;
        unit.direction = Isometric.getDirection(enemy.x - unit.x, enemy.y - unit.y);

        if (unit.attackCooldown <= 0) {
          unit.attackCooldown = 28;
          const uElev = this.getTileElev(Math.floor(unit.x), Math.floor(unit.y));
          const eElev = this.getTileElev(Math.floor(enemy.x), Math.floor(enemy.y));

          if (UNIT_CONFIGS[unit.type].isRanged) {
            // Shoot Parabolic Arrow
            this.fireProjectile(unit, enemy.x, enemy.y, enemy.id, null, uElev, eElev);
            this.audio?.playAttackRanged();
          } else {
            // Melee Strike
            const dmg = this.calculateDamage(unit.attack, enemy.armorMelee, uElev, eElev);
            enemy.hp -= dmg;
            this.audio?.playAttackMelee();
            if (enemy.hp <= 0) {
              enemy.hp = 0;
              enemy.state = UnitState.DEAD;
              this.updatePlayerPop(enemy.player);
              unit.targetUnitId = null;
              unit.state = UnitState.IDLE;
            }
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
          unit.attackCooldown = 32;
          const uElev = this.getTileElev(Math.floor(unit.x), Math.floor(unit.y));
          const bElev = this.getTileElev(bldg.x, bldg.y);

          if (UNIT_CONFIGS[unit.type].isRanged) {
            this.fireProjectile(unit, bCenterX, bCenterY, null, bldg.id, uElev, bElev);
            this.audio?.playAttackRanged();
          } else {
            const dmg = this.calculateDamage(unit.attack, bldg.armorMelee, uElev, bElev);
            bldg.hp -= dmg;
            this.audio?.playBuildingHit();
            if (bldg.hp <= 0) {
              this.buildings.delete(bldg.id);
              this.updatePlayerPop(bldg.player);
              unit.targetBuildingId = null;
              unit.state = UnitState.IDLE;
            }
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

          if (resNode.type === ResourceType.WOOD) {
            this.audio?.playChop();
          } else {
            this.audio?.playHammer();
          }

          if (unit.carrying.amount >= 10) {
            this.sendVillagerToSmartDropsite(unit);
          }
        }
      }
      return;
    }
  }

  private fireProjectile(
    unit: Unit,
    targetX: number,
    targetY: number,
    targetUnitId: number | null,
    targetBldgId: number | null,
    sElev: number,
    tElev: number
  ) {
    this.projectiles.push({
      id: this.nextEntityId++,
      player: unit.player,
      startX: unit.x,
      startY: unit.y,
      startElev: sElev,
      currentX: unit.x,
      currentY: unit.y,
      currentHeight: 0,
      targetX,
      targetY,
      targetElev: tElev,
      targetUnitId,
      targetBuildingId: targetBldgId,
      speed: UNIT_CONFIGS[unit.type].projectileSpeed || 6.5,
      damage: unit.attack,
      isPierce: true,
      progress: 0,
    });
  }

  private sendVillagerToSmartDropsite(unit: Unit) {
    if (!unit.carrying) return;

    // Find nearest dropsite accepting this resource
    let bestDist = Infinity;
    let closestBldg: Building | null = null;

    for (const b of this.buildings.values()) {
      if (b.player === unit.player && b.isConstructed) {
        const accepts = BUILDING_CONFIGS[b.type].acceptsResources;
        if (accepts && accepts.includes(unit.carrying.type)) {
          const d = Math.hypot(b.x + b.sizeX / 2 - unit.x, b.y + b.sizeY / 2 - unit.y);
          if (d < bestDist) {
            bestDist = d;
            closestBldg = b;
          }
        }
      }
    }

    if (closestBldg) {
      const dropX = closestBldg.x + closestBldg.sizeX / 2;
      const dropY = closestBldg.y + closestBldg.sizeY / 2;
      unit.path = Pathfinding.findPath(unit.x, unit.y, dropX, dropY, (x, y) => this.isTileWalkable(x, y));
      const originRes = unit.targetResource;

      const interval = setInterval(() => {
        if (!unit.carrying) {
          clearInterval(interval);
          return;
        }
        if (Math.hypot(dropX - unit.x, dropY - unit.y) < 2.5) {
          const p = this.players.get(unit.player);
          if (p) {
            p.resources[unit.carrying.type] += unit.carrying.amount;
          }
          unit.carrying = null;
          clearInterval(interval);
          if (originRes) {
            unit.targetResource = originRes;
          }
        }
      }, 500);
    }
  }

  private updateBuilding(building: Building) {
    // Watch Tower & Town Center auto-defense arrows
    if (building.isConstructed && BUILDING_CONFIGS[building.type].attacks) {
      if (building.attackCooldown > 0) building.attackCooldown--;
      if (building.attackCooldown <= 0) {
        const range = BUILDING_CONFIGS[building.type].attackRange || 7;
        const bX = building.x + building.sizeX / 2;
        const bY = building.y + building.sizeY / 2;

        // Find closest enemy unit
        let targetEnemy: Unit | null = null;
        let minDist = range;
        for (const u of this.units.values()) {
          if (u.player !== building.player && u.state !== UnitState.DEAD) {
            const d = Math.hypot(u.x - bX, u.y - bY);
            if (d < minDist) {
              minDist = d;
              targetEnemy = u;
            }
          }
        }

        if (targetEnemy) {
          building.attackCooldown = 35;
          const bElev = this.getTileElev(building.x, building.y);
          const tElev = this.getTileElev(Math.floor(targetEnemy.x), Math.floor(targetEnemy.y));
          const bonusArrows = building.garrisonedUnits.length; // More arrows with garrisoned villagers!
          const totalAtk = 5 + bonusArrows * 2;

          this.projectiles.push({
            id: this.nextEntityId++,
            player: building.player,
            startX: bX,
            startY: bY,
            startElev: bElev + 1,
            currentX: bX,
            currentY: bY,
            currentHeight: 0,
            targetX: targetEnemy.x,
            targetY: targetEnemy.y,
            targetElev: tElev,
            targetUnitId: targetEnemy.id,
            targetBuildingId: null,
            speed: 7.0,
            damage: totalAtk,
            isPierce: true,
            progress: 0,
          });
          this.audio?.playAttackRanged();
        }
      }
    }

    // Production Queue
    if (building.isConstructed && building.productionQueue.length > 0) {
      building.productionTimer++;
      const currentUnitType = building.productionQueue[0];
      const trainDuration = UNIT_CONFIGS[currentUnitType].trainTime * TICKS_PER_SECOND;

      if (building.productionTimer >= trainDuration) {
        building.productionTimer = 0;
        building.productionQueue.shift();

        const spawnX = building.x + building.sizeX / 2;
        const spawnY = building.y + building.sizeY + 0.5;
        const unit = this.spawnUnit(currentUnitType, building.player, spawnX, spawnY);
        if (building.rallyPoint) {
          unit.path = Pathfinding.findPath(spawnX, spawnY, building.rallyPoint.x, building.rallyPoint.y, (x, y) => this.isTileWalkable(x, y));
        }
      }
    }
  }

  /**
   * Town Bell alarm (TRIBE_COMMAND_TOWN_BELL)
   * Garrisons all player villagers into Town Center.
   */
  public triggerTownBell(playerId = 1) {
    const tc = Array.from(this.buildings.values()).find(b => b.player === playerId && b.type === BuildingType.TOWN_CENTER);
    if (!tc) return;

    this.audio?.playTownBell();
    for (const u of this.units.values()) {
      if (u.player === playerId && u.type === UnitType.VILLAGER && u.state !== UnitState.DEAD) {
        u.previousTask = {
          resource: u.targetResource ? { ...u.targetResource } : undefined,
          buildingId: u.targetConstructBuildingId || undefined
        };
        u.targetResource = null;
        u.targetConstructBuildingId = null;
        u.path = Pathfinding.findPath(u.x, u.y, tc.x + 2, tc.y + 2, (x, y) => this.isTileWalkable(x, y));

        // When arrived, garrison
        const checkArrival = setInterval(() => {
          if (Math.hypot(tc.x + 2 - u.x, tc.y + 2 - u.y) < 2.5) {
            u.state = UnitState.GARRISONED;
            tc.garrisonedUnits.push(u.id);
            clearInterval(checkArrival);
          }
        }, 400);
      }
    }
  }

  /**
   * Back to Work (TRIBE_COMMAND_GO_BACK_TO_WORK)
   * Unloads garrisoned villagers and sends them back to former tasks.
   */
  public triggerBackToWork(playerId = 1) {
    const tc = Array.from(this.buildings.values()).find(b => b.player === playerId && b.type === BuildingType.TOWN_CENTER);
    if (!tc || tc.garrisonedUnits.length === 0) return;

    for (const uid of tc.garrisonedUnits) {
      const u = this.units.get(uid);
      if (u) {
        u.state = UnitState.IDLE;
        u.x = tc.x + 2;
        u.y = tc.y + tc.sizeY + 0.5;
        if (u.previousTask?.resource) {
          u.targetResource = u.previousTask.resource;
          u.path = Pathfinding.findPath(u.x, u.y, u.targetResource.x, u.targetResource.y, (x, y) => this.isTileWalkable(x, y));
        } else if (u.previousTask?.buildingId) {
          u.targetConstructBuildingId = u.previousTask.buildingId;
        }
      }
    }
    tc.garrisonedUnits = [];
    this.audio?.playHorn();
  }

  /**
   * Buy commodity at Market (100 units of Wood, Food, or Stone)
   */
  public buyCommodity(type: ResourceType, playerId = 1): { success: boolean; msg: string } {
    const player = this.players.get(playerId);
    if (!player) return { success: false, msg: 'Player not found' };

    const costInGold = this.marketPrices[type];
    if (player.resources.gold < costInGold) {
      return { success: false, msg: `Not enough gold! Need ${costInGold} Gold.` };
    }

    player.resources.gold -= costInGold;
    if (type === ResourceType.WOOD) player.resources.wood += 100;
    else if (type === ResourceType.FOOD) player.resources.food += 100;
    else if (type === ResourceType.STONE) player.resources.stone += 100;

    this.marketPrices[type] = Math.min(999, this.marketPrices[type] + 3);
    this.audio?.playSelect();
    return { success: true, msg: `Bought 100 ${type} for ${costInGold} Gold.` };
  }

  /**
   * Sell commodity at Market (100 units of Wood, Food, or Stone)
   */
  public sellCommodity(type: ResourceType, playerId = 1): { success: boolean; msg: string } {
    const player = this.players.get(playerId);
    if (!player) return { success: false, msg: 'Player not found' };

    let currentRes = 0;
    if (type === ResourceType.WOOD) currentRes = player.resources.wood;
    else if (type === ResourceType.FOOD) currentRes = player.resources.food;
    else if (type === ResourceType.STONE) currentRes = player.resources.stone;

    if (currentRes < 100) {
      return { success: false, msg: `Not enough ${type}! Need 100 to sell.` };
    }

    const goldGain = Math.max(20, Math.floor(this.marketPrices[type] * 0.7));
    if (type === ResourceType.WOOD) player.resources.wood -= 100;
    else if (type === ResourceType.FOOD) player.resources.food -= 100;
    else if (type === ResourceType.STONE) player.resources.stone -= 100;

    player.resources.gold += goldGain;
    this.marketPrices[type] = Math.max(20, this.marketPrices[type] - 3);
    this.audio?.playSelect();
    return { success: true, msg: `Sold 100 ${type} for ${goldGain} Gold.` };
  }

  /**
   * Research Feudal Age at Town Center
   */
  public advanceToFeudalAge(playerId = 1): { success: boolean; msg: string } {
    const player = this.players.get(playerId);
    if (!player) return { success: false, msg: 'Player not found' };
    if (player.age !== 'Dark Age') return { success: false, msg: 'Already advanced!' };

    if (player.resources.food < 500) {
      return { success: false, msg: 'Need 500 Food to advance to Feudal Age!' };
    }

    player.resources.food -= 500;
    player.age = 'Feudal Age';
    this.audio?.playHorn();
    return { success: true, msg: 'Advanced to Feudal Age! Feudal units and military unlocked.' };
  }

  public garrisonUnit(unitId: number, buildingId: number) {
    const unit = this.units.get(unitId);
    const bldg = this.buildings.get(buildingId);
    if (!unit || !bldg || unit.state === UnitState.DEAD) return;

    if (bldg.garrisonedUnits.length >= 15) return;
    unit.state = UnitState.GARRISONED;
    bldg.garrisonedUnits.push(unit.id);
  }


  private updateAI() {
    const ai = this.players.get(2);
    if (!ai) return;

    // Train Villagers
    const tc = Array.from(this.buildings.values()).find(b => b.player === 2 && b.type === BuildingType.TOWN_CENTER);
    if (tc && tc.productionQueue.length < 2 && ai.resources.food >= 50 && ai.pop < ai.maxPop) {
      ai.resources.food -= 50;
      tc.productionQueue.push(UnitType.VILLAGER);
    }

    // Assign idle villagers
    for (const unit of this.units.values()) {
      if (unit.player === 2 && unit.type === UnitType.VILLAGER && unit.state === UnitState.IDLE) {
        let bestRes: ResourceNode | null = null;
        let minDist = Infinity;
        for (const res of this.resources.values()) {
          const d = Math.hypot(res.x - unit.x, res.y - unit.y);
          if (d < minDist && res.amount > 0) {
            minDist = d;
            bestRes = res;
          }
        }
        if (bestRes) {
          unit.targetResource = { x: bestRes.x, y: bestRes.y, type: bestRes.type };
          unit.path = Pathfinding.findPath(unit.x, unit.y, bestRes.x, bestRes.y, (x, y) => this.isTileWalkable(x, y));
        }
      }
    }

    // Build Military
    const barracks = Array.from(this.buildings.values()).find(b => b.player === 2 && b.type === BuildingType.BARRACKS);
    if (barracks && barracks.productionQueue.length < 2 && ai.resources.food >= 60 && ai.resources.gold >= 20 && ai.pop < ai.maxPop) {
      ai.resources.food -= 60;
      ai.resources.gold -= 20;
      barracks.productionQueue.push(UnitType.MILITIA);
    }

    // Attack Player 1
    const army = Array.from(this.units.values()).filter(u => u.player === 2 && u.type !== UnitType.VILLAGER);
    if (army.length >= 4) {
      const p1Tc = Array.from(this.buildings.values()).find(b => b.player === 1 && b.type === BuildingType.TOWN_CENTER);
      const targetPoint = p1Tc ? { x: p1Tc.x + 2, y: p1Tc.y + 2 } : { x: 12, y: 12 };
      for (const m of army) {
        if (m.state === UnitState.IDLE) {
          m.path = Pathfinding.findPath(m.x, m.y, targetPoint.x, targetPoint.y, (x, y) => this.isTileWalkable(x, y));
        }
      }
    }
  }

  private updateVision() {
    for (let y = 0; y < MAP_SIZE; y++) {
      for (let x = 0; x < MAP_SIZE; x++) {
        this.visible[y][x] = false;
      }
    }

    const sightSources: { x: number; y: number; radius: number }[] = [];
    for (const u of this.units.values()) {
      if (u.player === 1 && u.state !== UnitState.DEAD && u.state !== UnitState.GARRISONED) {
        sightSources.push({ x: u.x, y: u.y, radius: 5.5 });
      }
    }
    for (const b of this.buildings.values()) {
      if (b.player === 1) {
        sightSources.push({ x: b.x + b.sizeX / 2, y: b.y + b.sizeY / 2, radius: 8 });
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

  public getTileElev(x: number, y: number): number {
    if (this.isValidTile(x, y)) {
      return this.elevations[y][x];
    }
    return 0;
  }
}
