import { BuildingType, UnitType, BUILDING_CONFIGS, UNIT_CONFIGS } from '../core/Constants';
import { GameState } from '../core/GameState';
import { AudioManager } from '../audio/AudioManager';

export class UIManager {
  private resWood: HTMLElement;
  private resFood: HTMLElement;
  private resGold: HTMLElement;
  private resStone: HTMLElement;
  private resPop: HTMLElement;
  private fpsCounter: HTMLElement;

  private selectionName: HTMLElement;
  private selectionHpBar: HTMLElement;
  private selectionStats: HTMLElement;
  private portraitIcon: HTMLElement;
  private commandCard: HTMLElement;
  private gameBanner: HTMLElement;

  private bannerTimeout: number | null = null;
  public activeBuildingPlacement: BuildingType | null = null;

  constructor(private audio: AudioManager) {
    this.resWood = document.getElementById('res-wood-val')!;
    this.resFood = document.getElementById('res-food-val')!;
    this.resGold = document.getElementById('res-gold-val')!;
    this.resStone = document.getElementById('res-stone-val')!;
    this.resPop = document.getElementById('res-pop-val')!;
    this.fpsCounter = document.getElementById('fps-counter')!;

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
        this.selectionStats.textContent = `HP: ${Math.ceil(unit.hp)} / ${unit.maxHp} • Attack: ${unit.attack} • Speed: ${unit.speed}`;

        // Populate Commands (e.g. Villager build commands)
        this.renderUnitCommands(state, unit.type);
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
        this.selectionStats.textContent = `HP: ${Math.ceil(bldg.hp)} / ${bldg.maxHp} • ${bldg.isConstructed ? 'Complete' : `Constructing (${Math.floor(bldg.buildProgress)}%)`}`;

        // Populate Building Production Commands
        this.renderBuildingCommands(state, bldg);
        return;
      }
    }

    // Nothing selected
    this.portraitIcon.textContent = '🛡️';
    this.selectionName.textContent = 'No Selection';
    this.selectionHpBar.style.width = '0%';
    this.selectionStats.textContent = 'Select units or buildings';
    this.commandCard.innerHTML = '';
  }

  private renderUnitCommands(state: GameState, type: UnitType) {
    if (type !== UnitType.VILLAGER) {
      this.commandCard.innerHTML = '';
      return;
    }

    // Villager build menu
    this.commandCard.innerHTML = `
      <button class="cmd-btn" id="btn-build-house" title="Build House">
        <span class="cmd-icon">🛖</span>
        <span>House</span>
        <span class="cmd-cost">25 W</span>
      </button>
      <button class="cmd-btn" id="btn-build-barracks" title="Build Barracks">
        <span class="cmd-icon">⚔️</span>
        <span>Barracks</span>
        <span class="cmd-cost">175 W</span>
      </button>
      <button class="cmd-btn" id="btn-build-farm" title="Build Farm">
        <span class="cmd-icon">🌽</span>
        <span>Farm</span>
        <span class="cmd-cost">60 W</span>
      </button>
    `;

    document.getElementById('btn-build-house')?.addEventListener('click', () => {
      this.startBuildingPlacement(state, BuildingType.HOUSE);
    });
    document.getElementById('btn-build-barracks')?.addEventListener('click', () => {
      this.startBuildingPlacement(state, BuildingType.BARRACKS);
    });
    document.getElementById('btn-build-farm')?.addEventListener('click', () => {
      this.startBuildingPlacement(state, BuildingType.FARM);
    });
  }

  private renderBuildingCommands(state: GameState, bldg: { type: BuildingType; id: number; productionQueue: UnitType[] }) {
    if (bldg.type === BuildingType.TOWN_CENTER) {
      this.commandCard.innerHTML = `
        <button class="cmd-btn" id="btn-train-villager" title="Train Villager">
          <span class="cmd-icon">🧑‍🌾</span>
          <span>Villager</span>
          <span class="cmd-cost">50 F</span>
        </button>
      `;
      document.getElementById('btn-train-villager')?.addEventListener('click', () => {
        this.trainUnit(state, bldg.id, UnitType.VILLAGER);
      });
    } else if (bldg.type === BuildingType.BARRACKS) {
      this.commandCard.innerHTML = `
        <button class="cmd-btn" id="btn-train-militia" title="Train Militia">
          <span class="cmd-icon">🗡️</span>
          <span>Militia</span>
          <span class="cmd-cost">60 F 20 G</span>
        </button>
      `;
      document.getElementById('btn-train-militia')?.addEventListener('click', () => {
        this.trainUnit(state, bldg.id, UnitType.MILITIA);
      });
    } else {
      this.commandCard.innerHTML = '';
    }
  }

  private startBuildingPlacement(state: GameState, type: BuildingType) {
    const p1 = state.players.get(1);
    if (!p1) return;

    const cost = BUILDING_CONFIGS[type].cost;
    if (p1.resources.wood < cost.wood || p1.resources.stone < cost.stone) {
      this.showBanner('Not enough resources to build!');
      this.audio.playCommand();
      return;
    }

    this.activeBuildingPlacement = type;
    this.showBanner(`Placing ${BUILDING_CONFIGS[type].name} (Left-click ground to place)`);
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
      p1.resources.gold < cost.gold
    ) {
      this.showBanner('Not enough resources to train unit!');
      this.audio.playCommand();
      return;
    }

    p1.resources.food -= cost.food;
    p1.resources.wood -= cost.wood;
    p1.resources.gold -= cost.gold;

    bldg.productionQueue.push(unitType);
    this.showBanner(`Training ${UNIT_CONFIGS[unitType].name}`);
    this.audio.playSelect();
  }
}
