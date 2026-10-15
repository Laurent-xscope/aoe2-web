export const MAP_SIZE = 48; // 48x48 tile map
export const TILE_WIDTH = 96;
export const TILE_HEIGHT = 48;
export const ELEV_HEIGHT = 20;

export const TICKS_PER_SECOND = 30;
export const TICK_MS = 1000 / TICKS_PER_SECOND;

export enum TerrainType {
  GRASS = 0,
  DIRT = 1,
  DESERT = 2,
  WATER = 3,
  FOREST = 4,
  GOLD = 5,
  STONE = 6,
}

export enum UnitType {
  VILLAGER = 'villager',
  MILITIA = 'militia',
  MAN_AT_ARMS = 'man_at_arms',
  ARCHER = 'archer',
  SCOUT = 'scout',
}

export enum BuildingType {
  TOWN_CENTER = 'town_center',
  HOUSE = 'house',
  BARRACKS = 'barracks',
  MILL = 'mill',
  FARM = 'farm',
}

export enum UnitState {
  IDLE = 'idle',
  MOVING = 'moving',
  ATTACKING = 'attacking',
  GATHERING = 'gathering',
  BUILDING = 'building',
  DYING = 'dying',
  DEAD = 'dead',
}

export enum ResourceType {
  WOOD = 'wood',
  FOOD = 'food',
  GOLD = 'gold',
  STONE = 'stone',
}

export interface Cost {
  food: number;
  wood: number;
  gold: number;
  stone: number;
}

export interface UnitConfig {
  name: string;
  hp: number;
  attack: number;
  range: number;
  speed: number;
  cost: Cost;
  trainTime: number;
  icon: string;
}

export const UNIT_CONFIGS: Record<UnitType, UnitConfig> = {
  [UnitType.VILLAGER]: {
    name: 'Villager',
    hp: 40,
    attack: 3,
    range: 1,
    speed: 0.8,
    cost: { food: 50, wood: 0, gold: 0, stone: 0 },
    trainTime: 25,
    icon: '🧑‍🌾'
  },
  [UnitType.MILITIA]: {
    name: 'Militia',
    hp: 45,
    attack: 4,
    range: 1,
    speed: 0.9,
    cost: { food: 60, wood: 0, gold: 20, stone: 0 },
    trainTime: 21,
    icon: '🗡️'
  },
  [UnitType.MAN_AT_ARMS]: {
    name: 'Man-at-Arms',
    hp: 55,
    attack: 6,
    range: 1,
    speed: 0.9,
    cost: { food: 60, wood: 0, gold: 20, stone: 0 },
    trainTime: 21,
    icon: '⚔️'
  },
  [UnitType.ARCHER]: {
    name: 'Archer',
    hp: 35,
    attack: 4,
    range: 4,
    speed: 0.95,
    cost: { food: 0, wood: 25, gold: 45, stone: 0 },
    trainTime: 35,
    icon: '🏹'
  },
  [UnitType.SCOUT]: {
    name: 'Scout Cavalry',
    hp: 60,
    attack: 5,
    range: 1,
    speed: 1.5,
    cost: { food: 80, wood: 0, gold: 0, stone: 0 },
    trainTime: 30,
    icon: '🐎'
  }
};

export const BUILDING_CONFIGS = {
  [BuildingType.TOWN_CENTER]: {
    name: 'Town Center',
    sizeX: 4,
    sizeY: 4,
    hp: 2400,
    cost: { food: 0, wood: 275, gold: 0, stone: 100 },
    providesPop: 5,
    icon: '🏛️'
  },
  [BuildingType.HOUSE]: {
    name: 'House',
    sizeX: 2,
    sizeY: 2,
    hp: 550,
    cost: { food: 0, wood: 25, gold: 0, stone: 0 },
    providesPop: 5,
    icon: '🛖'
  },
  [BuildingType.BARRACKS]: {
    name: 'Barracks',
    sizeX: 3,
    sizeY: 3,
    hp: 1200,
    cost: { food: 0, wood: 175, gold: 0, stone: 0 },
    providesPop: 0,
    icon: '⚔️'
  },
  [BuildingType.MILL]: {
    name: 'Mill',
    sizeX: 2,
    sizeY: 2,
    hp: 600,
    cost: { food: 0, wood: 100, gold: 0, stone: 0 },
    providesPop: 0,
    icon: '🌾'
  },
  [BuildingType.FARM]: {
    name: 'Farm',
    sizeX: 3,
    sizeY: 3,
    hp: 480,
    cost: { food: 0, wood: 60, gold: 0, stone: 0 },
    providesPop: 0,
    icon: '🌽'
  }
};

export const PLAYER_COLORS = {
  1: { name: 'Britons (Blue)', hex: '#1e60d5', minimap: '#2e86de', textColor: '#54a0ff' },
  2: { name: 'Franks (Red)', hex: '#d63031', minimap: '#ee5253', textColor: '#ff7675' }
};
