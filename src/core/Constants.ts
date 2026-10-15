export const MAP_SIZE = 56; // 56x56 tile battlefield
export const TILE_WIDTH = 96;
export const TILE_HEIGHT = 48;
export const ELEV_HEIGHT = 22;

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
  CROSSBOWMAN = 'crossbowman',
  SKIRMISHER = 'skirmisher',
  SCOUT = 'scout',
  KNIGHT = 'knight',
  MONK = 'monk',
}

export enum BuildingType {
  TOWN_CENTER = 'town_center',
  HOUSE = 'house',
  BARRACKS = 'barracks',
  MILL = 'mill',
  LUMBER_CAMP = 'lumber_camp',
  MINING_CAMP = 'mining_camp',
  FARM = 'farm',
  ARCHERY_RANGE = 'archery_range',
  STABLE = 'stable',
  BLACKSMITH = 'blacksmith',
  MARKET = 'market',
  MONASTERY = 'monastery',
  WATCH_TOWER = 'watch_tower',
}

export enum UnitState {
  IDLE = 'idle',
  MOVING = 'moving',
  ATTACKING = 'attacking',
  GATHERING = 'gathering',
  BUILDING = 'building',
  REPAIRING = 'repairing',
  GARRISONED = 'garrisoned',
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
  armorMelee: number;
  armorPierce: number;
  range: number;
  speed: number;
  projectileSpeed?: number;
  cost: Cost;
  trainTime: number;
  icon: string;
  isRanged?: boolean;
}

export const UNIT_CONFIGS: Record<UnitType, UnitConfig> = {
  [UnitType.VILLAGER]: {
    name: 'Villager',
    hp: 40,
    attack: 3,
    armorMelee: 0,
    armorPierce: 0,
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
    armorMelee: 0,
    armorPierce: 1,
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
    armorMelee: 1,
    armorPierce: 1,
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
    armorMelee: 0,
    armorPierce: 0,
    range: 4.5,
    speed: 0.95,
    projectileSpeed: 6.5,
    isRanged: true,
    cost: { food: 0, wood: 25, gold: 45, stone: 0 },
    trainTime: 35,
    icon: '🏹'
  },
  [UnitType.CROSSBOWMAN]: {
    name: 'Crossbowman',
    hp: 40,
    attack: 5,
    armorMelee: 0,
    armorPierce: 0,
    range: 5,
    speed: 0.95,
    projectileSpeed: 7.0,
    isRanged: true,
    cost: { food: 0, wood: 25, gold: 45, stone: 0 },
    trainTime: 30,
    icon: '🎯'
  },
  [UnitType.SKIRMISHER]: {
    name: 'Skirmisher',
    hp: 30,
    attack: 3,
    armorMelee: 0,
    armorPierce: 3,
    range: 4,
    speed: 1.0,
    projectileSpeed: 6.0,
    isRanged: true,
    cost: { food: 25, wood: 35, gold: 0, stone: 0 },
    trainTime: 22,
    icon: '🪃'
  },
  [UnitType.SCOUT]: {
    name: 'Scout Cavalry',
    hp: 60,
    attack: 5,
    armorMelee: 0,
    armorPierce: 2,
    range: 1,
    speed: 1.5,
    cost: { food: 80, wood: 0, gold: 0, stone: 0 },
    trainTime: 30,
    icon: '🐎'
  },
  [UnitType.KNIGHT]: {
    name: 'Knight',
    hp: 100,
    attack: 10,
    armorMelee: 2,
    armorPierce: 2,
    range: 1,
    speed: 1.35,
    cost: { food: 60, wood: 0, gold: 75, stone: 0 },
    trainTime: 30,
    icon: '🛡️'
  },
  [UnitType.MONK]: {
    name: 'Monk',
    hp: 30,
    attack: 0,
    armorMelee: 0,
    armorPierce: 0,
    range: 4,
    speed: 0.7,
    cost: { food: 0, wood: 0, gold: 100, stone: 0 },
    trainTime: 51,
    icon: '✝️'
  }
};

export interface BuildingConfig {
  name: string;
  sizeX: number;
  sizeY: number;
  hp: number;
  armorMelee: number;
  armorPierce: number;
  cost: Cost;
  providesPop: number;
  slpKey: string;
  icon: string;
  acceptsResources?: ResourceType[];
  attacks?: boolean;
  attackRange?: number;
}

export const BUILDING_CONFIGS: Record<BuildingType, BuildingConfig> = {
  [BuildingType.TOWN_CENTER]: {
    name: 'Town Center',
    sizeX: 4,
    sizeY: 4,
    hp: 2400,
    armorMelee: 3,
    armorPierce: 5,
    cost: { food: 0, wood: 275, gold: 0, stone: 100 },
    providesPop: 5,
    slpKey: 'slp_103',
    icon: '🏛️',
    acceptsResources: [ResourceType.WOOD, ResourceType.FOOD, ResourceType.GOLD, ResourceType.STONE],
    attacks: true,
    attackRange: 6
  },
  [BuildingType.HOUSE]: {
    name: 'House',
    sizeX: 2,
    sizeY: 2,
    hp: 550,
    armorMelee: 0,
    armorPierce: 7,
    cost: { food: 0, wood: 25, gold: 0, stone: 0 },
    providesPop: 5,
    slpKey: 'slp_104',
    icon: '🛖'
  },
  [BuildingType.BARRACKS]: {
    name: 'Barracks',
    sizeX: 3,
    sizeY: 3,
    hp: 1200,
    armorMelee: 1,
    armorPierce: 8,
    cost: { food: 0, wood: 175, gold: 0, stone: 0 },
    providesPop: 0,
    slpKey: 'slp_102',
    icon: '⚔️'
  },
  [BuildingType.MILL]: {
    name: 'Mill',
    sizeX: 2,
    sizeY: 2,
    hp: 600,
    armorMelee: 0,
    armorPierce: 7,
    cost: { food: 0, wood: 100, gold: 0, stone: 0 },
    providesPop: 0,
    slpKey: 'slp_105',
    icon: '🌾',
    acceptsResources: [ResourceType.FOOD]
  },
  [BuildingType.LUMBER_CAMP]: {
    name: 'Lumber Camp',
    sizeX: 2,
    sizeY: 2,
    hp: 600,
    armorMelee: 0,
    armorPierce: 7,
    cost: { food: 0, wood: 100, gold: 0, stone: 0 },
    providesPop: 0,
    slpKey: 'slp_1007',
    icon: '🪵',
    acceptsResources: [ResourceType.WOOD]
  },
  [BuildingType.MINING_CAMP]: {
    name: 'Mining Camp',
    sizeX: 2,
    sizeY: 2,
    hp: 600,
    armorMelee: 0,
    armorPierce: 7,
    cost: { food: 0, wood: 100, gold: 0, stone: 0 },
    providesPop: 0,
    slpKey: 'slp_1006',
    icon: '⛏️',
    acceptsResources: [ResourceType.GOLD, ResourceType.STONE]
  },
  [BuildingType.FARM]: {
    name: 'Farm',
    sizeX: 3,
    sizeY: 3,
    hp: 480,
    armorMelee: 0,
    armorPierce: 0,
    cost: { food: 0, wood: 60, gold: 0, stone: 0 },
    providesPop: 0,
    slpKey: 'slp_105',
    icon: '🌽',
    acceptsResources: [ResourceType.FOOD]
  },
  [BuildingType.ARCHERY_RANGE]: {
    name: 'Archery Range',
    sizeX: 3,
    sizeY: 3,
    hp: 1500,
    armorMelee: 1,
    armorPierce: 8,
    cost: { food: 0, wood: 175, gold: 0, stone: 0 },
    providesPop: 0,
    slpKey: 'slp_1008',
    icon: '🏹'
  },
  [BuildingType.STABLE]: {
    name: 'Stable',
    sizeX: 3,
    sizeY: 3,
    hp: 1500,
    armorMelee: 1,
    armorPierce: 8,
    cost: { food: 0, wood: 175, gold: 0, stone: 0 },
    providesPop: 0,
    slpKey: 'slp_1009',
    icon: '🐴'
  },
  [BuildingType.BLACKSMITH]: {
    name: 'Blacksmith',
    sizeX: 3,
    sizeY: 3,
    hp: 1800,
    armorMelee: 1,
    armorPierce: 8,
    cost: { food: 0, wood: 150, gold: 0, stone: 0 },
    providesPop: 0,
    slpKey: 'slp_1018',
    icon: '⚒️'
  },
  [BuildingType.MARKET]: {
    name: 'Market',
    sizeX: 3,
    sizeY: 3,
    hp: 1800,
    armorMelee: 1,
    armorPierce: 8,
    cost: { food: 0, wood: 175, gold: 0, stone: 0 },
    providesPop: 0,
    slpKey: 'slp_1019',
    icon: '⚖️'
  },
  [BuildingType.MONASTERY]: {
    name: 'Monastery',
    sizeX: 3,
    sizeY: 3,
    hp: 1800,
    armorMelee: 1,
    armorPierce: 8,
    cost: { food: 0, wood: 175, gold: 0, stone: 0 },
    providesPop: 0,
    slpKey: 'slp_1020',
    icon: '⛪'
  },
  [BuildingType.WATCH_TOWER]: {
    name: 'Watch Tower',
    sizeX: 2,
    sizeY: 2,
    hp: 1020,
    armorMelee: 1,
    armorPierce: 7,
    cost: { food: 0, wood: 50, gold: 0, stone: 125 },
    providesPop: 0,
    slpKey: 'slp_1122',
    icon: '🗼',
    attacks: true,
    attackRange: 8
  }
};

export const PLAYER_COLORS = {
  1: { name: 'Britons (Blue)', hex: '#1e60d5', minimap: '#2e86de', textColor: '#54a0ff' },
  2: { name: 'Franks (Red)', hex: '#d63031', minimap: '#ee5253', textColor: '#ff7675' }
};
