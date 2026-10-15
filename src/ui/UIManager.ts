import { BuildingType, UnitType, ResourceType, BUILDING_CONFIGS, UNIT_CONFIGS } from '../core/Constants';
import { GameState, Building } from '../core/GameState';
import { AudioManager } from '../audio/AudioManager';

export class UIManager {
  private resWood: HTMLElement;
  private resFood: HTMLElement;
  private resGold: HTMLElement;
  private resStone: HTMLElement;
  private resPop: HTMLElement;
  private fpsCounter: HTMLElement;
  private ageIndicator: HTMLElement;

  private selectionName: HTMLElement;
  private selectionHpBar: HTMLElement;
  private selectionStats: HTMLElement;
  private portraitIcon: HTMLElement;
  private commandCard: HTMLElement;
  private gameBanner: HTMLElement;

  private bannerTimeout: number | null = null;
  public activeBuildingPlacement: BuildingType | null = null;
  private villagerTab: 'eco' | 'military' = 'eco';

  // Last selection tracker to avoid unnecessary DOM rebuilding every frame
  private lastSelectedUnitId: number | null = null;
  private lastSelectedUnitCount = 0;
  private lastSelectedBldgId: number | null = null;
  private lastBldgQueueLen = -1;
  private lastBldgGarrisonLen = -1;

  constructor(private audio: AudioManager) {
    this.resWood = document.getElementById('res-wood-val')!;
    this.resFood = document.getElementById('res-food-val')!;
    this.resGold = document.getElementById('res-gold-val')!;
    this.resStone = document.getElementById('res-stone-val')!;
    this.resPop = document.getElementById('res-pop-val')!;
    this.fpsCounter = document.getElementById('fps-counter')!;
    this.ageIndicator = document.getElementById('age-indicator')!;

    this.selectionName = document.getElementById('selection-name')!;
    this.selectionHpBar = document.getElementById('selection-hp-bar')!;
    this.selectionStats = document.getElementById('selection-stats')!;
    this.portraitIcon = document.getElementById('portrait-icon')!;
    this.commandCard = document.getElementById('command-card')!;
    this.gameBanner = document.getElementById('game-banner')!;
  }

  public showBanner(msg: string) {
    this.gameBanner.textContent = msg;
    this.gameBanner.style.opacity = '1';
    if (this.bannerTimeout) clearTimeout(this.bannerTimeout);
    this.bannerTimeout = window.setTimeout(() => {
      this.gameBanner.style.opacity = '0';
    }, 2800);
  }

  public update(state: GameState, fps: number) {
    const p1 = state.players.get(1);
    if (!p1) return;

    // Update Resource Counters
    this.resWood.textContent = Math.floor(p1.resources.wood).toString();
    this.resFood.textContent = Math.floor(p1.resources.food).toString();
    this.resGold.textContent = Math.floor(p1.resources.gold).toString();
    this.resStone.textContent = Math.floor(p1.resources.stone).toString();
    this.resPop.textContent = `${p1.pop} / ${p1.maxPop}`;
    this.fpsCounter.textContent = `${Math.round(fps)} FPS`;

    // Update Age Indicator
    if (this.ageIndicator.textContent !== `${p1.age} • Britons`) {
      this.ageIndicator.textContent = `${p1.age} • Britons`;
    }

    // Update Selection Card & Commands
    this.updateSelection(state);
  }

  private updateSelection(state: GameState) {
    const selectedUnitIds = Array.from(state.selectedUnitIds);
    const selectedBldgId = state.selectedBuildingId;

    if (selectedUnitIds.length > 0) {
      const unit = state.units.get(selectedUnitIds[0]);
      if (unit) {
        const cfg = UNIT_CONFIGS[unit.type];
        this.portraitIcon.textContent = cfg.icon;
        this.selectionName.textContent = selectedUnitIds.length > 1 ? `${selectedUnitIds.length} Units` : cfg.name;
        const hpPct = Math.max(0, Math.min(1, unit.hp / unit.maxHp));
        this.selectionHpBar.style.width = `${hpPct * 100}%`;
        this.selectionStats.textContent = `HP: ${Math.ceil(unit.hp)} / ${unit.maxHp} • Atk: ${unit.attack} • Armor: ${unit.armorMelee}/${unit.armorPierce} • Spd: ${unit.speed}`;

        // Only rebuild DOM when selection changed
        if (this.lastSelectedUnitId !== unit.id || this.lastSelectedUnitCount !== selectedUnitIds.length) {
          this.lastSelectedUnitId = unit.id;
          this.lastSelectedUnitCount = selectedUnitIds.length;
          this.lastSelectedBldgId = null;
          this.renderUnitCommands(state, unit.type);
        }
        return;
      }
    }

    if (selectedBldgId !== null) {
      const bldg = state.buildings.get(selectedBldgId);
      if (bldg) {
        const cfg = BUILDING_CONFIGS[bldg.type];
        this.portraitIcon.textContent = cfg.icon;
        this.selectionName.textContent = cfg.name;
        const hpPct = Math.max(0, Math.min(1, bldg.hp / bldg.maxHp));
        this.selectionHpBar.style.width = `${hpPct * 100}%`;
        const statusText = bldg.isConstructed ? 'Complete' : `Foundation (${Math.floor(bldg.buildProgress)}%)`;
        this.selectionStats.textContent = `HP: ${Math.ceil(bldg.hp)} / ${bldg.maxHp} • Armor: ${bldg.armorMelee}/${bldg.armorPierce} • ${statusText}`;

        const qLen = bldg.productionQueue.length;
        const gLen = bldg.garrisonedUnits.length;
        if (this.lastSelectedBldgId !== bldg.id || this.lastBldgQueueLen !== qLen || this.lastBldgGarrisonLen !== gLen) {
          this.lastSelectedBldgId = bldg.id;
          this.lastBldgQueueLen = qLen;
          this.lastBldgGarrisonLen = gLen;
          this.lastSelectedUnitId = null;
          this.renderBuildingCommands(state, bldg);
        }
        return;
      }
    }

    // Nothing selected
    if (this.lastSelectedUnitId !== null || this.lastSelectedBldgId !== null) {
      this.lastSelectedUnitId = null;
      this.lastSelectedUnitCount = 0;
      this.lastSelectedBldgId = null;
      this.portraitIcon.textContent = '🛡️';
      this.selectionName.textContent = 'No Selection';
      this.selectionHpBar.style.width = '0%';
      this.selectionStats.textContent = 'Select units or buildings';
      this.commandCard.innerHTML = '';
    }
  }

  private renderUnitCommands(state: GameState, type: UnitType) {
    if (type !== UnitType.VILLAGER) {
      this.commandCard.innerHTML = '';
      return;
    }

    const isEco = this.villagerTab === 'eco';

    this.commandCard.innerHTML = `
      <div class="cmd-header">
        <button class="cmd-tab-btn ${isEco ? 'active' : ''}" id="tab-eco">Economic</button>
        <button class="cmd-tab-btn ${!isEco ? 'active' : ''}" id="tab-mil">Military</button>
      </div>
      <div class="cmd-grid" id="villager-cmd-grid"></div>
    `;

    document.getElementById('tab-eco')?.addEventListener('click', () => {
      this.villagerTab = 'eco';
      this.renderUnitCommands(state, type);
    });
    document.getElementById('tab-mil')?.addEventListener('click', () => {
      this.villagerTab = 'military';
      this.renderUnitCommands(state, type);
    });

    const grid = document.getElementById('villager-cmd-grid');
    if (!grid) return;

    if (isEco) {
      grid.innerHTML = `
        <button class="cmd-btn" id="btn-build-house" title="Build House (+5 Pop)">
          <span class="cmd-icon">🛖</span>
          <span>House</span>
          <span class="cmd-cost">25 W</span>
        </button>
        <button class="cmd-btn" id="btn-build-lumber" title="Build Lumber Camp (Wood dropsite)">
          <span class="cmd-icon">🪵</span>
          <span>Lumber</span>
          <span class="cmd-cost">100 W</span>
        </button>
        <button class="cmd-btn" id="btn-build-mining" title="Build Mining Camp (Gold/Stone dropsite)">
          <span class="cmd-icon">⛏️</span>
          <span>Mining</span>
          <span class="cmd-cost">100 W</span>
        </button>
        <button class="cmd-btn" id="btn-build-mill" title="Build Mill (Food dropsite)">
          <span class="cmd-icon">🌾</span>
          <span>Mill</span>
          <span class="cmd-cost">100 W</span>
        </button>
        <button class="cmd-btn" id="btn-build-farm" title="Build Farm (Continuous food)">
          <span class="cmd-icon">🌽</span>
          <span>Farm</span>
          <span class="cmd-cost">60 W</span>
        </button>
      `;

      document.getElementById('btn-build-house')?.addEventListener('click', () => this.startBuildingPlacement(state, BuildingType.HOUSE));
      document.getElementById('btn-build-lumber')?.addEventListener('click', () => this.startBuildingPlacement(state, BuildingType.LUMBER_CAMP));
      document.getElementById('btn-build-mining')?.addEventListener('click', () => this.startBuildingPlacement(state, BuildingType.MINING_CAMP));
      document.getElementById('btn-build-mill')?.addEventListener('click', () => this.startBuildingPlacement(state, BuildingType.MILL));
      document.getElementById('btn-build-farm')?.addEventListener('click', () => this.startBuildingPlacement(state, BuildingType.FARM));
    } else {
      grid.innerHTML = `
        <button class="cmd-btn" id="btn-build-barracks" title="Build Barracks (Infantry)">
          <span class="cmd-icon">⚔️</span>
          <span>Barracks</span>
          <span class="cmd-cost">175 W</span>
        </button>
        <button class="cmd-btn" id="btn-build-range" title="Build Archery Range (Archers)">
          <span class="cmd-icon">🏹</span>
          <span>Range</span>
          <span class="cmd-cost">175 W</span>
        </button>
        <button class="cmd-btn" id="btn-build-stable" title="Build Stable (Cavalry)">
          <span class="cmd-icon">🐴</span>
          <span>Stable</span>
          <span class="cmd-cost">175 W</span>
        </button>
        <button class="cmd-btn" id="btn-build-blacksmith" title="Build Blacksmith (Upgrades)">
          <span class="cmd-icon">⚒️</span>
          <span>Smith</span>
          <span class="cmd-cost">150 W</span>
        </button>
        <button class="cmd-btn" id="btn-build-tower" title="Build Watch Tower (Defensive arrow fire)">
          <span class="cmd-icon">🗼</span>
          <span>Tower</span>
          <span class="cmd-cost">50W 125S</span>
        </button>
        <button class="cmd-btn" id="btn-build-market" title="Build Market (Commodity Trading)">
          <span class="cmd-icon">⚖️</span>
          <span>Market</span>
          <span class="cmd-cost">175 W</span>
        </button>
        <button class="cmd-btn" id="btn-build-monastery" title="Build Monastery (Monks & Relics)">
          <span class="cmd-icon">⛪</span>
          <span>Monastery</span>
          <span class="cmd-cost">175 W</span>
        </button>
      `;

      document.getElementById('btn-build-barracks')?.addEventListener('click', () => this.startBuildingPlacement(state, BuildingType.BARRACKS));
      document.getElementById('btn-build-range')?.addEventListener('click', () => this.startBuildingPlacement(state, BuildingType.ARCHERY_RANGE));
      document.getElementById('btn-build-stable')?.addEventListener('click', () => this.startBuildingPlacement(state, BuildingType.STABLE));
      document.getElementById('btn-build-blacksmith')?.addEventListener('click', () => this.startBuildingPlacement(state, BuildingType.BLACKSMITH));
      document.getElementById('btn-build-tower')?.addEventListener('click', () => this.startBuildingPlacement(state, BuildingType.WATCH_TOWER));
      document.getElementById('btn-build-market')?.addEventListener('click', () => this.startBuildingPlacement(state, BuildingType.MARKET));
      document.getElementById('btn-build-monastery')?.addEventListener('click', () => this.startBuildingPlacement(state, BuildingType.MONASTERY));
    }
  }

  private renderBuildingCommands(state: GameState, bldg: Building) {
    const p1 = state.players.get(1);
    if (!p1) return;

    if (bldg.type === BuildingType.TOWN_CENTER) {
      const qText = bldg.productionQueue.length > 0 ? `Queue: ${bldg.productionQueue.length}` : 'Idle';
      const gText = `Garrison: ${bldg.garrisonedUnits.length} / 15`;

      this.commandCard.innerHTML = `
        <div class="cmd-header" style="font-size: 10px; color: #d4af37; justify-content: space-between;">
          <span>${gText}</span>
          <span>${qText}</span>
        </div>
        <div class="cmd-grid">
          <button class="cmd-btn" id="btn-train-villager" title="Train Villager">
            <span class="cmd-icon">🧑‍🌾</span>
            <span>Villager</span>
            <span class="cmd-cost">50 F</span>
          </button>
          <button class="cmd-btn" id="btn-town-bell" title="Town Bell (Garrison all villagers)">
            <span class="cmd-icon">🔔</span>
            <span>Town Bell</span>
            <span class="cmd-cost">Alarm</span>
          </button>
          <button class="cmd-btn" id="btn-back-to-work" title="Back to Work (Return villagers)">
            <span class="cmd-icon">🔨</span>
            <span>Back Work</span>
            <span class="cmd-cost">Release</span>
          </button>
          ${p1.age === 'Dark Age' ? `
          <button class="cmd-btn" id="btn-feudal-age" title="Advance to Feudal Age">
            <span class="cmd-icon">🏰</span>
            <span>Feudal</span>
            <span class="cmd-cost">500 F</span>
          </button>` : ''}
        </div>
      `;

      document.getElementById('btn-train-villager')?.addEventListener('click', () => this.trainUnit(state, bldg.id, UnitType.VILLAGER));
      document.getElementById('btn-town-bell')?.addEventListener('click', () => {
        state.triggerTownBell(1);
        this.showBanner('Town Bell sounded! Villagers taking refuge.');
      });
      document.getElementById('btn-back-to-work')?.addEventListener('click', () => {
        state.triggerBackToWork(1);
        this.showBanner('Villagers returning to work.');
      });
      document.getElementById('btn-feudal-age')?.addEventListener('click', () => {
        const res = state.advanceToFeudalAge(1);
        this.showBanner(res.msg);
        this.renderBuildingCommands(state, bldg);
      });

    } else if (bldg.type === BuildingType.BARRACKS) {
      const qText = bldg.productionQueue.length > 0 ? `Queue: ${bldg.productionQueue.length}` : 'Idle';

      this.commandCard.innerHTML = `
        <div class="cmd-header" style="font-size: 10px; color: #d4af37; justify-content: flex-end;">
          <span>${qText}</span>
        </div>
        <div class="cmd-grid">
          <button class="cmd-btn" id="btn-train-militia" title="Train Militia">
            <span class="cmd-icon">🗡️</span>
            <span>Militia</span>
            <span class="cmd-cost">60 F 20 G</span>
          </button>
          <button class="cmd-btn" id="btn-train-manatarms" title="Train Man-at-Arms">
            <span class="cmd-icon">⚔️</span>
            <span>Man-at-Arms</span>
            <span class="cmd-cost">60 F 20 G</span>
          </button>
        </div>
      `;

      document.getElementById('btn-train-militia')?.addEventListener('click', () => this.trainUnit(state, bldg.id, UnitType.MILITIA));
      document.getElementById('btn-train-manatarms')?.addEventListener('click', () => this.trainUnit(state, bldg.id, UnitType.MAN_AT_ARMS));

    } else if (bldg.type === BuildingType.ARCHERY_RANGE) {
      const qText = bldg.productionQueue.length > 0 ? `Queue: ${bldg.productionQueue.length}` : 'Idle';

      this.commandCard.innerHTML = `
        <div class="cmd-header" style="font-size: 10px; color: #d4af37; justify-content: flex-end;">
          <span>${qText}</span>
        </div>
        <div class="cmd-grid">
          <button class="cmd-btn" id="btn-train-archer" title="Train Archer">
            <span class="cmd-icon">🏹</span>
            <span>Archer</span>
            <span class="cmd-cost">25 W 45 G</span>
          </button>
          <button class="cmd-btn" id="btn-train-skirmisher" title="Train Skirmisher">
            <span class="cmd-icon">🪃</span>
            <span>Skirmish</span>
            <span class="cmd-cost">25 F 35 W</span>
          </button>
          <button class="cmd-btn" id="btn-train-crossbow" title="Train Crossbowman">
            <span class="cmd-icon">🎯</span>
            <span>Crossbow</span>
            <span class="cmd-cost">25 W 45 G</span>
          </button>
        </div>
      `;

      document.getElementById('btn-train-archer')?.addEventListener('click', () => this.trainUnit(state, bldg.id, UnitType.ARCHER));
      document.getElementById('btn-train-skirmisher')?.addEventListener('click', () => this.trainUnit(state, bldg.id, UnitType.SKIRMISHER));
      document.getElementById('btn-train-crossbow')?.addEventListener('click', () => this.trainUnit(state, bldg.id, UnitType.CROSSBOWMAN));

    } else if (bldg.type === BuildingType.STABLE) {
      const qText = bldg.productionQueue.length > 0 ? `Queue: ${bldg.productionQueue.length}` : 'Idle';

      this.commandCard.innerHTML = `
        <div class="cmd-header" style="font-size: 10px; color: #d4af37; justify-content: flex-end;">
          <span>${qText}</span>
        </div>
        <div class="cmd-grid">
          <button class="cmd-btn" id="btn-train-scout" title="Train Scout Cavalry">
            <span class="cmd-icon">🐎</span>
            <span>Scout</span>
            <span class="cmd-cost">80 F</span>
          </button>
          <button class="cmd-btn" id="btn-train-knight" title="Train Knight">
            <span class="cmd-icon">🛡️</span>
            <span>Knight</span>
            <span class="cmd-cost">60 F 75 G</span>
          </button>
        </div>
      `;

      document.getElementById('btn-train-scout')?.addEventListener('click', () => this.trainUnit(state, bldg.id, UnitType.SCOUT));
      document.getElementById('btn-train-knight')?.addEventListener('click', () => this.trainUnit(state, bldg.id, UnitType.KNIGHT));

    } else if (bldg.type === BuildingType.MONASTERY) {
      const qText = bldg.productionQueue.length > 0 ? `Queue: ${bldg.productionQueue.length}` : 'Idle';

      this.commandCard.innerHTML = `
        <div class="cmd-header" style="font-size: 10px; color: #d4af37; justify-content: flex-end;">
          <span>${qText}</span>
        </div>
        <div class="cmd-grid">
          <button class="cmd-btn" id="btn-train-monk" title="Train Monk (Heal & Convert)">
            <span class="cmd-icon">✝️</span>
            <span>Monk</span>
            <span class="cmd-cost">100 G</span>
          </button>
        </div>
      `;

      document.getElementById('btn-train-monk')?.addEventListener('click', () => this.trainUnit(state, bldg.id, UnitType.MONK));

    } else if (bldg.type === BuildingType.MARKET) {
      const wPrice = state.marketPrices[ResourceType.WOOD];
      const fPrice = state.marketPrices[ResourceType.FOOD];
      const sPrice = state.marketPrices[ResourceType.STONE];

      this.commandCard.innerHTML = `
        <div class="cmd-header" style="font-size: 10px; color: #d4af37;">
          <span>Market Commodity Exchange (100 units)</span>
        </div>
        <div class="cmd-grid" style="grid-template-columns: repeat(6, 1fr);">
          <button class="cmd-btn" id="btn-buy-wood" title="Buy 100 Wood for ${wPrice} Gold">
            <span class="cmd-icon">🪵</span>
            <span>+100 W</span>
            <span class="cmd-cost">${wPrice} G</span>
          </button>
          <button class="cmd-btn" id="btn-sell-wood" title="Sell 100 Wood for ${Math.floor(wPrice * 0.7)} Gold">
            <span class="cmd-icon">🪵</span>
            <span>-100 W</span>
            <span class="cmd-cost">+${Math.floor(wPrice * 0.7)} G</span>
          </button>
          <button class="cmd-btn" id="btn-buy-food" title="Buy 100 Food for ${fPrice} Gold">
            <span class="cmd-icon">🌾</span>
            <span>+100 F</span>
            <span class="cmd-cost">${fPrice} G</span>
          </button>
          <button class="cmd-btn" id="btn-sell-food" title="Sell 100 Food for ${Math.floor(fPrice * 0.7)} Gold">
            <span class="cmd-icon">🌾</span>
            <span>-100 F</span>
            <span class="cmd-cost">+${Math.floor(fPrice * 0.7)} G</span>
          </button>
          <button class="cmd-btn" id="btn-buy-stone" title="Buy 100 Stone for ${sPrice} Gold">
            <span class="cmd-icon">🪨</span>
            <span>+100 S</span>
            <span class="cmd-cost">${sPrice} G</span>
          </button>
          <button class="cmd-btn" id="btn-sell-stone" title="Sell 100 Stone for ${Math.floor(sPrice * 0.7)} Gold">
            <span class="cmd-icon">🪨</span>
            <span>-100 S</span>
            <span class="cmd-cost">+${Math.floor(sPrice * 0.7)} G</span>
          </button>
        </div>
      `;

      document.getElementById('btn-buy-wood')?.addEventListener('click', () => {
        const res = state.buyCommodity(ResourceType.WOOD, 1);
        this.showBanner(res.msg);
        this.renderBuildingCommands(state, bldg);
      });
      document.getElementById('btn-sell-wood')?.addEventListener('click', () => {
        const res = state.sellCommodity(ResourceType.WOOD, 1);
        this.showBanner(res.msg);
        this.renderBuildingCommands(state, bldg);
      });
      document.getElementById('btn-buy-food')?.addEventListener('click', () => {
        const res = state.buyCommodity(ResourceType.FOOD, 1);
        this.showBanner(res.msg);
        this.renderBuildingCommands(state, bldg);
      });
      document.getElementById('btn-sell-food')?.addEventListener('click', () => {
        const res = state.sellCommodity(ResourceType.FOOD, 1);
        this.showBanner(res.msg);
        this.renderBuildingCommands(state, bldg);
      });
      document.getElementById('btn-buy-stone')?.addEventListener('click', () => {
        const res = state.buyCommodity(ResourceType.STONE, 1);
        this.showBanner(res.msg);
        this.renderBuildingCommands(state, bldg);
      });
      document.getElementById('btn-sell-stone')?.addEventListener('click', () => {
        const res = state.sellCommodity(ResourceType.STONE, 1);
        this.showBanner(res.msg);
        this.renderBuildingCommands(state, bldg);
      });

    } else if (bldg.type === BuildingType.WATCH_TOWER) {
      const gText = `Garrison: ${bldg.garrisonedUnits.length} / 5 units`;
      const arrowAtk = 5 + bldg.garrisonedUnits.length * 2;

      this.commandCard.innerHTML = `
        <div class="cmd-header" style="font-size: 10px; color: #d4af37;">
          <span>${gText} • Attack: ${arrowAtk} Pierce</span>
        </div>
        <div style="font-size: 11px; color: #b89b58; padding: 6px;">
          Right-click this tower with units to garrison them for increased arrow volleys.
        </div>
      `;
    } else {
      this.commandCard.innerHTML = `
        <div style="font-size: 11px; color: #999; padding: 6px;">
          Building completed. No active production commands.
        </div>
      `;
    }
  }

  private startBuildingPlacement(state: GameState, type: BuildingType) {
    const p1 = state.players.get(1);
    if (!p1) return;

    const cost = BUILDING_CONFIGS[type].cost;
    if (p1.resources.wood < cost.wood || p1.resources.stone < cost.stone) {
      this.showBanner(`Not enough resources! Need ${cost.wood ? cost.wood + 'W ' : ''}${cost.stone ? cost.stone + 'S' : ''}`);
      this.audio.playCommand();
      return;
    }

    this.activeBuildingPlacement = type;
    this.showBanner(`Placing ${BUILDING_CONFIGS[type].name} (Left-click ground to place, Right-click to cancel)`);
    this.audio.playSelect();
  }

  private trainUnit(state: GameState, bldgId: number, unitType: UnitType) {
    const p1 = state.players.get(1);
    const bldg = state.buildings.get(bldgId);
    if (!p1 || !bldg) return;

    if (p1.pop >= p1.maxPop) {
      this.showBanner('Housing limit reached! Build more Houses.');
      this.audio.playCommand();
      return;
    }

    const cost = UNIT_CONFIGS[unitType].cost;
    if (
      p1.resources.food < cost.food ||
      p1.resources.wood < cost.wood ||
      p1.resources.gold < cost.gold ||
      p1.resources.stone < cost.stone
    ) {
      this.showBanner('Not enough resources to train unit!');
      this.audio.playCommand();
      return;
    }

    p1.resources.food -= cost.food;
    p1.resources.wood -= cost.wood;
    p1.resources.gold -= cost.gold;
    p1.resources.stone -= cost.stone;

    bldg.productionQueue.push(unitType);
    this.showBanner(`Queued: ${UNIT_CONFIGS[unitType].name}`);
    this.audio.playSelect();
  }
}
