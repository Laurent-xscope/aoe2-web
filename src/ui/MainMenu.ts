import { AudioManager } from '../audio/AudioManager';

export interface GameLaunchConfig {
  mapType: string;
  civ: string;
  difficulty: string;
}

export class MainMenu {
  private container: HTMLElement;
  private skirmishModal: HTMLElement;

  constructor(private audio: AudioManager, private onStartGame: (cfg: GameLaunchConfig) => void) {
    this.container = document.createElement('div');
    this.container.id = 'main-menu-screen';
    this.container.innerHTML = `
      <div class="menu-overlay"></div>
      
      <!-- Top / Center Logo -->
      <div class="menu-brand">
        <h1 class="aoe-title">AGE OF EMPIRES II</h1>
        <div class="aoe-sub-title">HD EDITION &bull; THE CONQUERORS</div>
      </div>

      <!-- Main Navigation Buttons -->
      <div class="menu-button-stack">
        <button class="aoe-menu-btn" id="menu-btn-single">
          <span class="btn-text">SINGLE PLAYER</span>
        </button>
        <button class="aoe-menu-btn" id="menu-btn-campaign">
          <span class="btn-text">CAMPAIGN</span>
        </button>
        <button class="aoe-menu-btn" id="menu-btn-multiplayer">
          <span class="btn-text">MULTIPLAYER</span>
        </button>
        <button class="aoe-menu-btn" id="menu-btn-options">
          <span class="btn-text">OPTIONS</span>
        </button>
        <button class="aoe-menu-btn" id="menu-btn-history">
          <span class="btn-text">HISTORY</span>
        </button>
      </div>

      <!-- Skirmish Setup Modal -->
      <div class="skirmish-modal-backdrop" id="skirmish-modal" style="display: none;">
        <div class="skirmish-dialog">
          <div class="skirmish-header">
            <h2>STANDARD GAME SETUP</h2>
            <button class="skirmish-close-btn" id="btn-close-skirmish">&times;</button>
          </div>
          <div class="skirmish-body">
            <div class="setting-row">
              <label>Game Type:</label>
              <select id="sel-game-type" class="aoe-select">
                <option value="random_map">Random Map</option>
                <option value="deathmatch">Deathmatch</option>
                <option value="regicide">Regicide</option>
              </select>
            </div>
            <div class="setting-row">
              <label>Location / Map:</label>
              <select id="sel-map" class="aoe-select">
                <option value="arabia">Arabia (Open desert warfare)</option>
                <option value="black_forest">Black Forest (Defensive chokepoints)</option>
                <option value="coastal">Coastal (Water & land)</option>
                <option value="arena">Arena (Walled city)</option>
              </select>
            </div>
            <div class="setting-row">
              <label>Your Civilization:</label>
              <select id="sel-civ" class="aoe-select">
                <option value="britons">Britons (Longbowmen & Archery)</option>
                <option value="franks">Franks (Paladins & Cavalry)</option>
                <option value="byzantines">Byzantines (Defensive fortifications)</option>
                <option value="goths">Goths (Infantry swarms)</option>
              </select>
            </div>
            <div class="setting-row">
              <label>Difficulty:</label>
              <select id="sel-difficulty" class="aoe-select">
                <option value="standard">Standard</option>
                <option value="moderate">Moderate</option>
                <option value="hard">Hard</option>
              </select>
            </div>
          </div>
          <div class="skirmish-footer">
            <button class="aoe-action-btn cancel-btn" id="btn-cancel-game">BACK</button>
            <button class="aoe-action-btn start-btn" id="btn-confirm-start">START GAME</button>
          </div>
        </div>
      </div>
    `;

    document.body.appendChild(this.container);
    this.skirmishModal = document.getElementById('skirmish-modal')!;

    this.setupEvents();
  }

  private setupEvents() {
    const singleBtn = document.getElementById('menu-btn-single');
    const campaignBtn = document.getElementById('menu-btn-campaign');
    const multiBtn = document.getElementById('menu-btn-multiplayer');
    const optionsBtn = document.getElementById('menu-btn-options');
    const historyBtn = document.getElementById('menu-btn-history');

    const addHoverSound = (el: HTMLElement | null) => {
      el?.addEventListener('mouseenter', () => this.audio.playSelect());
    };

    [singleBtn, campaignBtn, multiBtn, optionsBtn, historyBtn].forEach(addHoverSound);

    singleBtn?.addEventListener('click', () => {
      this.audio.playCommand();
      this.skirmishModal.style.display = 'flex';
    });

    campaignBtn?.addEventListener('click', () => {
      this.audio.playCommand();
      this.skirmishModal.style.display = 'flex';
    });

    multiBtn?.addEventListener('click', () => {
      this.audio.playCommand();
      alert('Multiplayer peer-to-peer lobby via WebRTC is configured for room play.');
    });

    optionsBtn?.addEventListener('click', () => {
      this.audio.playCommand();
      alert('Video & Audio settings: Full HD Isometric Diamond Projection active (60 FPS).');
    });

    historyBtn?.addEventListener('click', () => {
      this.audio.playCommand();
      alert('Age of Empires II: The Age of Kings was originally developed by Ensemble Studios and published by Microsoft in 1999.');
    });

    document.getElementById('btn-close-skirmish')?.addEventListener('click', () => {
      this.skirmishModal.style.display = 'none';
    });

    document.getElementById('btn-cancel-game')?.addEventListener('click', () => {
      this.skirmishModal.style.display = 'none';
    });

    document.getElementById('btn-confirm-start')?.addEventListener('click', () => {
      const civSel = (document.getElementById('sel-civ') as HTMLSelectElement).value;
      const mapSel = (document.getElementById('sel-map') as HTMLSelectElement).value;
      const diffSel = (document.getElementById('sel-difficulty') as HTMLSelectElement).value;

      this.audio.playCivJingle(civSel === 'franks' ? 'french' : 'british');
      this.audio.stopThemeMusic();
      this.hide();

      this.onStartGame({
        civ: civSel,
        mapType: mapSel,
        difficulty: diffSel
      });
    });
  }

  public show() {
    this.container.style.display = 'flex';
    this.audio.playThemeMusic();
  }

  public hide() {
    this.container.style.display = 'none';
  }
}
